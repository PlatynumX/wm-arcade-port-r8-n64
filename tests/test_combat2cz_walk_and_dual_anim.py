#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path('.')
r=(root/'src/fix39/wm_fix39_runtime.c').read_text()
h=(root/'src/fix39/wm_arcade_source_animation_runtime.h').read_text()
c=(root/'src/fix39/wm_arcade_source_animation_runtime.c').read_text()
need_r=['source_leg_walk_label','source_torso_walk_label','common_anim_label(a,source_leg_walk_label','common_torso_label(a,source_torso_walk_label','g.source_torso[ai].mode_shadow','wm_source_anim_runtime_set_secondary(&g.source_torso[i], true)']
need_h=['mode_shadow','count_shadow','secondary','wm_source_anim_runtime_set_secondary']
need_c=['wm_source_anim_runtime_tick_impl','primary_mode=a->anim_mode','s->mode_shadow=a->anim_mode','s->program==p && !(mode&WM_ARCADE_MODE_END)']
for x in need_r: assert x in r,x
for x in need_h: assert x in h,x
for x in need_c: assert x in c,x
print('Combat2CZ walk animation + independent ANIMODE2 runtime contract: PASS')
