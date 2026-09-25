#include "wm/arcade/wm_arcade_shawn.h"
#include "wm/arcade/wm_arcade_attach_anim.h"
#include "wm/arcade/wm_arcade_jjxm.h"
#include <string.h>

/*
 * Dedicated direct C translation boundary for SHAWN.ASM.
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
static const wm_arcade_input_step_t s_toward_skick[]={STEP(WM_B_SKICK,WM_J_ALL),STEP(WM_J_TOWARD,WM_J_REAL_LR),STEP(WM_J_TOWARD,WM_J_REAL_LR)};
static const wm_arcade_input_step_t s_away_skick[]={STEP(WM_B_SKICK,WM_J_ALL),STEP(WM_J_AWAY,WM_J_REAL_LR|WM_J_UP|WM_J_DOWN),STEP(WM_J_AWAY,WM_J_REAL_LR|WM_J_UP|WM_J_DOWN)};

static const wm_arcade_input_pattern_t secret_patterns[]={
    {"charge_flying_kick",NULL,0,0},
    {"grab_fling",s_grab_fling,3,32},
    {"hip_toss",s_hip_toss,3,32},
    {"grab_fling2",s_grab_fling2,1,10},
    {"hip_toss2",s_hip_toss2,1,10},
    {"neck_grab",s_neck32,3,32},
    {"frankensteiner",s_toward_skick,3,32},
    {"jump_kick",s_away_skick,3,32}
};
/* WRESTLE2.ASM's shn_smove_table, as the assembler built it.
   The finishing-move entries every one of these tables carries sit
   inside `.if NUM_SHAWN_FINISHES`, and GAME.EQU:587 sets that switch to
   0 -- so none of them was assembled. Generated as
   wm_wrestler_smoves[] (wm/wrestler_anim_tables.h); a source-tool test
   holds this copy to it. */
static const char *const special_processes[]={
    "shn_hdhold_combo2",
    "shn_hdhold_combo1",
    "shn_charge_suplex",
    "shn_swirl_speedkick",
    "shn_sliding_kicktoss",
    "shn_hdhold_suplex",
    "shn_hdhold_frank",
    "shn_hdhold_kicktoss",
    "shn_hdhold_butts",
    "shn_flipslam",
    "shn_grab_toss_air",
    "std_walk_fast",
    "std_taunt"
};

const wm_arcade_wrestler_profile_t wm_arcade_profile_shawn={
    WM_ROSTER_SHAWN,"Shawn Michaels","SHAWN.ASM",3329,"shn",WM_BTN_SKICK,85,
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
    "shn_stand2_anim",
    "shn_stand4_anim",
    "shn_torso2_anim",
    "shn_torso4_anim",
    "shn_fall_back_anim",
    "shn_4_block_anim",
    "shn_4_push_anim",
    "shn_run2_anim",
    "shn_climb_down_anim",
    "shn_2_pin_anim",
    "shn_4_pin_anim",
    "shn_2_raise_arm_anim",
    "shn_4_raise_arm_anim",
    "shn_2_punch_anim",
    "shn_4_punch_anim",
    "shn_2_butt_anim",
    "shn_4_butt_anim",
    "shn_2_falling_punch_anim",
    "shn_4_falling_punch_anim",
    "shn_2_kick_anim",
    "shn_4_kick_anim",
    "shn_2_knee_anim",
    "shn_4_knee_anim",
    "shn_2_stomp_anim",
    "shn_4_stomp_anim",
    "shn_flying_kick_anim",
    "shn_belbow_anim",
    "shn_bstomp_anim",
    "shn_3_head_hold2_anim",
    "shn_3_head_hold_anim",
    "shn_3_head_held_stand_anim",
    "shn_fstein_anim",
    "shn_sliding_kicktoss_anim",
    "shn_sliding_kicktoss_anim",
    "KICK",
};

static void setmode(wm_arcade_actor_t*a,uint16_t m){if(a&&a->player_mode!=WM_PMODE_DEAD)a->player_mode=m;}
static int groundish(const wm_arcade_actor_t*o){return o&&(o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD);}
/* MACROS.H:51 FACE24 is `btst MOVE_UP_BIT,a14 / jrnz` -- the "2"
   form is the one facing UP, not the one facing right. This tested
   WM_MOVE_RIGHT, so every FACE24 selection in this file picked the
   wrong one of the pair whenever facing and travel disagreed. Bret
   and Razor had it right; these six did not. */
