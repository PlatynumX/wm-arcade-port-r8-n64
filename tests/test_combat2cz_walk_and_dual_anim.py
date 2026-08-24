#!/usr/bin/env python3
from pathlib import Path
import sys
if len(sys.argv)>1:
    root=Path(sys.argv[1])
    r=(root/'src/fix39/wm_fix39_runtime.c').read_text(errors='ignore')
    h=(root/'src/fix39/wm_arcade_source_animation_runtime.h').read_text(errors='ignore')
    c=(root/'src/fix39/wm_arcade_source_animation_runtime.c').read_text(errors='ignore')
else:
    root=Path(__file__).resolve().parents[1]
    r=(root/'tools/fix39_execute_walk_source_patch.py').read_text(errors='ignore')+(root/'tools/fix39_dual_anim_channel_patch.py').read_text(errors='ignore')
    h=(root/'tools/fix39_dual_anim_channel_patch.py').read_text(errors='ignore')
    c=h
need_r=['source_leg_walk_label','source_torso_walk_label','common_anim_label(a,source_leg_walk_label','common_torso_label(a,source_torso_walk_label','g.source_torso[ai].mode_shadow','wm_source_anim_runtime_set_secondary(&g.source_torso[i], true)']
need_h=['mode_shadow','count_shadow','secondary','wm_source_anim_runtime_set_secondary']
need_c=['wm_source_anim_runtime_tick_impl','primary_mode=a->anim_mode','s->mode_shadow=a->anim_mode','s->program==p && !(mode&WM_ARCADE_MODE_END)']
for x in need_r: assert x in r,x
for x in need_h: assert x in h,x
for x in need_c: assert x in c,x
print('Combat2CZ walk animation + independent ANIMODE2 runtime contract: PASS')
