#!/usr/bin/env python3
from pathlib import Path
import sys
repo=Path(sys.argv[1]); p=repo/'src/fix39/wm_fix39_runtime.c'; t=p.read_text()
anchor='static void source_anim_code(wm_arcade_actor_t *a,const char *label,void *user)\n'
if anchor not in t: raise SystemExit('source_anim_code anchor missing; run native callback patch first')
if 'source_native_get_leap' not in t:
    pos=t.find(anchor)
    helper=r'''/* Combat2DH: source-backed ANI_CODE completion pass.
 * These are direct translations of native routines present in the supplied
 * WRESTLE/WRESTLE2/FINISEQ/character ASM payload. */
static void source_native_get_leap(wm_arcade_actor_t *a)
{
    int away=0;
    if(!a)return;
    a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;
    if((a->x_vel|a->z_vel)==0){a->anim_mode|=WM_ARCADE_MODE_STATUS;return;}
    if(a->new_facing_dir==WM_MOVE_UP_LEFT||a->new_facing_dir==WM_MOVE_DOWN_LEFT)away=WM_MOVE_RIGHT;
    else if(a->new_facing_dir==WM_MOVE_UP_RIGHT||a->new_facing_dir==WM_MOVE_DOWN_RIGHT)away=WM_MOVE_LEFT;
    if(away && (a->move_dir&away))a->anim_mode|=WM_ARCADE_MODE_STATUS;
}
static void source_native_adjust_facing(wm_arcade_actor_t *a,int taker)
{
    if(!a)return;
    if(taker){a->facing_dir&=~(WM_MOVE_LEFT|WM_MOVE_DOWN);a->facing_dir|=(WM_MOVE_RIGHT|WM_MOVE_UP);a->obj_control&=(uint16_t)~WM_OBJ_FLIPH;}
    else{a->facing_dir|=(WM_MOVE_LEFT|WM_MOVE_DOWN);a->facing_dir&=~(WM_MOVE_RIGHT|WM_MOVE_UP);a->obj_control|=WM_OBJ_FLIPH;}
}
static void source_native_clear_link(wm_arcade_actor_t *a){if(a)a->attach_proc=0;}
static void source_native_inc_loop(wm_arcade_actor_t *a)
{
    if(!a)return; ++a->usr_var1; a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;
    if(a->usr_var1>2)a->anim_mode|=WM_ARCADE_MODE_STATUS;
}
static void source_native_face_inside(wm_arcade_actor_t *a,int use_opp)
{
    int outside=0, yes=0, reverse;
    wm_arcade_actor_t *o;
    if(!a)return;
    if(use_opp){o=source_native_opp(a);outside=o?o->in_ring:0;}
    if(a->x_int<WM_RING_X_CENTER){if(outside)yes=(a->new_facing_dir&WM_MOVE_LEFT)!=0;}
    else{if(!outside)yes=1;else yes=(a->new_facing_dir&WM_MOVE_LEFT)!=0;}
    reverse=(a->wrestler_num==3||a->wrestler_num==6);
    if(yes^reverse)a->obj_control|=WM_OBJ_FLIPH;else a->obj_control&=(uint16_t)~WM_OBJ_FLIPH;
}
static void source_native_set_xdrift(wm_arcade_actor_t *a)
{
    if(!a||a->in_ring)return;
    if(abs32(a->x_int-WM_RING_X_CENTER)<0x60)return;
    a->x_vel=(a->x_int>WM_RING_X_CENTER)?-0x30000:0x30000;
}
static void source_native_hit_nearest(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *o=source_native_opp(a); if(!a||!o)return;
    a->who_i_hit=o; o->status_flags|=WM_STATUS_PINNED;
}
static void source_native_set_tbukl_airmode(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *o=source_native_opp(a); if(!a)return;
    a->player_mode=(o&&o->player_mode==WM_PMODE_DEAD)?WM_PMODE_INAIR:WM_PMODE_INAIR2;
}
static void source_native_set_my_pal(wm_arcade_actor_t *a){if(!a)return;a->obj_pal=a->my_pal;a->status_flags&=~WM_STATUS_TEMP_PAL;}
static void source_native_set_pinable_bit(wm_arcade_actor_t *a)
{
    if(a)a->status_flags|=WM_STATUS_PINABLE;
}
static void source_native_set_opp_xy(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *o=a?a->who_i_hit:0;int32_t xv;if(!o)return;
    o->y_vel=0x20000;xv=-0x20000;if(!(o->new_facing_dir&WM_MOVE_RIGHT))xv=-xv;o->x_vel=xv;
}
static void source_native_guy_flag(wm_arcade_actor_t *a,int in)
{
    (void)a; if(in)g.native_guy_in=1;else g.native_guy_up=1;
}
static void source_native_is_guy_flag(wm_arcade_actor_t *a,int in)
{
    int v;if(!a)return;v=in?g.native_guy_in:g.native_guy_up;
    a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;if(v)a->anim_mode|=WM_ARCADE_MODE_STATUS;
}
static void source_native_change_primary(wm_arcade_actor_t *a,const char *label)
{
    int i=actor_index(a);if(i<0||!label)return;
    if(wm_source_anim_runtime_change(&g.source_anim[i],a,(uint8_t)a->wrestler_num,label))wm_source_anim_runtime_tick(&g.source_anim[i],a);
}
static void source_native_stand_or_dizzy(wm_arcade_actor_t *a,int dizzy)
{
    static const char *stand[9]={"hrt_stand_anim","rzr_stand_anim","und_stand_anim","yok_stand_anim","shn_stand_anim","bam_stand_anim","dnk_stand_anim",0,"lex_stand_anim"};
    static const char *diz[9]={"hrt_fdizzy_anim","rzr_fdizzy_anim","und_fdizzy_anim","yok_fdizzy_anim","shn_fdizzy_anim","bam_fdizzy_anim","dnk_fdizzy_anim",0,"lex_fdizzy_anim"};
    int w;if(!a)return;w=a->wrestler_num;if(w<0||w>8)return;source_native_change_primary(a,dizzy?diz[w]:stand[w]);
}
static void source_native_attach_victim(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *o=a?a->who_i_hit:0;int i;if(!a||!o)return;
    if(o->player_mode!=WM_PMODE_DEAD)o->player_mode=WM_PMODE_PUPPET;
    o->attach_proc=a;a->attach_proc=o;o->getup_time=0;wm_arcade_wrestler_collisions_off(o);
    i=actor_index(o);if(i>=0&&wm_source_anim_runtime_change(&g.source_anim[i],o,(uint8_t)o->wrestler_num,"wres_slave_anim"))wm_source_anim_runtime_tick(&g.source_anim[i],o);
}
static void source_native_x_flip(wm_arcade_actor_t *a){if(a)a->facing_dir^=(WM_MOVE_LEFT|WM_MOVE_RIGHT);}
static void source_native_setup_run(wm_arcade_actor_t *a)
{
    static const char *run[9]={"hrt_run_anim","rzr_run_anim","und_run_anim","yok_run_anim","shn_run_anim","bam_run_anim","dnk_run_anim","dnk_run_anim","lex_run_anim"};
    int h,w;if(!a)return;h=a->stick_val_cur&(WM_MOVE_LEFT|WM_MOVE_RIGHT);if(!h)h=a->facing_dir&(WM_MOVE_LEFT|WM_MOVE_RIGHT);
    if(h!=(a->facing_dir&(WM_MOVE_LEFT|WM_MOVE_RIGHT))){int v=a->facing_dir&(WM_MOVE_UP|WM_MOVE_DOWN);a->new_facing_dir=h|v;a->facing_dir=a->new_facing_dir;}
    a->getup_time=0;a->usr_var1=0;a->run_time=0;a->move_dir=a->facing_dir&(WM_MOVE_LEFT|WM_MOVE_RIGHT);
    a->facing_dir=a->move_dir|(a->new_facing_dir&(WM_MOVE_UP|WM_MOVE_DOWN));w=a->wrestler_num;if(w>=0&&w<=8)source_native_change_primary(a,run[w]);
    a->player_mode=WM_PMODE_RUNNING;a->delay_butns=1;
}
static void source_native_dead_or_dying(wm_arcade_actor_t *a)
{
    if(!a)return;a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;if(a->i_will_die||a->life<=0)a->anim_mode|=WM_ARCADE_MODE_STATUS;
}
static void source_native_get_xvel(wm_arcade_actor_t *a)
{
    int right;int32_t xv;if(!a)return;right=(a->facing_dir&WM_MOVE_RIGHT)!=0;xv=a->x_vel;
    if(xv==0){a->x_vel=right?0x20000:-0x20000;return;}
    if((right&&xv<0)||(!right&&xv>=0)){a->x_vel=0;return;}
    a->x_vel=right?0x40000:-0x40000;
}
static void source_native_choose_dir(wm_arcade_actor_t *a)
{
    int dir;if(!a)return;dir=(a->obj_control&WM_OBJ_FLIPH)?WM_MOVE_RIGHT:WM_MOVE_LEFT;dir|=WM_MOVE_DOWN;
    a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;if(a->new_facing_dir&WM_MOVE_UP){a->anim_mode|=WM_ARCADE_MODE_STATUS;dir=(dir&~WM_MOVE_DOWN)|WM_MOVE_UP;}a->facing_dir=dir;
}
static void source_native_ck_flip(wm_arcade_actor_t *a)
{
    int f;if(!a)return;if(a->x_int<=WM_RING_X_CENTER){f=WM_MOVE_RIGHT|WM_MOVE_DOWN;if(a->obj_control&WM_OBJ_FLIPH){a->obj_control^=WM_OBJ_FLIPH;f^=(WM_MOVE_LEFT|WM_MOVE_RIGHT);}}
    else{f=WM_MOVE_LEFT|WM_MOVE_DOWN;if(!(a->obj_control&WM_OBJ_FLIPH)){a->obj_control^=WM_OBJ_FLIPH;f^=(WM_MOVE_RIGHT|WM_MOVE_RIGHT);}}a->facing_dir=f;
}
'''
    # add two finish-state fields into runtime state immediately after native_opp_xvel if present
    field='    int32_t native_opp_xvel[WM_FIX39_ACTOR_COUNT];\n'
    if field in t and 'native_guy_up' not in t:
        t=t.replace(field,field+'    int32_t native_guy_up;\n    int32_t native_guy_in;\n',1)
    pos=t.find(anchor)
    if pos<0: raise SystemExit('source_anim_code anchor lost after field insertion')
    t=t[:pos]+helper+t[pos:]