static int face2(const wm_arcade_actor_t*a){return a&&(a->facing_dir&WM_MOVE_UP);}
static const char *face_label(const char*l2,const char*l4,const wm_arcade_actor_t*a){return face2(a)?l2:l4;}
/*
 * ANIM.ASM's two primary-animation entry points. :4532 change_anim1
 * returns without restarting when the request names the animation
 * already running and that animation has not ended; :4542
 * change_anim1a is the label on the instruction after both tests, so
 * entering there always replays from frame 0.
 *
 * anim() is change_anim1a because that is what almost every call site
 * in this file is. anim1() is the guarded one, and every use of it
 * below names the source line it comes from.
 */
static void anim(wm_arcade_actor_t*a,const char*l,const wm_arcade_shawn_callbacks_t*c){if(c&&c->change_anim_restart&&l)c->change_anim_restart(a,l,c->user);}
static void anim1(wm_arcade_actor_t*a,const char*l,const wm_arcade_shawn_callbacks_t*c){if(c&&c->change_anim_label&&l)c->change_anim_label(a,l,c->user);}
static void snd(wm_arcade_actor_t*a,const char*l,const wm_arcade_shawn_callbacks_t*c){if(c&&c->sound_label&&l)c->sound_label(a,l,c->user);}
static void startsp(wm_arcade_actor_t*a,const char*l,const wm_arcade_shawn_callbacks_t*c){if(!a||!l)return;if(c&&c->resolve_label_token)a->special_move_addr=c->resolve_label_token(l,c->user);if(c&&c->start_special_label)c->start_special_label(a,l,c->user);}

static int do_block(wm_arcade_actor_t*a,const wm_arcade_shawn_env_t*e,const wm_arcade_shawn_callbacks_t*c){
    if(e&&e->blocking_off)return 0;
    anim(a,L.block,c);
    a->block_time=0;
    snd(a,"BLOCK_WOOSH",c);
    setmode(a,WM_PMODE_BLOCK);
    return 1;
}
/*
 * SHAWN.ASM's EIGHT JJXM tables -- he is the only wrestler with more
 * than six. His mode_running splits the punch group in two (#punch and
 * #punchkick against #super_punch and #graboh) and the kick group in
 * two as well, so the running super punch is a flip slam where the
 * running punch is only ever a run stomp.
 */
static const char *jjxm(const char*section,const char*entry,
                        const wm_arcade_actor_t*a,const wm_arcade_actor_t*o){
    return wm_jjxm_pick("SHAWN",section,entry,a,o);
}
#define is(t,n) wm_jjxm_is((t),(n))

/* SHAWN.ASM:1971 #punch_punch, alias std_punch. */
static void shn_punch_punch(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    anim(a,face_label(L.punch2,L.punch4,a),c); snd(a,"PUNCH",c);
}
/* :1982 #punch_hdbutt, which is `jruc #hdbutt` to :2071. */
static void shn_punch_hdbutt(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    anim1(a,face_label(L.close2,L.close4,a),c); /* :2074 change_anim1 */
    snd(a,"HDBUTT",c);
}
/* :1986 #punch_lbdrop -- Shawn's ground attack is the FALLING PUNCH. */
static void shn_punch_lbdrop(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    anim(a,face_label("shn_2_falling_punch_anim","shn_4_falling_punch_anim",a),c);
    snd(a,"LBOWDROP",c);
}
/* :2050 #spunch_slap, which mode_normal's #graboh is an alias of. */
static void shn_spunch_slap(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    anim(a,face_label("shn_2_slap_anim","shn_4_slap_anim",a),c); snd(a,"SPUNCH",c);
}
/* :2060 #spunch_special. The threshold is 64 on X only, and past it
   the move is the plain punch rather than the slap. */
