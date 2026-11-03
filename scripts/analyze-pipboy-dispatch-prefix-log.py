#!/usr/bin/env python3
"""Validate bounded private OG163 PipboyMenu native prefixes; never export game code."""
import argparse
from hashlib import sha256
import importlib.util
import json
from pathlib import Path
import re

SOURCE=Path(__file__).with_name("analyze-pipboy-codeobj-type-log.py")
spec=importlib.util.spec_from_file_location("og_codeobj_type",SOURCE)
typeqa=importlib.util.module_from_spec(spec)
spec.loader.exec_module(typeqa)
STATUS=re.compile(
    r"PipboyDispatchNativeReadOnlyQA: status=([a-z-]+) "
    r"callbackAbiVerified=(true|false) writeAuthorized=(true|false) "
    r"\(NO invocation\)")
WINDOW=re.compile(
    r"PipboyDispatchNativeReadOnlyQA: method=(call|map) "
    r"window=(\d+) rva=(\d+) hex=([0-9a-f]+)")
EXPECTED={"call":0x00B94080,"map":0x00B93D30}
ERRORS={"module-unavailable","dos-unreadable","dos-invalid","nt-unreadable",
        "pe-mismatch","vtable-unreadable","vtable-mismatch","code-unreadable"}
WINDOW_BYTES=192
WINDOW_COUNT=4

def inspect(text):
    base=typeqa.inspect(text)
    if base["member_type"]!="object" or base["binding_state"]!="setquestactive-not-closure":
        raise ValueError("native prefix requires verified object binding")
    if text.count("PipboyDispatchNativeReadOnlyQA: status=")!=1:
        raise ValueError("not one dispatch status")
    matches=STATUS.findall(text)
    if len(matches)!=1:
        raise ValueError("invalid status record")
    status,abi,writer=matches[0]
    if abi!="false" or writer!="false":
        raise ValueError("forbidden write or ABI authorization")
    windows=WINDOW.findall(text)
    total=text.count("PipboyDispatchNativeReadOnlyQA: method=")
    if len(windows)!=total:
        raise ValueError("malformed native prefix record")
    if status in ERRORS:
        if windows:raise ValueError("captured bytes after failed gate")
        return {"status":status,"capture":"read-only-blocked",
                "callback_abi_verified":False,"write_authorized":False}
    if status!="captured" or len(windows)!=2*WINDOW_COUNT:
        raise ValueError("incomplete native prefix windows")
    samples={}
    for label,base_rva in EXPECTED.items():
        pieces=[(int(index),int(rva),hx)
                for kind,index,rva,hx in windows if kind==label]
        if len(pieces)!=WINDOW_COUNT:
            raise ValueError("missing or duplicate method window")
        pieces.sort()
        if [i for i,_,_ in pieces]!=list(range(WINDOW_COUNT)):
            raise ValueError("out-of-order or repeated window")
        for index,rva,hx in pieces:
            if rva!=base_rva+index*WINDOW_BYTES or len(hx)!=WINDOW_BYTES*2:
                raise ValueError("unexpected RVA or byte length")
        # No game code is printed, exported or written to files.
        data=bytes.fromhex("".join(hx for _,_,hx in pieces))
        samples[label]={"rva":hex(base_rva),"bytes":len(data),
                        "sha256":sha256(data).hexdigest()}
    return {"status":"captured","capture":"read-only-bounded",
            "methods":samples,"callback_abi_verified":False,
            "write_authorized":False}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("log",type=Path)
    args=p.parse_args()
    q=args.log
    if (q.is_symlink() or not q.is_file() or
        not typeqa.load_schema().BASE_MODULE.FILENAME.fullmatch(q.name) or
        q.stat().st_size>2_000_000):
        p.error("expected bounded private F4SE read-only session log")
    try:
        result=inspect(q.read_text(encoding="utf8",errors="replace"))
    except ValueError as exc:p.error(str(exc))
    print(json.dumps(result,indent=2))

if __name__=="__main__":main()