# Repair two earlier partial translations to match source semantics available here.
old='''static void source_native_check_xvel(wm_arcade_actor_t*a){if(!a)return;\nif(a->facing_dir&WM_MOVE_RIGHT){if(a->x_vel<=0)a->x_vel=0x20000;}else if(a->x_vel>=0)a->x_vel=-0x20000;a->z_vel=0;}'''
new='''static void source_native_check_xvel(wm_arcade_actor_t*a){wm_arcade_actor_t*o=source_native_opp(a);if(!a)return;\na->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;if(!o||o->in_ring)return;\nif(a->x_int<=WM_RING_X_CENTER){if(a->x_vel<0){a->x_vel=0x18000;a->anim_mode|=WM_ARCADE_MODE_STATUS;}}\nelse if(a->x_vel>=0){a->x_vel=-0x18000;a->anim_mode|=WM_ARCADE_MODE_STATUS;}}'''
if old in t:t=t.replace(old,new,1)
# Insert new dispatch before diagnostic fallback (after existing set_position line).
needle='''    if(!strcmp(label,"set_position")){source_native_set_position(a);return;}\n'''
if 'if(!strcmp(label,"get_leap"))' not in t:
    if needle not in t: raise SystemExit('native dispatch anchor missing')
    extra=r'''    if(!strcmp(label,"get_leap")){source_native_get_leap(a);return;}
    if(!strcmp(label,"adjust_facing")){source_native_adjust_facing(a,0);return;}
    if(!strcmp(label,"adjust_taker_facing")){source_native_adjust_facing(a,1);return;}
    if(!strcmp(label,"clear_link")){source_native_clear_link(a);return;}
    if(!strcmp(label,"inc_loop")){source_native_inc_loop(a);return;}
    if(!strcmp(label,"face_inside")){source_native_face_inside(a,0);return;}
    if(!strcmp(label,"tbukl_flip")){source_native_face_inside(a,1);return;}
    if(!strcmp(label,"set_xdrift")){source_native_set_xdrift(a);return;}
    if(!strcmp(label,"hit_nearest")){source_native_hit_nearest(a);return;}
    if(!strcmp(label,"set_tbukl_airmode")){source_native_set_tbukl_airmode(a);return;}
    if(!strcmp(label,"set_my_pal")){source_native_set_my_pal(a);return;}
    if(!strcmp(label,"set_pinable_bit")){source_native_set_pinable_bit(a);return;}
    if(!strcmp(label,"set_opp_xy")){source_native_set_opp_xy(a);return;}
    if(!strcmp(label,"guy_is_up")){source_native_guy_flag(a,0);return;}
    if(!strcmp(label,"guy_is_in")){source_native_guy_flag(a,1);return;}
    if(!strcmp(label,"is_guy_up")){source_native_is_guy_flag(a,0);return;}
    if(!strcmp(label,"is_he_in")){source_native_is_guy_flag(a,1);return;}
    if(!strcmp(label,"stand_wrestler")){source_native_stand_or_dizzy(a,0);return;}
    if(!strcmp(label,"dizzy_wrestler")){source_native_stand_or_dizzy(a,1);return;}
    if(!strcmp(label,"attach_victim")){source_native_attach_victim(a);return;}
    if(!strcmp(label,"x_flip")){source_native_x_flip(a);return;}
    if(!strcmp(label,"setup_run")){source_native_setup_run(a);return;}
    if(!strcmp(label,"dead_or_dying")){source_native_dead_or_dying(a);return;}
    if(!strcmp(label,"get_xvel")){source_native_get_xvel(a);return;}
    if(!strcmp(label,"choose_dir")){source_native_choose_dir(a);return;}
    if(!strcmp(label,"ck_flip")){source_native_ck_flip(a);return;}
'''
    t=t.replace(needle,needle+extra,1)
p.write_text(t)
print('Combat2DH missing ANI_CODE native routine completion applied')
