"""Synthetic-only native command-name PE metadata fixture."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "pe_names", ROOT / "scripts/inspect-quest-interface-names.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
rtti_spec = importlib.util.spec_from_file_location(
    "pe_fixture", ROOT / "tests/test_quest_command_rtti.py")
fixture = importlib.util.module_from_spec(rtti_spec)
rtti_spec.loader.exec_module(fixture)


def sample():
    data = bytearray(fixture.fake_pe(has_type=False))
    for offset, name in zip((0xA10, 0xA50, 0xA90), module.CALLBACK_NAMES):
        raw = name.encode("ascii") + b"\0"
        data[offset:offset + len(raw)] = raw
    return bytes(data)


class CallbackMetadataTests(unittest.TestCase):
    def test_names_are_metadata_not_function_pointers(self):
        result = module.scan(sample())
        self.assertEqual(result["status"], "callback-names-in-PE-metadata-only")
        self.assertFalse(result["native_callback_address_proven"])
        self.assertFalse(result["selected_entry_fields_proven"])
        self.assertFalse(result["safe_to_invoke_native_callback"])
        self.assertEqual([x["section"] for x in result["names"]],
                         [".rdata"] * 3)
        self.assertEqual([x["rva"] for x in result["names"]],
                         ["0x2410", "0x2450", "0x2490"])

    def test_rip_patterns_are_untrusted_byte_candidates(self):
        import struct
        data = bytearray(sample())
        # Synthetic x64 lea rcx, [rip+displacement] at code RVA 0x1010
        # into the test string RVA 0x2410; no game code is copied.
        offset, entry_rva, string_rva = 0x410, 0x1010, 0x2410
        data[offset:offset + 7] = (
            b"\x48\x8d\x0d" +
            struct.pack("<i", string_rva - (entry_rva + 7)))
        result = module.scan(bytes(data))
        name = result["names"][0]
        self.assertEqual(name["on_disk_rip_pattern_candidate_count"], 1)
        self.assertEqual(name["on_disk_rip_pattern_rvas"], ["0x1010"])
        self.assertFalse(result["on_disk_instruction_boundaries_proven"])
        self.assertFalse(result["loaded_image_code_correspondence_proven"])
        self.assertFalse(result["native_callback_address_proven"])
        # Exclude a matching displacement if ModRM is not RIP-relative.
        data[offset + 2] = 0x11
        self.assertEqual(
            module.scan(bytes(data))["names"][0]
            ["on_disk_rip_pattern_candidate_count"], 0)

    def test_missing_duplicate_and_wrong_section(self):
        data = bytearray(sample())
        data[0xA10:0xA10 + len(module.CALLBACK_NAMES[0])] = b"x" * len(module.CALLBACK_NAMES[0])
        with self.assertRaisesRegex(ValueError, "onQuestSelection"):
            module.scan(bytes(data))
        data = bytearray(sample())
        entry = b"onQuestSelection\0"
        data[0xB00:0xB00 + len(entry)] = entry
        with self.assertRaisesRegex(ValueError, "onQuestSelection"):
            module.scan(bytes(data))
        data = bytearray(sample())
        data[0xA10:0xA10 + len(entry)] = b"x" * len(entry)
        data[0x450:0x450 + len(entry)] = entry  # executable .text
        with self.assertRaisesRegex(ValueError, "onQuestSelection"):
            module.scan(bytes(data))

    def test_malformed_and_bounded(self):
        with self.assertRaises(ValueError):
            module.scan(b"not a PE")
        with self.assertRaisesRegex(ValueError, "bounded"):
            module.scan(b"x" * (module.MAX_EXE_BYTES + 1))


if __name__ == "__main__":
    unittest.main()
