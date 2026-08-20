#include "wm/arcade/wm_arcade_yoko.h"
#include "wm/arcade/wm_arcade_attach_anim.h"
#include <string.h>

/*
 * Dedicated direct C translation boundary for YOKO.ASM.
 * The arcade source owns its own secret list, mode table and action tables;
 * this module intentionally duplicates that wrestler-local control plumbing
 * rather than routing through a generic six-wrestler behavior engine.
 */

enum action { A_NONE=0,A_PUNCH,A_BLOCK,A_SPUNCH,A_KICK,A_PUNCHKICK,A_SKICK,A_GRABOH };
static const uint8_t action_table[32]={
 A_NONE,A_PUNCH,A_BLOCK,A_BLOCK,A_SPUNCH,A_SPUNCH,A_BLOCK,A_BLOCK,
 A_KICK,A_PUNCHKICK,A_BLOCK,A_BLOCK,A_SPUNCH,A_PUNCHKICK,A_BLOCK,A_BLOCK,
 A_SKICK,A_SKICK,A_BLOCK,A_BLOCK,A_GRABOH,A_GRABOH,A_BLOCK,A_BLOCK,
 A_SKICK,A_PUNCHKICK,A_BLOCK,A_BLOCK,A_GRABOH,A_GRABOH,A_BLOCK,A_BLOCK
};

#define STEP(v,i) {(uint16_t)(v),(uint16_t)(i)}
static const wm_arcade_input_step_t s_grab_fling[]={STEP(WM_B_SPUNCH,WM_J_ALL),STEP(WM_J_AWAY,WM_J_REAL_LR),STEP(WM_J_AWAY,WM_J_REAL_LR)};
static const wm_arcade_input_step_t s_hip_toss[]={STEP(WM_B_PUNCH,WM_J_ALL),STEP(WM_J_AWAY,WM_J_REAL_LR),STEP(WM_J_AWAY,WM_J_REAL_LR)};
static const wm_arcade_input_step_t s_grab_fling2[]={STEP(WM_B_SPUNCH|WM_J_AWAY,WM_J_REAL_LR|WM_J_UP|WM_J_DOWN)};
static const wm_arcade_input_step_t s_hip_toss2[]={STEP(WM_B_PUNCH|WM_J_AWAY,WM_J_REAL_LR|WM_J_UP|WM_J_DOWN)};
static const wm_arcade_input_step_t s_neck30[]={STEP(WM_B_SPUNCH,WM_J_REAL_LR|WM_J_TOWARD|WM_J_AWAY|WM_J_UP),STEP(WM_J_TOWARD,WM_J_REAL_LR),STEP(WM_J_TOWARD,WM_J_REAL_LR)};
static const wm_arcade_input_step_t s_toward_skick[]={STEP(WM_B_SKICK,WM_J_ALL),STEP(WM_J_TOWARD,WM_J_REAL_LR),STEP(WM_J_TOWARD,WM_J_REAL_LR)};
static const wm_arcade_input_step_t s_toward_punch[]={STEP(WM_B_PUNCH,WM_J_ALL),STEP(WM_J_TOWARD,WM_J_REAL_LR),STEP(WM_J_TOWARD,WM_J_REAL_LR)};
static const wm_arcade_input_step_t s_punch_qcf[]={STEP(WM_B_PUNCH,WM_J_ALL),STEP(WM_J_TOWARD,WM_J_REAL_LR),STEP(WM_J_DOWN_TOWARD,WM_J_REAL_LR),STEP(WM_J_DOWN,WM_J_REAL_LR)};

static const wm_arcade_input_pattern_t secret_patterns[]={
    {"charge_salt",NULL,0,0},
    {"neck_grab",s_neck30,3,30},
    {"grab_fling",s_grab_fling,3,32},
    {"hip_toss",s_hip_toss,3,32},
    {"grab_fling2",s_grab_fling2,1,10},
    {"hip_toss2",s_hip_toss2,1,10},
    {"scissors",s_toward_skick,3,32},
    {"gut_push",s_toward_punch,3,40},
    {"jabs",s_punch_qcf,4,50}
};
static const char *const special_processes[]={
    "yok_hdhold_combo1",
    "yok_hdhold_scissor",
    "yok_hdhold_suplex",
    "yok_salt_throw",
    "yok_grab_toss_air",
    "yok_hdhold_combo2",
    "std_walk_fast",
    "std_taunt",
    "yok_finish_move1",
    "yok_finish_move2"
};

