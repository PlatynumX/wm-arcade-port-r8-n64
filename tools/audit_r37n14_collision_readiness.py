#!/usr/bin/env python3
from __future__ import annotations
import pathlib, sys
root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
rt = (root / "src/fix39/wm_fix39_runtime.c").read_text(encoding="utf-8")
cm = (root / "CMakeLists.txt").read_text(encoding="utf-8")
start = rt.find("/* R37N14 / COLLIS.ASM readiness translation.")
end = rt.find("/* WRESTLE.ASM master loop: check_collisions -> final_confine. */", start)
block = rt[start:end] if start >= 0 and end > start else ""
checks = [
    ("R37N14 block present", bool(block)),
    ("all-valid readiness retained only as diagnostic", "live_collision_boxes_ready_for_active();" in block),
    ("no all-valid combat gate", "if (g.status.collision_boxes_ready)" not in block),
    ("defender validity is pair-local", "g.frame_box_valid[i]" in block and "valid_defenders[vi]" in block),
    ("attacker does not require own hurt validity", "g.frame_box_valid[ai]" not in block),
    ("attack box built independently", "wm_arcade_set_attack_box(attacker)" in block),
    ("pair acceptance uses direct COLLIS port", "wm_arcade_try_attack_hit(attacker, victim" in block),
    ("legacy whole-list helper bypassed", "wm_arcade_check_wrestler_collisions(" not in block),
    ("even/odd source attacker order", "g.status.round_tickcount & 1u" in block),
    ("defender scan remains forward", "for (vi = 0u; vi < g.active_actor_count; ++vi)" in block),
    ("first successful wrestler hit exits", "outer < g.active_actor_count && !hit" in block and "hit = 1;" in block),
    ("object collisions use valid defender surface", "wm_arcade_object_collisions(\n                &g.special_lists, valid_defenders" in block),
    ("N13 scheduler untouched", "R37N13 direct MPROC.ASM + WRESTLE.ASM process translation" in rt),
    ("model registered", "wm_r37n14_collision_readiness_model" in cm),
    ("audit registered", "wm_r37n14_collision_readiness_audit" in cm),
]
failed=[]
for name, ok in checks:
    print(("PASS: " if ok else "FAIL: ") + name)
    if not ok: failed.append(name)
if failed: raise SystemExit(1)
print("R37N14 COLLIS.ASM readiness structural audit: PASS")
