#!/usr/bin/env python3
from pathlib import Path
import sys
repo=Path(sys.argv[1]); p=repo/'src/fix39/wm_fix39_runtime.c'; t=p.read_text()
# scratch used by SHNSEQ3 store_opp_xvel/merge_xvels
anchor='    WmFix39CameraState camera;\n'
if 'int32_t native_opp_xvel[WM_FIX39_ACTOR_COUNT];' not in t:
    if anchor not in t: raise SystemExit('camera state anchor missing')
    t=t.replace(anchor,anchor+'    int32_t native_opp_xvel[WM_FIX39_ACTOR_COUNT];\n',1)
# Correct CI no_bk_xvel: original clears X and Z when flying backward.
t=t.replace('if(x<0)a->x_vel=0;','if(x<0){a->x_vel=0;a->z_vel=0;}',1)
insert='static void source_anim_code(wm_arcade_actor_t *a,const char *label,void *user)\n'
if 'source_native_pause_opp' not in t:
    pos=t.find(insert)
    if pos<0: raise SystemExit('source_anim_code anchor missing')
    h=r'''static wm_arcade_actor_t *source_native_opp(wm_arcade_actor_t *a)
{
    if(!a) return 0;
    return a->who_i_hit ? a->who_i_hit : (a->attach_proc ? a->attach_proc : a->smart_target);
}
static void source_native_pause_opp(wm_arcade_actor_t*a){wm_arcade_actor_t*o=source_native_opp(a);if(!o)return;
o->ani_count=25;o->anim_mode|=WM_ARCADE_MODE_UNINT;}
static void source_native_zero_butn(wm_arcade_actor_t*a){if(a)a->delay_butns=0;}
static void source_native_store_opp_xvel(wm_arcade_actor_t*a){int i=actor_index(a);wm_arcade_actor_t*o=a?a->smart_target:0;if(i>=0&&o)g.native_opp_xvel[i]=o->x_vel;}
static void source_native_merge_xvels(wm_arcade_actor_t*a){int i=actor_index(a);if(i>=0&&a)a->x_vel=(g.native_opp_xvel[i]+a->x_vel)>>2;}
static void source_native_reverse_xvel(wm_arcade_actor_t*a){if(a)a->x_vel=-(a->x_vel>>2);}
static void source_native_clear_opp_counts(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->attach_proc:0;if(!o)return;
o->punchb_count=o->blockb_count=o->spunchb_count=o->kickb_count=o->skickb_count=0;}
static void source_native_head_grab_time(wm_arcade_actor_t*a){if(!a)return;
a->last_headhold=g.status.pcnt;source_native_clear_opp_counts(a);}
static void source_native_check_xvel(wm_arcade_actor_t*a){if(!a)return;
if(a->facing_dir&WM_MOVE_RIGHT){if(a->x_vel<=0)a->x_vel=0x20000;}else if(a->x_vel>=0)a->x_vel=-0x20000;a->z_vel=0;}
static void source_native_set_opp_facing(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->attach_proc:0;if(o)o->facing_dir^=(WM_MOVE_LEFT|WM_MOVE_RIGHT);}
static void source_native_half_vels(wm_arcade_actor_t*a){if(a){a->x_vel>>=1;a->y_vel=0x20000;}}
static void source_native_set_opp_xflip(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->attach_proc:0;if(o)o->obj_control^=WM_OBJ_FLIPH;}
static void source_native_reattach(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_i_hit:0;if(!a||!o)return;
a->attach_proc=o;o->attach_proc=a;}
static void source_native_ck_dead_opp(wm_arcade_actor_t*a){wm_arcade_actor_t*o=source_native_opp(a);if(!a)return;
a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;if(o&&o->life<=0)a->anim_mode|=WM_ARCADE_MODE_STATUS;}
static void source_native_go_high(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->smart_target:0;if(o&&o->wrestler_num!=3)o->y_vel=0x40000;}
static void source_native_halve_bk_xvel(wm_arcade_actor_t*a){int32_t x;if(!a)return;
x=a->x_vel;if(!(a->facing_dir&WM_MOVE_RIGHT))x=-x;if(x<0)a->x_vel>>=1;}
static void source_native_fix_bnc_flip(wm_arcade_actor_t*a){if(!a)return;
if(a->x_int<WM_RING_X_CENTER)a->obj_control|=WM_OBJ_FLIPH;else a->obj_control&=(uint16_t)~WM_OBJ_FLIPH;}
static void source_native_set_dir_face(wm_arcade_actor_t*a){if(!a)return;
if(a->in_ring)a->new_facing_dir=(a->x_int<WM_RING_X_CENTER)?10:6;else a->new_facing_dir=(a->x_int<WM_RING_X_CENTER)?6:10;}
static void source_native_set_trgt(wm_arcade_actor_t*a){if(!a)return;
a->tgt_xoff=(a->x_int<WM_RING_X_CENTER)?(WM_RING_X_CENTER-0xf8-60):(WM_RING_X_CENTER+0xf8+60);a->tgt_zoff=WM_RING_Z_CENTER;a->tgt_yoff=WM_MAT_Y;}
static void source_native_ckspin(wm_arcade_actor_t*a){if(a&&!(a->facing_dir&WM_MOVE_UP))a->anim_mode|=WM_ARCADE_MODE_STATUS;}
static void source_native_not_in_ring(wm_arcade_actor_t*a){if(a)a->in_ring=1;}
static void source_native_set_zvel1(wm_arcade_actor_t*a){if(!a)return;
if(a->facing_dir&WM_MOVE_UP)a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;else a->anim_mode|=WM_ARCADE_MODE_STATUS;}
static void source_native_set_zvel2(wm_arcade_actor_t*a){if(a)a->z_vel=-0x50000;}
static void source_native_set_zvel3(wm_arcade_actor_t*a){if(a)a->z_vel=-0x7c000;}
static void source_native_check_raisearm(wm_arcade_actor_t*a){if(!a)return;
if(a->status_flags&WM_STATUS_DID_RAISEARM)a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;else a->anim_mode|=WM_ARCADE_MODE_STATUS;}
static void source_native_buckoff_vels(wm_arcade_actor_t*a){if(!a)return;
a->x_vel=(a->x_int<=WM_RING_X_CENTER)?0x20000:-0x20000;a->z_vel=(a->z_int<=WM_RING_Z_CENTER)?0x40000:-0x40000;a->y_vel=0x50000;}
static void source_native_tgt_ground(wm_arcade_actor_t*a){if(a)a->tgt_yoff=0;}
static void source_native_zero_x(wm_arcade_actor_t*a){if(a&&a->closest_xdist<=64)a->x_vel=0;}
static void source_native_free_toss_check(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->smart_target:0;if(!a)return;
a->anim_mode|=WM_ARCADE_MODE_STATUS;if(o&&abs32(o->z_int-a->z_int)>=15)a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;}
static void source_native_setup_freetoss(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_i_hit:0;if(!a)return;
a->anim_mode=0;if(o){o->immobilize_time=20;a->smart_target=o;}}
static void source_native_clr_climb(wm_arcade_actor_t*a){if(a){a->climbing_thru=0;a->safe_time=1;}}
static void source_native_set_opp_y(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_i_hit:0;int32_t x;if(!o)return;
o->y_vel=0x50000;o->z_vel=0x20000;x=-0x30000;if(!(o->new_facing_dir&WM_MOVE_RIGHT))x=-x;o->x_vel=x;}
static void source_native_set_wrestler_xflip(wm_arcade_actor_t*a){if(!a)return;
if(a->facing_dir&WM_MOVE_RIGHT)a->obj_control&=(uint16_t)~WM_OBJ_FLIPH;else a->obj_control|=WM_OBJ_FLIPH;}
static void source_native_hit_ground(wm_arcade_actor_t*a){if(!a)return;
a->y_int=a->ground_y;a->y_fixed=a->ground_y<<16;}
static void source_native_setopp_deadanim(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->attach_proc:0;if(o)o->status_flags|=WM_STATUS_DEAD_ANIM;}
static void source_native_opp_grav(wm_arcade_actor_t*a,int low){wm_arcade_actor_t*o=a?a->who_i_hit:0;if(o)o->gravity=WM_ARCADE_GRAVITY-(low?0x1000:0);}
static void source_native_ckongrnd(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->smart_target:0;if(!a)return;
a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;if(o&&o->player_mode==WM_PMODE_ONGROUND)a->anim_mode|=WM_ARCADE_MODE_STATUS;}
static void source_native_get_off(wm_arcade_actor_t*a,int which){if(!a)return;
if(which==4){a->z_vel=-0x20000;a->y_vel=0x10000;}else{a->z_vel=0x30000;a->y_vel=0x20000;}}
static void source_native_delay_whoihit(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_i_hit:0;if(o)o->delay_meter=55;}
static void source_native_set_immob(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_i_hit:0;if(o)o->immobilize_time=60;}
static void source_native_target_whoihit(wm_arcade_actor_t*a){if(!a)return;
a->status_flags|=WM_STATUS_SMART_ATTACK;a->smart_target=a->who_i_hit;}
static void source_native_blocked_vels(wm_arcade_actor_t*a){if(a){a->y_vel=0x30000;a->x_vel=-(a->x_vel>>1);}}
static void source_native_optimal_position(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_i_hit:0;int32_t dx;if(!a||!o)return;
dx=0x460000;if(a->facing_dir&WM_MOVE_LEFT)dx=-dx;o->x_fixed=a->x_fixed+dx;o->x_int=o->x_fixed>>16;}
static void source_native_set_position(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_hit_me:0;if(!a||!o)return;/* Source position writes are commented out; palette side effects only. */a->my_pal=a->obj_pal;}
static void __attribute__((unused)) source_native_pause_state(wm_arcade_actor_t*a){if(a)a->anim_mode|=WM_ARCADE_MODE_UNINT;}
'''
    t=t[:pos]+h+t[pos:]