const wm_arcade_wrestler_profile_t wm_arcade_profile_yoko={
    WM_ROSTER_YOKO,"Yokozuna","YOKO.ASM",2788,"yok",WM_BTN_PUNCH,85,
    secret_patterns,sizeof secret_patterns/sizeof secret_patterns[0],
    special_processes,sizeof special_processes/sizeof special_processes[0]
};

struct labels {
    const char *stand2,*stand4,*torso2,*torso4,*fall,*block,*push,*run,*climbdown,*pin2,*pin4,*raise2,*raise4;
    const char *punch2,*punch4,*close2,*close4,*ground2,*ground4,*kick2,*kick4,*knee2,*knee4,*stomp2,*stomp4;
    const char *flykick,*turn_punch,*turn_kick,*headhold2,*headhold,*headheld;
};
static const struct labels L={
    "yok_stand2_anim",
    "yok_stand4_anim",
    "yok_torso2_anim",
    "yok_torso4_anim",
    "yok_fall_back_anim",
    "yok_4_block_anim",
    "yok_4_push_anim",
    "yok_run2_anim",
    "yok_climb_down_anim",
    "yok_2_pin_anim",
    "yok_4_pin_anim",
    "yok_2_raise_arm_anim",
    "yok_4_raise_arm_anim",
    "yok_2_punch_anim",
    "yok_4_punch_anim",
    "yok_heldheadbutt_rpt_anim",
    "yok_heldheadbutt_rpt_anim",
    "yok_2_lbowdrop_anim",
    "yok_4_lbowdrop_anim",
    "yok_2_kick_anim",
    "yok_4_kick_anim",
    "yok_2_knee_anim",
    "yok_4_knee_anim",
    "yok_2_stomp_anim",
    "yok_4_stomp_anim",
    "yok_scissor_anim",
    "yok_tbukl_buttdrop_anim",
    "yok_tbukl_buttdrop_anim",
    "yok_3_head_hold2_anim",
    "yok_3_head_hold_anim",
    "yok_3_head_held_stand_anim"
};

static void setmode(wm_arcade_actor_t*a,uint16_t m){if(a&&a->player_mode!=WM_PMODE_DEAD)a->player_mode=m;}
static int groundish(const wm_arcade_actor_t*o){return o&&(o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD);}
static int nearxy(const wm_arcade_actor_t*a,int x,int z){return a&&a->closest_xdist<x&&a->closest_zdist<z;}
static int face2(const wm_arcade_actor_t*a){return a&&(a->facing_dir&WM_MOVE_RIGHT);}
static const char *face_label(const char*l2,const char*l4,const wm_arcade_actor_t*a){return face2(a)?l2:l4;}
static void anim(wm_arcade_actor_t*a,const char*l,const wm_arcade_yoko_callbacks_t*c){if(c&&c->change_anim_label&&l)c->change_anim_label(a,l,c->user);}
static void snd(wm_arcade_actor_t*a,const char*l,const wm_arcade_yoko_callbacks_t*c){if(c&&c->sound_label&&l)c->sound_label(a,l,c->user);}
static void startsp(wm_arcade_actor_t*a,const char*l,const wm_arcade_yoko_callbacks_t*c){if(!a||!l)return;if(c&&c->resolve_label_token)a->special_move_addr=c->resolve_label_token(l,c->user);if(c&&c->start_special_label)c->start_special_label(a,l,c->user);}

