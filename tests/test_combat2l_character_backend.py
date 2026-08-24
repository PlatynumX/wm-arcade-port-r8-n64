from pathlib import Path
root=Path(__file__).resolve().parents[1]
app=(root/'tools/apply_fix39.py').read_text()
rt=(root/'src/fix39/wm_fix39_runtime.c').read_text()
sa=(root/'src/fix39/wm_arcade_source_attack_frames.c').read_text()
gen=(root/'tools/fix39_character_assets.py').read_text()
assert 'wm_character_sprite_find' in app
assert 'wm_character_visual(f->roster_id' in app
assert 'wm_demo_set_roster' in app
assert 'wm_fix39_match_bind_source_frame_attack' in app
assert 'wm_arcade_character_attack_for_source_frame' in rt
assert 'wm_arcade_character_attack_for_source_frame' in sa
for x in ['bret','razor','taker','yoko','shawn','bam','doink','lex']:
    assert f"'{x}'" in gen

# Combat2P regression: upstream wm_demo_fighter is an anonymous typedef, not
# `typedef struct wm_demo_fighter`. The patcher must anchor the real shape.
assert 'anchor = "typedef struct {\\n    wm_visual_state visual;\\n"' in app
assert 'typedef struct wm_demo_fighter' not in app

print('Combat2P character backend structural test: PASS')
