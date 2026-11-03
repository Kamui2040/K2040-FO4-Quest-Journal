"""Isolated PE export and mod ZIP checks; no Fallout runtime is involved."""
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
from zipfile import ZipFile

SCRIPT=Path(__file__).resolve().parents[1]/"scripts"/"stage-test-package.py"
spec=importlib.util.spec_from_file_location("stage_test_package", SCRIPT)
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def fake_pe(machine=0x8664, dll=True, exports=("F4SEPlugin_Query","F4SEPlugin_Load")):
    """Minimal file-backed export directory fixture; not an executable DLL."""
    blob=bytearray(0x800)
    blob[:2]=b"MZ"
    struct.pack_into("<I",blob,0x3c,0x80)
    blob[0x80:0x84]=b"PE\x00\x00"
    struct.pack_into("<HHIIIHH",blob,0x84,machine,1,0,0,0,240,0x2000 if dll else 0)
    opt=0x98
    struct.pack_into("<H",blob,opt,0x20b)
    struct.pack_into("<I",blob,opt+108,16)
    struct.pack_into("<II",blob,opt+112,0x1100,0x100)
    sec=opt+240
    struct.pack_into("<IIII",blob,sec+8,0x600,0x1000,0x600,0x200)
    n=len(exports)
    struct.pack_into("<IIIII",blob,0x300+20,n,n,0x1140,0x1160,0x1180)
    for i,name in enumerate(exports):
        struct.pack_into("<I",blob,0x340+4*i,0x1300+0x10*i)
        struct.pack_into("<I",blob,0x360+4*i,0x1200+0x20*i)
        struct.pack_into("<H",blob,0x380+2*i,i)
        raw=name.encode("ascii")+b"\x00"
        blob[0x400+0x20*i:0x400+0x20*i+len(raw)]=raw
    return bytes(blob)


class PackageTests(unittest.TestCase):
    def test_rejects_wrong_architecture_and_executable(self):
        for blob in (fake_pe(machine=0x14c), fake_pe(dll=False), b"not a DLL",
                     fake_pe()[:600]):
            with self.subTest(size=len(blob)):
                with self.assertRaises(ValueError):
                    mod.inspect_pe(blob)

    def test_requires_both_unforwarded_loader_exports(self):
        self.assertTrue(mod.inspect_pe(fake_pe()))
        for exports in ((),("F4SEPlugin_Load",),("F4SEPlugin_Query",),("SomeOtherDLL",)):
            with self.subTest(exports=exports):
                with self.assertRaises(ValueError):
                    mod.inspect_pe(fake_pe(exports=exports))
        blob=bytearray(fake_pe())
        # Export pointer table points outside the mapped PE sections.
        struct.pack_into("<I",blob,0x360,0xBAD000)
        with self.assertRaises(ValueError):
            mod.inspect_pe(bytes(blob))
        blob=bytearray(fake_pe())
        # A forwarded export is not an implementation of F4SEPlugin_Query.
        struct.pack_into("<I",blob,0x340,0x1108)
        with self.assertRaises(ValueError):
            mod.inspect_pe(bytes(blob))

    def test_grouped_and_standalone_variants(self):
        common={"Data/F4SE/Plugins/K2040_Quest_Journal.dll":b"test"}
        for mode in ("grouped","standalone"):
            full=mod.with_mcm(common,mode)
            self.assertEqual(len(full),5 if mode=="grouped" else 4)
            self.assertEqual("Data/MCM/Config/K2040_Mods/config.json" in full,mode=="grouped")
            if mode=="grouped":
                self.assertEqual(mod.hashlib.sha256(
                    full[mod.PARENT_PATH]).hexdigest(),mod.CANONICAL_PARENT_SHA256)
            config=json.loads(full["Data/MCM/Config/K2040_Quest_Journal/config.json"])
            self.assertEqual(config.get("ownerModName"),"K2040_Mods" if mode=="grouped" else None)
            self.assertEqual(config["modName"],"K2040_Quest_Journal")
            ids=[a["id"] for a in config["pages"][0]["content"] if a.get("type")=="hotkey"]
            self.assertEqual(ids,["questJournalToggle","questTrackerToggle"])
        self.assertEqual(mod.with_mcm(common,"none"),common)

    def test_requires_every_view_asset(self):
        with tempfile.TemporaryDirectory() as folder:
            base=Path(folder)
            (base/"test.dll").write_bytes(fake_pe())
            (base/"view").mkdir()
            with self.assertRaises(ValueError):
                mod.gather(base/"test.dll",base/"view")

    def test_package_only_expected_files_and_refuses_overwrite(self):
        with tempfile.TemporaryDirectory() as folder:
            base=Path(folder)
            (base/"test.dll").write_bytes(fake_pe())
            (base/"view").mkdir()
            for asset in mod.ASSETS:
                (base/"view"/asset).write_text("test",encoding="utf-8")
            files=mod.gather(base/"test.dll",base/"view")
            for notice in ("LICENSE", "THIRD_PARTY_NOTICES.md", "README.md", "docs/COMPATIBILITY.md"):
                self.assertTrue(files[notice],notice)
            output=base/"test.zip"
            digest=mod.build_zip(files,output)
            self.assertEqual(len(digest),64)
            with ZipFile(output) as archive:
                self.assertEqual(set(archive.namelist()),set(files))
            with self.assertRaises(FileExistsError):
                mod.build_zip(files,output)


if __name__=="__main__":
    unittest.main()