static int do_block(wm_arcade_actor_t*a,const wm_arcade_yoko_env_t*e,const wm_arcade_yoko_callbacks_t*c){
    if(e&&e->blocking_off)return 0;
    anim(a,L.block,c);
    a->block_time=0;
    snd(a,"BLOCK_WOOSH",c);
    setmode(a,WM_PMODE_BLOCK);
    return 1;
}
static void basic_punch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_yoko_callbacks_t*c){
    int cx=62,cz=95; int gx=160,gz=140;
    if(groundish(o)&&nearxy(a,gx,gz)){anim(a,face_label(L.ground2,L.ground4,a),c);snd(a,"LBOWDROP_T1/LBOWDROP_T2",c);return;}
    if(nearxy(a,cx,cz)){anim(a,face_label(L.close2,L.close4,a),c);snd(a,"HDBUTT_T1/HDBUTT_T2",c);}
    else {anim(a,face_label(L.punch2,L.punch4,a),c);snd(a,"PUNCH_T1/PUNCH_T2",c);}
}
static void basic_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_yoko_callbacks_t*c){
    int cx=60,cz=50;
    if(groundish(o)&&nearxy(a,160,140))anim(a,face_label(L.stomp2,L.stomp4,a),c);
    else if(nearxy(a,cx,cz))anim(a,face_label(L.knee2,L.knee4,a),c);
    else anim(a,face_label(L.kick2,L.kick4,a),c);
    snd(a,"KICK_T1/KICK_T2",c);
}
static void super_punch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_yoko_callbacks_t*c){
    if(groundish(o)&&nearxy(a,160,140)){anim(a,face_label(L.ground2,L.ground4,a),c);return;}
    if(a->stick_val_cur&WM_MOVE_DOWN) anim(a,"yok_4_uppercut_anim",c); else if(a->closest_xdist<=50&&a->closest_zdist<92) anim(a,face_label("yok_2_jabs_anim","yok_4_jabs_anim",a),c); else anim(a,face_label("yok_2_slap2_anim","yok_4_slap2_anim",a),c); snd(a,"SPUNCH",c);
}
static void super_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_yoko_callbacks_t*c){if(!groundish(o)){anim(a,"yok_scissor_anim",c);setmode(a,WM_PMODE_INAIR);snd(a,"GRABHOLD_T1/GRABHOLD_T2",c);}else basic_kick(a,o,c);}

