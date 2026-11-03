#!/usr/bin/env python3
"""Strictly read-only analysis of a PRIVATE Quest Journal selector QA session.

Never reads Fallout4.exe, interacts with a game process, or writes in the
repository. Raw instruction bytes remain in the input F4SE log; the default
output is metadata only. Use --disassemble to inspect local code mnemonics.
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
import stat
import subprocess
import sys
import tempfile
from dataclasses import asdict, dataclass
from pathlib import Path

MAX_LOG_BYTES = 2 * 1024 * 1024
CODE_BYTES = 192
SLOT_COUNT = 4
CODE_START = 0x1000
CODE_END = 0x02C0C000
START_TOKEN = "SelectorReadOnlyQA: READY; no writer linked; one capture/process"
SUBMIT_TOKEN = "SelectorReadOnlyQA: one bounded game-thread task submitted"
END_TOKEN = "SelectorReadOnlyQA: END result=captured (no game mutation)"
LOG_PATTERN = re.compile(
    r"^K2040_Quest_Journal_Selector_ReadOnly_\d{8}_\d{6}_\d+\.log$"
)
ROW_AUDIT_MARKER = "SelectorReadOnlyQA: row-audit="
ROW_AUDIT_TRAILER = " (anonymous, NOT command ABI)"
SAFE_STATUSES = {
    "invalid-row", "misc-root-type-six-candidate",
    "misc-root-type-unknown", "misc-root-type-mismatch",
    "unresolved-native-quest", "form-mismatch", "missing-instance",
    "instance-mismatch", "missing-row-type", "conflicting-row-type-keys",
    "type-mismatch", "structural-candidate-NOT-command-ABI",
}
SCHEMA_PATTERN = re.compile(r"^[A-Za-z0-9_]{1,24}:[0-8]$")
SLOT_PATTERN = re.compile(
    r"SelectorReadOnlyQA: slot=(\d) RVA=0x([0-9A-Fa-f]+) "
    r"bytes=([0-9a-f]+) \(read only\)"
)


class CaptureError(ValueError):
    pass


@dataclass(frozen=True)
class Slot:
    index: int
    rva: int
    hex_bytes: str


def parse_lines(content: str) -> tuple[Slot, ...]:
    """Require one complete, isolated 4-slot read-only capture."""
    lines = content.splitlines()
    if sum(START_TOKEN in line for line in lines) != 1:
        raise CaptureError("missing or repeated selector-only startup marker")
    if sum(SUBMIT_TOKEN in line for line in lines) != 1:
        raise CaptureError("missing or repeated one-shot submit marker")
    if sum(END_TOKEN in line for line in lines) != 1:
        raise CaptureError("missing or repeated successful read-only end marker")

    captured: dict[int, Slot] = {}
    for line in lines:
        if "SelectorReadOnlyQA: slot=" not in line:
            continue
        matched = SLOT_PATTERN.search(line)
        if not matched:
            raise CaptureError("malformed selector slot record")
        i = int(matched.group(1))
        rva = int(matched.group(2), 16)
        payload = matched.group(3)
        if i not in range(SLOT_COUNT) or i in captured:
            raise CaptureError("unexpected or duplicate selector slot")
        if len(payload) != CODE_BYTES * 2:
            raise CaptureError("truncated or overlong selector prefix")
        if rva < CODE_START or rva + CODE_BYTES > CODE_END:
            raise CaptureError("slot RVA outside OG executable code range")
        captured[i] = Slot(i, rva, payload)

    if set(captured) != set(range(SLOT_COUNT)):
        raise CaptureError("not all four validated selector slots are present")
    if len({v.rva for v in captured.values()}) != SLOT_COUNT:
        raise CaptureError("duplicate slot addresses")
    # The file is just a code sample; an address does not establish
    # function boundaries or the values of three runtime arguments.
    return tuple(captured[i] for i in range(SLOT_COUNT))


def parse_row_audit(content: str) -> dict | None:
    """Optional newer capture: anonymous structural row statistics only.

    Older, already accepted prefix-only captures legitimately lack this
    record. Never pretend that means real row identity was validated.
    """
    lines = [line for line in content.splitlines()
             if ROW_AUDIT_MARKER in line]
    if not lines:
        return None
    if len(lines) != 1:
        raise CaptureError("duplicate row-audit record")
    line = lines[0]
    start = line.find(ROW_AUDIT_MARKER) + len(ROW_AUDIT_MARKER)
    end = line.find(ROW_AUDIT_TRAILER, start)
    if end < 0 or end - start > 4096:
        raise CaptureError("invalid or oversized row-audit payload")
    try:
        item = json.loads(line[start:end])
    except json.JSONDecodeError as error:
        raise CaptureError("invalid row-audit JSON") from error
    if not isinstance(item, dict) or item.get("available") is not True:
        raise CaptureError("row-audit source unavailable")
    # A previous user-accepted read-only log used a v1 schema which
    # mistakenly counted normal-quest objective leaves as Misc children.
    # Preserve its raw findings but NEVER silently relabel them.
    version = item.get("auditVersion", 1)
    if type(version) is not int or version not in (1, 2):
        raise CaptureError("unsupported row-audit format version")
    number_fields = (
        "rootRows", "miscRows", "syntheticRows", "typedRowTypeFields",
        "typedInstanceFields", "questTypeKeys", "typeKeys", "bothKeys",
        "typedRootTypeFields", "typedMiscTypeFields"
    )
    if version == 2:
        number_fields += ("ordinaryObjectiveRows", "unknownParentChildren")
    for key in number_fields:
        value = item.get(key)
        if type(value) is not int or not 0 <= value <= 512:
            raise CaptureError("row-audit bounded count missing or invalid")
    total = item["rootRows"] + item["miscRows"] + item["syntheticRows"]
    all_rows = total
    if version == 2:
        all_rows += item["ordinaryObjectiveRows"] + item["unknownParentChildren"]
    if all_rows > 512 or item.get("truncated") is not False:
        raise CaptureError("row-audit truncated or too many rows")
    if version == 2 and item["unknownParentChildren"]:
        raise CaptureError("row-audit has children with unclassified parents")
    if item["typedRowTypeFields"] > total or (
        item["typedRootTypeFields"] > item["rootRows"] or
        item["typedMiscTypeFields"] > item["miscRows"] or
        item["bothKeys"] > item["typedRowTypeFields"]):
        raise CaptureError("inconsistent row-audit counts")
    statuses = item.get("statusCounts")
    if not isinstance(statuses, list) or len(statuses) != len(SAFE_STATUSES):
        raise CaptureError("row-audit status vector incomplete")
    labels = set()
    counted = 0
    for stat in statuses:
        if not isinstance(stat, dict) or set(stat) != {"kind", "count"}:
            raise CaptureError("malformed row-audit category")
        label, count = stat["kind"], stat["count"]
        if label not in SAFE_STATUSES or label in labels or (
            type(count) is not int or not 0 <= count <= 512):
            raise CaptureError("unrecognized or duplicate row-audit category")
        labels.add(label)
        counted += count
    if counted != total:
        raise CaptureError("row-audit status totals inconsistent")
    schema_names = ("schemaQuestRoot", "schemaMiscChild",
                    "schemaSyntheticRoot")
    if version == 2:
        schema_names += ("schemaObjectiveDisplay",)
    for name in schema_names:
        fields = item.get(name)
        if (not isinstance(fields, list) or len(fields)>24 or
            any(not isinstance(f,str) or
                not SCHEMA_PATTERN.fullmatch(f) for f in fields)):
            raise CaptureError("unsafe or malformed row-audit schema")
    return item


def read_log(path: Path) -> tuple[Slot, ...]:
    if not LOG_PATTERN.fullmatch(path.name):
        raise CaptureError("not a per-session Quest Journal selector-only log")
    if path.is_symlink():
        raise CaptureError("refusing a symbolic-link log")
    details = path.stat()
    if not stat.S_ISREG(details.st_mode) or details.st_size > MAX_LOG_BYTES:
        raise CaptureError("log must be a bounded regular file")
    return parse_lines(path.read_text(encoding="utf-8", errors="replace"))


def disassemble(slot: Slot) -> str:
    """Use local binutils only, with no network or shell interpolation."""
    objdump = shutil.which("objdump")
    if not objdump:
        raise CaptureError("binutils objdump not installed")
    with tempfile.TemporaryDirectory(prefix="k2040-selector-decode-") as temp:
        sample = Path(temp) / "sample.bin"
        sample.write_bytes(bytes.fromhex(slot.hex_bytes))
        result = subprocess.run(
            [objdump, "-D", "-b", "binary", "-m", "i386:x86-64",
             "-M", "intel", f"--adjust-vma={slot.rva}", str(sample)],
            capture_output=True, text=True, timeout=12, check=False
        )
        if result.returncode:
            raise CaptureError("local x64 decoder returned an error")
        # Instruction boundaries are not guaranteed beyond the actual
        # function, and no disassembly validates semantic arguments.
        return "\n".join(
            line for line in result.stdout.splitlines()
            if re.match(r"^\s*[0-9a-f]+:\s+", line)
        )


def main(argv: list[str] | None = None) -> int:
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("log", type=Path, help="private, per-launch selector log")
    cli.add_argument("--disassemble", action="store_true",
                     help="locally decode instructions to stdout (private)")
    args = cli.parse_args(argv)
    try:
        slots = read_log(args.log)
        # Keep backwards compatibility: old successful prefixes-only logs
        # need not contain the later structural survey.
        audit = parse_row_audit(
            args.log.read_text(encoding="utf-8", errors="replace"))
        print(json.dumps({
            "capture": "complete-read-only",
            "slot_count": len(slots),
            "bytes_per_slot": CODE_BYTES,
            "method_rvas": [f"0x{s.rva:X}" for s in slots],
            "row_audit_present": audit is not None,
            "row_audit_version": (audit.get("auditVersion", 1)
                                  if audit is not None else None),
            "legacy_parent_classification": (audit is not None and
                                              "auditVersion" not in audit),
            "row_audit": audit,
            "known": "bounded method prefixes only",
            "not_proven": [
                "runtime command argument values",
                "third selector argument meaning",
                "function boundaries after each method entry",
                "generic native quest-tracking safety"
            ]
        }, indent=2))
        if args.disassemble:
            for slot in slots:
                print(f"\nSELECTOR SLOT {slot.index} — RVA 0x{slot.rva:X}")
                print(disassemble(slot))
    except (CaptureError, OSError, subprocess.TimeoutExpired) as error:
        print(f"INCOMPLETE/UNSAFE: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
