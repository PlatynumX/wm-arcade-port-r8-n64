#!/usr/bin/env python3
from __future__ import annotations
import argparse
import json
import pathlib
import zipfile
import sys

REQUIRED = {
    'wwf-wrestlemania/ATTRACT.ASM',
    'wwf-wrestlemania/HSTD.ASM',
    'wwf-wrestlemania/BRET.ASM',
    'wwf-wrestlemania/HRTSEQ1.ASM',
    'wwf-wrestlemania/FINISEQ.ASM',
    'wwf-wrestlemania/ANIM.EQU',
    'SOURCE_TEXT_MANIFEST.json',
}


def verify(path: pathlib.Path, *, min_files: int = 50, min_bytes: int = 65536) -> dict:
    if not path.is_file():
        raise ValueError(f'bundle not found: {path}')
    size = path.stat().st_size
    if size < min_bytes:
        raise ValueError(f'bundle too small: {size} bytes (minimum {min_bytes})')
    try:
        with zipfile.ZipFile(path) as z:
            bad = z.testzip()
            if bad:
                raise ValueError(f'corrupt ZIP member: {bad}')
            names = set(z.namelist())
            missing = sorted(REQUIRED - names)
            if missing:
                raise ValueError('missing required source files: ' + ', '.join(missing))
            try:
                meta = json.loads(z.read('SOURCE_TEXT_MANIFEST.json'))
            except (KeyError, json.JSONDecodeError) as exc:
                raise ValueError(f'invalid source manifest: {exc}') from exc
            file_count = int(meta.get('file_count', 0))
            if file_count < min_files:
                raise ValueError(f'too few source files: {file_count} (minimum {min_files})')
            listed = meta.get('files')
            if not isinstance(listed, list) or len(listed) != file_count:
                raise ValueError('manifest file list/count mismatch')
    except zipfile.BadZipFile as exc:
        raise ValueError(f'not a valid ZIP: {exc}') from exc
    return {'bytes': size, 'file_count': file_count}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('bundle', type=pathlib.Path)
    ap.add_argument('--min-files', type=int, default=50)
    ap.add_argument('--min-bytes', type=int, default=65536)
    ns = ap.parse_args()
    info = verify(ns.bundle, min_files=ns.min_files, min_bytes=ns.min_bytes)
    print(f"source bundle verified: {info['file_count']} files, {info['bytes']} bytes")
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f'verify_source_text_bundle: error: {exc}', file=sys.stderr)
        raise SystemExit(2)