static void shn_spunch_special(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    if(a->closest_xdist>64){ shn_punch_punch(a,c); return; }
    anim1(a,face_label("shn_2_pummel_anim","shn_4_pummel_anim",a),c); /* :2066 */
    snd(a,"HDBUTT",c);
}
/* :2081 #spunch_lbowdrop -- the hair grab. */
static void shn_spunch_lbowdrop(wm_arcade_actor_t*a,wm_arcade_actor_t*o,
                                const wm_arcade_shawn_callbacks_t*c){
    int hair=0;
    if(o&&o->player_mode!=WM_PMODE_DEAD){
        int32_t dx=a->x_fixed-o->x_fixed;
        if(dx<0)dx=-dx;
        if((dx>>16)>=0x20&&
           ((a->obj_control&WM_OBJ_FLIPH)!=(o->obj_control&WM_OBJ_FLIPH)))
            hair=1;
    }
    /* :2113 change_anim1. Shawn's #no arm is change_anim1a -- Bret,
       Taker, Yoko, Lex and Doink all guard BOTH arms and he does not. */
    if(hair) anim1(a,face_label("shn_2_hair_pickup_anim","shn_4_hair_pickup_anim",a),c);
    else     anim(a,face_label("shn_2_falling_punch_anim","shn_4_falling_punch_anim",a),c);
    snd(a,"LBOWDROP",c);
}
/* :2180 #kick_kick (std_kick), :2190 #kick_knee (std_knee), :2204
   #kick_stomp -- and :2283 #skick_stomp, the same stomp again. */
static void shn_kick_kick(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    anim(a,face_label(L.kick2,L.kick4,a),c); snd(a,"KICK",c);
}
static void shn_kick_knee(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    anim(a,face_label(L.knee2,L.knee4,a),c); snd(a,"KICK",c);
}
static void shn_kick_stomp(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    anim(a,face_label(L.stomp2,L.stomp4,a),c); snd(a,"KICK",c);
}
/* :2170 #kick_TB */
static void shn_kick_tb(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    anim(a,"shn_spinkick_TB_anim",c); snd(a,"KICK",c);
}
/* :2274 #skick_kick */
static void shn_skick_kick(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    anim(a,"shn_spinkick_anim",c); snd(a,"FLYKICK",c);
}
/* :2262 #skick_frank -- the frankensteiner, and ck_ignore does not
   refuse it outright: a refused one FALLS THROUGH to the spin kick. */
static void shn_skick_frank(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    if(c&&c->ck_ignore&&c->ck_ignore(a,c->user)){ shn_skick_kick(a,c); return; }
    anim(a,"shn_fstein_anim",c); snd(a,"KICK",c);
}

static void basic_punch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_callbacks_t*c){
    const char*t=jjxm("mode_normal","#punch",a,o);
    if(is(t,"#punch_hdbutt"))      shn_punch_hdbutt(a,c);
    else if(is(t,"#punch_lbdrop")) shn_punch_lbdrop(a,c);
    else if(is(t,"#punch_punch"))  shn_punch_punch(a,c);
}
static void basic_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_callbacks_t*c){
    const char*t=jjxm("mode_normal","#kick",a,o);
    if(is(t,"#kick_knee"))       shn_kick_knee(a,c);
    else if(is(t,"#kick_stomp")) shn_kick_stomp(a,c);
    else if(is(t,"#kick_TB"))    shn_kick_tb(a,c);
    else if(is(t,"#kick_kick"))  shn_kick_kick(a,c);
}
static void super_punch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_callbacks_t*c){
    const char*t=jjxm("mode_normal","#super_punch",a,o);
    if(is(t,"#spunch_special"))       shn_spunch_special(a,c);
    else if(is(t,"#spunch_lbowdrop")) shn_spunch_lbowdrop(a,o,c);
    else if(is(t,"#spunch_slap"))     shn_spunch_slap(a,c);
    else if(is(t,"std_punch"))        shn_punch_punch(a,c);
}
static void super_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_callbacks_t*c){
    const char*t=jjxm("mode_normal","#super_kick",a,o);
    if(is(t,"#skick_frank"))      shn_skick_frank(a,c);
    else if(is(t,"#skick_stomp")) shn_kick_stomp(a,c);
    else if(is(t,"#kick_TB"))     shn_kick_tb(a,c);
    else if(is(t,"#skick_kick"))  shn_skick_kick(a,c);
    else if(is(t,"std_kick"))     shn_kick_kick(a,c);
}

