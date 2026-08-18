#!/usr/bin/env python3
"""Package the historical source's text inputs for source-port review.

Binary WIMP/art/sound blobs are intentionally omitted. The N64 workflow still
fetches them directly from the historical repository when converting assets.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import zipfile
import sys

TEXT_EXTS = {
    '.asm', '.equ', '.inc', '.lod', '.txt', '.doc', '.mak', '.bat', '.cmd',
    '.h', '.c', '.s', '.ld', '.lnk', '.cfg', '.def', '.lst'
}
TEXT_NAMES = {'makefile', 'readme', 'readme.md'}


def collect(root: pathlib.Path) -> list[pathlib.Path]:
    files=[]
    for p in root.rglob('*'):
        if not p.is_file():
            continue
        if p.name.lower() in TEXT_NAMES or p.suffix.lower() in TEXT_EXTS:
            files.append(p)
    return sorted(files, key=lambda p: p.relative_to(root).as_posix().lower())


def build(root: pathlib.Path, out: pathlib.Path) -> dict:
    if not root.is_dir():
        raise ValueError(f'source root not found: {root}')
    files=collect(root)
    if not files:
        raise ValueError('no source text files found')
    manifest=[]
    out.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(out, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for p in files:
            rel=p.relative_to(root).as_posix()
            data=p.read_bytes()
            z.writestr(f'wwf-wrestlemania/{rel}', data)
            manifest.append({
                'path': rel,
                'bytes': len(data),
                'sha256': hashlib.sha256(data).hexdigest(),
            })
        meta={'file_count':len(manifest),'files':manifest}
        z.writestr('SOURCE_TEXT_MANIFEST.json', json.dumps(meta, indent=2, sort_keys=True)+'\n')
    return {'file_count':len(manifest),'bytes':sum(x['bytes'] for x in manifest)}


def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--root', required=True, type=pathlib.Path)
    ap.add_argument('--out', required=True, type=pathlib.Path)
    ns=ap.parse_args()
    info=build(ns.root, ns.out)
    print(f"source text bundle: {info['file_count']} files, {info['bytes']} bytes -> {ns.out}")
    return 0

if __name__=='__main__':
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f'source_text_bundle: error: {exc}', file=sys.stderr)
        raise SystemExit(2)
