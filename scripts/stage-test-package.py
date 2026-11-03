#!/usr/bin/env python3
"""Stage a mod-manager ZIP from a completed Windows x64 DLL; never install it."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
from zipfile import ZipFile, ZIP_DEFLATED, ZipInfo

ROOT=Path(__file__).resolve().parents[1]
PARENT_PATH="Data/MCM/Config/K2040_Mods/config.json"
CANONICAL_PARENT_SHA256="8eccc0c1313d1ec982e6094650835e7f83b42e03c56d8435a81ff55c3aba49eb"

DLL=ROOT/"dist"/"Data"/"F4SE"/"Plugins"/"K2040_Quest_Journal.dll"
OUTPUT=ROOT/"dist"/"K2040_Quest_Journal-test-package.zip"
ASSETS=("journal.html","tracker.html","styles.css","app.js",
        "quest-selects.js","quest-model.js","journal-settings.js","demo-data.js")


def inspect_pe(blob):
    """Reject non-x64 DLLs or absent F4SE entry points without executing code."""
    def unpack(fmt, offset):
        if offset < 0 or offset + struct.calcsize(fmt) > len(blob):
            raise ValueError("truncated PE header or export table")
        return struct.unpack_from(fmt, blob, offset)
    if len(blob) < 0x40 or blob[:2] != b"MZ":
        raise ValueError("not a Windows PE image (missing MZ)")
    pe = unpack("<I", 0x3c)[0]
    if pe > len(blob)-24 or blob[pe:pe+4] != b"PE\x00\x00":
        raise ValueError("invalid PE signature or file header")
    machine, nsections, _, _, _, opt_size, characteristics = unpack("<HHIIIHH", pe+4)
    opt = pe+24
    if machine != 0x8664 or nsections < 1 or nsections > 96 or unpack("<H", opt)[0] != 0x20b:
        raise ValueError("expected a 64-bit x64 PE32+ binary")
    if not characteristics & 0x2000:
        raise ValueError("image is not marked as a DLL")
    if opt_size < 120 or opt + opt_size > len(blob):
        raise ValueError("invalid optional PE32+ header")
    if unpack("<I", opt+108)[0] < 1:
        raise ValueError("PE has no export directory")
    exp_rva, exp_size = unpack("<II", opt+112)
    if exp_rva == 0 or exp_size < 40:
        raise ValueError("PE has no export directory")
    sections = []
    for i in range(nsections):
        sh = opt+opt_size+i*40
        vsize, vaddr, raw_size, raw_ptr = unpack("<IIII", sh+8)
        if raw_ptr+raw_size > len(blob):
            raise ValueError("truncated PE section data")
        sections.append((vaddr, max(vsize,raw_size), raw_ptr, raw_size))
    def rva_offset(rva, size):
        for start, span, raw, raw_size in sections:
            if rva >= start and rva-start+size <= raw_size and rva-start+size <= span:
                return raw+rva-start
        raise ValueError("PE export RVA is outside file-backed sections")
    e = rva_offset(exp_rva, 40)
    nfunctions, nnames, funcs, names, ordinals = unpack("<IIIII", e+20)
    if not (1 <= nnames <= 65536 and 1 <= nfunctions <= 65536):
        raise ValueError("invalid PE export counts")
    name_ptrs = rva_offset(names, nnames*4)
    ord_ptrs = rva_offset(ordinals, nnames*2)
    func_ptrs = rva_offset(funcs, nfunctions*4)
    found = set()
    for i in range(nnames):
        ordinal = unpack("<H", ord_ptrs+2*i)[0]
        if ordinal >= nfunctions:
            raise ValueError("invalid PE export ordinal")
        func_rva = unpack("<I", func_ptrs+4*ordinal)[0]
        name_rva = unpack("<I", name_ptrs+4*i)[0]
        pos = rva_offset(name_rva, 1)
        end = blob.find(b"\x00", pos, min(pos+256,len(blob)))
        if end < 0:
            raise ValueError("unterminated PE export name")
        name = blob[pos:end]
        if name in (b"F4SEPlugin_Query", b"F4SEPlugin_Load"):
            if func_rva == 0 or exp_rva <= func_rva < exp_rva+exp_size:
                raise ValueError("missing or forwarded F4SE export implementation")
            found.add(name)
    required = {b"F4SEPlugin_Query", b"F4SEPlugin_Load"}
    if found != required:
        missing = ", ".join(name.decode("ascii") for name in sorted(required-found))
        raise ValueError("required F4SE exports missing: "+missing)
    return True

def gather(dll, view):
    binary=dll.read_bytes()
    inspect_pe(binary)
    files={"Data/F4SE/Plugins/K2040_Quest_Journal.dll":binary}
    for name in ASSETS:
        path=view/name
        if not path.is_file() or path.is_symlink():
            raise ValueError("missing or invalid Prisma view asset: "+name)
        files["Data/PrismaUI_F4/views/K2040QuestJournal/"+name]=path.read_bytes()
    # Public installers accompany their GPL source reference and rights
    # notices, unlike the internal gameplay-only QA package archives.
    for name in ("LICENSE", "THIRD_PARTY_NOTICES.md", "README.md", "docs/COMPATIBILITY.md"):
        source=ROOT/name
        if not source.is_file() or source.is_symlink():
            raise ValueError("missing public release notice: "+name)
        files[name]=source.read_bytes()
    return files


def with_mcm(files, mode, base=ROOT/"mcm"):
    """Add exactly one child MCM layout. Never ship grouped+standalone at once."""
    if mode=="none":
        return dict(files)
    if mode not in ("grouped","standalone"):
        raise ValueError("unknown MCM package variant")
    base=base if mode=="grouped" else base/"standalone"
    prefix="Data/MCM/Config/K2040_Quest_Journal/"
    result=dict(files)
    for name in ("config.json","keybinds.json","settings.ini"):
        file=base/prefix/name
        if not file.is_file() or file.is_symlink():
            raise ValueError("missing child MCM asset: "+str(file))
        result[prefix+name]=file.read_bytes()
    if mode=="grouped":
        parent=base/PARENT_PATH
        if not parent.is_file() or parent.is_symlink():
            raise ValueError("missing canonical K2040 Mods parent MCM file")
        raw=parent.read_bytes()
        if hashlib.sha256(raw).hexdigest()!=CANONICAL_PARENT_SHA256:
            raise ValueError("parent MCM content differs from the canonical K2040 template")
        if json.loads(raw).get("pages")!=[]:
            raise ValueError("parent must not contain child pages")
        result[PARENT_PATH]=raw
    config=json.loads(result[prefix+"config.json"])
    if config.get("modName")!="K2040_Quest_Journal":
        raise ValueError("child MCM identity mismatch")
    if (config.get("ownerModName")=="K2040_Mods") != (mode=="grouped"):
        raise ValueError("MCM owner mismatch for packaging variant")
    return result



def build_zip(files, output):
    output.parent.mkdir(parents=True,exist_ok=True)
    if output.exists():
        raise FileExistsError("refusing to overwrite an existing test archive: "+str(output))
    with ZipFile(output,"x",compression=ZIP_DEFLATED,compresslevel=9) as archive:
        for name in sorted(files):
            entry=ZipInfo(name,date_time=(2020,1,1,0,0,0))
            entry.compress_type=ZIP_DEFLATED
            entry.external_attr=0o644<<16
            archive.writestr(entry,files[name],compress_type=ZIP_DEFLATED,compresslevel=9)
    return hashlib.sha256(output.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dll",type=Path,default=DLL)
    parser.add_argument("--view",type=Path,default=ROOT/"view")
    parser.add_argument("--output",type=Path,default=OUTPUT)
    parser.add_argument("--mcm-mode",choices=("grouped","standalone","none"),default="grouped")
    args=parser.parse_args()
    try:
        files=gather(args.dll,args.view)
        files=with_mcm(files,args.mcm_mode)
        digest=build_zip(files,args.output)
    except (OSError,ValueError) as error:
        print("Packaging blocked:",error,file=sys.stderr)
        return 1
    print(f"Staged {len(files)} mod files: {args.output}")
    print("SHA-256:",digest)
    print("F4SE loader export names verified; NOT installed or game-tested. Verify imports and target runtime.")
    return 0


if __name__=="__main__":
    raise SystemExit(main())
