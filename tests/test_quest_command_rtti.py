"""Synthetic x64 PE/RTTI contracts; never bundle or inspect game assets in tests."""
import importlib.util
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "quest_command_rtti", ROOT / "scripts" / "inspect-quest-command-rtti.py")
inspector = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inspector)


def fake_pe(valid_col=True, executable=True, has_type=True):
    data = bytearray(0x1000)
    struct.pack_into("<2sI", data, 0, b"MZ", 0)
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HH", data, 0x84, 0x8664, 2)
    struct.pack_into("<H", data, 0x84 + 16, 0xF0)
    struct.pack_into("<H", data, 0x98, 0x20B)
    image = 0x140000000
    struct.pack_into("<Q", data, 0x98 + 24, image)
    for index, name, va, raw, size, flags in (
        (0, b".text", 0x1000, 0x400, 0x200, 0x60000020 if executable else 0x40000040),
        (1, b".rdata", 0x2000, 0x600, 0x800, 0x40000040),
    ):
        offset = 0x188 + index * 40
        data[offset:offset + len(name)] = name
        struct.pack_into("<IIII", data, offset + 8, size, va, size, raw)
        struct.pack_into("<I", data, offset + 36, flags)
    if has_type:
        data[0x710:0x710 + len(inspector.TYPE_NAME)] = inspector.TYPE_NAME
    struct.pack_into("<IIIIII", data, 0x850, 1, 0, 0, 0x2100,
                     0x2380, 0x2250 if valid_col else 0x2251)
    struct.pack_into("<Q", data, 0x8F8, image + 0x2250)
    struct.pack_into("<QQQQ", data, 0x900, *(image + 0x1010 + 16 * i for i in range(4)))
    return bytes(data)


class CommandRttiTests(unittest.TestCase):
    def test_identifies_structural_vtable_only(self):
        self.assertEqual(inspector.find_vtables(fake_pe()),
                         [(0x2100, 0x2250, 0x2300,
                           (0x1010, 0x1020, 0x1030, 0x1040))])

    def test_rejects_missing_type_bad_self_and_nonexecutable_methods(self):
        for change in (dict(has_type=False), dict(valid_col=False),
                       dict(executable=False)):
            with self.subTest(change=change):
                self.assertEqual(inspector.find_vtables(fake_pe(**change)), [])

    def test_rejects_malformed_and_non_pe32plus_input(self):
        with self.assertRaises(ValueError):
            inspector.find_vtables(b"bad")
        image = bytearray(fake_pe())
        struct.pack_into("<H", image, 0x84, 0x14C)
        with self.assertRaisesRegex(ValueError, "x64"):
            inspector.find_vtables(bytes(image))
        image = bytearray(fake_pe())
        struct.pack_into("<I", image, 0x188 + 20, 0xFFFF)
        with self.assertRaisesRegex(ValueError, "section extends"):
            inspector.find_vtables(bytes(image))


if __name__ == "__main__":
    unittest.main()
