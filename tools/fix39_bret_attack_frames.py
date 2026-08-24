#!/usr/bin/env python3
from __future__ import annotations
import argparse, re
from pathlib import Path

ATTACK_RE = re.compile(r'ANI_ATTACK_ON(_Z)?\s*,\s*(AMODE_[A-Z0-9_]+)\s*,\s*([^;]+)', re.I)
FRAME_RE = re.compile(r'\bWL\s+[^,]+,\s*([A-Z][A-Z0-9_]*\+FR\d+)\b', re.I)
OFF_RE = re.compile(r'ANI_ATTACK_OFF\b', re.I)

def atom(s: str) -> int:
    s = s.strip()
    m = re.fullmatch(r'([+-]?)([0-9A-Fa-f]+)h', s)
    if m:
        v = int(m.group(2), 16)
        return -v if m.group(1) == '-' else v
    return int(s, 0)

def expr(s: str) -> int:
    s = s.replace(' ', '')
    return sum(atom(p) for p in re.findall(r'[+-]?[^+-]+', s))

def parse(text: str):
    rows = []
    active = None
    for raw in text.splitlines():
        line = raw.split(';', 1)[0].strip()
        m = ATTACK_RE.search(line)
        if m:
            vals = [x.strip() for x in m.group(3).split(',')]
            need = 6 if m.group(1) else 4
            if len(vals) < need:
                active = None
                continue
            try:
                nums = [expr(v) for v in vals[:need]]
            except Exception:
                active = None
                continue
            active = (bool(m.group(1)), m.group(2).upper(), nums)
            continue
        if OFF_RE.search(line):
            active = None
            continue
        if active:
            fm = FRAME_RE.search(line)
            if fm:
                rows.append((fm.group(1).upper(),) + active)
    seen, out = set(), []
    for row in rows:
        if row[0] not in seen:
            seen.add(row[0])
            out.append(row)
    return out

def emit(rows):
    lines = [
        '/* generated from historical HRTSEQ2.ASM; do not edit */',
        '#ifndef WM_ARCADE_BRET_ATTACK_FRAMES_GENERATED_H',
        '#define WM_ARCADE_BRET_ATTACK_FRAMES_GENERATED_H',
        '#define WM_FIX39_BRET_ATTACK_FRAMES_GENERATED 1',
        'static const wm_arcade_source_attack_frame_t wm_bret_source_attack_frames[] = {'
    ]
    for frame, uses_z, mode, nums in rows:
        cmode = 'WM_' + mode
        if uses_z:
            x, y, z, w, h, d = nums
            lines.append(f'    {{"{frame}", 1, {cmode}, {x}, {y}, {z}, {w}, {h}, {d}}},')
        else:
            x, y, w, h = nums
            lines.append(f'    {{"{frame}", 0, {cmode}, {x}, {y}, -40, {w}, {h}, 80}},')
    lines += ['};', f'#define WM_FIX39_BRET_ATTACK_FRAME_COUNT {len(rows)}u', '#endif', '']
    return '\n'.join(lines)

def self_test():
    sample = '''
.word ANI_ATTACK_ON_Z, AMODE_PUNCH,30,91,-45,50,15,45
WL 3,H2PL3B+FR4
.word ANI_ATTACK_OFF
.word ANI_ATTACK_ON,AMODE_UPRCUT,-6,40,64,90
WL 3,H4UP3C+FR6
.word ANI_ATTACK_OFF
'''
    rows = parse(sample)
    assert len(rows) == 2
    assert rows[0][0] == 'H2PL3B+FR4'
    assert rows[1][0] == 'H4UP3C+FR6'
    assert 'WM_AMODE_PUNCH' in emit(rows)
    print('Fix39 Bret source attack-frame generator self-test: PASS')

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--source')
    ap.add_argument('--out')
    ap.add_argument('--self-test', action='store_true')
    a = ap.parse_args()
    if a.self_test:
        self_test()
        return
    if not a.source or not a.out:
        ap.error('--source and --out required')
    rows = parse(Path(a.source).read_text(errors='replace'))
    if not rows:
        raise SystemExit('no source attack frames recovered')
    Path(a.out).write_text(emit(rows))
    print(f'Generated {len(rows)} exact Bret attack-frame bindings -> {a.out}')

if __name__ == '__main__':
    main()