static wm_arcade_shawn_step_result_t mode_normal(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_env_t*e,const wm_arcade_shawn_callbacks_t*c){
    uint8_t ac;
    if(a->anim_mode&WM_MODE_UNINT)return WM_SHAWN_STEP_IDLE;
    if(a->i_will_die&&!a->immobilize_time){anim(a,L.fall,c);if(c&&c->adjust_health)c->adjust_health(a,-10,c->user);setmode(a,WM_PMODE_DEAD);a->i_will_die=0;return WM_SHAWN_STEP_ACTION;}
    if(o&&o->player_mode==WM_PMODE_DEAD&&!(o->status_flags&WM_STATUS_ZOMBIE)){
        int reciprocal=a->attach_proc&&a->attach_proc->attach_proc==a;
        if(!reciprocal){
            if((c&&c->teammate_pin&&c->teammate_pin(a,c->user))||(c&&c->raisearm_check&&c->raisearm_check(a,c->user))){anim(a,face_label(L.raise2,L.raise4,a),c);if(c->set_raisearm_bit)c->set_raisearm_bit(a,c->user);if(c->drone_change_back)c->drone_change_back(a,c->user);return WM_SHAWN_STEP_ACTION;}
            if(a->but_val_cur&&c&&c->can_pin&&c->can_pin(a,o,c->user)){
            anim(a,face_label(L.pin2,L.pin4,a),c); a->status_flags |= WM_STATUS_DID_PIN;
                if(c->drone_change_back)c->drone_change_back(a,c->user);
                return WM_SHAWN_STEP_ACTION;
            }
        }
    }
    if(a->immobilize_time){a->move_dir=0;if(c&&c->execute_walk)c->execute_walk(a,c->user);return WM_SHAWN_STEP_EXTERNAL;}
    if((a->but_val_cur&WM_BTN_BLOCK)&&do_block(a,e,c))return WM_SHAWN_STEP_ACTION;
    ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
    if((a->but_val_cur&WM_BTN_ATTACK_MASK)==(WM_BTN_PUNCH|WM_BTN_KICK))ac=A_PUNCHKICK;
    switch(ac){case A_PUNCH:basic_punch(a,o,c);break;case A_BLOCK:(void)do_block(a,e,c);break;case A_SPUNCH:super_punch(a,o,c);break;case A_KICK:basic_kick(a,o,c);break;case A_PUNCHKICK:anim(a,"start_run_anim",c);break;case A_SKICK:super_kick(a,o,c);break;case A_GRABOH:shn_spunch_slap(a,c);break;default:break;}
    if(a->anim_mode&WM_MODE_UNINT)return WM_SHAWN_STEP_ACTION;
    a->move_dir=a->stick_val_cur;
    if(c&&c->climb_turnbuckle&&c->climb_turnbuckle(a,c->user)){if(c->jump_rope_audio)c->jump_rope_audio(a,c->user);return WM_SHAWN_STEP_EXTERNAL;}
    if(c&&c->execute_walk)c->execute_walk(a,c->user);
    return WM_SHAWN_STEP_ACTION;
}
/* ---- the FOUR mode_running tables ------------------------------ */

/* SHAWN.ASM:2585 #kick_runstomp, alias attack_runstomp. ck_ignore
   refuses it, and it plays no sound of its own. This is the label
   REACT4's hit_stomp compares against for the scream. */
static wm_arcade_shawn_step_result_t shn_attack_runstomp(
        wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    if(c&&c->ck_ignore&&c->ck_ignore(a,c->user)) return WM_SHAWN_STEP_IDLE;
    anim(a,"shn_run_stomp_anim",c);
    return WM_SHAWN_STEP_ACTION;
}
/* :2293 attack_flykick. TWO refusals: ck_ignore, and then its own
   `calla get_opp_plyrmode` -- "don't do it if the bad guy is on the
   ground" -- which is a second, independent look at the opponent's
   mode after the table has already keyed on it. */
