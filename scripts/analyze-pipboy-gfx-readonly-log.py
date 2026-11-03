#!/usr/bin/env python3
"""Validate one private OG163 read-only native GFx selection capture."""
from __future__ import annotations
import argparse
import json
import re
from pathlib import Path

FILENAME=re.compile(
    r"K2040_Quest_Journal_PipboyGfx_ReadOnly_\d{8}_\d{6}_\d+\.log")
STATUSES={
    "menu-unavailable", "page-unavailable","tab-unavailable",
    "quest-selection-unavailable","root-identity-unavailable",
    "ordinary-root-selected","misc-root-child-not-selected",
    "native-misc-list-child-selected"}
PATTERN=re.compile(
    r"PipboyGfxReadOnlyQA: result=([a-z-]+) "
    r"rootFormIDTyped=(true|false) syntheticMiscParent=(true|false) "
    r"nativeListSelection=(true|false) childFormIDTyped=(true|false) "
    r"childInstanceTyped=(true|false) childTypeTyped=(true|false) "
    r"childQuestTypeTyped=(true|false) tripleVerified=(true|false) "
    r"writeAuthorized=(true|false) \(NO game mutation\)")
START="PipboyGfxReadOnlyQA: READY;"
END="PipboyGfxReadOnlyQA: END one-shot read-only"
SUBMIT="PipboyGfxReadOnlyQA: UI task submitted"


def inspect(content: str) -> dict:
    if content.count(START)!=1 or content.count(SUBMIT)!=1 or content.count(END)!=1:
        raise ValueError("incomplete or repeated QA lifecycle")
    records=PATTERN.findall(content)
    if len(records)!=1 or records[0][0] not in STATUSES:
        raise ValueError("missing, repeated, or invalid GFx observation")
    kind=records[0][0]
    flags=[x=="true" for x in records[0][1:]]
    root,misc,from_list,form,instance,kind_field,quest_type,triple,writer=flags
    if writer or triple:
        raise ValueError("unexpected native write or command-triple authorization")
    if kind=="native-misc-list-child-selected":
        if not root or not misc or not from_list:
            raise ValueError("inconsistent Misc selection provenance")
    elif any((from_list,form,instance,kind_field,quest_type)):
        raise ValueError("nested selection fields reported outside Misc list")
    if kind=="ordinary-root-selected" and (not root or misc):
        raise ValueError("inconsistent ordinary root")
    if kind=="misc-root-child-not-selected" and (not root or not misc):
        raise ValueError("inconsistent synthetic Misc parent")
    return {
        "capture":"complete-read-only", "selection_status":kind,
        "root_id_typed":root, "synthetic_misc_parent":misc,
        "native_list_selection":from_list,
        "child_field_presence":{
            "formID":form,"instance":instance,
            "type":kind_field,"questType":quest_type},
        "native_command_triple_verified":False,
        "native_write_authorized":False}


def main(argv=None) -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("log",type=Path)
    args=p.parse_args(argv)
    q=args.log
    if q.is_symlink() or not FILENAME.fullmatch(q.name) or not q.is_file():
        p.error("expected a private per-session regular QA log")
    if q.stat().st_size>2_000_000:
        p.error("QA log exceeds 2 MiB bound")
    try:
        report=inspect(q.read_text(encoding="utf-8",errors="replace"))
    except ValueError as error:
        p.error(str(error))
    print(json.dumps(report,indent=2))
    return 0


if __name__=="__main__":
    raise SystemExit(main())
