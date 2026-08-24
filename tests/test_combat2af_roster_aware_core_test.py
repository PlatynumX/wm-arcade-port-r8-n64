#!/usr/bin/env python3
from pathlib import Path
s = (Path(__file__).resolve().parents[1] / 'tools' / 'apply_fix39.py').read_text()
# The patch must select a non-Bret roster before movement begins.
assert "wm_demo_set_roster(&d, 4u, 0u);" in s
# Audit every movement visual asserted by upstream test_demo_four_way_and_run.
for slot in ("WM_CV_WALK6", "WM_CV_WALK4", "WM_CV_RUN", "WM_CV_WALK2"):
    assert f"wm_character_visual(4u, {slot})" in s, slot
# Explicit regression against the original two-Brets fallback.
assert "wm_character_visual(0u, WM_CV_WALK6)" in s
assert "stale Bret movement assertions survived" in s
# Make sure all four old upstream symbols are covered by the replacement map.
for old in (
    "&wm_bret_walk6_f4_anim",
    "&wm_bret_walk4_f4_anim",
    "&wm_bret_run_anim",
    "&wm_bret_walk2_f2_anim",
):
    assert old in s, old
print('test_combat2ae_roster_aware_core_test: OK')
