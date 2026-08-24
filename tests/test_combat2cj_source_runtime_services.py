#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
r=(root/'src/fix39/wm_fix39_runtime.c').read_text()
v=(root/'src/fix39/wm_arcade_source_animation_runtime.c').read_text()
need=[
 'wm_source_anim_services_t source_anim_services;',
 'wm_source_anim_runtime_bind(&g.source_anim[i],&g.source_anim_services);',
 'g.source_anim_services.code=source_anim_code;',
 'g.source_anim_services.combat_runtime=&g.combat_runtime;',
 'g.source_anim_services.react=&g.react_callbacks;',
 'source_anim_native_am_i_dead',
 'source_anim_native_ckzpos',
 'if (a->delay_meter > 0) a->getup_time = 0;',
 'ANI_ATTACK_ON/OFF',
]
for x in need:
    assert x in r,x
assert 'wm_arcade_character_attack_for_source_frame' not in r[r.find('void wm_fix39_match_tick'):r.find('const wm_arcade_actor_t *wm_fix39_actor')]
assert 'if (a->player_mode == WM_PMODE_ONGROUND) a->player_mode = WM_PMODE_NORMAL;' not in r
assert 'case 0: NEXT(); /* ANIM.ASM _ani_zip' in v
assert 'if(a->i_will_die)' in v and 'xxx_dead_anim' in v
print('Combat2CJ source runtime services/getup/attack ownership: PASS')
