#!/usr/bin/env python3
from __future__ import annotations
import pathlib,re
R=pathlib.Path(__file__).resolve().parents[1]
rt=(R/'src/fix39/wm_arcade_source_animation_runtime.c').read_text(errors='replace')
h=(R/'src/fix39/wm_arcade_source_animation_runtime.h').read_text(errors='replace')
cases=set(map(int,re.findall(r'case\s+(\d+)\s*:',rt)))
# ANIM.EQU defines 0..130 and explicitly comments out 43 as ANI_UNUSED.
assert cases == (set(range(131))-{43}), sorted((set(range(131))-{43})-cases)
for token in (
 'wm_source_anim_runtime_tick','wm_source_anim_runtime_change','WM_SRC_INS_FRAME',
 'wm_arcade_ani_attack_on','wm_arcade_ani_attack_off','wm_arcade_ani_attack_on_z',
 'wm_arcade_anim_detach','wm_arcade_anim_set_attach_from_whoihit',
 'wm_source_target_offsets','source_vm_fault'):
    assert token in rt or token in h, token
# Native ANI_CODE remains an explicit service call rather than being silently dropped.
assert 's->services->code' in rt
assert 'void (*code)' in h
print('Combat2CE ANIM.ASM opcode runtime: PASS (130 active opcodes; 43 is source-unused)')
