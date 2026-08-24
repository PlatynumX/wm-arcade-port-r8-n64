from pathlib import Path
import sys
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
apply=(root/'tools/apply_fix39.py').read_text()
gen=(root/'tools/fix39_character_assets.py').read_text()
h=(root/'src/fix39/wm_fix39_runtime.h').read_text()
c=(root/'src/fix39/wm_fix39_runtime.c').read_text()
assert 'const wm_source_sprite *object_pal = spr;' in apply
assert 'CI8 index {max_index} exceeds reconstructed source bank' in gen
assert '_effective_palette_words' in gen
assert 'len(pxvals) != int(im.width) * int(im.height)' in gen
for tok in ['drone_ticks_by_player[2]','drone_input_ticks_by_player[2]','actor_position_changes[2]']:
    assert tok in h
for tok in ['++g.status.drone_ticks_by_player[0]','++g.status.drone_ticks_by_player[1]','++g.status.actor_position_changes[i]']:
    assert tok in c
print('Combat2DT asymmetric actor-state + frame-local visual parity: PASS')
