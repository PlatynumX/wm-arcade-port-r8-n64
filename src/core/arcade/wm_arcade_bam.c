#include "wm/arcade/wm_arcade_bam.h"
#include "wm/arcade/wm_arcade_attach_anim.h"
#include "wm/arcade/wm_arcade_jjxm.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include <string.h>

/*
 * Dedicated direct C translation boundary for BAM.ASM.
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
static const wm_arcade_input_step_t s_neck32[]={STEP(WM_B_SPUNCH,WM_J_ALL),STEP(WM_J_TOWARD,WM_J_REAL_LR),STEP(WM_J_TOWARD,WM_J_REAL_LR)};
static const wm_arcade_input_step_t s_away_skick[]={STEP(WM_B_SKICK,WM_J_ALL),STEP(WM_J_AWAY,WM_J_REAL_LR|WM_J_UP|WM_J_DOWN),STEP(WM_J_AWAY,WM_J_REAL_LR|WM_J_UP|WM_J_DOWN)};
static const wm_arcade_input_step_t s_punch_dd[]={STEP(WM_B_PUNCH,WM_J_ALL),STEP(WM_J_DOWN,WM_J_REAL_LR),STEP(WM_J_DOWN,WM_J_REAL_LR)};

static const wm_arcade_input_pattern_t secret_patterns[]={
    {"firepnch",NULL,0,0},
    {"neck_grab",s_neck32,3,32},
    {"grab_fling",s_grab_fling,3,32},
    {"hip_toss",s_hip_toss,3,32},
    {"grab_fling2",s_grab_fling2,1,10},
    {"hip_toss2",s_hip_toss2,1,10},
    {"jumpkick",s_away_skick,3,32},
    {"grab_fling2",s_grab_fling2,1,10},
    {"hip_toss2",s_hip_toss2,1,10},
    {"napalm",s_punch_dd,3,50}
};
/* WRESTLE2.ASM's bam_smove_table, as the assembler built it.
   The finishing-move entries every one of these tables carries sit
   inside `.if NUM_BAM_FINISHES`, and GAME.EQU:581 sets that switch to
   0 -- so none of them was assembled. Generated as
   wm_wrestler_smoves[] (wm/wrestler_anim_tables.h); a source-tool test
   holds this copy to it. */
static const char *const special_processes[]={
    "bam_charge_neckbreaker",
    "bam_hdhold_combo1",
    "bam_hdhold_pile",
    "bam_hdhold_pogo",
    "bam_hdhold_combo2",
    "bam_grab_toss_air",
    "std_walk_fast",
    "std_taunt"
};

const wm_arcade_wrestler_profile_t wm_arcade_profile_bam={
    WM_ROSTER_BAM,"Bam Bam Bigelow","BAM.ASM",2937,"bam",WM_BTN_PUNCH,85,
    secret_patterns,sizeof secret_patterns/sizeof secret_patterns[0],
    special_processes,sizeof special_processes/sizeof special_processes[0]
};

struct labels {
    const char *stand2,*stand4,*torso2,*torso4,*fall,*block,*push,*run,*climbdown,*pin2,*pin4,*raise2,*raise4;
    const char *punch2,*punch4,*close2,*close4,*ground2,*ground4,*kick2,*kick4,*knee2,*knee4,*stomp2,*stomp4;
    const char *flykick,*turn_punch,*turn_kick,*headhold2,*headhold,*headheld;
    /* DOINK.ASM:3316 bozo_check's two call sites -- the power move
       at the top of mode_headhold and the reversal at the top of
       mode_headheld. The pair alternates on PCNT's low bit. */
    const char *bozo_a,*bozo_b,*bozo_hh_b,*bozo_snd;
};
static const struct labels L={
    "bam_stand2_anim",
    "bam_stand4_anim",
    "bam_torso2_anim",
    "bam_torso4_anim",
    "bam_fall_back_anim",
    "bam_4_block_anim",
    "bam_4_push_anim",
    "bam_run2_anim",
    "bam_climb_down_anim",
    "bam_2_pin_anim",
    "bam_4_pin_anim",
    "bam_2_raise_arm_anim",
    "bam_4_raise_arm_anim",
    "bam_2_punch_anim",
    "bam_4_punch_anim",
    "bam_2_butt_anim",
    "bam_4_butt_anim",
    "bam_2_lbowdrop_anim",
    "bam_4_lbowdrop_anim",
    "bam_2_kick_anim",
    "bam_4_kick_anim",
    "bam_2_knee_anim",
    "bam_4_knee_anim",
    "bam_2_stomp_anim",
    "bam_4_stomp_anim",
    "bam_flying_kick_anim",
    "bam_bellyflop_anim",
    "bam_bellyflop_anim",
    "bam_3_head_hold2_anim",
    "bam_3_head_hold_anim",
    "bam_3_head_held_stand_anim",
    "bam_pogo_anim",
    "bam_neckbreaker_anim",
    "bam_neckbreaker_anim",
    "GRABHOLD",
};

