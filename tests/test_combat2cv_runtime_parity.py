from pathlib import Path
import sys
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path('.')
r=(root/'src/fix39/wm_fix39_runtime.c').read_text(); h=(root/'src/fix39/wm_fix39_runtime.h').read_text(); n=(root/'src/platform/n64/main.c').read_text()
for s in ['.check_combo_go = source_check_combo_go_port','.adjust_health = source_adjust_health_port','.set_raisearm_bit = source_set_raisearm_bit_port']:
    assert r.count(s)>=3,s
assert 'source_count_button_presses(&g.actors[i])' in r
assert 'master_keep_attached after overlap_collision' in r
assert 'wm_fix39_actor_source_torso_frame' in r and 'wm_fix39_actor_source_torso_frame' in h
assert 'wm_fix39_actor_source_torso_frame((size_t)fix39_runtime_draw_index)' in n
assert 'if(a&&tf) torso=wm_character_sprite_find' in n
print('Combat2CV source runtime/renderer ownership parity: PASS')
