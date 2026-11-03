#!/usr/bin/env python3
"""Read-only x64 PE metadata survey of vanilla Pip-Boy callback strings.

Never treats an .rdata string RVA as a native function address. Never
extracts or writes original executable code, memory, or private data.
"""
import argparse
from hashlib import sha256
import importlib.util
import json
from pathlib import Path
import stat
import struct

MAX_EXE_BYTES = 128 * 1024 * 1024
CALLBACK_NAMES = ("onQuestSelection", "SetQuestActive", "onObjectivesReady")


def load_pe_type():
    source = Path(__file__).with_name("inspect-quest-command-rtti.py")
    spec = importlib.util.spec_from_file_location("quest_command_rtti_base", source)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.PE64


def scan_on_disk_rip_patterns(data: bytes, pe, targets: dict[str, int]) -> dict:
    """Conservative, bounded x64 RIP-relative byte-pattern candidates.

    A raw byte pattern is NOT a verified instruction boundary, a
    registered native function, or evidence of the loaded SteamStub
    image. Do not use any result as a callable address.
    """
    found = {name: [] for name in targets}
    by_target = {rva: name for name, rva in targets.items()}
    prefixes = (b"\x48\x8d", b"\x48\x8b",
                b"\x4c\x8d", b"\x4c\x8b")
    for section_name, start_rva, _, raw_offset, raw_size, flags in pe.sections:
        if not flags & 0x20000000 or not raw_size:
            continue
        view = data[raw_offset:raw_offset + raw_size]
        for prefix in prefixes:
            offset = 0
            while (hit := view.find(prefix, offset)) != -1:
                offset = hit + 1
                if hit + 7 > len(view):
                    break
                modrm = view[hit + 2]
                if modrm & 0xC7 != 0x05:
                    continue
                displacement = struct.unpack_from("<i", view, hit + 3)[0]
                destination = (start_rva + hit + 7 + displacement) & 0xFFFFFFFF
                name = by_target.get(destination)
                if name is not None:
                    found[name].append(f"0x{start_rva + hit:X}")
    return {name: sorted(set(values)) for name, values in found.items()}


def scan(data: bytes) -> dict:
    """Require unique null-terminated strings in a nonexecutable section."""
    if len(data) > MAX_EXE_BYTES:
        raise ValueError("PE image exceeds bounded size")
    pe = load_pe_type()(data)
    found = []
    for name in CALLBACK_NAMES:
        needle = name.encode("ascii") + b"\0"
        locations = []
        pos = 0
        while (hit := data.find(needle, pos)) >= 0:
            pos = hit + 1
            if hit and (65 <= data[hit - 1] <= 90 or
                        97 <= data[hit - 1] <= 122):
                continue  # reject substring of a longer identifier
            rva = pe.rva(hit)
            if rva is None:
                continue
            section = pe.section(rva)
            if section and not section[5] & 0x20000000 and (
                    rva + len(needle) <= section[1] + section[2]):
                locations.append((rva, section[0]))
        if len(locations) != 1:
            raise ValueError(f"{name}: expected one nonexecuting metadata string")
        rva, section = locations[0]
        # An absent literal pointer reference does NOT exclude code that
        # uses RIP-relative instruction references or dynamic registration.
        literal = []
        for fmt, val in (("<I", rva), ("<Q", pe.image_base + rva)):
            needle_ref = struct.pack(fmt, val)
            hits = 0
            start = 0
            while (pos := data.find(needle_ref, start)) >= 0:
                start = pos + 1
                ref_rva = pe.rva(pos)
                if ref_rva is None:
                    continue
                ref_section = pe.section(ref_rva)
                if ref_section and not ref_section[5] & 0x20000000:
                    hits += 1
            literal.append(hits)
        found.append({"name": name, "rva": f"0x{rva:X}",
                      "section": section, "noncode_literal_ref_counts": literal})
    patterns = scan_on_disk_rip_patterns(
        data, pe, {entry["name"]: int(entry["rva"], 16) for entry in found})
    for entry in found:
        # Counts reflect on-disk candidate bytes only; not disassembly.
        entry["on_disk_rip_pattern_candidate_count"] = len(patterns[entry["name"]])
        entry["on_disk_rip_pattern_rvas"] = patterns[entry["name"]][:16]
    return {
        "status": "callback-names-in-PE-metadata-only",
        "names": found,
        "on_disk_instruction_boundaries_proven": False,
        "loaded_image_code_correspondence_proven": False,
        "native_callback_address_proven": False,
        "selected_entry_fields_proven": False,
        "safe_to_invoke_native_callback": False,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path, help="legally installed local Fallout4.exe")
    parser.add_argument("--expected-sha256", default=None)
    args = parser.parse_args(argv)
    if args.exe.is_symlink() or not args.exe.is_file():
        parser.error("input must be a regular non-symlink file")
    if not stat.S_ISREG(args.exe.stat().st_mode) or args.exe.stat().st_size > MAX_EXE_BYTES:
        parser.error("file is not a bounded regular executable")
    try:
        payload = args.exe.read_bytes()
        if args.expected_sha256 and sha256(payload).hexdigest() != args.expected_sha256.lower():
            raise ValueError("unexpected PE identity")
        summary = scan(payload)
    except (OSError, ValueError, struct.error) as exc:
        parser.error(str(exc))
    print(json.dumps(summary, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