static void setmode(wm_arcade_actor_t*a,uint16_t m){if(a&&a->player_mode!=WM_PMODE_DEAD)a->player_mode=m;}
static int groundish(const wm_arcade_actor_t*o){return o&&(o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD);}
static int face2(const wm_arcade_actor_t*a){return a&&(a->facing_dir&WM_MOVE_RIGHT);}
static const char *face_label(const char*l2,const char*l4,const wm_arcade_actor_t*a){return face2(a)?l2:l4;}
static void anim(wm_arcade_actor_t*a,const char*l,const wm_arcade_bam_callbacks_t*c){if(c&&c->change_anim_label&&l)c->change_anim_label(a,l,c->user);}
static void snd(wm_arcade_actor_t*a,const char*l,const wm_arcade_bam_callbacks_t*c){if(c&&c->sound_label&&l)c->sound_label(a,l,c->user);}
static void startsp(wm_arcade_actor_t*a,const char*l,const wm_arcade_bam_callbacks_t*c){if(!a||!l)return;if(c&&c->resolve_label_token)a->special_move_addr=c->resolve_label_token(l,c->user);if(c&&c->start_special_label)c->start_special_label(a,l,c->user);}

static int do_block(wm_arcade_actor_t*a,const wm_arcade_bam_env_t*e,const wm_arcade_bam_callbacks_t*c){
    if(e&&e->blocking_off)return 0;
    anim(a,L.block,c);
    a->block_time=0;
    snd(a,"BLOCK_WOOSH",c);
    setmode(a,WM_PMODE_BLOCK);
    return 1;
}
/*
 * BAM.ASM's six JJXM tables (JJXM.H; wm/arcade/wm_arcade_jjxm.h).
 * Two details the approximation had wrong beyond the usual: Bam Bam's
 * hair grab and elbow drop both play KICK rather than LBOWDROP, and
 * his super kick is one animation for BOTH arms of the close test --
 * #skick_special and #skick_kick are two labels on one instruction.
 */
static const char *jjxm(const char*section,const char*entry,
                        const wm_arcade_actor_t*a,const wm_arcade_actor_t*o){
    return wm_jjxm_pick("BAM",section,entry,a,o);
}
#define is(t,n) wm_jjxm_is((t),(n))

/* BAM.ASM:1494 #punch_punch, alias std_punch. */
static void bam_punch_punch(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    anim(a,face_label(L.punch2,L.punch4,a),c); snd(a,"PUNCH",c);
}
/* :1504 #punch_hdbutt */
static void bam_punch_hdbutt(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    anim(a,face_label(L.close2,L.close4,a),c); snd(a,"HDBUTT",c);
}
/* :1513 #punch_lbowdrop */
static void bam_punch_lbowdrop(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    anim(a,face_label(L.ground2,L.ground4,a),c); snd(a,"LBOWDROP",c);
}
/* :1597 #spunch_jump -- the slap, and the only place Bam Bam's super
   punch plays SPUNCH at all. */
