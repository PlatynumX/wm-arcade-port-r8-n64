#!/usr/bin/env python3
from pathlib import Path
import sys
import re
repo=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
n64=repo/'src/platform/n64/main.c'
rt=repo/'src/fix39/wm_fix39_runtime.c'
if not n64.is_file() or not rt.is_file():
    raise SystemExit('Combat2CS ownership audit: generated N64/runtime source missing')
a=n64.read_text(errors='replace')
r=rt.read_text(errors='replace')
m=re.search(r'static\s+bool\s+fix39_tick_gameplay_demo\s*\([^;{}]*\)\s*\{', a, re.S)
if not m: raise SystemExit('Combat2CS ownership audit: SHOW_GAMEPLAY helper definition missing')
start=m.start()
brace=a.find('{',m.start()); d=0; end=None
for i in range(brace,len(a)):
    if a[i]=='{': d+=1
    elif a[i]=='}':
        d-=1
        if d==0: end=i+1; break
if end is None: raise SystemExit('Combat2CS ownership audit: malformed helper')
fn=a[start:end]
for bad in ['wm_demo_tick(', 'wm_demo_reset_match(', 'wm_demo_set_roster(',
            'wm_fix39_match_sync_presenter_pose(', 'wm_fix39_match_bind_source_frame_attack(',
            'wm_fix39_match_bind_bret_source_frame_attack(', 'fix39_bind_demo_frame_boxes(']:
    if bad in fn: raise SystemExit('Combat2CS ownership audit: forbidden attract authority '+bad)
for need in ['wm_fix39_match_begin(', 'wm_fix39_match_set_cpu_vs_cpu(true)', 'wm_fix39_match_tick(']:
    if need not in fn: raise SystemExit('Combat2CS ownership audit: missing source match path '+need)
# No presentation API may mutate actor gameplay state.
s=r.find('void wm_fix39_match_sync_presenter_pose')
e=r.find('void wm_fix39_match_bind_source_frame_attack',s)
if s<0 or e<0: raise SystemExit('Combat2CS ownership audit: presenter compatibility seam missing')
pf=r[s:e]
for bad in ['a->player_mode =','a->x_int =','a->z_int =','a->facing_dir =','a->move_dir =']:
    if bad in pf: raise SystemExit('Combat2CS ownership audit: presenter mutates gameplay '+bad)
print('Combat2CS final generated-tree ATTR single-authority behavioral audit: PASS')
