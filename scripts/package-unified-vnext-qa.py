#!/usr/bin/env python3
"""Deterministically combine Quest Journal and one matching K2040 VNext hub.

Only stage an Amethyst QA archive; never install, deploy, publish or update
a user's MCM keybind registry or any persisted game/mod settings.
"""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[1]
STAGE_SCRIPT=ROOT/"scripts"/"stage-test-package.py"
spec=importlib.util.spec_from_file_location("quest_stage",STAGE_SCRIPT)
stager=importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(stager)

CORE_REQUIRED={
    "Data/F4SE/Plugins/K2040_Unified_Settings.dll",
    "Data/MCM/Config/K2040_Mods/config.json",
    "Data/MCM/Config/K2040_Unified_Settings/config.json",
    "Data/MCM/Config/K2040_Unified_Settings/keybinds.json",
    "Data/MCM/Config/K2040_Unified_Settings/settings.ini",
    "Data/PrismaUI_F4/views/K2040_Settings_Hub/index.html",
    "Data/PrismaUI_F4/views/K2040_Settings_Hub/hub.js",
    "Data/PrismaUI_F4/views/K2040_Settings_Hub/hub-model.js",
    "Data/PrismaUI_F4/views/K2040_Settings_Hub/hub.css",
}
PROVIDER="Data/K2040/Settings/Providers/quest-journal.json"
READ_ME="""K2040 Quest Journal + Unified Settings VNext - OG 1.10.163 QA
Install this ONE ZIP in a separate Amethyst QA profile. This is not a release.
Requirements: original Fallout 4 1.10.163, matching F4SE, PrismaUI_F4 and MCM.
Do not enable another Quest Journal version at the same time.

The bundle contains the Quest Journal, the matching Unified Settings hub
DLL, full-screen hub UI/MCM, and Quest Journal's tagged settings provider.
Use Ctrl+Shift+B to open the central Settings editor (the hub's provisional
fallback) or remap it through K2040 Mods > K2040 Settings in MCM. The Journal
and tracker gameplay shortcuts are configured inside the shared
hub Controls category, never through Quest Journal MCM entries.

The 4 visual themes and 6 built-in palettes are managed globally or per-mod
inside the hub, including user-named custom palettes; the Quest Journal
reads validated transient colors and never edits the hub appearance store.
The Journal's existing local INI settings remain intact. Its one-way
Pip-Boy mirror toggle remains directly in the Journal, not the hub.
The mirror never alters Fallout 4 quests or tracking flags.
The Settings Hub Appearance category now offers "Edit this mod's HUD layout"
when the mini tracker is visible. Click/drag to move and use the hub's
mouse wheel (+/-1%) or +/- keys (+/-10%) to resize proportionally.
Esc offers Save, Abort or Retry; only Save writes Quest Journal's own INI.
Older corner/width preferences remain effective until a live layout is saved.
Changing legacy width or anchor later exits the custom geometry mode.

All source/Windows x64 build checks are separate from actual in-game tests.
Check opening, editing/persistence, theme + palette overrides, gamepad
accept/back, quest tracking, and save/reload before any release promotion.
This archive contains no mod save files, MCM Settings registry or game assets.
See the owner's docs/UNIFIED_SETTINGS_VNEXT_QA.md for source/check evidence.
"""
def git(repo:Path,*args:str)->str:
    return subprocess.check_output(["git","-C",str(repo),*args],text=True).strip()
def exports(dll:Path)->str:
    return subprocess.check_output(["llvm-readobj","--coff-exports",str(dll)],text=True)
def assert_exports(dll:Path, names:tuple[str,...])->None:
    data=dll.read_bytes()
    stager.inspect_pe(data)
    output=exports(dll)
    for name in names:
        if "Name: "+name not in output:
            raise ValueError(f"{dll.name} lacks required {name} export")
def gather_core(hub:Path)->dict[str,bytes]:
    stage=hub/"dist"/"Data"
    if not stage.is_dir():
        raise ValueError("hub has no built dist/Data stage")
    collected={}
    for file in stage.rglob("*"):
        if file.is_symlink():
            raise ValueError("hub stage symlink forbidden")
        if not file.is_file():
            continue
        relative="Data/"+file.relative_to(stage).as_posix()
        if relative not in CORE_REQUIRED:
            raise ValueError("unexpected hub stage file "+relative)
        if not 0<file.stat().st_size<10_000_000:
            raise ValueError("invalid hub stage size "+relative)
        collected[relative]=file.read_bytes()
        if not relative.endswith(".dll"):
            source=hub/relative
            if not source.is_file() or collected[relative]!=source.read_bytes():
                raise ValueError("hub staged asset differs from checked out source: "+relative)
    if set(collected)!=CORE_REQUIRED:
        raise ValueError("incomplete hub core: "+str(sorted(CORE_REQUIRED-set(collected))))
    assert_exports(stage/"F4SE"/"Plugins"/"K2040_Unified_Settings.dll",
                   ("K2040_CopyAppearanceV1","K2040_CopyEffectiveThemeV1"))
    keys=json.loads(collected["Data/MCM/Config/K2040_Unified_Settings/keybinds.json"])
    if [x["id"] for x in keys.get("keybinds",[])]!=["openUnifiedSettings"]:
        raise ValueError("hub MCM registry definition mismatch")
    return collected