static void bam_spunch_jump(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    anim(a,face_label("bam_2_slap_anim","bam_4_slap_anim",a),c); snd(a,"SPUNCH",c);
}
/* :1607 #spunch_special. One threshold, 55 on X, and past it the move
   is the jump slap. */
static void bam_spunch_special(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    if(a->closest_xdist>55){ bam_spunch_jump(a,c); return; }
    anim(a,face_label("bam_2_butts_anim","bam_4_butts_anim",a),c); snd(a,"HDBUTT",c);
}
/* :1636 #spunch_lbowdrop -- the hair grab, and note the sound: KICK on
   both arms, not LBOWDROP. */
static void bam_spunch_lbowdrop(wm_arcade_actor_t*a,wm_arcade_actor_t*o,
                                const wm_arcade_bam_callbacks_t*c){
    int hair=0;
    if(o&&o->player_mode!=WM_PMODE_DEAD){
        int32_t dx=a->x_fixed-o->x_fixed;
        if(dx<0)dx=-dx;
        if((dx>>16)>=0x20&&
           ((a->obj_control&WM_OBJ_FLIPH)!=(o->obj_control&WM_OBJ_FLIPH)))
            hair=1;
    }
    if(hair) anim(a,face_label("bam_2_hair_pickup_anim","bam_4_hair_pickup_anim",a),c);
    else     anim(a,face_label(L.ground2,L.ground4,a),c);
    snd(a,"KICK",c);
}
/* :2665 do_pile. Bam Bam's runs FIND_AND_KILL_ENDLESS FIRST, before
   the USR_VAR2 test -- so a headheld opponent ends an endless combo
   whether or not the pile driver comes out. Without the stick down it
   falls into mode_headhold's own #punch, the knee to the head. */
static void bam_do_pile(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);
    if(!a->usr_var2) return;
    if(a->stick_val_cur&WM_MOVE_DOWN){
        snd(a,"GRABHOLD",c); anim(a,"bam_3_pile_driver_anim",c); return;
    }
    if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);
    snd(a,"UPRCUT",c); anim(a,"bam_4_knee2_anim",c);
}
/* :1741 #kick_kick (std_kick), :1751 #kick_knee (std_knee), :1761
   #kick_stomp (alias attack_stomp). */
static void bam_kick_kick(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    anim(a,face_label(L.kick2,L.kick4,a),c); snd(a,"KICK",c);
}
static void bam_kick_knee(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    anim(a,face_label(L.knee2,L.knee4,a),c); snd(a,"KICK",c);
}
static void bam_kick_stomp(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    anim(a,face_label(L.stomp2,L.stomp4,a),c); snd(a,"KICK",c);
}
/* :1731 #kick_TB */
static void bam_kick_tb(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    anim(a,"bam_kick_TB_anim",c); snd(a,"KICK",c);
}
/* :1827 #skick_special and :1828 #skick_kick -- two labels, one body,
   so the super-kick table's near/far split has the same answer either
   side of it. SPUNCH, not FLYKICK. */
static void bam_skick_kick(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    anim(a,face_label("bam_2_superkick_anim","bam_4_superkick_anim",a),c);
    snd(a,"SPUNCH",c);
}
/* :1851 #graboh, "both super buttons at the same time". */
static void bam_graboh(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    anim(a,"bam_4_graboh_anim",c); snd(a,"GRABHOLD",c);
}

