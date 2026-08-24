#!/usr/bin/env python3
from pathlib import Path
R=Path(__file__).resolve().parents[1]
r=(R/'src/fix39/wm_arcade_source_animation_runtime.c').read_text(errors='replace')
h=(R/'src/fix39/wm_arcade_source_animation_catalog.h').read_text(errors='replace')
p=(R/'tools/fix39_combat_completion_patch.py').read_text(errors='replace')
# Legacy BP checked that startup friction was flattened into a catalog-init token.
# Combat2CE replaces that flattened model with the complete ANIM.ASM VM: friction
# is now executed by the source ANI_FRICTION opcode (22), while the catalog keeps
# old init metadata only for backwards-compatible generated data consumers.
for s in ['ani_speed','obj_friction','WM_ARCADE_MODE_FRICTION','case 22:']:
    assert s in r, f'missing source animation runtime state/opcode: {s}'
assert 'WM_SRC_ANIM_INIT_FRICTION' in h, 'legacy catalog friction metadata disappeared'
assert 'a->obj_friction=av(i,0)' in r, 'ANI_FRICTION no longer writes OBJ_FRICTION'
assert 'a->anim_mode|=WM_ARCADE_MODE_FRICTION' in r, 'ANI_FRICTION no longer enables friction mode'
assert 'ANIM.ASM::animate_wrestler' in p
assert 'wm_source_anim_runtime_tick(&g.source_anim[i],&g.actors[i])' in p
assert 'wm_arcade_wimp_frame_box_from_sprite' in p
assert 'wm_arcade_character_attack_for_source_frame' in p
assert 'wm_arcade_wrestler_collisions_off(victim)' in p
assert 'presenter_attack' not in p[p.find('/* ANIM/COLLIS source ownership'):p.find('/* WRESTLE.ASM update_links') if '/* WRESTLE.ASM update_links' in p else None]
assert 'WM_BRET_ANIM_GRABFLING_FACE24' in p and 'WM_MOVE_UP' in p
print('Combat2CF source animation/runtime ownership: PASS (full ANIM.ASM VM contract)')