static wm_arcade_yoko_step_result_t mode_normal(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_yoko_env_t*e,const wm_arcade_yoko_callbacks_t*c){
    uint8_t ac;
    if(a->anim_mode&WM_MODE_UNINT)return WM_YOKO_STEP_IDLE;
    if(a->i_will_die&&!a->immobilize_time){anim(a,L.fall,c);if(c&&c->adjust_health)c->adjust_health(a,-10,c->user);setmode(a,WM_PMODE_DEAD);a->i_will_die=0;return WM_YOKO_STEP_ACTION;}
    if(o&&o->player_mode==WM_PMODE_DEAD&&!(o->status_flags&WM_STATUS_ZOMBIE)){
        int reciprocal=a->attach_proc&&a->attach_proc->attach_proc==a;
        if(!reciprocal){
            if((c&&c->teammate_pin&&c->teammate_pin(a,c->user))||(c&&c->raisearm_check&&c->raisearm_check(a,c->user))){anim(a,face_label(L.raise2,L.raise4,a),c);if(c->set_raisearm_bit)c->set_raisearm_bit(a,c->user);return WM_YOKO_STEP_ACTION;}
            if(a->but_val_cur&&c&&c->can_pin&&c->can_pin(a,o,c->user)){
            anim(a,face_label(L.pin2,L.pin4,a),c); a->status_flags |= WM_STATUS_DID_PIN;
                if(c->drone_change_back)c->drone_change_back(a,c->user);
                return WM_YOKO_STEP_ACTION;
            }
        }
    }
    if(a->immobilize_time){a->move_dir=0;if(c&&c->execute_walk)c->execute_walk(a,c->user);return WM_YOKO_STEP_EXTERNAL;}
    if((a->but_val_cur&WM_BTN_BLOCK)&&do_block(a,e,c))return WM_YOKO_STEP_ACTION;
    ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
    if((a->but_val_cur&WM_BTN_ATTACK_MASK)==(WM_BTN_PUNCH|WM_BTN_KICK))ac=A_PUNCHKICK;
    switch(ac){case A_PUNCH:basic_punch(a,o,c);break;case A_BLOCK:(void)do_block(a,e,c);break;case A_SPUNCH:super_punch(a,o,c);break;case A_KICK:basic_kick(a,o,c);break;case A_PUNCHKICK:anim(a,"start_run_anim",c);break;case A_SKICK:case A_GRABOH:super_kick(a,o,c);break;default:break;}
    if(a->anim_mode&WM_MODE_UNINT)return WM_YOKO_STEP_ACTION;
    a->move_dir=a->stick_val_cur;
    if(c&&c->climb_turnbuckle&&c->climb_turnbuckle(a,c->user)){if(c->jump_rope_audio)c->jump_rope_audio(a,c->user);return WM_YOKO_STEP_EXTERNAL;}
    if(c&&c->execute_walk)c->execute_walk(a,c->user);
    return WM_YOKO_STEP_ACTION;
}
static wm_arcade_yoko_step_result_t mode_running(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_yoko_env_t*e,const wm_arcade_yoko_callbacks_t*c){
    (void)o;
    int32_t v=0x00060000; a->run_time++; if(!a->usr_var1){if(c&&c->bounce_off_ropes)c->bounce_off_ropes(a,c->user);if(e&&e->hyper_speed_on>0&&e->hyper_speed_on<15)v<<=e->hyper_speed_on;if(!(a->move_dir&WM_MOVE_RIGHT))v=-v;a->x_vel=v;}
    if(a->stick_val_cur&WM_MOVE_UP)a->z_vel=-0x00020000;else if(a->stick_val_cur&WM_MOVE_DOWN)a->z_vel=0x00020000;else a->z_vel=0; if(a->getup_time||a->delay_butns)return WM_YOKO_STEP_IDLE;
    switch(action_table[a->but_val_down&WM_BTN_ATTACK_MASK]){
    case A_BLOCK:a->x_vel>>=1;setmode(a,WM_PMODE_NORMAL);(void)do_block(a,e,c);return WM_YOKO_STEP_ACTION;
    case A_KICK:case A_SKICK:if(c&&c->ck_ignore&&c->ck_ignore(a,c->user))return WM_YOKO_STEP_IDLE;anim(a,"yok_scissor_anim",c);setmode(a,WM_PMODE_INAIR);return WM_YOKO_STEP_ACTION;
    case A_PUNCH:case A_SPUNCH:case A_PUNCHKICK:case A_GRABOH:anim(a,face_label("yok_2_run_slap_anim","yok_4_run_slap_anim",a),c);return WM_YOKO_STEP_ACTION;
    default:return WM_YOKO_STEP_IDLE;}
}
static wm_arcade_yoko_step_result_t mode_bouncing(wm_arcade_actor_t*a,const wm_arcade_yoko_callbacks_t*c){a->x_vel=0;a->z_vel=0;if(a->anim_mode&WM_MODE_END){a->move_dir^=(WM_MOVE_LEFT+WM_MOVE_RIGHT);a->facing_dir=(a->new_facing_dir&(WM_MOVE_UP+WM_MOVE_DOWN))|a->move_dir;anim(a,L.run,c);setmode(a,WM_PMODE_RUNNING);return WM_YOKO_STEP_ACTION;}return WM_YOKO_STEP_IDLE;}
static wm_arcade_yoko_step_result_t mode_turn(wm_arcade_actor_t*a,const wm_arcade_yoko_callbacks_t*c){
    uint8_t ac; if(a->stick_val_cur&WM_MOVE_DOWN){anim(a,L.climbdown,c);setmode(a,WM_PMODE_CLIMBTURNBKL);return WM_YOKO_STEP_ACTION;} ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];if(ac==A_NONE)return WM_YOKO_STEP_IDLE;setmode(a,WM_PMODE_INAIR);anim(a,L.turn_punch,c);snd(a,"TURNDIVE",c);if(c&&c->jump_rope_audio)c->jump_rope_audio(a,c->user);return WM_YOKO_STEP_ACTION;
}
static wm_arcade_yoko_step_result_t mode_block(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_yoko_env_t*e,const wm_arcade_yoko_callbacks_t*c){
    a->block_time++;if(a->block_time>=160&&o&&o->player_mode==WM_PMODE_BLOCK&&a->closest_xdist<0x61&&a->closest_xdist>=45&&a->closest_zdist<30){setmode(a,WM_PMODE_NORMAL);anim(a,L.push,c);snd(a,"PUSH",c);return WM_YOKO_STEP_ACTION;}
    if(!(a->but_val_down&WM_BTN_ATTACK_MASK))return WM_YOKO_STEP_IDLE;
    if(!(a->but_val_cur&WM_BTN_BLOCK)){setmode(a,WM_PMODE_NORMAL);return WM_YOKO_STEP_ACTION;}
    if((a->but_val_down&3)==1||(a->but_val_down&3)==3){setmode(a,WM_PMODE_NORMAL);anim(a,L.push,c);return WM_YOKO_STEP_ACTION;}
    (void)e;
    return WM_YOKO_STEP_IDLE;
}
static wm_arcade_yoko_step_result_t mode_headhold(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_yoko_callbacks_t*c){
    uint8_t ac;if(!o||o->player_mode!=WM_PMODE_HEADHELD){a->z_fixed-=6<<16;setmode(a,WM_PMODE_NORMAL);return WM_YOKO_STEP_ACTION;}if(a->anim_mode&WM_MODE_UNINT)return WM_YOKO_STEP_IDLE;ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
    if(ac==A_PUNCH||ac==A_SPUNCH||ac==A_KICK||ac==A_PUNCHKICK){if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);}

    if(ac==A_PUNCH||ac==A_KICK||ac==A_PUNCHKICK){anim(a,L.close4,c);return WM_YOKO_STEP_ACTION;}return WM_YOKO_STEP_IDLE;
}

