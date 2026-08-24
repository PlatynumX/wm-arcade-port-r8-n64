from pathlib import Path
s=Path('src/fix39/wm_fix39_runtime.c').read_text()
fn=s[s.index('void wm_fix39_match_tick'):]
for x in ['wm_ring_inring_field(', 'live_source_face_opponents();', 'refresh_distances();',
          'wm_arcade_drone_main(', 'live_keep_onscreen_from_camera();',
          'wm_arcade_wrestler_veladd(', 'wm_arcade_wrestler_friction(',
          'wm_arcade_move_wrestler(', 'attach_proc->attach_proc',
          'wm_arcade_resolve_overlap(', 'WM_ARCADE_MODE_NOAUTOFLIP']:
    assert x in fn, x
assert fn.index('wm_ring_inring_field(') < fn.index('live_source_face_opponents();')
assert fn.index('live_source_face_opponents();') < fn.index('refresh_distances();') < fn.index('wm_arcade_drone_main(')
assert fn.index('wm_arcade_drone_main(') < fn.index('live_keep_onscreen_from_camera();')
assert fn.index('live_keep_onscreen_from_camera();') < fn.index('wm_arcade_wrestler_veladd(') < fn.index('wm_arcade_wrestler_friction(')
assert fn.index('wm_arcade_wrestler_friction(') < fn.index('wm_arcade_move_wrestler(')
assert fn.index('wm_arcade_move_wrestler(') < fn.index('attach_proc->attach_proc') < fn.index('wm_arcade_resolve_overlap(')
print('Combat2BO WRESTLE.ASM wrestler_main source-order regression: PASS')
