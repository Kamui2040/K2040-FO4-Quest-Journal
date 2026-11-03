"""Synthetic safety fixtures for original Fallout 4 SWF/BA2 metadata-only reader."""
import importlib.util
import struct
import tempfile
import unittest
import zlib
from pathlib import Path

P = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "og163", P / "scripts/inspect-og163-interface-abc.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def abc(names):
    b = bytearray(struct.pack("<HH", 16, 46))
    b.extend(bytes([1, 1, 1, len(names) + 1]))
    for name in names:
        encoded = name.encode()
        b.extend(bytes([len(encoded)]))
        b.extend(encoded)
    return bytes(b)


def swf(names):
    body = bytes(5) + abc(names)
    frame = bytes([8, 0, 0, 30, 1, 0])
    tags = struct.pack("<HI", (82 << 6) | 63, len(body)) + body + bytes(2)
    raw = frame + tags
    return b"CWS" + bytes([15]) + struct.pack("<I", len(raw) + 8) + zlib.compress(raw)


def ba2(parts):
    offset = 24 + len(parts) * 36
    records, payloads, names = [], [], []
    for name, payload in parts.items():
        records.append(struct.pack("<I4sIIQIII", 0, b"swf\0", 0, 0,
                                   offset, 0, len(payload), 0xBAADF00D))
        payloads.append(payload)
        n = name.encode()
        names.append(struct.pack("<H", len(n)) + n)
        offset += len(payload)
    return (struct.pack("<4sI4sIQ", b"BTDX", 1, b"GNRL", len(parts), offset)
            + b"".join(records + payloads + names))


class VanillaInspector(unittest.TestCase):
    def test_only_abc_strings(self):
        self.assertEqual(mod.abc_names(abc(["SetQuestActive", "private-name"])),
                         ["SetQuestActive"])
        self.assertEqual(mod.abc_names(abc([])), [])
        with self.assertRaises(ValueError):
            mod.abc_names(b"broken")

    def test_bounded_swf(self):
        self.assertEqual(mod.scan_swf(swf(["QuestsTab"]))["abc_tags"], 1)
        with self.assertRaises(ValueError):
            mod.scan_swf(swf(["QuestsTab"])[:-1])

    def test_archive_and_symlink_rejection(self):
        parts = {
            "Interface\\PipboyMenu.swf": swf(["BGSCodeObj"]),
            "Interface\\Pipboy_DataPage.swf": swf(["SetQuestActive"]),
        }
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp) / "synthetic.ba2"
            p.write_bytes(ba2(parts))
            got = mod.scan_ba2(p)
            self.assertIn("SetQuestActive",
                          got["Pipboy_DataPage"]["abc_strings_present"])
            self.assertEqual(len(list(Path(temp).iterdir())), 1)
            alias = Path(temp) / "alias.ba2"
            alias.symlink_to(p)
            with self.assertRaises(ValueError):
                mod.scan_ba2(alias)
            p.write_bytes(ba2(dict(list(parts.items())[:1])))
            with self.assertRaises(ValueError):
                mod.scan_ba2(p)


if __name__ == "__main__":
    unittest.main()