static void basic_punch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bam_callbacks_t*c){
    const char*t=jjxm("mode_normal","#punch",a,o);
    if(is(t,"#punch_hdbutt"))        bam_punch_hdbutt(a,c);
    else if(is(t,"#punch_lbowdrop")) bam_punch_lbowdrop(a,c);
    else if(is(t,"#punch_punch"))    bam_punch_punch(a,c);
}
static void basic_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bam_callbacks_t*c){
    const char*t=jjxm("mode_normal","#kick",a,o);
    if(is(t,"#kick_knee"))       bam_kick_knee(a,c);
    else if(is(t,"#kick_stomp")) bam_kick_stomp(a,c);
    else if(is(t,"#kick_TB"))    bam_kick_tb(a,c);
    else if(is(t,"#kick_kick"))  bam_kick_kick(a,c);
}
static void super_punch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bam_callbacks_t*c){
    const char*t=jjxm("mode_normal","#super_punch",a,o);
    if(is(t,"#spunch_special"))       bam_spunch_special(a,c);
    else if(is(t,"#spunch_lbowdrop")) bam_spunch_lbowdrop(a,o,c);
    else if(is(t,"#spunch_jump"))     bam_spunch_jump(a,c);
    else if(is(t,"do_pile"))          bam_do_pile(a,c);
    else if(is(t,"std_punch"))        bam_punch_punch(a,c);
}
static void super_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bam_callbacks_t*c){
    const char*t=jjxm("mode_normal","#super_kick",a,o);
    if(is(t,"#skick_special"))    bam_skick_kick(a,c);
    else if(is(t,"#skick_kick"))  bam_skick_kick(a,c);
    else if(is(t,"attack_stomp")) bam_kick_stomp(a,c);
    else if(is(t,"#kick_TB"))     bam_kick_tb(a,c);
    else if(is(t,"std_kick"))     bam_kick_kick(a,c);
}

static wm_arcade_bam_step_result_t mode_normal(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bam_env_t*e,const wm_arcade_bam_callbacks_t*c){
    uint8_t ac;
    if(a->anim_mode&WM_MODE_UNINT)return WM_BAM_STEP_IDLE;
    if(a->i_will_die&&!a->immobilize_time){anim(a,L.fall,c);if(c&&c->adjust_health)c->adjust_health(a,-10,c->user);setmode(a,WM_PMODE_DEAD);a->i_will_die=0;return WM_BAM_STEP_ACTION;}
    if(o&&o->player_mode==WM_PMODE_DEAD&&!(o->status_flags&WM_STATUS_ZOMBIE)){
        int reciprocal=a->attach_proc&&a->attach_proc->attach_proc==a;
        if(!reciprocal){
            if((c&&c->teammate_pin&&c->teammate_pin(a,c->user))||(c&&c->raisearm_check&&c->raisearm_check(a,c->user))){anim(a,face_label(L.raise2,L.raise4,a),c);if(c->set_raisearm_bit)c->set_raisearm_bit(a,c->user);if(c->drone_change_back)c->drone_change_back(a,c->user);return WM_BAM_STEP_ACTION;}
            if(a->but_val_cur&&c&&c->can_pin&&c->can_pin(a,o,c->user)){
            anim(a,face_label(L.pin2,L.pin4,a),c); a->status_flags |= WM_STATUS_DID_PIN;
                if(c->drone_change_back)c->drone_change_back(a,c->user);
                return WM_BAM_STEP_ACTION;
            }
        }
    }
    if(a->immobilize_time){a->move_dir=0;if(c&&c->execute_walk)c->execute_walk(a,c->user);return WM_BAM_STEP_EXTERNAL;}
    if((a->but_val_cur&WM_BTN_BLOCK)&&do_block(a,e,c))return WM_BAM_STEP_ACTION;
    ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
    if((a->but_val_cur&WM_BTN_ATTACK_MASK)==(WM_BTN_PUNCH|WM_BTN_KICK))ac=A_PUNCHKICK;
    switch(ac){case A_PUNCH:basic_punch(a,o,c);break;case A_BLOCK:(void)do_block(a,e,c);break;case A_SPUNCH:super_punch(a,o,c);break;case A_KICK:basic_kick(a,o,c);break;case A_PUNCHKICK:anim(a,"start_run_anim",c);break;case A_SKICK:super_kick(a,o,c);break;case A_GRABOH:bam_graboh(a,c);break;default:break;}
    if(a->anim_mode&WM_MODE_UNINT)return WM_BAM_STEP_ACTION;
    a->move_dir=a->stick_val_cur;
    if(c&&c->climb_turnbuckle&&c->climb_turnbuckle(a,c->user)){if(c->jump_rope_audio)c->jump_rope_audio(a,c->user);return WM_BAM_STEP_EXTERNAL;}
    if(c&&c->execute_walk)c->execute_walk(a,c->user);
    return WM_BAM_STEP_ACTION;
}
/* ---- the two mode_running tables ------------------------------- */