static wm_arcade_shawn_step_result_t shn_attack_flykick(
        wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_callbacks_t*c){
    if(c&&c->ck_ignore&&c->ck_ignore(a,c->user)) return WM_SHAWN_STEP_IDLE;
    if(o&&(o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD))
        return WM_SHAWN_STEP_IDLE;
    setmode(a,WM_PMODE_INAIR);
    anim(a,"shn_flying_kick_anim",c);
    snd(a,"FLYKICK",c);
    return WM_SHAWN_STEP_ACTION;
}
/* :2643 #attack_fstein */
static wm_arcade_shawn_step_result_t shn_attack_fstein(
        wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    if(c&&c->ck_ignore&&c->ck_ignore(a,c->user)) return WM_SHAWN_STEP_IDLE;
    anim(a,"shn_fstein_anim",c);
    setmode(a,WM_PMODE_INAIR);
    snd(a,"GRABHOLD",c);
    return WM_SHAWN_STEP_ACTION;
}
/* :2526 #punch_flipslam. The Undertaker's facing gate, and its sound
   is a MIXED WRSND pair: HIPTOSS_T1 with PUNCH_T2. */
static wm_arcade_shawn_step_result_t shn_punch_flipslam(
        wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    if(!((a->facing_dir & a->new_facing_dir) & (WM_MOVE_LEFT|WM_MOVE_RIGHT)))
        return WM_SHAWN_STEP_IDLE;
    anim(a,"shn_flipslam_anim",c);
    snd(a,"HIPTOSS_PUNCH",c);
    return WM_SHAWN_STEP_ACTION;
}

static wm_arcade_shawn_step_result_t shn_run_punch(
        wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_callbacks_t*c){
    const char*t=jjxm("mode_running","#punch",a,o);
    if(is(t,"attack_runstomp")) return shn_attack_runstomp(a,c);
    if(is(t,"#punch_rets"))     return WM_SHAWN_STEP_IDLE;   /* `rets` */
    return WM_SHAWN_STEP_IDLE;
}
static wm_arcade_shawn_step_result_t shn_run_spunch(
        wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_callbacks_t*c){
    const char*t=jjxm("mode_running","#super_punch",a,o);
    if(is(t,"#punch_flipslam")) return shn_punch_flipslam(a,c);
    if(is(t,"attack_runstomp")) return shn_attack_runstomp(a,c);
    if(is(t,"#spunch_rets"))    return WM_SHAWN_STEP_IDLE;   /* `rets` */
    return WM_SHAWN_STEP_IDLE;
}
static wm_arcade_shawn_step_result_t shn_run_kick(
        wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_callbacks_t*c){
    const char*t=jjxm("mode_running","#kick",a,o);
    if(is(t,"#kick_runstomp")) return shn_attack_runstomp(a,c);
    if(is(t,"attack_flykick")) return shn_attack_flykick(a,o,c);
    return WM_SHAWN_STEP_IDLE;
}
static wm_arcade_shawn_step_result_t shn_run_skick(
        wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_callbacks_t*c){
    const char*t=jjxm("mode_running","#super_kick",a,o);
    if(is(t,"#attack_fstein")) return shn_attack_fstein(a,c);
    if(is(t,"attack_flykick")) return shn_attack_flykick(a,o,c);
    if(is(t,"attack_runstomp")) return shn_attack_runstomp(a,c);
    return WM_SHAWN_STEP_IDLE;
}

