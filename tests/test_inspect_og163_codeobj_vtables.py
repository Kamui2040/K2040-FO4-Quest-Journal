"""Synthetic PE vtable provenance checks. Contains no game executable bytes."""
import importlib.util
from pathlib import Path
import struct
import unittest

from test_quest_command_rtti import fake_pe

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "codeobj_rtti", ROOT / "scripts/inspect-og163-codeobj-vtables.py")
inspector = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inspector)


def triple_fixture():
    """Three synthetic RTTI classes in one bounded x64 PE image."""
    buf = bytearray(fake_pe())
    image = 0x140000000
    for i, cls in enumerate(inspector.CLASS_NAMES):
        step = i * 0x240
        type_at = 0x710 + step
        col_at = 0x850 + step
        ptr_at = 0x8F8 + step
        table_at = 0x900 + step
        name = (".?AV" + cls + "@@").encode() + bytes([0])
        buf[type_at - 16:type_at + len(name)] = bytes(16) + name
        type_rva = 0x2000 + type_at - 16 - 0x600
        col_rva = 0x2000 + col_at - 0x600
        struct.pack_into("<IIIIII", buf, col_at, 1, 0, 0, type_rva,
                         0x2380, col_rva)
        struct.pack_into("<Q", buf, ptr_at, image + col_rva)
        call_rva = 0x1020 if i < 2 else 0x1070
        map_rva = 0x1030 if i < 2 else 0x1080
        struct.pack_into("<QQQQ", buf, table_at,
                         image + 0x1010 + 16*i,
                         image + call_rva,
                         image + map_rva,
                         image + 0x1040)
    return bytes(buf)


class OG163CodeObjectVtableTests(unittest.TestCase):
    def test_three_distinct_classes_and_pipboy_overrides(self):
        result = inspector.compare_vtables(triple_fixture())
        self.assertTrue(result["pipboy_overrides_both_slots"])
        self.assertFalse(result["native_callback_abi_verified"])
        self.assertFalse(result["native_write_authorized"])
        self.assertEqual(
            result["virtual_slot_rvas"]["PipboyMenu"]["MapCodeObjectFunctions"],
            "0x1080")

    def test_comparison_denies_missing_override(self):
        data = bytearray(triple_fixture())
        struct.pack_into("<Q", data, 0x900 + 2*0x240 + 2*8,
                         0x140000000 + 0x1030)
        with self.assertRaisesRegex(ValueError, "override evidence"):
            inspector.compare_vtables(bytes(data))

    def test_primary_col_requires_zero_offset(self):
        data = bytearray(triple_fixture())
        struct.pack_into("<I", data, 0x850 + 2*0x240 + 4, 16)
        with self.assertRaisesRegex(ValueError, "primary RTTI"):
            inspector.compare_vtables(bytes(data))

    def test_rejects_bad_pe_and_wrong_type_name(self):
        with self.assertRaisesRegex(ValueError, "bounded"):
            inspector.compare_vtables(b"bad")
        module = inspector.rtti_module()
        with self.assertRaisesRegex(ValueError, "RTTI type name"):
            module.find_vtables(triple_fixture(), b"not-a-class")
        self.assertEqual(module.find_vtables(triple_fixture(),
            b".?AVMissingClass@@"+bytes([0])), [])


if __name__ == "__main__":
    unittest.main()
