#!/usr/bin/env python3
from pathlib import Path
import re, sys
root=Path(sys.argv[1] if len(sys.argv)>1 else '.')
fail=[]
# Presentation harness must not own or write combat state.
for rel in ['src/platform/n64/main.c','src/core/app.c','src/core/demo.c']:
    p=root/rel
    if not p.exists(): continue
    t=p.read_text(errors='ignore')
    if 'wm_demo_tick(&app->demo' in t: fail.append(f'{rel}: wm_demo_tick still owns attract gameplay')
    if 'wm_fix39_match_sync_presenter_pose(' in t: fail.append(f'{rel}: presenter pose feeds combat runtime')
# Presenter sync must be diagnostic-only if retained for tooling.
rp=root/'src/fix39/wm_fix39_runtime.c'
if rp.exists():
    t=rp.read_text(errors='ignore')
    pos=t.find('void wm_fix39_match_sync_presenter_pose')
    if pos>=0:
        prefix=t[max(0,pos-160):pos]
        if 'WM_FIX39_DIAGNOSTIC_PRESENTER_POSE' not in prefix:
            fail.append('wm_fix39_runtime.c: presenter sync is live in normal build')
# Attract adapter may not expose an explicitly unimplemented gameplay API.
for rel in ['src/fix39/wmania_attract_adapter.h','src/fix39/wmania_attract_adapter.c']:
    p=root/rel
    if p.exists() and 'show_gameplay_demo_unimplemented' in p.read_text(errors='ignore'):
        fail.append(f'{rel}: unimplemented attract gameplay API remains')
# Generated headers used by live combat may never be placeholders after canonical generation.
for name in [
 'wm_arcade_character_attack_frames_generated.h',
 'wm_arcade_drone_source_ranges_generated.h',
 'wm_arcade_drone_source_scripts_generated.h',
 'wm_arcade_drone_source_tables_generated.h',
]:
    p=root/'src/fix39'/name
    if not p.exists(): fail.append(f'missing generated header: {name}'); continue
    t=p.read_text(errors='ignore')
    if 'PLACEHOLDER ONLY' in t or 'build-time placeholder' in t:
        fail.append(f'{name}: placeholder survived canonical generation')
# DRONE source graph must have non-empty scripts, services and translated bodies.
checks={
 'wm_arcade_drone_source_scripts_generated.h':r'WM_FIX39_DRONE_SCRIPT_COUNT\s+([1-9][0-9]*)',
 'wm_arcade_drone_source_services_generated.h':r'WM_FIX39_DRONE_SERVICE_COUNT\s+([1-9][0-9]*)',
 'wm_arcade_drone_source_bodies_generated.h':r'WM_FIX39_DRONE_TRANSLATED_BODY_COUNT\s+([1-9][0-9]*)',
}
for name,pat in checks.items():
    p=root/'src/fix39'/name
    if not p.exists() or not re.search(pat,p.read_text(errors='ignore')):
        fail.append(f'{name}: live source data is empty/unregenerated')
if fail:
    print('STRICT SOURCE PARITY AUDIT: FAIL')
    for x in fail: print(' -',x)
    raise SystemExit(1)
print('STRICT SOURCE PARITY AUDIT: PASS')
