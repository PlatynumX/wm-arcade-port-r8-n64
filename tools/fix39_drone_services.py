#!/usr/bin/env python3
from __future__ import annotations
import argparse, re, tempfile, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fix39_drone_scripts as scripts  # type: ignore

def fail(msg: str):
    raise SystemExit(msg)

def strip_comment(s: str) -> str:
    return s.split(';',1)[0].strip()

def audit(source: Path, generated: Path, out: Path) -> tuple[bool,int]:
    txt = source.read_text(encoding='latin-1', errors='replace')
    gen = generated.read_text(encoding='utf-8', errors='replace')
    # Exact executable seam list was produced by the C3 decoder from reachable scripts.
    m = re.search(r'#define\s+WM_FIX39_DRONE_C4_SEAM_COUNT\s+(\d+)', gen)
    if not m: fail('generated script header missing C4 seam count')
    count = int(m.group(1))
    labels = re.findall(r'^\s*"([^"]+)",\s*$', gen, flags=re.M)
    # Keep only the leading seam-table entries. The generated file contains many later strings.
    sm = re.search(r'wm_fix39_drone_c4_seam_labels\[[^\]]+\]\s*=\s*\{(.*?)\};', gen, re.S)
    if not sm: fail('generated script header missing seam table')
    seam_labels = re.findall(r'"([^"]+)"', sm.group(1))
    if len(seam_labels) != count:
        fail(f'seam count mismatch: macro={count}, table={len(seam_labels)}')

    # Verify DRONE.ASM really routes script command #1 through drone_seek.
    seek_call = bool(re.search(r'(?im)^\s*callr\s+drone_seek\b', txt))
    if not seek_call: fail('historical DRONE.ASM has no callr drone_seek in script interpreter')

    # Validate seam symbols against the SAME assembler-aware source image used by
    # the C3 decoder.  Raw text regexes are not sufficient here: historical
    # labels may be emitted by SUBR/SUBRP macros, `equ $` aliases, expanded
    # DRONE-local macros, or inline packed-data labels.  C5a incorrectly
    # rejected those valid source symbols because it searched only literal
    # `#label` text.
    image = scripts.build_image(source)
    missing = []
    for label in seam_labels:
        base = label.split('@EXGPC_', 1)[0] if '@EXGPC_' in label else label
        if scripts.symkey(base) not in image.labels:
            missing.append(label)
    if missing:
        fail('source executable seam label(s) not found in assembler-aware image: ' + ', '.join(missing))

    lines = [
        '# Fix39 V13e Chunk 5b — DRONE executable-service source audit',
        'source_seek_call=drone_seek',
        f'c4_executable_seam_count={count}',
    ]
    lines += [f'seam={x}' for x in seam_labels]
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text('\n'.join(lines)+'\n', encoding='utf-8')
    return seek_call, count

def self_test():
    with tempfile.TemporaryDirectory() as td:
        d=Path(td); src=d/'DRONE.ASM'; gen=d/'g.h'; out=d/'audit.txt'
        src.write_text('''#foo\n move a0,a0\n#bar\n move a1,a1\n#scplp\n callr drone_seek\n''', encoding='latin-1')
        gen.write_text('''#define WM_FIX39_DRONE_C4_SEAM_COUNT 2\nstatic const char *const wm_fix39_drone_c4_seam_labels[2] = {\n "foo",\n "bar@EXGPC_0010",\n};\n''')
        seek,n=audit(src,gen,out)
        assert seek and n==2 and 'seam=foo' in out.read_text()
    print('Fix39 DRONE executable-service source audit self-test: PASS')

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--source'); ap.add_argument('--generated'); ap.add_argument('--out'); ap.add_argument('--self-test', action='store_true')
    a=ap.parse_args()
    if a.self_test: self_test(); return
    if not (a.source and a.generated and a.out): fail('--source --generated --out required')
    seek,n=audit(Path(a.source),Path(a.generated),Path(a.out)); print(f'Verified drone_seek and {n} executable source seams -> {a.out}')
if __name__=='__main__': main()
