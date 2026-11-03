#!/usr/bin/env python3
"""Inspect OG Fallout 4 vanilla Pip-Boy SWF ABC names in place; never export assets."""
import argparse
import json
import struct
import zlib
from pathlib import Path

WANTED = {"interface\\pipboymenu.swf": "PipboyMenu",
          "interface\\pipboy_datapage.swf": "Pipboy_DataPage"}
KEYS = ("SetQuestActive", "onQuestSelection", "BGSCodeObj",
        "InitCodeObj", "QuestsTab", "PopulatePipboyInfoObj")
CAP = 8 * 1024 * 1024


class Reader:
    def __init__(self, b):
        self.b, self.i = b, 0

    def take(self, n):
        if n < 0 or self.i + n > len(self.b):
            raise ValueError("truncated input")
        v = self.b[self.i:self.i+n]
        self.i += n
        return v

    def u(self):
        result = 0
        for i in range(5):
            x = self.take(1)[0]
            result |= (x & 127) << (7*i)
            if x < 128:
                if result > 0x3fffffff:
                    raise ValueError("ABC integer overflow")
                return result
        raise ValueError("invalid ABC integer")


def abc_names(blob):
    r = Reader(blob)
    if r.take(4) != struct.pack("<HH", 16, 46):
        raise ValueError("unsupported ABC version")
    for pool in ("int", "uint", "double"):
        count = r.u()
        if count > 100000:
            raise ValueError("oversized ABC pool")
        for _ in range(count-1):
            r.take(8) if pool == "double" else r.u()
    count = r.u()
    if count > 100000:
        raise ValueError("oversized string pool")
    strs = [""]
    for _ in range(count-1):
        size = r.u()
        if size > 65536:
            raise ValueError("oversized string")
        strs.append(r.take(size).decode("utf-8", "replace"))
    return [name for name in KEYS if name in strs]


def scan_swf(blob):
    if len(blob) < 12 or blob[:3] not in (b"CWS", b"FWS"):
        raise ValueError("unsupported SWF")
    size = struct.unpack_from("<I", blob, 4)[0]
    if not 12 <= size <= CAP:
        raise ValueError("oversized SWF")
    if blob[:3] == b"CWS":
        z = zlib.decompressobj()
        payload = z.decompress(blob[8:], CAP + 1)
        if not z.eof or z.unconsumed_tail or z.unused_data:
            raise ValueError("invalid SWF zlib")
        blob = blob[:8] + payload
    if len(blob) != size:
        raise ValueError("incorrect SWF expanded length")
    bits = blob[8] >> 3
    pos = 8 + (5+4*bits+7)//8 + 4
    found = set()
    tag_count = 0
    for _ in range(20000):
        if pos+2 > len(blob):
            raise ValueError("unterminated SWF tags")
        tag = struct.unpack_from("<H", blob, pos)[0]
        pos += 2
        code, n = tag >> 6, tag & 63
        if n == 63:
            if pos+4 > len(blob):
                raise ValueError("truncated SWF tag")
            n = struct.unpack_from("<I", blob, pos)[0]
            pos += 4
        if pos+n > len(blob):
            raise ValueError("invalid SWF tag bound")
        if code == 82:
            entry = blob[pos:pos+n]
            zero = entry.find(bytes([0]), 4)
            if zero < 4:
                raise ValueError("invalid DoABC")
            found.update(abc_names(entry[zero+1:]))
            tag_count += 1
        pos += n
        if code == 0:
            break
    else:
        raise ValueError("SWF tag count overflow")
    return {"swf_version": blob[3], "abc_tags": tag_count,
            "abc_strings_present": [k for k in KEYS if k in found]}


def scan_ba2(path):
    if path.is_symlink() or not path.is_file():
        raise ValueError("require original regular BA2 file")
    total = path.stat().st_size
    if not 60 <= total <= 2_000_000_000:
        raise ValueError("unsupported archive size")
    with path.open("rb") as f:
        magic, version, kind, count, names_offset = struct.unpack(
            "<4sI4sIQ", f.read(24))
        if (magic, version, kind) != (b"BTDX", 1, b"GNRL"):
            raise ValueError("require original Fallout 4 GNRL v1")
        if count > 10000 or 24+count*36 > names_offset or names_offset > total:
            raise ValueError("invalid BA2 file table")
        records = [f.read(36) for _ in range(count)]
        f.seek(names_offset)
        matched = {}
        for record in records:
            nb = f.read(2)
            if len(nb) != 2:
                raise ValueError("truncated name table")
            size = struct.unpack("<H", nb)[0]
            if not 1 <= size <= 1024 or f.tell()+size > total:
                raise ValueError("invalid BA2 name")
            name = f.read(size).decode("utf-8", "replace").lower().replace("/", "\\")
            if name in WANTED:
                if name in matched:
                    raise ValueError("duplicate requested SWF")
                matched[name] = record
        if set(matched) != set(WANTED):
            raise ValueError("missing original Pip-Boy page SWFs")
        result = {}
        for name, record in matched.items():
            offset, packed, unpacked = struct.unpack_from("<QII", record, 16)
            length = packed or unpacked
            if not 0 < length <= CAP or not 0 < unpacked <= CAP:
                raise ValueError("oversized SWF payload")
            if offset+length > names_offset:
                raise ValueError("invalid BA2 payload offset")
            f.seek(offset)
            data = f.read(length)
            if packed:
                dec = zlib.decompressobj()
                data = dec.decompress(data, CAP+1)
                if not dec.eof or dec.unused_data or dec.unconsumed_tail:
                    raise ValueError("invalid BA2 compression")
            if len(data) != unpacked:
                raise ValueError("invalid BA2 decompressed size")
            result[WANTED[name]] = scan_swf(data)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path,
                        help="original, non-symlink Fallout4 - Interface.ba2")
    args = parser.parse_args()
    try:
        result = scan_ba2(args.archive)
    except (ValueError, OSError, struct.error, zlib.error) as e:
        parser.error(str(e))
    print(json.dumps({"status": "read-only-vanilla-abc-string-pool",
                      "game_assets_exported": False, "swfs": result}, indent=2))


if __name__ == "__main__":
    main()