/* BAM.ASM:2010 #punch_clothesline -- the same two gates as Doink's,
   down to the eleven-long #mv_tbl of BIT NUMBERS tested against
   MOVE_DIR. */
static const uint8_t bam_cline_mv_tbl[11] = {
    0, 0, 0, 0, 0, 3 /* MOVE_RIGHT_BIT */, 3,
    0, 0, 2 /* MOVE_LEFT_BIT */, 2
};
static wm_arcade_bam_step_result_t bam_punch_clothesline(
        wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    unsigned idx;
    if(a->move_dir&WM_MOVE_LEFT){
        if(!(a->x_int>WM_RING_X_MID-0x70)) return WM_BAM_STEP_IDLE;
    } else {
        if(!(a->x_int<WM_RING_X_MID+0x70)) return WM_BAM_STEP_IDLE;
    }
    idx=(unsigned)(a->new_facing_dir&0xff);
    if(idx<sizeof bam_cline_mv_tbl/sizeof bam_cline_mv_tbl[0]&&
       (a->move_dir&(1<<bam_cline_mv_tbl[idx])))
        return WM_BAM_STEP_IDLE;
    anim(a,"bam_fly_cline_anim",c);
    setmode(a,WM_PMODE_INAIR);
    a->run_time=0;
    snd(a,"FLYKICK",c);
    return WM_BAM_STEP_ACTION;
}
/* :2068 #punch_bellyflop, alias attack_bellyflop -- the butt drop. */
static wm_arcade_bam_step_result_t bam_attack_bellyflop(
        wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    setmode(a,WM_PMODE_INAIR);
    a->run_time=0;
    anim(a,"bam_3_butt_drop_anim",c);
    snd(a,"FLYKICK",c);
    return WM_BAM_STEP_ACTION;
}
/* :2137 #kick_flyingkick */
static wm_arcade_bam_step_result_t bam_kick_flyingkick(
        wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    if(c&&c->ck_ignore&&c->ck_ignore(a,c->user)) return WM_BAM_STEP_IDLE;
    anim(a,"bam_flying_kick_anim",c);
    setmode(a,WM_PMODE_INAIR);
    snd(a,"FLYKICK",c);
    return WM_BAM_STEP_ACTION;
}

static wm_arcade_bam_step_result_t bam_run_punch(
        wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bam_callbacks_t*c){
    const char*t=jjxm("mode_running","#punch",a,o);
    if(is(t,"#punch_clothesline")) return bam_punch_clothesline(a,c);
    if(is(t,"#punch_bellyflop"))   return bam_attack_bellyflop(a,c);
    if(is(t,"#punch_rets"))        return WM_BAM_STEP_IDLE;   /* `rets` */
    return WM_BAM_STEP_IDLE;
}
static wm_arcade_bam_step_result_t bam_run_kick(
        wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bam_callbacks_t*c){
    const char*t=jjxm("mode_running","#kick",a,o);
    if(is(t,"#kick_flyingkick")) return bam_kick_flyingkick(a,c);
    if(is(t,"attack_bellyflop")) return bam_attack_bellyflop(a,c);
    if(is(t,"#kick_rets"))       return WM_BAM_STEP_IDLE;     /* `rets` */
    return WM_BAM_STEP_IDLE;
}