# Insert dispatch lines before diagnostic fallback.
needle='''    if(!strcmp(label,"make_norm")){source_anim_native_make_norm(a);return;}\n'''
if needle not in t: raise SystemExit('dispatch anchor missing')
extra=r'''    if(!strcmp(label,"pause_opp")){source_native_pause_opp(a);return;}
    if(!strcmp(label,"zero_butn")){source_native_zero_butn(a);return;}
    if(!strcmp(label,"store_opp_xvel")){source_native_store_opp_xvel(a);return;}
    if(!strcmp(label,"merge_xvels")){source_native_merge_xvels(a);return;}
    if(!strcmp(label,"reverse_xvel")){source_native_reverse_xvel(a);return;}
    if(!strcmp(label,"clear_opp_counts")){source_native_clear_opp_counts(a);return;}
    if(!strcmp(label,"head_grab_time")){source_native_head_grab_time(a);return;}
    if(!strcmp(label,"check_xvel")){source_native_check_xvel(a);return;}
    if(!strcmp(label,"set_opp_facing")){source_native_set_opp_facing(a);return;}
    if(!strcmp(label,"half_vels")){source_native_half_vels(a);return;}
    if(!strcmp(label,"set_opp_xflip")){source_native_set_opp_xflip(a);return;}
    if(!strcmp(label,"reattach")){source_native_reattach(a);return;}
    if(!strcmp(label,"ck_dead_opp")){source_native_ck_dead_opp(a);return;}
    if(!strcmp(label,"go_high")){source_native_go_high(a);return;}
    if(!strcmp(label,"halve_bk_xvel")){source_native_halve_bk_xvel(a);return;}
    if(!strcmp(label,"fix_bnc_flip")){source_native_fix_bnc_flip(a);return;}
    if(!strcmp(label,"SET_DIR_FACE")){source_native_set_dir_face(a);return;}
    if(!strcmp(label,"set_trgt")){source_native_set_trgt(a);return;}
    if(!strcmp(label,"ckspin")){source_native_ckspin(a);return;}
    if(!strcmp(label,"NOT_IN_RING")){source_native_not_in_ring(a);return;}
    if(!strcmp(label,"set_zvel1")){source_native_set_zvel1(a);return;}
    if(!strcmp(label,"set_zvel2")){source_native_set_zvel2(a);return;}
    if(!strcmp(label,"set_zvel3")){source_native_set_zvel3(a);return;}
    if(!strcmp(label,"check_raisearm_bit")){source_native_check_raisearm(a);return;}
    if(!strcmp(label,"set_buckoff_vels")){source_native_buckoff_vels(a);return;}
    if(!strcmp(label,"tgt_ground")){source_native_tgt_ground(a);return;}
    if(!strcmp(label,"zero_x")||!strcmp(label,"zero_x_4")){source_native_zero_x(a);return;}
    if(!strcmp(label,"free_toss_check")){source_native_free_toss_check(a);return;}
    if(!strcmp(label,"setup_freetoss")){source_native_setup_freetoss(a);return;}
    if(!strcmp(label,"clr_climb")){source_native_clr_climb(a);return;}
    if(!strcmp(label,"set_opp_y")){source_native_set_opp_y(a);return;}
    if(!strcmp(label,"set_wrestler_xflip")){source_native_set_wrestler_xflip(a);return;}
    if(!strcmp(label,"hit_ground")){source_native_hit_ground(a);return;}
    if(!strcmp(label,"setopp_deadanim")){source_native_setopp_deadanim(a);return;}
    if(!strcmp(label,"SET_OPP_GRAV_LOW")){source_native_opp_grav(a,1);return;}
    if(!strcmp(label,"SET_OPP_GRAV_NORM")){source_native_opp_grav(a,0);return;}
    if(!strcmp(label,"ckongrnd")){source_native_ckongrnd(a);return;}
    if(!strcmp(label,"get_off")){source_native_get_off(a,0);return;}
    if(!strcmp(label,"get_off4")){source_native_get_off(a,4);return;}
    if(!strcmp(label,"delay_whoihit")){source_native_delay_whoihit(a);return;}
    if(!strcmp(label,"set_immob")){source_native_set_immob(a);return;}
    if(!strcmp(label,"target_whoihit")){source_native_target_whoihit(a);return;}
    if(!strcmp(label,"blocked_vels")){source_native_blocked_vels(a);return;}
    if(!strcmp(label,"SET_OPTIMAL_POSITION")){source_native_optimal_position(a);return;}
    if(!strcmp(label,"set_position")){source_native_set_position(a);return;}
'''
t=t.replace(needle,needle+extra,1)
p.write_text(t)
print('Combat2CL native ANI_CODE state callback translation applied')
