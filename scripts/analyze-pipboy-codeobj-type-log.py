#!/usr/bin/env python3
"""Validate one private OG163 code-object type census without exposing gameplay data."""
import argparse
import importlib.util
import json
from pathlib import Path
import re

def load_schema():
    source=Path(__file__).with_name("analyze-pipboy-gfx-schema-readonly-log.py")
    spec=importlib.util.spec_from_file_location("og163_schema_qa",source)
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

LINE=re.compile(
    r"PipboyCodeObjectReadOnlyQA: state=([a-z-]+) memberType=([a-z-]+) "
    r"callbackAbiVerified=(true|false) writeAuthorized=(true|false) "
    r"\(NO invocation\)")
PRESENT={"setquestactive-not-closure","setquestactive-closure"}
MISSING={"menu-unavailable","code-object-unavailable",
         "code-object-wrong-type","setquestactive-absent"}
TYPES={"undefined","null","boolean","int","uint","number",
       "string","stringw","object","array","display-object",
       "closure","unknown"}

def inspect(text):
    report=load_schema().inspect(text)
    if text.count("PipboyCodeObjectReadOnlyQA:")!=1:
        raise ValueError("missing or repeated binding observation")
    values=LINE.findall(text)
    if len(values)!=1:
        raise ValueError("unrecognized binding record")
    state,member,abi,writer=values[0]
    if abi!="false" or writer!="false":
        raise ValueError("forbidden ABI or native writer authorization")
    if state in PRESENT:
        if member not in TYPES:
            raise ValueError("unrecognized observed member type")
        if (state=="setquestactive-closure")!=(member=="closure"):
            raise ValueError("inconsistent closure classification")
    elif state in MISSING:
        if member!="not-observed":
            raise ValueError("observed type without known member")
    else:
        raise ValueError("unrecognized binding state")
    return {"capture":"complete-read-only","binding_state":state,
            "member_type":member,"native_callback_abi_verified":False,
            "native_write_authorized":False,
            "selection_status":report["selection"]}

def main(argv=None):
    args=argparse.ArgumentParser(description=__doc__)
    args.add_argument("log",type=Path)
    path=args.parse_args(argv).log
    if (path.is_symlink() or not path.is_file()
            or not load_schema().BASE_MODULE.FILENAME.fullmatch(path.name)
            or path.stat().st_size>2_000_000):
        args.error("require bounded, private, regular F4SE QA session log")
    try:
        output=inspect(path.read_text(encoding="utf-8",errors="replace"))
    except ValueError as error:
        args.error(str(error))
    print(json.dumps(output,indent=2))
    return 0

if __name__=="__main__":
    raise SystemExit(main())
