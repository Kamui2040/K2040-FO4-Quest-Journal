#!/usr/bin/env python3
"""Build one-file opt-in native MCM Categorizer page override.

Default Quest Journal stays grouped under K2040 Mods. This overlay
modifies ONLY the Quest Journal's child config; no third-party or other
K2040 mod files and no user MCM settings are included.
"""
import argparse
import hashlib
import json
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "mcm/Data/MCM/Config/K2040_Quest_Journal/config.json"
TARGET = "Data/MCM/Config/K2040_Quest_Journal/config.json"


def ungroup(source: bytes) -> bytes:
    original = json.loads(source)
    assert original.get("modName") == "K2040_Quest_Journal"
    assert original.get("ownerModName") == "K2040_Mods"
    assert len(original.get("pages", [])) == 1
    assert original["pages"][0].get("pageDisplayName") == "Quest Journal"
    ids = [item["id"] for item in original["pages"][0]["content"]
           if item.get("type") == "hotkey"]
    assert ids == ["questJournalToggle", "questTrackerToggle"]
    original.pop("ownerModName")
    return (json.dumps(original, ensure_ascii=False, indent=2) + "\n").encode()


def package(dest: Path) -> str:
    dest = dest.resolve()
    if dest == ROOT or ROOT in dest.parents:
        raise ValueError("Compatibility ZIP must be outside the repository")
    if dest.exists():
        raise FileExistsError("Will not overwrite an existing test package")
    content = ungroup(SOURCE.read_bytes())
    dest.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(dest, "x", compression=ZIP_DEFLATED, compresslevel=9) as z:
        item = ZipInfo(TARGET, (1980, 1, 1, 0, 0, 0))
        item.compress_type = ZIP_DEFLATED
        z.writestr(item, content)
    with ZipFile(dest) as z:
        assert z.namelist() == [TARGET] and z.testzip() is None
        assert z.read(TARGET) == content
    return hashlib.sha256(dest.read_bytes()).hexdigest()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print("PASS one-file optional MCM Categorizer overlay", package(args.output))