wm_arcade_yoko_step_result_t wm_arcade_move_yoko(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_yoko_env_t*e,const wm_arcade_yoko_callbacks_t*c){
    if(!a)return WM_YOKO_STEP_IDLE;
    if(c&&c->check_secret_moves)c->check_secret_moves(a,secret_patterns,sizeof secret_patterns/sizeof secret_patterns[0],c->user);
    switch(a->player_mode){case WM_PMODE_NORMAL:case 18:case 22:case 23:return mode_normal(a,o,e,c);case WM_PMODE_RUNNING:return mode_running(a,o,e,c);case WM_PMODE_ATTACHED:if(c&&c->keep_attached)c->keep_attached(a,c->user);else(void)wm_arcade_keep_attached(a);if(!a->attach_proc){setmode(a,WM_PMODE_NORMAL);a->anim_mode=0;}return WM_YOKO_STEP_EXTERNAL;case WM_PMODE_BOUNCING:return mode_bouncing(a,c);case WM_PMODE_ONTURNBKL:return mode_turn(a,c);case WM_PMODE_BLOCK:return mode_block(a,o,e,c);case WM_PMODE_DEAD:if(c&&c->mode_dead)c->mode_dead(a,c->user);return WM_YOKO_STEP_EXTERNAL;case WM_PMODE_WAITANIM:if((a->anim_mode&WM_MODE_END)&&c&&c->code_addr)c->code_addr(a,(uint32_t)a->code_addr,c->user);return WM_YOKO_STEP_EXTERNAL;case WM_PMODE_MASTER:if(c&&c->master_keep_attached)c->master_keep_attached(a,c->user);else(void)wm_arcade_master_keep_attached(a);return WM_YOKO_STEP_EXTERNAL;case WM_PMODE_HEADHOLD:return mode_headhold(a,o,c);case WM_PMODE_HEADHELD:if((a->anim_mode&WM_MODE_NOGRAVITY)&&c&&c->mode_choking)c->mode_choking(a,c->user);else if(!a->attach_proc&&a->y_int<=a->ground_y)anim(a,L.headheld,c);return WM_YOKO_STEP_EXTERNAL;case WM_PMODE_PUPPET:if(c&&c->mode_puppet)c->mode_puppet(a,c->user);return WM_YOKO_STEP_EXTERNAL;case WM_PMODE_INAIR2:if(c&&c->mode_inair2)c->mode_inair2(a,c->user);return WM_YOKO_STEP_EXTERNAL;case WM_PMODE_CHOKING:if(c&&c->mode_choking)c->mode_choking(a,c->user);return WM_YOKO_STEP_EXTERNAL;default:return WM_YOKO_STEP_IDLE;}
}

