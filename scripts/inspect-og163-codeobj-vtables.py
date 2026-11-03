#!/usr/bin/env python3
"""Read-only OG163 RTTI/vtable comparison; never treat a vtable RVA as callable."""
import argparse
from hashlib import sha256
import importlib.util
import json
from pathlib import Path
import struct

OG163_SHA256 = "55f57947db9e05575122fae1088f0b0247442f11e566b56036caa0ac93329c36"
MAX_PE_BYTES = 128 * 1024 * 1024
CLASS_NAMES = ("IMenu", "GameMenuBase", "PipboyMenu")


def rtti_module():
    source = Path(__file__).with_name("inspect-quest-command-rtti.py")
    spec = importlib.util.spec_from_file_location("quest_rtti_readonly", source)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def primary_vtable(pe, data, candidates, label):
    """Require one primary (COL.offset=0, cdOffset=0) and valid PE section."""
    primary = []
    for _descriptor, col_rva, table_rva, slots in candidates:
        section = pe.section(col_rva)
        if section is None or section[5] & 0x20000000:
            continue
        _name, sec_rva, _vsize, raw, rawsize, _flags = section
        file_at = raw + col_rva - sec_rva
        if file_at < raw or file_at + 24 > raw + rawsize:
            continue
        signature, offset, cd_offset, _type, _hierarchy, self_rva = struct.unpack_from(
            "<IIIIII", data, file_at)
        if signature == 1 and offset == 0 and cd_offset == 0 and self_rva == col_rva:
            primary.append((table_rva, slots))
    if len(primary) != 1:
        raise ValueError(f"{label}: expected one validated primary RTTI vtable")
    return primary[0]


def compare_vtables(data):
    if not isinstance(data, bytes) or not 0x100 <= len(data) <= MAX_PE_BYTES:
        raise ValueError("invalid bounded x64 PE input")
    module = rtti_module()
    pe = module.PE64(data)
    found = {}
    for name in CLASS_NAMES:
        candidates = module.find_vtables(data, (".?AV" + name + "@@").encode() + b"\0")
        found[name] = primary_vtable(pe, data, candidates, name)
    base1, base2, pip = (found[n][1] for n in CLASS_NAMES)
    # Vtable slot ordering established from matching pinned CommonLibF4 layout:
    # ~FunctionHandler, Call, MapCodeObjectFunctions; no native invocation.
    if base1[1:3] != base2[1:3]:
        raise ValueError("base class dispatch slots disagree")
    if pip[1] == base1[1] or pip[2] == base1[2]:
        raise ValueError("PipboyMenu override evidence incomplete")
    return {
        "status": "structural-pe-rtti-only",
        "native_callback_abi_verified": False,
        "native_write_authorized": False,
        "primary_vtables": {n: hex(found[n][0]) for n in CLASS_NAMES},
        "virtual_slot_rvas": {
            n: {"Call": hex(found[n][1][1]),
                "MapCodeObjectFunctions": hex(found[n][1][2])}
            for n in CLASS_NAMES
        },
        "pipboy_overrides_both_slots": True,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path, help="owner's original OG163 Fallout4.exe")
    args = parser.parse_args()
    p = args.exe
    if p.is_symlink() or not p.is_file():
        parser.error("only original regular non-symlink executable is accepted")
    if p.stat().st_size > MAX_PE_BYTES:
        parser.error("exe exceeds size ceiling")
    data = p.read_bytes()
    if sha256(data).hexdigest() != OG163_SHA256:
        parser.error("unexpected executable hash; target only OG163 known fixture")
    try:
        result = compare_vtables(data)
    except (ValueError, struct.error) as exc:
        parser.error(str(exc))
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
