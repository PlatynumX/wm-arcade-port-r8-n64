#!/usr/bin/env python3
from pathlib import Path
R=Path(__file__).resolve().parents[1]
c=(R/'src/fix39/wm_arcade_source_animation_catalog.c').read_text(errors='replace')
g=(R/'tools/fix39_source_animation_catalog.py').read_text(errors='replace')
assert c.count('{"') >= 1000, 'source animation catalog unexpectedly small'
for s in ['hrt_stand2_anim','hrt_stand4_anim','rzr_4_uprcut_anim','hrt_2_grabfling_anim','hrt_4_grabfling_anim']:
    assert s in c, f'missing source animation {s}'
h=(R/'src/fix39/wm_arcade_source_animation_catalog.h').read_text(errors='replace')
assert 'WM_SRC_ANIM_INIT_FRICTION' in h, 'source friction initialization flag missing'
assert "'rzr_4_uprcut_anim'" in g, 'Razor source UPPERCUT4 mapping drifted'
assert 'WM_MOVE_UP' in g, 'FACE24 source selection must use MOVE_UP'
print('Combat2BQ source animation catalog: PASS')
