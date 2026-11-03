#!/usr/bin/env python3
"""Create only a fresh, verified OG163 schema-only Amethyst import ZIP."""
from __future__ import annotations
import argparse,hashlib,os
from pathlib import Path
from zipfile import ZipFile,ZipInfo,ZIP_DEFLATED

def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    cli=argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--stage',required=True,type=Path)
    cli.add_argument('--output',required=True,type=Path)
    a=cli.parse_args()
    src=a.stage.resolve(strict=True)
    out=a.output.expanduser().resolve(strict=False)
    home=(Path.home()/'Downloads').resolve(strict=True)
    repo=Path(__file__).resolve().parents[1]
    if src.is_symlink() or not src.is_dir() or not src.is_relative_to(home) or src.is_relative_to(repo):
        cli.error('stage must be private, isolated and under Downloads')
    if out.is_symlink() or out.exists() or out.parent!=home or out.suffix.lower()!='.zip':
        cli.error('refuse existing, linked, or non-Downloads archive')
    banned=('steamapps','steamlibrary','amethyst','/data/f4se/')
    if any(x in str(src).lower() for x in banned):cli.error('unsafe game/deployment location')
    files=sorted((p for p in src.rglob('*') if p.is_file()),key=lambda p:p.relative_to(src).as_posix())
    names=[p.relative_to(src).as_posix() for p in files]
    if len(files)!=12 or any(p.is_symlink() or not p.is_file() for p in files):
        cli.error('unexpected payload count/type')
    if not all(n.startswith(('F4SE/Plugins/','MCM/Config/','PrismaUI_F4/views/K2040QuestJournal/')) for n in names):
        cli.error('unexpected payload path')
    dll=src/'F4SE/Plugins/K2040_Quest_Journal.dll'
    b=dll.read_bytes()
    if not b.startswith(b'MZ') or len(b)>5_000_000:
        cli.error('missing Windows DLL')
    for marker in (b'PipboyGfxReadOnlyQA:',b'PipboySchemaReadOnlyQA:'):
        if marker not in b:cli.error('missing read-only QA marker')
    for forbidden in (b'JournalDryRunQA:',b'SelectorReadOnlyQA:',b'SingleMiscWriteQA:',
                      b'DogmeatProductionQA:',b'NativeWriteQA:'):
        if forbidden in b:cli.error('writer or other QA marker present')
    tmp=out.with_suffix('.zip.tmp')
    if tmp.exists():cli.error('existing unclassified temp package')
    try:
        with ZipFile(tmp,'w',ZIP_DEFLATED,compresslevel=9) as z:
            for p,n in zip(files,names):
                info=ZipInfo(n,(2020,1,1,0,0,0))
                info.compress_type=ZIP_DEFLATED
                info.external_attr=0o100644<<16
                z.writestr(info,p.read_bytes(),compress_type=ZIP_DEFLATED,compresslevel=9)
        with ZipFile(tmp) as z:
            if z.testzip() is not None or z.namelist()!=names or any(z.read(n)!=p.read_bytes() for p,n in zip(files,names)):
                cli.error('CRC, manifest, or content mismatch')
        os.replace(tmp,out)
    finally:
        tmp.unlink(missing_ok=True)
    print('PASS schema-only ZIP: 12 original project files, no game deployment')
    print('ZIP_SHA256',sha(out.read_bytes()))
    print('DLL_SHA256',sha(b))
    print('ZIP_BYTES',out.stat().st_size)
if __name__=='__main__':main()
