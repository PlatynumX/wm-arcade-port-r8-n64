#!/usr/bin/env python3
from pathlib import Path
import sys,re
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path('.')

if len(sys.argv)>1:
    t=(root/'src/fix39/wm_fix39_runtime.c').read_text(errors='ignore')
else:
    t=(root/'tools/fix39_ani_code_completion_patch.py').read_text(errors='ignore')
required=['get_leap','adjust_facing','adjust_taker_facing','clear_link','inc_loop','face_inside','tbukl_flip','set_xdrift','hit_nearest','set_tbukl_airmode','set_my_pal','set_pinable_bit','set_opp_xy','guy_is_up','guy_is_in','is_guy_up','is_he_in','stand_wrestler','dizzy_wrestler','attach_victim','x_flip','setup_run','dead_or_dying','get_xvel','choose_dir','ck_flip']
missing=[x for x in required if f'!strcmp(label,"{x}")' not in t]
assert not missing, missing
assert 'source_native_get_leap' in t
assert 'WM_STATUS_PINNED' in t
assert 'wm_source_anim_runtime_change(&g.source_anim[i]' in t
assert 'a->x_vel=0x18000' in t and 'a->x_vel=-0x18000' in t
print(f'Combat2DI ANI_CODE completion contract: PASS ({len(required)} newly bound routines)')
