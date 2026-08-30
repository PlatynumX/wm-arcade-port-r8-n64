#!/usr/bin/env python3
from __future__ import annotations
import pathlib
import sys

root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
rt = (root / 'src/fix39/wm_fix39_runtime.c').read_text(encoding='utf-8')
proc = (root / 'src/fix39/wm_arcade_wrestler_process.c').read_text(encoding='utf-8')
cm = (root / 'CMakeLists.txt').read_text(encoding='utf-8')


start = rt.find('/* R37N11 / MPROC.ASM + WRESTLE.ASM wrestler process tail.')
end = rt.find('/* R37N11: DRONE.ASM now executes inside each runnable wrestler_main')
block = rt[start:end] if start >= 0 and end > start else ''

def pos(needle: str) -> int:
    return block.find(needle)


move = pos('(void)wm_arcade_move_wrestler(a, 0, &move_cb);')
links = pos('wm_arcade_update_links(a);')
overlap = pos('wm_arcade_resolve_overlap(a, &g.actors[j])')
attach = pos('wm_arcade_master_keep_attached(a)')
xflip = pos('wm_arcade_set_wrestler_xflip(a)')
joy = pos('wm_arcade_update_joy_dtime(a)')
countdown = pos('wm_arcade_wrestler_countdown_tail(')
drone = pos('live_drone_wrestler_slice(i);')
sleep = pos('wm_arcade_wrestler_process_sleep_loop(a);')
getup = pos('wm_arcade_getup_process_tick(&g.getup[i], a);')

checks = [
    ('PTIME module compiled', 'src/fix39/wm_arcade_wrestler_process.c' in cm),
    ('runtime binds PTIME dispatcher', 'wm_arcade_wrestler_process_dispatch_ready(a)' in rt),
    ('initial wrestler PTIME is one', 'a->ptime = 1;' in rt),
    ('per-wrestler tail order exists', min(move, links, overlap, attach, xflip, joy, countdown, drone, sleep) >= 0),
    ('move -> links', 0 <= move < links),
    ('links -> overlap', 0 <= links < overlap),
    ('overlap -> KEEPATTACHED', 0 <= overlap < attach),
    ('KEEPATTACHED -> xflip', 0 <= attach < xflip),
    ('xflip -> joy dtime', 0 <= xflip < joy),
    ('joy dtime -> countdown', 0 <= joy < countdown),
    ('countdown -> DRONE', 0 <= countdown < drone),
    ('DRONE -> SLEEPR/PTIME', 0 <= drone < sleep),
    ('GETUP remains separate from wrestler-ready block', getup > sleep and 'Do not gate it on wrestler PTIME/KOD' in rt),
    ('old global DRONE block removed', rt.count('wm_arcade_drone_main(') == 1),
    ('PTIME uses 16-bit stored word', 'ptime = (uint16_t)actor->ptime;' in proc and '(int16_t)ptime' in proc),
    ('PTIME decrements before signed wake test', 'ptime = (uint16_t)(ptime - 1u);' in proc),
    ('KOD long sleep preserved', '0x7fff' in proc and 'WM_STATUS_KOD' in proc),
    ('R37N11 regression registered', 'wm_r37n11_grapple_process_regression' in cm),
    ('R37N11 audit registered', 'wm_r37n11_grapple_process_audit' in cm),
    ('R37N11 model registered', 'wm_r37n11_grapple_process_model' in cm),
]

failed = []
for name, ok in checks:
    print(('PASS: ' if ok else 'FAIL: ') + name)
    if not ok:
        failed.append(name)
if failed:
    raise SystemExit(1)
print('R37N11 grapple-process structural audit: PASS')
