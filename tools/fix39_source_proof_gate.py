#!/usr/bin/env python3
from pathlib import Path
import sys,re
repo=Path(sys.argv[1])
fail=[]; note=[]
def text(rel):
    p=repo/rel
    return p.read_text(errors='ignore') if p.exists() else ''
rt=text('src/fix39/wm_fix39_runtime.c')
vm=text('src/fix39/wm_arcade_source_animation_runtime.c')
# Presentation must never own gameplay coordinates.
if 'wm_fix39_match_sync_presenter_pose(' in rt and 'WM_FIX39_DIAGNOSTIC_PRESENTER_POSE' not in rt:
    fail.append('presenter pose can feed live combat state outside diagnostic guard')
# Known source-exact ANIM restart properties must survive.
for tok in ('source_anim_services','wm_source_anim_runtime_change','wm_source_anim_runtime_tick'):
    if tok not in rt: fail.append(f'animation bridge missing {tok}')
# Unproven shortcuts must not be silently treated as parity.
if re.search(r'live_no_teammates\s*\([^)]*\)\s*\{[^}]*return\s+0\s*;',rt,re.S):
    fail.append('live_no_teammates is a constant shortcut, not source-proven')
# Every live ANI_CODE miss must remain visible/fail-closed, never silently success.
if 'external_special_label=label' not in rt:
    fail.append('unresolved ANI_CODE labels are not preserved for parity diagnostics')
# Generated DRONE placeholders are forbidden in live tree.
for rel in ('src/fix39/wm_arcade_drone_source_ranges_generated.h','src/fix39/wm_arcade_drone_source_scripts_generated.h','src/fix39/wm_arcade_drone_source_tables_generated.h'):
    t=text(rel)
    if 'PLACEHOLDER ONLY' in t or 'build-time placeholder' in t: fail.append(f'{rel}: placeholder survived')
# Ban known speculative WIMP transformations from live converter copied into repo.
ca=text('tools/fix39_character_assets.py')
if "return remapped" in ca or "'image-one-based'" in ca:
    fail.append('live WIMP converter contains unproven one-based pixel remap')
# Inferred helpers may remain for research fixtures, but live _source_ci8_view must not call them.
m=re.search(r'def _source_ci8_view\(.*?\n(?=def )',ca,re.S)
if m and ('_source_palette_window(' in m.group(0) or '_effective_palette_words(' in m.group(0)):
    fail.append('live WIMP conversion uses inferred palette reconstruction')
if fail:
    print('SOURCE PROOF GATE: FAIL')
    for x in fail: print(' -',x)
    raise SystemExit(1)
print('SOURCE PROOF GATE: PASS')
print('NOTE: PASS means no known unproven shortcut is silently promoted; it does not claim unresolved WIMP semantics are solved.')
