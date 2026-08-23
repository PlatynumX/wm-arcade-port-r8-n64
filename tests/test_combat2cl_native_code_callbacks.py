from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
p=(root/'tools/fix39_native_code_callbacks_patch.py').read_text()
required=['pause_opp','store_opp_xvel','merge_xvels','reverse_xvel','clear_opp_counts','head_grab_time','check_xvel','set_opp_facing','half_vels','reattach','ck_dead_opp','halve_bk_xvel','SET_DIR_FACE','free_toss_check','setup_freetoss','set_opp_y','set_wrestler_xflip','hit_ground','setopp_deadanim','SET_OPP_GRAV_LOW','SET_OPP_GRAV_NORM','ckongrnd','get_off4','set_immob','target_whoihit','blocked_vels','SET_OPTIMAL_POSITION']
for s in required:
    assert f'"{s}"' in p, s
assert 'if(x<0){a->x_vel=0;a->z_vel=0;}' in p
print('Combat2CL native ANI_CODE callback translation contract: PASS')
