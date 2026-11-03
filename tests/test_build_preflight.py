"""Preflight tests use fixtures only; no Windows SDK, game, or DLL is needed."""
import importlib.util
from pathlib import Path
import tempfile
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "build-preflight.py"
spec = importlib.util.spec_from_file_location("build_preflight", SCRIPT)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class PreflightTests(unittest.TestCase):
    def test_missing_deps_fail_closed(self):
        with tempfile.TemporaryDirectory() as t:
            result = mod.inspect(t, t, "linux", which=lambda _: None, get_revision=lambda _: None)
            self.assertFalse(all(ok for _, ok, _ in result))
            self.assertFalse(dict((label, ok) for label, ok, _ in result)["CommonLibF4 pinned revision"])

    def test_recognizes_extracted_sdk(self):
        with tempfile.TemporaryDirectory() as t:
            version = Path(t) / "Windows Kits" / "10" / "Include" / "10.0.26100.0"
            for name in ("shared", "um", "ucrt"):
                (version / name).mkdir(parents=True)
            self.assertEqual(mod.sdk_include_roots(t), [version])

    def test_incomplete_sdk_rejected(self):
        with tempfile.TemporaryDirectory() as t:
            version = Path(t) / "Windows Kits" / "10" / "Include" / "10.0.26100.0"
            (version / "shared").mkdir(parents=True)
            self.assertEqual(mod.sdk_include_roots(t), [])


if __name__ == "__main__":
    unittest.main()