static wm_arcade_shawn_step_result_t mode_running(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_env_t*e,const wm_arcade_shawn_callbacks_t*c){
    int32_t v=0x00060000; a->run_time++; if(!a->usr_var1){if(c&&c->bounce_off_ropes)c->bounce_off_ropes(a,c->user);if(e&&e->hyper_speed_on>0&&e->hyper_speed_on<15)v<<=e->hyper_speed_on;if(!(a->move_dir&WM_MOVE_RIGHT))v=-v;a->x_vel=v;}
    if(a->stick_val_cur&WM_MOVE_UP)a->z_vel=-0x00020000;else if(a->stick_val_cur&WM_MOVE_DOWN)a->z_vel=0x00020000;else a->z_vel=0; if(a->getup_time||a->delay_butns)return WM_SHAWN_STEP_IDLE;
    switch(action_table[a->but_val_down&WM_BTN_ATTACK_MASK]){
    case A_BLOCK:a->x_vel>>=1;setmode(a,WM_PMODE_NORMAL);(void)do_block(a,e,c);return WM_SHAWN_STEP_ACTION;
    /* Four tables, not two -- see the note above shn_run_punch. */
    case A_PUNCH:case A_PUNCHKICK:return shn_run_punch(a,o,c);
    case A_SPUNCH:case A_GRABOH:return shn_run_spunch(a,o,c);
    case A_KICK:return shn_run_kick(a,o,c);
    case A_SKICK:return shn_run_skick(a,o,c);
    default:return WM_SHAWN_STEP_IDLE;}
}
static wm_arcade_shawn_step_result_t mode_bouncing(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){a->x_vel=0;a->z_vel=0;if(a->anim_mode&WM_MODE_END){a->move_dir^=(WM_MOVE_LEFT+WM_MOVE_RIGHT);a->facing_dir=(a->new_facing_dir&(WM_MOVE_UP+WM_MOVE_DOWN))|a->move_dir;anim(a,L.run,c);setmode(a,WM_PMODE_RUNNING);return WM_SHAWN_STEP_ACTION;}return WM_SHAWN_STEP_IDLE;}
static wm_arcade_shawn_step_result_t mode_turn(wm_arcade_actor_t*a,const wm_arcade_shawn_callbacks_t*c){
    uint8_t ac; if(a->stick_val_cur&WM_MOVE_DOWN){anim(a,L.climbdown,c);setmode(a,WM_PMODE_CLIMBTURNBKL);return WM_SHAWN_STEP_ACTION;} ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];if(ac==A_NONE)return WM_SHAWN_STEP_IDLE;setmode(a,WM_PMODE_INAIR);anim(a,(ac==A_KICK||ac==A_SKICK)?L.turn_kick:L.turn_punch,c);snd(a,"TURNDIVE",c);if(c&&c->jump_rope_audio)c->jump_rope_audio(a,c->user);return WM_SHAWN_STEP_ACTION;
}
static wm_arcade_shawn_step_result_t mode_block(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_env_t*e,const wm_arcade_shawn_callbacks_t*c){
    a->block_time++;if(a->block_time>=160&&o&&o->player_mode==WM_PMODE_BLOCK&&a->closest_xdist<0x61&&a->closest_xdist>=45&&a->closest_zdist<30){setmode(a,WM_PMODE_NORMAL);anim(a,L.push,c);snd(a,"PUSH",c);return WM_SHAWN_STEP_ACTION;}
    if(!(a->but_val_down&WM_BTN_ATTACK_MASK))return WM_SHAWN_STEP_IDLE;
    if(!(a->but_val_cur&WM_BTN_BLOCK)){setmode(a,WM_PMODE_NORMAL);return WM_SHAWN_STEP_ACTION;}
    if((a->but_val_down&3)==1||(a->but_val_down&3)==3){setmode(a,WM_PMODE_NORMAL);anim(a,L.push,c);return WM_SHAWN_STEP_ACTION;}
    (void)e;
    return WM_SHAWN_STEP_IDLE;
}
static wm_arcade_shawn_step_result_t mode_headhold(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_env_t*e,const wm_arcade_shawn_callbacks_t*c){
    /* "Bozo power move": `callr bozo_check / jrnc #fail`, and
       everything below this is #fail. Six of the eight
       dispatchers did not call it at all, so the move was
       missing rather than merely inert. */
    if(c&&c->bozo_check&&c->bozo_check(a,c->user)){
        snd(a,L.bozo_snd,c);
        anim(a,(e&&(e->pcnt&1))?L.bozo_b:L.bozo_a,c);
        return WM_SHAWN_STEP_ACTION;
    }
    uint8_t ac;if(!o||o->player_mode!=WM_PMODE_HEADHELD){a->z_fixed-=6<<16;setmode(a,WM_PMODE_NORMAL);return WM_SHAWN_STEP_ACTION;}if(a->anim_mode&WM_MODE_UNINT)return WM_SHAWN_STEP_IDLE;ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
    if(ac==A_PUNCH||ac==A_SPUNCH||ac==A_KICK||ac==A_PUNCHKICK){if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);}
    if(ac==A_SKICK){anim(a,"shn_knee_fstein_anim",c);return WM_SHAWN_STEP_ACTION;}
    if(ac==A_PUNCH||ac==A_KICK||ac==A_PUNCHKICK){anim(a,L.close4,c);return WM_SHAWN_STEP_ACTION;}return WM_SHAWN_STEP_IDLE;
}

