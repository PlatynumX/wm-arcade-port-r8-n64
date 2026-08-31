#!/usr/bin/env python3
from __future__ import annotations
import pathlib
import sys

root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
rt = (root / "src/fix39/wm_fix39_runtime.c").read_text(encoding="utf-8")
proc = (root / "src/fix39/wm_arcade_wrestler_process.c").read_text(encoding="utf-8")
cm = (root / "CMakeLists.txt").read_text(encoding="utf-8")
mk = (root / "Makefile").read_text(encoding="utf-8")

start = rt.find("/* R37N12 / MPROC.ASM + WRESTLE.ASM complete wrestler_main coroutine.")
end = rt.find("/* SPECIAL.ASM process state and COLLIS.ASM object collisions are already", start)
block = rt[start:end] if start >= 0 and end > start else ""

def pos(s):
    return block.find(s)

checks = [
    ("process state compiled in CMake", "src/fix39/wm_arcade_wrestler_process.c" in cm),
    ("process state compiled in N64 Makefile", "src/fix39/wm_arcade_wrestler_process.c" in mk),
    ("runtime owns process control blocks", "wrestler_process[WM_FIX39_ACTOR_COUNT]" in rt),
    ("initial wake is calc_closest", "WM_WRESTLER_RESUME_CALC_CLOSEST" in block and "source_calc_closest_one(i);" in block),
    ("steady wake is post-SLEEPR", "WM_WRESTLER_RESUME_POST_SLEEP" in proc),
    ("PTIME word decrement", "ptime = (uint16_t)(ptime - 1u);" in proc and "(int16_t)ptime" in proc),
    ("KOD long sleep", "0x7fff" in proc and "WM_STATUS_KOD" in proc),
    ("no setup calc_closest shortcut", "source_calc_closest_all();" not in rt),
    ("single-process facing", "live_source_face_opponent_one(i);" in block),
    ("single-process calc_closest2", "source_calc_closest2_one(i);" in block),
    ("animation before move", 0 <= pos("wm_source_anim_runtime_tick(&g.source_anim[i], a);") < pos("(void)wm_arcade_move_wrestler(a, 0, &move_cb);")),
    ("second velocity exists", block.count("wm_arcade_wrestler_veladd(a, false, false);") == 2),
    ("move before second velocity", pos("(void)wm_arcade_move_wrestler(a, 0, &move_cb);") < block.rfind("wm_arcade_wrestler_veladd(a, false, false);")),
    ("second velocity before links", block.rfind("wm_arcade_wrestler_veladd(a, false, false);") < pos("wm_arcade_update_links(a);")),
    ("links before overlap", pos("wm_arcade_update_links(a);") < pos("wm_arcade_resolve_overlap(a, &g.actors[j])")),
    ("overlap before attachment", pos("wm_arcade_resolve_overlap(a, &g.actors[j])") < pos("wm_arcade_master_keep_attached(a)")),
    ("countdown before loop preamble", pos("wm_arcade_wrestler_countdown_tail(") < pos("a->in_ring = wm_ring_inring_field(")),
    ("loop preamble before drone", pos("a->in_ring = wm_ring_inring_field(") < pos("live_drone_wrestler_slice(i);")),
    ("drone before SLEEPR", pos("live_drone_wrestler_slice(i);") < pos("wm_arcade_wrestler_process_sleep(proc, a);")),
    ("GETUP remains distinct", "GETUP_PID is a distinct process" in block),
    ("R37N12 regression registered", "wm_r37n12_wrestler_main_coroutine_regression" in cm),
    ("R37N12 audit registered", "wm_r37n12_wrestler_main_coroutine_audit" in cm),
    ("R37N12 model registered", "wm_r37n12_wrestler_main_coroutine_model" in cm),
]

failed = []
for name, ok in checks:
    print(("PASS: " if ok else "FAIL: ") + name)
    if not ok:
        failed.append(name)
if failed:
    raise SystemExit(1)
print("R37N12 wrestler_main coroutine structural audit: PASS")
