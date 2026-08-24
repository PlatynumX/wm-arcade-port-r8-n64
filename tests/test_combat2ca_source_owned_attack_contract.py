from pathlib import Path
root=Path(__file__).resolve().parents[1]
r=(root/'src/fix39/wm_fix39_runtime.c').read_text()
assert 'wm_fix39_match_bind_source_frame_attack' in r
assert 'wm_arcade_character_attack_for_source_frame' in r
assert 'g.presenter_attack[i].valid' not in r
assert 'a->x_int = g.presenter_pose' not in r
assert 'a->z_int = g.presenter_pose' not in r
print('Combat2CA source-owned attack contract: PASS')