wm_arcade_shawn_step_result_t wm_arcade_move_shawn(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_shawn_env_t*e,const wm_arcade_shawn_callbacks_t*c){
    if(!a)return WM_SHAWN_STEP_IDLE;
    if(c&&c->check_secret_moves)c->check_secret_moves(a,secret_patterns,sizeof secret_patterns/sizeof secret_patterns[0],c->user);
    switch(a->player_mode){case WM_PMODE_NORMAL:case 18:case 22:case 23:return mode_normal(a,o,e,c);case WM_PMODE_RUNNING:return mode_running(a,o,e,c);case WM_PMODE_ATTACHED:if(c&&c->keep_attached)c->keep_attached(a,c->user);else(void)wm_arcade_keep_attached(a);if(!a->attach_proc){setmode(a,WM_PMODE_NORMAL);a->anim_mode=0;}return WM_SHAWN_STEP_EXTERNAL;case WM_PMODE_BOUNCING:return mode_bouncing(a,c);case WM_PMODE_ONTURNBKL:return mode_turn(a,c);case WM_PMODE_BLOCK:return mode_block(a,o,e,c);case WM_PMODE_DEAD:if(c&&c->mode_dead)c->mode_dead(a,c->user);return WM_SHAWN_STEP_EXTERNAL;case WM_PMODE_WAITANIM:if((a->anim_mode&WM_MODE_END)&&c&&c->code_addr)c->code_addr(a,(uint32_t)a->code_addr,c->user);return WM_SHAWN_STEP_EXTERNAL;case WM_PMODE_MASTER:if(c&&c->master_keep_attached)c->master_keep_attached(a,c->user);else(void)wm_arcade_master_keep_attached(a);return WM_SHAWN_STEP_EXTERNAL;case WM_PMODE_HEADHOLD:return mode_headhold(a,o,e,c);case WM_PMODE_HEADHELD:if((a->anim_mode&WM_MODE_NOGRAVITY)&&c&&c->mode_choking){c->mode_choking(a,c->user);return WM_SHAWN_STEP_EXTERNAL;}if(c&&c->bozo_check&&c->bozo_check(a,c->user)){if(c->do_reversal)c->do_reversal(a,c->user);if(c->do_reversal_message)c->do_reversal_message(a,c->user);snd(a,L.bozo_snd,c);anim(a,(e&&(e->pcnt&1))?L.bozo_hh_b:L.bozo_a,c);return WM_SHAWN_STEP_ACTION;}if(!a->attach_proc&&a->y_int<=a->ground_y)anim(a,L.headheld,c);return WM_SHAWN_STEP_EXTERNAL;case WM_PMODE_PUPPET:if(c&&c->mode_puppet)c->mode_puppet(a,c->user);return WM_SHAWN_STEP_EXTERNAL;case WM_PMODE_INAIR2:if(c&&c->mode_inair2)c->mode_inair2(a,c->user);return WM_SHAWN_STEP_EXTERNAL;case WM_PMODE_CHOKING:if(c&&c->mode_choking)c->mode_choking(a,c->user);return WM_SHAWN_STEP_EXTERNAL;
    /* Modes 10 and 24 are a bare `rets` in this file, and correctly so:
       ANI_SETPLYRMODE,MODE_OPPOVERHEAD appears only in BAMSEQ3,
       LEXSEQ3 and YOKSEQ3 and MODE_CHOKEHOLD only in UNDSEQ3, so
       this wrestler can never be in either. */
    case WM_PMODE_OPPOVERHEAD: case WM_PMODE_CHOKEHOLD: return WM_SHAWN_STEP_IDLE;
    default:return WM_SHAWN_STEP_IDLE;}
}

