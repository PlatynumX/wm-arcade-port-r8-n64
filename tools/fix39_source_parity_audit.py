#!/usr/bin/env python3
"""Fail-closed audit of stable source facts plus warnings for known parity gaps.

This deliberately distinguishes verified original-arcade facts from current
reimplementation choices. Stable facts fail the build if they drift. Known
architectural gaps are warnings until replaced with source-backed behavior.
"""
from pathlib import Path
import re, sys

ROOT = Path(__file__).resolve().parents[1]
RUNTIME = ROOT / 'src/fix39/wm_fix39_runtime.c'
ROSTER = ROOT / 'src/fix39/wm_arcade_roster.h'
COMBAT = ROOT / 'src/fix39/wm_arcade_combat.c'
PATCHER = ROOT / 'tools/apply_fix39.py'
COMPLETION = ROOT / 'tools/fix39_combat_completion_patch.py'

failures=[]
warnings=[]

def need(path, needle, why):
    text=path.read_text(errors='replace')
    if needle not in text:
        failures.append(f'{path.name}: {why} (missing {needle!r})')

def warn_if(path, needle, why):
    text=path.read_text(errors='replace')
    if needle in text:
        warnings.append(why)

# TABLES.ASM WRESTLERNUM table: 0 Bret,1 Razor,2 Taker,3 Yoko,4 Shawn,
# 5 Bam,6 Doink,7 Adam (unfinished),8 Lex.
for name,val in [('BRET',0),('RAZOR',1),('TAKER',2),('YOKO',3),('SHAWN',4),('BAM',5),('DOINK',6),('LEX',8)]:
    # tolerate enum naming around WM_ARCADE_ROSTER_*
    text=ROSTER.read_text(errors='replace')
    pat=rf'WM_ROSTER_{name}\s*=\s*{val}\b'
    if not re.search(pat,text): failures.append(f'wm_arcade_roster.h: source roster id drift for {name} != {val}')

# Source reset coordinates/facing from the translated start-match seam.
need(RUNTIME, '#define WM_FIX39_P1_FACING  9', 'P1 source-facing reset drift')
need(RUNTIME, '#define WM_FIX39_P2_FACING  6', 'P2 source-facing reset drift')
need(RUNTIME, '#define WM_FIX39_P1_START_X (WM_RING_X_CENTER - 85)', 'P1 source X reset drift')
need(RUNTIME, '#define WM_FIX39_P2_START_X (WM_RING_X_CENTER + 85)', 'P2 source X reset drift')

# COLLIS.ASM set_collision_boxes source dimensions.
ct=COMBAT.read_text(errors='replace')
for lit,why in [('-30','normal hurtbox rear Z offset'),('60','normal hurtbox Z depth'),('-15','ground hurtbox rear Z offset'),('30','ground hurtbox Z depth'),('-5','running hurtbox rear Z offset'),('10','running hurtbox Z depth')]:
    if lit not in ct: failures.append(f'wm_arcade_combat.c: source collision constant missing: {why} ({lit})')
# Mirroring and inclusive overlap should remain explicit in implementation.
if not ('flip' in ct.lower() or 'mirror' in ct.lower()): failures.append('wm_arcade_combat.c: no attack-box flip/mirror implementation found')
if not any(tok in ct for tok in ['<=','>=']): warnings.append('Collision overlap implementation should be manually rechecked for source edge-touch semantics (< / > rejection in COLLIS.ASM).')

# ATTR.ASM show_gameplay seeds a source wrestler id via RNDRNG0(7), maps 7->8,
# builds the source ladder, then launches start_match. The completion patch is
# the final authority when present; apply_fix39.py is only the pre-completion base.
cp=COMPLETION.read_text(errors='replace') if COMPLETION.exists() else ''
if not ('wm_fix39_match_begin(' in cp and 'wm_fix39_match_tick(' in cp and 'wm_fix39_match_set_cpu_vs_cpu(true)' in cp):
    failures.append('Attract SHOW_GAMEPLAY does not enter/tick the translated match runtime.')
if 'wm_fix39_attract_demo_plan' not in cp:
    warnings.append('ATTR.ASM plan helper symbol is absent; ownership audit intentionally validates behavior rather than wrapper naming.')
if 'wm_demo_tick(&app->demo' in cp:
    failures.append('fix39_combat_completion_patch.py: wm_demo_tick still owns attract gameplay')

# WRESTLE2.ASM scroll_world must consume translated wrestler world state.
# Presenter screen-space coordinates are presentation-only and must never be
# injected into source actor X/Z once camera ownership is translated.
pt=PATCHER.read_text(errors='replace')
rt=RUNTIME.read_text(errors='replace')
for seam in [
    'wm_fix39_match_sync_presenter_pose(0, app->demo.p1.screen_x',
    'wm_fix39_match_sync_presenter_pose(1, app->demo.p2.screen_x',
]:
    if seam in pt:
        failures.append(f'apply_fix39.py: presenter screen-space pose still overwrites translated world state ({seam})')
for seam in ['a->x_int = g.presenter_pose', 'a->z_int = g.presenter_pose']:
    if seam in rt:
        failures.append(f'wm_fix39_runtime.c: presenter pose still owns translated world coordinates ({seam})')
need(RUNTIME, 'live_scroll_world_attract();', 'WRESTLE2.ASM scroll_world live call missing')
need(RUNTIME, 'g.camera.worldtlx_fp16 = (WM_RING_X_CENTER - 200) << 16;', 'WRESTLE.ASM init_scroller X missing')
need(RUNTIME, 'g.camera.worldtly_fp16 = -(27 << 16);', 'WRESTLE.ASM init_scroller Y missing')

print('=== Fix39 original-source parity audit ===')
if failures:
    for x in failures: print('FAIL:',x)
else:
    print('Stable source facts: PASS')
for x in warnings: print('WARN:',x)
print(f'Summary: {len(failures)} failure(s), {len(warnings)} known parity gap warning(s)')
if failures: sys.exit(1)
