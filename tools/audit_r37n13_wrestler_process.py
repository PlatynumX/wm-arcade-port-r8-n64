#!/usr/bin/env python3
from __future__ import annotations
import pathlib
import sys

root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
rt = (root / "src/fix39/wm_fix39_runtime.c").read_text(encoding="utf-8")
smc = (root / "src/fix39/wm_arcade_smove_runtime.c").read_text(encoding="utf-8")
smh = (root / "src/fix39/wm_arcade_smove_runtime.h").read_text(encoding="utf-8")
fail = []

required_runtime = [
    "for (i = (unsigned)g.active_actor_count; i-- > 0u;)",
    "wm_arcade_getup_process_tick(&g.getup[i], a);",
    "wm_arcade_smove_runtime_tick_owner(&g.smoves, (uint8_t)i,",
    "wm_arcade_wrestler_process_dispatch_ready(a)",
    "wm_arcade_wrestler_process_sleep(proc, a);",
    "WRESTLE.ASM has the second veladd/friction pair",
]
for needle in required_runtime:
    if needle not in rt:
        fail.append(f"runtime missing: {needle}")

# Global SMOVE batch must be gone from wrestler_main.
if "wm_arcade_smove_runtime_tick(&g.smoves" in rt:
    fail.append("runtime still globally batch-ticks SMOVE watchdogs")

# The known N12 post-move double integration must be absent.
wrong = """(void)wm_arcade_move_wrestler(a, 0, &move_cb);\n\n                oldx = a->x_int; oldz = a->z_int;\n                wm_arcade_wrestler_veladd(a, false, false);"""
if wrong in rt:
    fail.append("post-move second wrestler_veladd still present")

if "void wm_arcade_smove_runtime_tick_owner(" not in smc:
    fail.append("SMOVE owner dispatcher implementation missing")
if "void wm_arcade_smove_runtime_tick_owner(" not in smh:
    fail.append("SMOVE owner dispatcher declaration missing")
if "if (p->owner_slot != owner_slot) continue;" not in smc:
    fail.append("SMOVE owner dispatcher does not filter owner_slot")

# Ordering within the loop must be GETUP -> SMOVE -> WMAIN dispatch test.
process_anchor = rt.find("for (i = (unsigned)g.active_actor_count; i-- > 0u;)")
if process_anchor >= 0:
    window = rt[process_anchor:process_anchor + 5000]
    a = window.find("wm_arcade_getup_process_tick")
    b = window.find("wm_arcade_smove_runtime_tick_owner")
    c = window.find("wm_arcade_wrestler_process_dispatch_ready")
    if not (0 <= a < b < c):
        fail.append("process lane is not GETUP -> SMOVE -> WMAIN")

if fail:
    print("R37N13 wrestler-process source-parity audit: FAIL")
    for x in fail:
        print(" -", x)
    raise SystemExit(1)

print("R37N13 wrestler-process source-parity audit: PASS")