static wm_arcade_bam_step_result_t mode_running(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bam_env_t*e,const wm_arcade_bam_callbacks_t*c){
    int32_t v=0x00060000; a->run_time++; if(!a->usr_var1){if(c&&c->bounce_off_ropes)c->bounce_off_ropes(a,c->user);if(e&&e->hyper_speed_on>0&&e->hyper_speed_on<15)v<<=e->hyper_speed_on;if(!(a->move_dir&WM_MOVE_RIGHT))v=-v;a->x_vel=v;}
    if(a->stick_val_cur&WM_MOVE_UP)a->z_vel=-0x00020000;else if(a->stick_val_cur&WM_MOVE_DOWN)a->z_vel=0x00020000;else a->z_vel=0; if(a->getup_time||a->delay_butns)return WM_BAM_STEP_IDLE;
    switch(action_table[a->but_val_down&WM_BTN_ATTACK_MASK]){
    case A_BLOCK:a->x_vel>>=1;setmode(a,WM_PMODE_NORMAL);(void)do_block(a,e,c);return WM_BAM_STEP_ACTION;
    case A_KICK:case A_SKICK:return bam_run_kick(a,o,c);
    case A_PUNCH:case A_SPUNCH:case A_PUNCHKICK:case A_GRABOH:return bam_run_punch(a,o,c);
    default:return WM_BAM_STEP_IDLE;}
}
static wm_arcade_bam_step_result_t mode_bouncing(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){a->x_vel=0;a->z_vel=0;if(a->anim_mode&WM_MODE_END){a->move_dir^=(WM_MOVE_LEFT+WM_MOVE_RIGHT);a->facing_dir=(a->new_facing_dir&(WM_MOVE_UP+WM_MOVE_DOWN))|a->move_dir;anim(a,L.run,c);setmode(a,WM_PMODE_RUNNING);return WM_BAM_STEP_ACTION;}return WM_BAM_STEP_IDLE;}
static wm_arcade_bam_step_result_t mode_turn(wm_arcade_actor_t*a,const wm_arcade_bam_callbacks_t*c){
    uint8_t ac; if(a->stick_val_cur&WM_MOVE_DOWN){anim(a,L.climbdown,c);setmode(a,WM_PMODE_CLIMBTURNBKL);return WM_BAM_STEP_ACTION;} ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];if(ac==A_NONE)return WM_BAM_STEP_IDLE;setmode(a,WM_PMODE_INAIR);anim(a,L.turn_punch,c);snd(a,"TURNDIVE",c);if(c&&c->jump_rope_audio)c->jump_rope_audio(a,c->user);return WM_BAM_STEP_ACTION;
}
static wm_arcade_bam_step_result_t mode_block(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bam_env_t*e,const wm_arcade_bam_callbacks_t*c){
    a->block_time++;if(a->block_time>=160&&o&&o->player_mode==WM_PMODE_BLOCK&&a->closest_xdist<0x61&&a->closest_xdist>=45&&a->closest_zdist<30){setmode(a,WM_PMODE_NORMAL);anim(a,L.push,c);snd(a,"PUSH",c);return WM_BAM_STEP_ACTION;}
    if(!(a->but_val_down&WM_BTN_ATTACK_MASK))return WM_BAM_STEP_IDLE;
    if(!(a->but_val_cur&WM_BTN_BLOCK)){setmode(a,WM_PMODE_NORMAL);return WM_BAM_STEP_ACTION;}
    if((a->but_val_down&3)==1||(a->but_val_down&3)==3){setmode(a,WM_PMODE_NORMAL);anim(a,L.push,c);return WM_BAM_STEP_ACTION;}
    (void)e;
    return WM_BAM_STEP_IDLE;
}
static wm_arcade_bam_step_result_t mode_headhold(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bam_env_t*e,const wm_arcade_bam_callbacks_t*c){
    /* "Bozo power move": `callr bozo_check / jrnc #fail`, and
       everything below this is #fail. Six of the eight
       dispatchers did not call it at all, so the move was
       missing rather than merely inert. */
    if(c&&c->bozo_check&&c->bozo_check(a,c->user)){
        snd(a,L.bozo_snd,c);
        anim(a,(e&&(e->pcnt&1))?L.bozo_b:L.bozo_a,c);
        return WM_BAM_STEP_ACTION;
    }
    uint8_t ac;if(!o||o->player_mode!=WM_PMODE_HEADHELD){a->z_fixed-=6<<16;setmode(a,WM_PMODE_NORMAL);return WM_BAM_STEP_ACTION;}if(a->anim_mode&WM_MODE_UNINT)return WM_BAM_STEP_IDLE;ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
    if(ac==A_PUNCH||ac==A_SPUNCH||ac==A_KICK||ac==A_PUNCHKICK){if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);}

    if(ac==A_PUNCH||ac==A_KICK||ac==A_PUNCHKICK){anim(a,L.close4,c);return WM_BAM_STEP_ACTION;}return WM_BAM_STEP_IDLE;
}