def build(hub:Path,output:Path)->tuple[int,int,str]:
    hub=hub.resolve()
    if output.exists() or output.is_symlink():
        raise FileExistsError("refusing existing QA archive")
    if ROOT in output.resolve().parents or hub in output.resolve().parents:
        raise ValueError("QA package output cannot be inside a source checkout")
    if git(ROOT,"branch","--show-current")!="feature/unified-editor-v2":
        raise ValueError("wrong Quest Journal branch")
    if git(hub,"branch","--show-current")!="feature/unified-settings-editor-v2":
        raise ValueError("wrong Unified Settings branch")
    if git(ROOT,"status","--porcelain") or git(hub,"status","--porcelain"):
        raise ValueError("uncommitted source or hub changes; package provenance blocked")
    if (ROOT/"src/K2040SettingsAdapterV2.h").read_bytes()!=\
       (hub/"include/K2040SettingsAdapterV2.h").read_bytes():
        raise ValueError("native settings V2 ABI header drift")
    if (ROOT/"src/K2040LayoutAdapterV2.h").read_bytes()!=\
       (hub/"include/K2040LayoutAdapterV2.h").read_bytes():
        raise ValueError("native layout V2 ABI header drift")
    dll=ROOT/"dist/Data/F4SE/Plugins/K2040_Quest_Journal.dll"
    assert_exports(dll,("K2040_GetSettingsAdapterV2","K2040_GetLayoutAdapterV2"))
    files=stager.with_mcm(stager.gather(dll,ROOT/"view"),"grouped")
    provider=json.loads(files[PROVIDER])
    if provider["modId"]!="quest-journal" or provider["plugin"]!=dll.name:
        raise ValueError("Quest Journal provider identity mismatch")
    for name,data in gather_core(hub).items():
        if name in files and files[name]!=data:
            raise ValueError("colliding shared hub/owner asset "+name)
        files[name]=data
    if any(n.startswith("Data/MCM/Config/K2040_Quest_Journal/") for n in files):
        raise ValueError("obsolete Quest Journal MCM child must not ship")
    if {n for n in files if n.endswith("/keybinds.json")} != {
        "Data/MCM/Config/K2040_Unified_Settings/keybinds.json"}:
        raise ValueError("the shared Settings Hub must own the sole MCM keybind")
    if any(n.startswith("Data/MCM/Settings/") or
           n.endswith("/FO4-Quest-Journal.ini") for n in files):
        raise ValueError("unexpected user preference or registry in QA archive")
    files["UNIFIED-SETTINGS-QA-README.txt"]=READ_ME.encode("utf-8")
    files["UNIFIED-SETTINGS-LICENSE"]= (hub/"LICENSE").read_bytes()
    files["SOURCE-REVISIONS.txt"]=(
       "Quest Journal "+git(ROOT,"rev-parse","HEAD")+"\n"+
       "Unified Settings "+git(hub,"rev-parse","HEAD")+"\n").encode()
    sha=stager.build_zip(files,output)
    with zipfile.ZipFile(output) as archive:
        if archive.testzip() is not None or set(archive.namelist())!=set(files):
            raise ValueError("created QA archive failed integrity check")
        for name,data in files.items():
            if archive.read(name)!=data:
                raise ValueError("created QA archive has wrong bytes "+name)
    return len(files),output.stat().st_size,sha

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--hub-root",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    try:
        count,size,sha=build(args.hub_root,args.output)
    except (ValueError,OSError,subprocess.CalledProcessError,KeyError,zipfile.BadZipFile) as error:
        print("FAIL QA packaging:",error,file=sys.stderr)
        sys.exit(1)
    print(f"PASS Amethyst QA archive: {count} files, {size} bytes")
    print("SHA256:",sha)
    print("OUTPUT:",args.output)
    print("No installation, deployment or gameplay test was performed.")
