from pathlib import Path
root = Path(__file__).resolve().parents[1]
r = (root/'src/fix39/wm_fix39_runtime.c').read_text()
a = (root/'tools/apply_fix39.py').read_text()
for token in [
    'source_dir_to_opponent',
    'a->new_facing_dir = desired;',
    'WM_ARCADE_MODE_NOAUTOFLIP',
    'if (desired & WM_MOVE_LEFT) a->obj_control |= WM_OBJ_FLIPH;',
    'live_source_face_opponents();',
]:
    assert token in r, token
for token in [
    'stale back-to-back flip',
    'app->demo.p1.flip_x = false;',
    'app->demo.p2.flip_x = true;',
    'app->demo.p1.flip_x = true;',
    'app->demo.p2.flip_x = false;',
]:
    assert token in a, token
# Source reset positions encode P1->P2 as right/up (9), P2->P1 left/down (6).
assert '#define WM_FIX39_P1_FACING  9' in r
assert '#define WM_FIX39_P2_FACING  6' in r
print('Combat2h source-facing authority: PASS')
