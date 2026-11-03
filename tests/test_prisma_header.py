"""Source-only Prisma header pinning tests. Never fetch or vendor framework code."""
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import prisma_header_pin as pin

spec = importlib.util.spec_from_file_location("stage_prisma_header",
                                            ROOT / "scripts" / "stage-prisma-header.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def fixture():
    # Deliberately not the official header; used only for SHA algorithm checks.
    return b"#pragma once\nnamespace PRISMA_UI_FLAT_API {}\n" + b" " * 4200 + b"PrismaUI_F4_GetAPI\n"


class PinTests(unittest.TestCase):
    def test_git_blob_hash_algorithm(self):
        import hashlib
        data = b"test"
        self.assertEqual(pin.git_blob_sha(data),
                         hashlib.sha1(b"blob 4\0test").hexdigest())

    def test_rejects_unauthorized_header_even_if_api_markers_appear(self):
        self.assertFalse(pin.official_header_matches(fixture()))
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / "wrong.h"
            source.write_bytes(fixture())
            with self.assertRaises(ValueError):
                mod.stage_header(source, Path(folder) / "vendor.h")
            self.assertFalse((Path(folder) / "vendor.h").exists())

    def test_staging_is_idempotent_and_never_overwrites(self):
        # Mock only the pin validator; the positive filesystem path does not
        # need to redistribute the actual licensed official header.
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / "source.h"
            target = Path(folder) / "build-only.h"
            source.write_bytes(fixture())
            with patch.object(mod, "official_header_matches", return_value=True):
                self.assertEqual(mod.stage_header(source, target), "staged")
                self.assertEqual(target.read_bytes(), fixture())
                self.assertEqual(mod.stage_header(source, target), "already staged")
                target.write_bytes(b"different file")
                with self.assertRaises(FileExistsError):
                    mod.stage_header(source, target)
                self.assertEqual(target.read_bytes(), b"different file")

    def test_missing_file_rejected_without_destination_changes(self):
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaises(ValueError):
                mod.stage_header(Path(folder) / "missing.h", Path(folder) / "vendor.h")

    def test_existing_different_header_not_overwritten(self):
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / "official.h"
            target = Path(folder) / "vendor.h"
            source.write_bytes(fixture())
            target.write_bytes(b"keep existing")
            with self.assertRaises(ValueError):
                mod.stage_header(source, target)
            self.assertEqual(target.read_bytes(), b"keep existing")


if __name__ == "__main__":
    unittest.main()