wm_arcade_bam_step_result_t wm_arcade_move_bam(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bam_env_t*e,const wm_arcade_bam_callbacks_t*c){
    if(!a)return WM_BAM_STEP_IDLE;
    if(c&&c->check_secret_moves)c->check_secret_moves(a,secret_patterns,sizeof secret_patterns/sizeof secret_patterns[0],c->user);
    switch(a->player_mode){case WM_PMODE_NORMAL:case 18:case 22:case 23:return mode_normal(a,o,e,c);case WM_PMODE_RUNNING:return mode_running(a,o,e,c);case WM_PMODE_ATTACHED:if(c&&c->keep_attached)c->keep_attached(a,c->user);else(void)wm_arcade_keep_attached(a);if(!a->attach_proc){setmode(a,WM_PMODE_NORMAL);a->anim_mode=0;}return WM_BAM_STEP_EXTERNAL;case WM_PMODE_BOUNCING:return mode_bouncing(a,c);case WM_PMODE_ONTURNBKL:return mode_turn(a,c);case WM_PMODE_BLOCK:return mode_block(a,o,e,c);case WM_PMODE_DEAD:if(c&&c->mode_dead)c->mode_dead(a,c->user);return WM_BAM_STEP_EXTERNAL;case WM_PMODE_WAITANIM:if((a->anim_mode&WM_MODE_END)&&c&&c->code_addr)c->code_addr(a,(uint32_t)a->code_addr,c->user);return WM_BAM_STEP_EXTERNAL;case WM_PMODE_MASTER:if(c&&c->master_keep_attached)c->master_keep_attached(a,c->user);else(void)wm_arcade_master_keep_attached(a);return WM_BAM_STEP_EXTERNAL;case WM_PMODE_HEADHOLD:return mode_headhold(a,o,e,c);case WM_PMODE_HEADHELD:if((a->anim_mode&WM_MODE_NOGRAVITY)&&c&&c->mode_choking){c->mode_choking(a,c->user);return WM_BAM_STEP_EXTERNAL;}if(c&&c->bozo_check&&c->bozo_check(a,c->user)){if(c->do_reversal)c->do_reversal(a,c->user);if(c->do_reversal_message)c->do_reversal_message(a,c->user);snd(a,L.bozo_snd,c);anim(a,(e&&(e->pcnt&1))?L.bozo_hh_b:L.bozo_a,c);return WM_BAM_STEP_ACTION;}if(!a->attach_proc&&a->y_int<=a->ground_y)anim(a,L.headheld,c);return WM_BAM_STEP_EXTERNAL;case WM_PMODE_PUPPET:if(c&&c->mode_puppet)c->mode_puppet(a,c->user);return WM_BAM_STEP_EXTERNAL;case WM_PMODE_INAIR2:if(c&&c->mode_inair2)c->mode_inair2(a,c->user);return WM_BAM_STEP_EXTERNAL;case WM_PMODE_CHOKING:if(c&&c->mode_choking)c->mode_choking(a,c->user);return WM_BAM_STEP_EXTERNAL;default:return WM_BAM_STEP_IDLE;}
}

