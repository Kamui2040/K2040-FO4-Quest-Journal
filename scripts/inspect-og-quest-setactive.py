#!/usr/bin/env python3
"""Read-only OG 1.10.163 candidate relocation / PE section inspector.

Checks published Address Library IDs against a local executable *on disk*.
Does not resolve runtime addresses, assume an ABI, or invoke quest functions.
Do not use its output alone to enable a native quest tracking backend.
"""
import argparse
import importlib
import struct
from pathlib import Path

pe_module = importlib.import_module("inspect-quest-command-rtti")

CANDIDATES = (
    ("TESQuest::GetActive", 690102, 0x05D6AA0),
    ("TESQuest::SetActive", 1567562, 0x05D6AC0),
)
SAMPLE_SIZE = 24


def lookup_ids(lines, expected):
    found = {}
    header = next(lines, "").strip()
    if header.lstrip("\ufeff") != "id,fo4_addr":
        raise ValueError("expected canonical id,fo4_addr CSV header")
    for line in lines:
        identifier, _, rva = line.strip().partition(",")
        if not identifier or not rva:
            continue
        try:
            entry = int(identifier, 10)
        except ValueError:
            continue
        if entry in expected:
            if entry in found:
                raise ValueError(f"duplicate relocation ID {entry}")
            found[entry] = int(rva, 16)
    if set(found) != set(expected):
        raise ValueError("missing expected relocation IDs")
    return found



def lookup_installed_address_library(blob, ids):
    """Read standard OG offset pairs, no relocations or game memory access.

    Header is uint64 record count followed by sorted uint64 ID / uint64 RVA
    pairs. Does not assume IDs are dense in the array.
    """
    if len(blob) < 8:
        raise ValueError("truncated Address Library header")
    count = struct.unpack_from("<Q", blob, 0)[0]
    if not count or len(blob) != 8 + count * 16:
        raise ValueError("Address Library record count/length mismatch")
    result = {}
    for ident in ids:
        low, high = 0, count
        while low < high:
            mid = (low + high) // 2
            candidate = struct.unpack_from("<Q", blob, 8 + mid * 16)[0]
            if candidate < ident:
                low = mid + 1
            else:
                high = mid
        if low == count:
            raise ValueError(f"Address Library missing ID {ident}")
        key, rva = struct.unpack_from("<QQ", blob, 8 + low * 16)
        if key != ident:
            raise ValueError(f"Address Library missing ID {ident}")
        result[ident] = rva
    return result

def read_candidates(mapping_csv, executable_bytes):
    with Path(mapping_csv).open(encoding="utf-8-sig") as stream:
        ids = lookup_ids(stream, {ident for _, ident, _ in CANDIDATES})
    pe = pe_module.PE64(executable_bytes)
    output = []
    for label, ident, expected in CANDIDATES:
        rva = ids[ident]
        if rva != expected:
            raise ValueError(f"{label}: OG mapping differs from documented reference")
        section = pe.section(rva)
        if not section or not section[5] & pe_module.IMAGE_SCN_MEM_EXECUTE:
            raise ValueError(f"{label}: relocation is not in an executable PE section")
        name, va, _, raw, raw_size, _ = section
        offset = raw + rva - va
        if offset < 0 or offset + SAMPLE_SIZE > raw + raw_size:
            raise ValueError(f"{label}: relocation extends beyond section file data")
        output.append((label, ident, rva, name, executable_bytes[offset:offset + SAMPLE_SIZE].hex(" ")))
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mapping", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--address-library", type=Path, default=None,
                        help="optional installed version-1-10-163-0.bin")
    args = parser.parse_args()
    if args.address_library:
        with args.mapping.open(encoding="utf-8-sig") as stream:
            csv_ids = lookup_ids(stream, {ident for _, ident, _ in CANDIDATES})
        installed = lookup_installed_address_library(
            args.address_library.read_bytes(), set(csv_ids)
        )
        if installed != csv_ids:
            raise ValueError("installed Address Library conflicts with public OG mapping")
        print("PASS: installed Address Library and independent OG mapping agree")
    for name, ident, rva, section, sample in read_candidates(args.mapping, args.exe.read_bytes()):
        print(f"{name}: ID {ident}, RVA 0x{rva:08X}, section {section}")
        print(f"  first {SAMPLE_SIZE} on-disk bytes: {sample}")
    print("UNVERIFIED: on-disk bytes and relocation mapping do not establish callable semantics,")
    print("ABI, preconditions, save lifecycle or runtime-memory image. No native call performed.")


if __name__ == "__main__":
    main()
