#!/usr/bin/env python3
"""Validate an anonymous isolated OG163 GFx selectedEntry schema census."""
import argparse
import importlib.util
import json
import re
from pathlib import Path
BASE=Path(__file__).resolve().parent
SPEC=importlib.util.spec_from_file_location("base_gfx_readonly",BASE/"analyze-pipboy-gfx-readonly-log.py")
BASE_MODULE=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE_MODULE)
# Must match the fixed compile-time name whitelist in ReadOnlySelectedSchema.h.
# This does not inspect, recover or print any unnamed properties.
ALLOWED_NAMES=("formID", "instance", "type", "questType", "active", "enabled",
               "objective", "index", "id", "questID", "questFormID", "instanceID",
               "category", "sortval", "text", "description")

def decode_names(mask: int)->list[str]:
    return [name for index,name in enumerate(ALLOWED_NAMES) if mask & (1 << index)]

PATTERN=re.compile(
    r"PipboySchemaReadOnlyQA: available=(true|false) overflow=(true|false) "
    r"total=(\d+) integer=(\d+) number=(\d+) boolean=(\d+) string=(\d+) "
    r"object=(\d+) array=(\d+) other=(\d+) nameMask=(\d+) numericNameMask=(\d+) "
    r"ordinalMask=(\d+) numericOrdinalMask=(\d+) "
    r"tripleVerified=(true|false) writeAuthorized=(true|false)")

def inspect(text: str)->dict:
    baseline=BASE_MODULE.inspect(text)
    if baseline["selection_status"]!="native-misc-list-child-selected":
        raise ValueError("schema requires a validated native Misc selection")
    results=PATTERN.findall(text)
    if len(results)!=1:
        raise ValueError("schema capture missing, invalid or repeated")
    available,overflow,*rest=results[0]
    *numbers,triple,writer=rest
    total,integer,number,boolean,string,object_,array,other,mask,numeric_mask,ordinal,numeric_ordinal=map(int,numbers)
    if available!="true" or overflow!="false" or triple!="false" or writer!="false":
        raise ValueError("unsafe, unavailable or truncated schema")
    if not 1<=total<=48 or sum((integer,number,boolean,string,object_,array,other))!=total:
        raise ValueError("schema counts inconsistent")
    if not 0<=mask<=65535 or not 0<=ordinal<=7 or not 0<=numeric_mask<=65535 or not 0<=numeric_ordinal<=7 or (numeric_mask & ~mask) or (numeric_ordinal & ~ordinal):
        raise ValueError("schema bitmask invalid")
    known_count=mask.bit_count()
    numeric_count=numeric_mask.bit_count()
    if known_count>total or numeric_count>integer+number or ordinal.bit_count()>total:
        raise ValueError("schema masks exceed member or numeric counts")
    return {"status":"anonymous-schema-read-only", "selection":baseline["selection_status"],
            "members":total,"types":{"integer":integer,"number":number,"boolean":boolean,
            "string":string,"object":object_,"array":array,"other":other},
            "whitelisted_name_mask":mask,"numeric_name_mask":numeric_mask,
            "known_fields":decode_names(mask),"numeric_known_fields":decode_names(numeric_mask),
            "unlisted_field_count":total-known_count,
            "unlisted_numeric_field_count":integer+number-numeric_count,
            "ordinal_name_mask":ordinal,"numeric_ordinal_mask":numeric_ordinal,
            "native_command_triple_verified":False,"native_write_authorized":False}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("log",type=Path)
    args=p.parse_args()
    path=args.log
    if path.is_symlink() or not BASE_MODULE.FILENAME.fullmatch(path.name) or not path.is_file():
        p.error("expected private regular per-session GFx log")
    if path.stat().st_size>2_000_000:p.error("log exceeds bound")
    try:report=inspect(path.read_text(encoding="utf-8",errors="replace"))
    except ValueError as e:p.error(str(e))
    print(json.dumps(report,indent=2))
if __name__=="__main__":main()
