"""Build-output gate tests; use synthetic PE data, never game binaries."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("check_built_dll", ROOT/"scripts"/"check-built-dll.py")
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

# Reuse existing synthetic PE fixture, not a real F4SE DLL.
from test_stage_test_package import fake_pe


class BuildDllTests(unittest.TestCase):
    def test_rejects_missing_wrong_exports_and_non_dll(self):
        with tempfile.TemporaryDirectory() as temp:
            folder=Path(temp)
            with self.assertRaises(ValueError):
                mod.check_dll(folder/"missing.dll")
            for i,blob in enumerate((b"bad", fake_pe(exports=("OnlyOtherExport",)),
                                     fake_pe(dll=False), fake_pe(machine=0x14c))):
                path=folder/f"invalid{i}.dll"
                path.write_bytes(blob)
                with self.subTest(index=i), self.assertRaises(ValueError):
                    mod.check_dll(path)

    def test_rejects_symlinks(self):
        with tempfile.TemporaryDirectory() as temp:
            folder=Path(temp)
            real=folder/"payload.dll"
            real.write_bytes(fake_pe())
            alias=folder/"link.dll"
            alias.symlink_to(real)
            with self.assertRaises(ValueError):
                mod.check_dll(alias)

    def test_shipping_rejects_every_known_experimental_writer_marker(self):
        for marker in mod.FORBIDDEN_SHIPPING_MARKERS:
            with self.subTest(marker=marker), self.assertRaises(ValueError):
                mod.reject_experimental_writer_markers(fake_pe()+marker)
        mod.reject_experimental_writer_markers(fake_pe())
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/"synthetic.dll"
            path.write_bytes(fake_pe())
            info=mod.check_dll(path,require_readonly_shipping=True)
            self.assertEqual(info["size"],len(fake_pe()))

    def test_accepts_exported_x64_pe_and_returns_digest(self):
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/"journal.dll"
            path.write_bytes(fake_pe())
            result=mod.check_dll(path)
            self.assertEqual(result["size"], len(fake_pe()))
            self.assertEqual(len(result["sha256"]), 64)


if __name__=="__main__":
    unittest.main()