static int reject_common(wm_arcade_actor_t*a,wm_arcade_actor_t*o){return !a||!o||(a->anim_mode&WM_MODE_UNINT)||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_HEADHELD||o->player_mode==WM_PMODE_ATTACHED;}
int wm_arcade_yoko_release_charge(wm_arcade_actor_t*a,wm_arcade_actor_t*o,uint16_t ticks,const wm_arcade_yoko_callbacks_t*c){if(!a||ticks<85)return 0;if(a->player_mode==WM_PMODE_HEADHELD||a->player_mode==WM_PMODE_HEADHOLD||(a->anim_mode&WM_MODE_UNINT))return 0;if(!o||o->player_mode==WM_PMODE_ATTACHED)return 0; anim(a,face_label("yok_2_salt_anim","yok_4_salt_anim",a),c); a->run_time=0; setmode(a,WM_PMODE_NORMAL); snd(a,"HDBUTT_T1/HDBUTT_T2",c); return 1;}
int wm_arcade_yoko_fire_secret(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_yoko_secret_id_t id,uint32_t pcnt,const wm_arcade_yoko_callbacks_t*c){
    switch(id){
    case WM_YOKO_SECRET_NECK_GRAB:
        if(reject_common(a,o)||groundish(o))return 0;
        if((uint32_t)(pcnt-a->last_headhold)<120) anim(a,"fake_head_hold3",c);
        else if(a->closest_xdist<=80) anim(a,L.headhold2,c); else anim(a,L.headhold,c);
        return 1; case WM_YOKO_SECRET_GRAB_FLING: if(reject_common(a,o))return 0; anim(a,face_label(L.headhold2,L.headhold,a),c); snd(a,"GRABFLING",c); return 1; case WM_YOKO_SECRET_GRAB_FLING2: if(reject_common(a,o))return 0; anim(a,face_label(L.headhold2,L.headhold,a),c); snd(a,"GRABFLING",c); return 1; case WM_YOKO_SECRET_HIP_TOSS: if(reject_common(a,o)||groundish(o))return 0; if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0; startsp(a,"hip_toss",c); snd(a,"GRABFLING",c); return 1; case WM_YOKO_SECRET_HIP_TOSS2: if(reject_common(a,o)||groundish(o))return 0; if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0; startsp(a,"hip_toss",c); snd(a,"GRABFLING",c); return 1;
    case WM_YOKO_SECRET_SCISSORS: if(reject_common(a,o)||groundish(o))return 0; anim(a,"yok_scissor_anim",c); return 1;
    case WM_YOKO_SECRET_GUT_PUSH: if(reject_common(a,o)||groundish(o))return 0; anim(a,face_label("yok_2_gut_push_anim","yok_4_gut_push_anim",a),c); return 1;
    case WM_YOKO_SECRET_JABS: if(reject_common(a,o))return 0; anim(a,face_label("yok_2_jabs_anim","yok_4_jabs_anim",a),c); return 1;
    default:return 0;}
}
int wm_arcade_yoko_fire_monitor(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_yoko_monitor_id_t id,const wm_arcade_yoko_env_t*e,int opponent_attack_is_leaping,const wm_arcade_yoko_callbacks_t*c){
    wm_arcade_actor_t*t;const char*s;(void)e;(void)opponent_attack_is_leaping;if(!a||(unsigned)id>=sizeof special_processes/sizeof special_processes[0])return 0;s=special_processes[(unsigned)id];
    if(strstr(s,"hdhold_")!=NULL){if(a->player_mode==WM_PMODE_HEADHELD){t=a->who_hit_me?a->who_hit_me:o;if(!t)return 0;a->smart_target=t;t->immobilize_time=15;if(c&&c->do_reversal)c->do_reversal(a,c->user);if(c&&c->do_reversal_message)c->do_reversal_message(a,c->user);}else{t=a->who_i_hit?a->who_i_hit:o;if(a->player_mode!=WM_PMODE_HEADHOLD||!t)return 0;a->smart_target=t;t->immobilize_time=15;}if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);}
    if(strstr(s,"combo")!=NULL&&c&&c->check_combo_go&&c->check_combo_go(a,c->user)<0)return 0;
    startsp(a,s,c);
    return 1;
}