static int reject_common(wm_arcade_actor_t*a,wm_arcade_actor_t*o){return !a||!o||(a->anim_mode&WM_MODE_UNINT)||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_HEADHELD||o->player_mode==WM_PMODE_ATTACHED;}
int wm_arcade_bam_release_charge(wm_arcade_actor_t*a,wm_arcade_actor_t*o,uint16_t ticks,const wm_arcade_bam_callbacks_t*c){if(!a||ticks<85)return 0;if(a->player_mode==WM_PMODE_HEADHELD||a->player_mode==WM_PMODE_HEADHOLD||(a->anim_mode&WM_MODE_UNINT))return 0;if(!o||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_ATTACHED)return 0; anim(a,face_label("bam_2_fpunch_anim","bam_4_fpunch_anim",a),c); snd(a,"SPUNCH",c); return 1;}
int wm_arcade_bam_fire_secret(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_bam_secret_id_t id,uint32_t pcnt,const wm_arcade_bam_callbacks_t*c){
    switch(id){
    case WM_BAM_SECRET_NECK_GRAB:
        if(reject_common(a,o)||groundish(o))return 0;
        if((uint32_t)(pcnt-a->last_headhold)<120) anim(a,"bam_3_fake_hold_anim",c);
        else if(a->closest_xdist<=100) anim(a,L.headhold2,c); else anim(a,L.headhold,c);
        return 1; case WM_BAM_SECRET_GRAB_FLING: if(reject_common(a,o))return 0; anim(a,face_label(L.headhold2,L.headhold,a),c); snd(a,"GRABFLING",c); return 1; case WM_BAM_SECRET_GRAB_FLING2: if(reject_common(a,o))return 0; anim(a,face_label(L.headhold2,L.headhold,a),c); snd(a,"GRABFLING",c); return 1; case WM_BAM_SECRET_GRAB_FLING2_DUP: if(reject_common(a,o))return 0; anim(a,face_label(L.headhold2,L.headhold,a),c); snd(a,"GRABFLING",c); return 1; case WM_BAM_SECRET_HIP_TOSS: if(reject_common(a,o)||groundish(o))return 0; if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0; anim(a,"bam_3_hiptoss_anim",c); snd(a,"HIPTOSS_PUNCH",c); return 1; case WM_BAM_SECRET_HIP_TOSS2: if(reject_common(a,o)||groundish(o))return 0; if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0; anim(a,"bam_3_hiptoss_anim",c); snd(a,"HIPTOSS_PUNCH",c); return 1; case WM_BAM_SECRET_HIP_TOSS2_DUP: if(reject_common(a,o)||groundish(o))return 0; if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0; anim(a,"bam_3_hiptoss_anim",c); snd(a,"HIPTOSS_PUNCH",c); return 1;
    case WM_BAM_SECRET_JUMPKICK: if(reject_common(a,o)||groundish(o))return 0; anim(a,"bam_4_jumpkick_anim",c); return 1;
    case WM_BAM_SECRET_NAPALM: if(reject_common(a,o))return 0; anim(a,face_label("bam_2_napalm_anim","bam_4_napalm_anim",a),c); return 1;
    default:return 0;}
}
int wm_arcade_bam_fire_monitor(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_bam_monitor_id_t id,const wm_arcade_bam_env_t*e,int opponent_attack_is_leaping,const wm_arcade_bam_callbacks_t*c){
    wm_arcade_actor_t*t;const char*s;(void)e;(void)opponent_attack_is_leaping;if(!a||(unsigned)id>=sizeof special_processes/sizeof special_processes[0])return 0;s=special_processes[(unsigned)id];
    if(strstr(s,"hdhold_")!=NULL){if(a->player_mode==WM_PMODE_HEADHELD){t=a->who_hit_me?a->who_hit_me:o;if(!t)return 0;a->smart_target=t;t->immobilize_time=15;if(c&&c->do_reversal)c->do_reversal(a,c->user);if(c&&c->do_reversal_message)c->do_reversal_message(a,c->user);}else{t=a->who_i_hit?a->who_i_hit:o;if(a->player_mode!=WM_PMODE_HEADHOLD||!t)return 0;a->smart_target=t;t->immobilize_time=15;}if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);}
    if(strstr(s,"combo")!=NULL&&c&&c->check_combo_go&&c->check_combo_go(a,c->user)<0)return 0;
    startsp(a,s,c);
    return 1;
}
