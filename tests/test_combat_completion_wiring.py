from pathlib import Path
root=Path(__file__).resolve().parents[1]
r=(root/'src/fix39/wm_fix39_runtime.c').read_text()
h=(root/'src/fix39/wm_fix39_runtime.h').read_text()
for token in [
    'wm_arcade_react123456789_reaction_callback',
    'wm_arcade_react5_good_run_hit_callback',
    'g.react_callbacks.reaction = live_reaction_dispatch',
    'wm_arcade_special_tick_source_state',
    'wm_arcade_special_velocity_add',
    'wm_arcade_object_collisions',
    'wm_fix39_match_bind_source_frame_attack',
    'reaction_dispatches',
]:
    assert token in r or token in h, token
assert 'Reject run-only hits instead of fabricating it' not in r
assert 'live_scroll_world_attract();' in r
assert 'a->x_int = g.presenter_pose' not in r
assert 'a->z_int = g.presenter_pose' not in r
print('Fix39 combat completion wiring regression with source-owned world pose: PASS')

assert 'g.presenter_attack[i].valid' not in r
