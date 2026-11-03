#!/usr/bin/env python3
"""Read-only MSVC RTTI locator for the vanilla Pip-Boy quest command.

Finds structural candidates, NOT safe-to-call functions. No binary bytes,
disassembly, inferred signatures, or game assets are distributed.
"""
import argparse
from pathlib import Path
import struct

TYPE_NAME = b".?AVToggleQuestActiveStatusCommand@@\0"
IMAGE_BASE_OFFSET = 24
IMAGE_SCN_MEM_EXECUTE = 0x20000000


def number(data, offset, fmt):
    size = struct.calcsize(fmt)
    if offset < 0 or offset + size > len(data):
        raise ValueError("truncated PE data")
    return struct.unpack_from(fmt, data, offset)[0]


class PE64:
    def __init__(self, data):
        self.data = data
        if len(data) < 0x100 or data[:2] != b"MZ":
            raise ValueError("not a DOS/PE image")
        pe = number(data, 0x3C, "<I")
        if data[pe:pe + 4] != b"PE\0\0":
            raise ValueError("missing PE signature")
        coff, optional = pe + 4, pe + 24
        count = number(data, coff + 2, "<H")
        optional_size = number(data, coff + 16, "<H")
        if number(data, coff, "<H") != 0x8664 or number(data, optional, "<H") != 0x20B:
            raise ValueError("expected x64 PE32+")
        if optional_size < 32 or count < 1 or count > 96:
            raise ValueError("invalid section table")
        self.image_base = number(data, optional + IMAGE_BASE_OFFSET, "<Q")
        self.sections = []
        table = optional + optional_size
        for index in range(count):
            off = table + index * 40
            if off + 40 > len(data):
                raise ValueError("truncated section table")
            name = data[off:off + 8].split(b"\0", 1)[0].decode("ascii", "replace")
            vsize, va, rawsize, raw = struct.unpack_from("<IIII", data, off + 8)
            flags = number(data, off + 36, "<I")
            if rawsize and raw + rawsize > len(data):
                raise ValueError("section extends beyond file")
            self.sections.append((name, va, max(vsize, rawsize), raw, rawsize, flags))

    def rva(self, offset):
        for _, va, _, raw, rawsize, _ in self.sections:
            if raw <= offset < raw + rawsize:
                return va + offset - raw
        return None

    def section(self, rva):
        for row in self.sections:
            if row[1] <= rva < row[1] + row[2]:
                return row
        return None


def find_vtables(data, type_name=TYPE_NAME):
    """Locate bounded RTTI candidates for an exact MSVC class type name."""
    if (not isinstance(type_name, bytes) or len(type_name) > 128 or
            not type_name.startswith(b".?AV") or not type_name.endswith(b"@@\0")):
        raise ValueError("invalid RTTI type name")
    pe = PE64(data)
    matches = []
    start = 0
    while (hit := data.find(type_name, start)) >= 0:
        start = hit + len(type_name)
        descriptor = pe.rva(hit - 16)
        if descriptor is None:
            continue
        pos = 0
        pattern = struct.pack("<I", descriptor)
        while (pos := data.find(pattern, pos)) >= 0:
            coloff = pos - 12
            pos += 1
            colrva = pe.rva(coloff)
            if colrva is None or coloff < 0 or coloff + 24 > len(data):
                continue
            signature, _, _, td, hierarchy, self_rva = struct.unpack_from("<IIIIII", data, coloff)
            if signature != 1 or td != descriptor or self_rva != colrva or not hierarchy:
                continue
            ref = struct.pack("<Q", pe.image_base + colrva)
            ptr = 0
            while (ptr := data.find(ref, ptr)) >= 0:
                ptr += 8
                table_rva = pe.rva(ptr)
                region = pe.section(table_rva) if table_rva is not None else None
                if region is None or region[5] & IMAGE_SCN_MEM_EXECUTE:
                    continue
                if ptr + 32 > len(data) or ptr + 32 > region[3] + region[4]:
                    continue
                methods = struct.unpack_from("<QQQQ", data, ptr)
                rvas = [address - pe.image_base for address in methods]
                if all((section := pe.section(rva)) is not None
                       and section[5] & IMAGE_SCN_MEM_EXECUTE for rva in rvas):
                    candidate = (descriptor, colrva, table_rva, tuple(rvas))
                    if candidate not in matches:
                        matches.append(candidate)
    return matches


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path, help="locally installed Fallout4.exe")
    args = parser.parse_args()
    if args.exe.is_symlink() or not args.exe.is_file():
        parser.error("input must be an existing regular file")
    try:
        candidates = find_vtables(args.exe.read_bytes())
    except (ValueError, OSError, struct.error) as exc:
        parser.error(str(exc))
    if not candidates:
        print("UNRESOLVED: no structurally valid RTTI/vtable candidate found")
        return 1
    for descriptor, col, table, methods in candidates:
        print(f"RTTI descriptor RVA: 0x{descriptor:X}; COL RVA: 0x{col:X}")
        print(f"Candidate vtable RVA: 0x{table:X}")
        print("Candidate virtual method RVAs:", ", ".join(f"0x{x:X}" for x in methods))
    print("METADATA ONLY: executable-section pointers are not verified callable methods.")
    print("Never invoke these offsets or enable Journal -> Pip-Boy writes based on this output.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
