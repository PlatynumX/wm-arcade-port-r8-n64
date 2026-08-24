#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
h = (root / "src/fix39/wm_arcade_wimp_frame.h").read_text()
c = (root / "src/fix39/wm_arcade_wimp_frame.c").read_text()
a = (root / "tools/apply_fix39.py").read_text()
completion = (root / "tools/fix39_combat_completion_patch.py").read_text()
runtime = (root / "src/fix39/wm_fix39_runtime.c").read_text()

# WIMP/IANI3 decoding remains source-backed and available to the translated runtime.
assert "WM_WIMP_IANI3_X_SLOT 5" in h
assert "WM_WIMP_IANI3_Y_SLOT 6" in h
assert "WM_WIMP_IANI3_Z_SLOT 7" in h
assert "WM_WIMP_IANI3_ID_SLOT 8" in h
assert "wm_arcade_wimp_frame_box_from_sprite" in c
assert "wm_fix39_match_set_frame_box" in runtime
assert "wm_fix39_match_clear_frame_box" in runtime

# Combat2CO+: ATTR gameplay must NOT bind collision boxes from the dormant wm_demo
# presenter. Source ANIM/WIMP state is owned by the translated match runtime.
assert "fix39_bind_demo_frame_boxes" not in a
assert "fix39_bind_demo_frame_boxes" not in completion
for text in (a, completion):
    assert "wm_demo_tick(&app->demo" not in text
    assert "wm_fix39_match_sync_presenter_pose(0, app->demo" not in text
    assert "wm_fix39_match_sync_presenter_pose(1, app->demo" not in text

print("Fix39 ATTR source-owned WIMP collision contract: PASS")
