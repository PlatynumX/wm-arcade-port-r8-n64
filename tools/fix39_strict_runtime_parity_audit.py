\
#!/usr/bin/env python3
from pathlib import Path
import argparse, re, sys

ap = argparse.ArgumentParser()
ap.add_argument("root", nargs="?", default=".")
ap.add_argument("--playable-lane", action="store_true",
                help="Allow only explicitly reported untranslated DRONE CALL bodies; final parity still fails on them.")
a = ap.parse_args()
root = Path(a.root)
fail = []
gap = []

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
    if not p.exists():
        fail.append(f'missing generated header: {name}')
        continue
    t=p.read_text(errors='ignore')
    if 'PLACEHOLDER ONLY' in t or 'build-time placeholder' in t:
        fail.append(f'{name}: placeholder survived canonical generation')

def count_macro(name, macro):
    p=root/'src/fix39'/name
    if not p.exists():
        fail.append(f'missing generated header: {name}')
        return None
    m=re.search(rf'{re.escape(macro)}\s+(\d+)', p.read_text(errors='ignore'))
    if not m:
        fail.append(f'{name}: {macro} missing')
        return None
    return int(m.group(1))

scripts = count_macro('wm_arcade_drone_source_scripts_generated.h',
                      'WM_FIX39_DRONE_SCRIPT_COUNT')
services = count_macro('wm_arcade_drone_source_services_generated.h',
                       'WM_FIX39_DRONE_SERVICE_COUNT')
bodies = count_macro('wm_arcade_drone_source_bodies_generated.h',
                     'WM_FIX39_DRONE_TRANSLATED_BODY_COUNT')

if scripts is not None and scripts <= 0:
    fail.append('wm_arcade_drone_source_scripts_generated.h: live source scripts are empty')
if services is not None and services <= 0:
    fail.append('wm_arcade_drone_source_services_generated.h: live source services are empty')

if bodies is not None and services is not None:
    if bodies > services:
        fail.append(f'DRONE translated body count impossible: {bodies}>{services}')
    elif bodies < services:
        msg = f'DRONE CALL bodies translated={bodies}/{services}'
        if a.playable_lane:
            # Development ROMs may carry this gap only because runtime dispatch
            # explicitly refuses to fake missing CALL bodies. This is not a final
            # source-parity pass.
            gap.append(msg)
        else:
            fail.append(msg)
    elif bodies <= 0:
        fail.append('DRONE translated bodies unexpectedly empty')

if fail:
    print('STRICT SOURCE PARITY AUDIT: FAIL')
    for x in fail:
        print(' -', x)
    raise SystemExit(1)

if gap:
    print('PLAYABLE SOURCE AUDIT: PASS WITH DECLARED DEVELOPMENT GAP')
    for x in gap:
        print(' -', x)
    print(' - final-parity mode remains blocked until every DRONE CALL body is translated')
else:
    print('STRICT SOURCE PARITY AUDIT: PASS')
