import json
import unittest
from pathlib import Path
from importlib.util import spec_from_file_location, module_from_spec
from tempfile import TemporaryDirectory
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
SPEC = spec_from_file_location('mcm_categorizer', ROOT/'scripts/package-mcm-categorizer-compat.py')
MOD = module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


class McmCategorizerCompatibility(unittest.TestCase):
    def test_exact_one_file_standalone_override(self):
        original = json.loads(MOD.SOURCE.read_bytes())
        converted = json.loads(MOD.ungroup(MOD.SOURCE.read_bytes()))
        self.assertNotIn("ownerModName", converted)
        orig = dict(original)
        orig.pop("ownerModName")
        self.assertEqual(converted, orig)
        with TemporaryDirectory() as root:
            target = Path(root)/'override.zip'
            self.assertEqual(len(MOD.package(target)), 64)
            with ZipFile(target) as z:
                self.assertEqual(z.namelist(), [MOD.TARGET])
                self.assertIsNone(z.testzip())
                self.assertEqual(json.loads(z.read(MOD.TARGET)), converted)
            with self.assertRaises(FileExistsError):
                MOD.package(target)

    def test_does_not_accept_other_mod_or_grouping(self):
        obj=json.loads(MOD.SOURCE.read_bytes())
        obj["modName"]="OtherMod"
        with self.assertRaises(AssertionError):
            MOD.ungroup(json.dumps(obj).encode())
        obj["modName"]="K2040_Quest_Journal"
        obj["ownerModName"]="OtherParent"
        with self.assertRaises(AssertionError):
            MOD.ungroup(json.dumps(obj).encode())