static int reject_common(wm_arcade_actor_t*a,wm_arcade_actor_t*o){return !a||!o||(a->anim_mode&WM_MODE_UNINT)||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_HEADHELD||o->player_mode==WM_PMODE_ATTACHED;}
int wm_arcade_shawn_release_charge(wm_arcade_actor_t*a,wm_arcade_actor_t*o,uint16_t ticks,const wm_arcade_shawn_callbacks_t*c){if(!a||ticks<85)return 0;if(a->player_mode==WM_PMODE_HEADHELD||a->player_mode==WM_PMODE_HEADHOLD||(a->anim_mode&WM_MODE_UNINT))return 0;if(!o||groundish(o)||o->player_mode==WM_PMODE_ATTACHED)return 0; setmode(a,WM_PMODE_INAIR); a->x_vel=1; anim(a,"shn_flying_kick_anim",c); snd(a,"FLYKICK",c); return 1;}
int wm_arcade_shawn_fire_secret(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_shawn_secret_id_t id,uint32_t pcnt,const wm_arcade_shawn_callbacks_t*c){
    switch(id){
    case WM_SHAWN_SECRET_NECK_GRAB:
        if(reject_common(a,o)||groundish(o))return 0;
        if((uint32_t)(pcnt-a->last_headhold)<120) anim(a,"shn_3_fake_hold_anim",c);
        else if(a->closest_xdist<=80) anim(a,L.headhold2,c); else anim(a,L.headhold,c);
        return 1; case WM_SHAWN_SECRET_GRAB_FLING: if(reject_common(a,o))return 0; anim(a,face_label(L.headhold2,L.headhold,a),c); snd(a,"GRABFLING",c); return 1; case WM_SHAWN_SECRET_GRAB_FLING2: if(reject_common(a,o))return 0; anim(a,face_label(L.headhold2,L.headhold,a),c); snd(a,"GRABFLING",c); return 1; case WM_SHAWN_SECRET_HIP_TOSS: if(reject_common(a,o)||groundish(o))return 0; if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0; anim(a,face_label("shn_2_hiptoss_anim","shn_4_hiptoss_anim",a),c); snd(a,"HIPTOSS_PUNCH",c); return 1; case WM_SHAWN_SECRET_HIP_TOSS2: if(reject_common(a,o)||groundish(o))return 0; if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0; anim(a,face_label("shn_2_hiptoss_anim","shn_4_hiptoss_anim",a),c); snd(a,"HIPTOSS_PUNCH",c); return 1;
    case WM_SHAWN_SECRET_FRANKENSTEINER: if(reject_common(a,o)||groundish(o))return 0; anim(a,"shn_fstein_anim",c); return 1;
    case WM_SHAWN_SECRET_JUMP_KICK: if(reject_common(a,o)||groundish(o))return 0; anim(a,"shn_4_jump_kick_anim",c); return 1;
    default:return 0;}
}
int wm_arcade_shawn_fire_monitor(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_shawn_monitor_id_t id,const wm_arcade_shawn_env_t*e,int opponent_attack_is_leaping,const wm_arcade_shawn_callbacks_t*c){
    wm_arcade_actor_t*t;const char*s;(void)e;(void)opponent_attack_is_leaping;if(!a||(unsigned)id>=sizeof special_processes/sizeof special_processes[0])return 0;s=special_processes[(unsigned)id];
    if(strstr(s,"hdhold_")!=NULL){if(a->player_mode==WM_PMODE_HEADHELD){t=a->who_hit_me?a->who_hit_me:o;if(!t)return 0;a->smart_target=t;t->immobilize_time=15;if(c&&c->do_reversal)c->do_reversal(a,c->user);if(c&&c->do_reversal_message)c->do_reversal_message(a,c->user);}else{t=a->who_i_hit?a->who_i_hit:o;if(a->player_mode!=WM_PMODE_HEADHOLD||!t)return 0;a->smart_target=t;t->immobilize_time=15;}if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);}
    if(strstr(s,"combo")!=NULL&&c&&c->check_combo_go&&c->check_combo_go(a,c->user)<0)return 0;
    startsp(a,s,c);
    return 1;
}
