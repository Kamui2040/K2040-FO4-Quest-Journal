import importlib
import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "scripts"))
module = importlib.import_module("inspect-og-quest-setactive")


class OgQuestSetActiveInspectorTests(unittest.TestCase):
    def test_canonical_ids_and_offsets(self):
        self.assertEqual(module.CANDIDATES, (
            ("TESQuest::GetActive", 690102, 0x05D6AA0),
            ("TESQuest::SetActive", 1567562, 0x05D6AC0),
        ))

    def test_mapping_supports_out_of_order_rows_and_hex(self):
        found = module.lookup_ids(iter([
            "id,fo4_addr\n",
            "10,001000\n",
            "1567562,05D6AC0\n",
            "690102,05D6AA0\n",
        ]), {690102, 1567562})
        self.assertEqual(found, {690102: 0x05D6AA0, 1567562: 0x05D6AC0})

    def test_installed_binary_sparse_ids(self):
        records = [(0, 0x1333020), (1, 0x806ED0),
                   (690102, 0x5D6AA0), (1567562, 0x5D6AC0)]
        blob = struct.pack("<Q", len(records)) + b"".join(
            struct.pack("<QQ", *record) for record in records
        )
        self.assertEqual(
            module.lookup_installed_address_library(blob, {690102, 1567562}),
            {690102: 0x5D6AA0, 1567562: 0x5D6AC0},
        )
        with self.assertRaises(ValueError):
            module.lookup_installed_address_library(blob, {42})

    def test_invalid_binary_header_fails_closed(self):
        with self.assertRaises(ValueError):
            module.lookup_installed_address_library(b"", {690102})
        with self.assertRaises(ValueError):
            module.lookup_installed_address_library(struct.pack("<Q", 2), {690102})

    def test_missing_or_malformed_header_fails_closed(self):
        with self.assertRaises(ValueError):
            module.lookup_ids(iter(["rva,id\n", "690102,05D6AA0\n"]), {690102})

    def test_missing_identifier_fails_closed(self):
        with self.assertRaises(ValueError):
            module.lookup_ids(iter(["id,fo4_addr\n", "690102,05D6AA0\n"]), {690102, 1567562})

    def test_duplicate_identifier_fails_closed(self):
        with self.assertRaises(ValueError):
            module.lookup_ids(iter(["id,fo4_addr\n", "690102,05D6AA0\n", "690102,05D6AA0\n"]), {690102})


if __name__ == "__main__":
    unittest.main()
