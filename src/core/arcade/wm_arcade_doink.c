#include "wm/arcade/wm_arcade_doink.h"
#include "wm/arcade/wm_arcade_attach_anim.h"
#include "wm/arcade/wm_arcade_jjxm.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include <string.h>

/*
 * Dedicated direct C translation boundary for DOINK.ASM.
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
static const wm_arcade_input_step_t s_punch_qcf[]={STEP(WM_B_PUNCH,WM_J_ALL),STEP(WM_J_TOWARD,WM_J_REAL_LR),STEP(WM_J_DOWN_TOWARD,WM_J_REAL_LR),STEP(WM_J_DOWN,WM_J_REAL_LR)};
static const wm_arcade_input_step_t s_box7[]={STEP(WM_B_PUNCH,WM_J_ALL),STEP(WM_B_PUNCH,WM_J_ALL),STEP(WM_B_PUNCH,WM_J_ALL),STEP(WM_B_PUNCH,WM_J_ALL),STEP(WM_B_PUNCH,WM_J_ALL),STEP(WM_B_PUNCH,WM_J_ALL),STEP(WM_B_PUNCH,WM_J_ALL)};

static const wm_arcade_input_pattern_t secret_patterns[]={
    {"charge_buzz",NULL,0,0},
    {"grab_fling",s_grab_fling,3,32},
    {"hip_toss",s_hip_toss,3,32},
    {"grab_fling2",s_grab_fling2,1,10},
    {"hip_toss2",s_hip_toss2,1,10},
    {"earslap",s_punch_qcf,4,50},
    {"hammer",s_toward_skick,3,32},
    {"neck_grab",s_neck30,3,30},
    {"boxing_pnch",s_box7,7,60}
};
/* WRESTLE2.ASM's dnk_smove_table, as the assembler built it.
   The finishing-move entries every one of these tables carries sit
   inside `.if NUM_DOINK_FINISHES`, and GAME.EQU:583 sets that switch to
   0 -- so none of them was assembled. Generated as
   wm_wrestler_smoves[] (wm/wrestler_anim_tables.h); a source-tool test
   holds this copy to it. */
static const char *const special_processes[]={
    "dnk_charge_flykick",
    "dnk_hdhold_slam",
    "dnk_hdhold_combo1",
    "dnk_hdhold_pile",
    "dnk_hdhold_combo2",
    "dnk_hdhold_buzz",
    "dnk_grab_toss_air",
    "std_walk_fast",
    "std_taunt"
};

const wm_arcade_wrestler_profile_t wm_arcade_profile_doink={
    WM_ROSTER_DOINK,"Doink","DOINK.ASM",4037,"dnk",WM_BTN_PUNCH,100,
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
    "dnk_stand2_anim",
    "dnk_stand4_anim",
    "dnk_torso2_anim",
    "dnk_torso4_anim",
    "dnk_fall_back_anim",
    "dnk_4_block_anim",
    "dnk_4_push_anim",
    "dnk_run2_anim",
    "dnk_climb_down_anim",
    "dnk_2_pin_anim",
    "dnk_4_pin_anim",
    "dnk_2_raise_arm_anim",
    "dnk_4_raise_arm_anim",
    "dnk_2_punch_anim",
    "dnk_4_punch_anim",
    "dnk_2_butt_anim",
    "dnk_4_butt_anim",
    "dnk_2_lbowdrop_anim",
    "dnk_4_lbowdrop_anim",
    "dnk_2_kick_anim",
    "dnk_4_kick_anim",
    "dnk_2_knee_anim",
    "dnk_4_knee_anim",
    "dnk_2_stomp_anim",
    "dnk_4_stomp_anim",
    "dnk_flying_kick_anim",
    "dnk_diveofftb_anim",
    "dnk_4_bstomp_anim",
    "dnk_3_head_hold2_anim",
    "dnk_3_head_hold_anim",
    "dnk_3_head_held_stand_anim",
    "dnk_3_head_slam_anim",
    "dnk_3_pile_driver_anim",
    "dnk_3_pile_driver_anim",
    "FLYKICK",
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
static void anim(wm_arcade_actor_t*a,const char*l,const wm_arcade_doink_callbacks_t*c){if(c&&c->change_anim_restart&&l)c->change_anim_restart(a,l,c->user);}
static void anim1(wm_arcade_actor_t*a,const char*l,const wm_arcade_doink_callbacks_t*c){if(c&&c->change_anim_label&&l)c->change_anim_label(a,l,c->user);}
static void snd(wm_arcade_actor_t*a,const char*l,const wm_arcade_doink_callbacks_t*c){if(c&&c->sound_label&&l)c->sound_label(a,l,c->user);}
static void startsp(wm_arcade_actor_t*a,const char*l,const wm_arcade_doink_callbacks_t*c){if(!a||!l)return;if(c&&c->resolve_label_token)a->special_move_addr=c->resolve_label_token(l,c->user);if(c&&c->start_special_label)c->start_special_label(a,l,c->user);}

static int do_block(wm_arcade_actor_t*a,const wm_arcade_doink_env_t*e,const wm_arcade_doink_callbacks_t*c){
    if(e&&e->blocking_off)return 0;
    anim1(a,L.block,c);                 /* :1949 change_anim1 */
    a->block_time=0;
    snd(a,"BLOCK_WOOSH",c);
    setmode(a,WM_PMODE_BLOCK);
    return 1;
}
/*
 * ------------------------------------------------------------------
 * DOINK.ASM's six JJXM tables (JJXM.H; wm/arcade/wm_arcade_jjxm.h).
 *
 * What stood here was one groundish()+near() approximation per button.
 * It got the two common rows of each table about right and lost
 * everything else: every row that names ONE target for a whole
 * opponent mode (do_pile against a HEADHELD opponent, the big boot
 * against a RUNNING or BOUNCING one, the turnbuckle spin kick against
 * an INAIR2 one, the unconditional punch against a CLIMBTURNBKL one),
 * the WAITANIM-and-below rows whose Z threshold is 62 rather than 60,
 * the modes with no row at all -- MODE_CHOKING, where the press does
 * nothing -- and the whole of mode_running, where the same buttons
 * mean a flying clothesline and a belly flop. super_kick was not an
 * approximation at all; it called basic_kick, so Doink's super kick
 * was his light kick.
 *
 * Each handler below is a target one of the tables names, under the
 * source's own label, so table and body can be read side by side.
 */
static const char *jjxm(const char*section,const char*entry,
                        const wm_arcade_actor_t*a,const wm_arcade_actor_t*o){
    return wm_jjxm_pick("DOINK",section,entry,a,o);
}
#define is(t,n) wm_jjxm_is((t),(n))

/* DOINK.ASM:1910 #punch_punch, which `std_punch` is an alias of. */
static void dnk_punch_punch(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    anim(a,face_label(L.punch2,L.punch4,a),c); snd(a,"PUNCH",c);
}
/* :1921 #punch_hdbutt */
static void dnk_punch_hdbutt(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    anim(a,face_label(L.close2,L.close4,a),c); snd(a,"HDBUTT",c);
}
/* :1931 #punch_lbowdrop */
static void dnk_punch_lbowdrop(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    anim(a,face_label(L.ground2,L.ground4,a),c); snd(a,"LBOWDROP",c);
}
/* :2012 #spunch_slap */
static void dnk_spunch_slap(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    anim(a,face_label("dnk_2_slap_anim","dnk_4_slap_anim",a),c); snd(a,"SPUNCH",c);
}
/* :2021 #spunch_special. Stick DOWN is the uppercut; otherwise an
   xdist past 60 falls all the way back to the plain punch, and only
   inside that does the double headbutt come out. */
static void dnk_spunch_special(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    if(a->stick_val_cur&WM_MOVE_DOWN){
        anim1(a,"dnk_4_uppercut_anim",c);   /* :2061 change_anim1 */
        snd(a,"SPUNCH",c); return;
    }
    if(a->closest_xdist>60){ dnk_punch_punch(a,c); return; }
    anim1(a,face_label("dnk_2_butts_anim","dnk_4_butts_anim",a),c); /* :2051 */
    snd(a,"HDBUTT",c);
}
/* :2067 #spunch_lbowdrop -- the hair grab. Three things have to hold:
   the man on the mat is not DEAD, he is at least 20h whole pixels away
   along X, and the two sprites' M_FLIPH bits DIFFER, which is what
   "at his head rather than his feet" amounts to. `cmp a0,a14 / jrz
   #no` means the SAME flip is the ordinary elbow drop. */
static void dnk_spunch_lbowdrop(wm_arcade_actor_t*a,wm_arcade_actor_t*o,
                                const wm_arcade_doink_callbacks_t*c){
    int hair=0;
    if(o&&o->player_mode!=WM_PMODE_DEAD){
        int32_t dx=a->x_fixed-o->x_fixed;
        if(dx<0)dx=-dx;
        if((dx>>16)>=0x20&&
           ((a->obj_control&WM_OBJ_FLIPH)!=(o->obj_control&WM_OBJ_FLIPH)))
            hair=1;
    }
    /* :2102 and :2107, both change_anim1 -- unlike :1931 #punch_lbowdrop,
       which is the same animation entered with change_anim1a. */
    if(hair) anim1(a,face_label("dnk_2_hair_pickup_anim","dnk_4_hair_pickup_anim",a),c);
    else     anim1(a,face_label(L.ground2,L.ground4,a),c);
    snd(a,"LBOWDROP",c);
}
/* :3451 do_pile, which lives in mode_headhold and which the
   mode_normal super-punch table jumps into when the CLOSEST opponent
   is HEADHELD -- a third wrestler attacking a man someone else is
   holding, so it is reachable only in buddy and eight-on-one matches.
   USR_VAR2 is Doink's repeated-uppercut flag and gates the whole
   routine. */
/* :3430 mode_headhold's #punch. Holding TOWARD the opponent is the
   PLURAL dnk_uppercuts_to_head_anim -- the combo animation -- and
   anything else is the single dnk_uppercut_to_head_anim. The source
   plays the sound BEFORE the animation on the first arm and AFTER it
   on the second; both orders are kept because the port's snd() and
   anim() are separate seams and the ordering is the only record of
   which arm ran. */
static void dnk_hdhold_punch(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);
    if(a->stick_val_cur==(uint16_t)(a->new_facing_dir&0x0c)){
        snd(a,"UPRCUT",c); anim(a,"dnk_uppercuts_to_head_anim",c); return;
    }
    anim(a,"dnk_uppercut_to_head_anim",c); snd(a,"UPRCUT",c);
}
static void dnk_do_pile(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    if(!a->usr_var2) return;            /* `jrz #z` -- nothing at all */
    if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);
    if(!(a->stick_val_cur&WM_MOVE_DOWN)){
        /* `jrz #punch`: falls into #punch, which calls
           FIND_AND_KILL_ENDLESS a second time. */
        dnk_hdhold_punch(a,c);
        return;
    }
    snd(a,"UPRCUT",c); anim(a,"dnk_3_pile_driver_anim",c);
}
/* :2169 #kick_kick, alias std_kick. */
static void dnk_kick_kick(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    anim(a,face_label(L.kick2,L.kick4,a),c); snd(a,"KICK",c);
}
/* :2180 #kick_knee, alias std_knee. */
static void dnk_kick_knee(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    anim(a,face_label(L.knee2,L.knee4,a),c); snd(a,"KICK",c);
}
/* :2190 #kick_stomp, and :2300 #skick_stomp, which is the same body. */
static void dnk_kick_stomp(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    anim(a,face_label(L.stomp2,L.stomp4,a),c); snd(a,"KICK",c);
}
/* :2255 #skick_TB */
static void dnk_skick_tb(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    anim(a,face_label("dnk_2_spin_kick_TB_anim","dnk_4_spin_kick_TB_anim",a),c);
    snd(a,"FLYKICK",c);
}
/* :2265 #skick_kick, alias #graboh. */
static void dnk_skick_kick(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    anim(a,face_label("dnk_2_spin_kick_anim","dnk_4_spin_kick_anim",a),c);
    snd(a,"FLYKICK",c);
}
/* :2275 #skick_special. Holding TOWARD the opponent turns the knee
   into the knee-fall; anything else is the plain knee. */
static void dnk_skick_special(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    if(a->stick_val_cur==(uint16_t)(a->new_facing_dir&0x0c)){
        anim1(a,"dnk_4_knee_fall_anim",c);  /* :2293 change_anim1 */
        snd(a,"GRABHOLD",c); return;
    }
    anim1(a,face_label(L.knee2,L.knee4,a),c); /* :2284 change_anim1 */
    snd(a,"FLYKICK",c);
}
/* :2311 #skick_bigboot */
static void dnk_skick_bigboot(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    anim(a,face_label("dnk_2_bigboot_anim","dnk_4_bigboot_anim",a),c);
    snd(a,"FLYKICK",c);
}

/* ---- the four mode_normal tables ---- */
static void basic_punch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_doink_callbacks_t*c){
    const char*t=jjxm("mode_normal","#punch",a,o);
    if(is(t,"#punch_hdbutt"))        dnk_punch_hdbutt(a,c);
    else if(is(t,"#punch_lbowdrop")) dnk_punch_lbowdrop(a,c);
    else if(is(t,"#punch_punch"))    dnk_punch_punch(a,c);
}
static void basic_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_doink_callbacks_t*c){
    const char*t=jjxm("mode_normal","#kick",a,o);
    if(is(t,"#kick_knee"))       dnk_kick_knee(a,c);
    else if(is(t,"#kick_stomp")) dnk_kick_stomp(a,c);
    else if(is(t,"#skick_TB"))   dnk_skick_tb(a,c);
    else if(is(t,"#kick_kick"))  dnk_kick_kick(a,c);
}
static void super_punch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_doink_callbacks_t*c){
    const char*t=jjxm("mode_normal","#super_punch",a,o);
    if(is(t,"#spunch_special"))       dnk_spunch_special(a,c);
    else if(is(t,"#spunch_lbowdrop")) dnk_spunch_lbowdrop(a,o,c);
    else if(is(t,"#spunch_slap"))     dnk_spunch_slap(a,c);
    else if(is(t,"do_pile"))          dnk_do_pile(a,c);
    else if(is(t,"std_punch"))        dnk_punch_punch(a,c);
}
static void super_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_doink_callbacks_t*c){
    const char*t=jjxm("mode_normal","#super_kick",a,o);
    if(is(t,"#skick_special"))      dnk_skick_special(a,c);
    else if(is(t,"#skick_kick"))    dnk_skick_kick(a,c);
    else if(is(t,"#skick_stomp"))   dnk_kick_stomp(a,c);
    else if(is(t,"#skick_bigboot")) dnk_skick_bigboot(a,c);
    else if(is(t,"#skick_TB"))      dnk_skick_tb(a,c);
    else if(is(t,"std_kick"))       dnk_kick_kick(a,c);
}

static wm_arcade_doink_step_result_t mode_normal(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_doink_env_t*e,const wm_arcade_doink_callbacks_t*c){
    uint8_t ac;
    if(a->anim_mode&WM_MODE_UNINT)return WM_DOINK_STEP_IDLE;
    if(a->i_will_die&&!a->immobilize_time){anim(a,L.fall,c);if(c&&c->adjust_health)c->adjust_health(a,-10,c->user);setmode(a,WM_PMODE_DEAD);a->i_will_die=0;return WM_DOINK_STEP_ACTION;}
    if(o&&o->player_mode==WM_PMODE_DEAD&&!(o->status_flags&WM_STATUS_ZOMBIE)){
        int reciprocal=a->attach_proc&&a->attach_proc->attach_proc==a;
        if(!reciprocal){
            if((c&&c->teammate_pin&&c->teammate_pin(a,c->user))||(c&&c->raisearm_check&&c->raisearm_check(a,c->user))){anim(a,face_label(L.raise2,L.raise4,a),c);if(c->set_raisearm_bit)c->set_raisearm_bit(a,c->user);if(c->drone_change_back)c->drone_change_back(a,c->user);return WM_DOINK_STEP_ACTION;}
            if(a->but_val_cur&&c&&c->can_pin&&c->can_pin(a,o,c->user)){
            anim(a,face_label(L.pin2,L.pin4,a),c); a->status_flags |= WM_STATUS_DID_PIN;
                if(c->drone_change_back)c->drone_change_back(a,c->user);
                return WM_DOINK_STEP_ACTION;
            }
        }
    }
    if(a->immobilize_time){a->move_dir=0;if(c&&c->execute_walk)c->execute_walk(a,c->user);return WM_DOINK_STEP_EXTERNAL;}
    if((a->but_val_cur&WM_BTN_BLOCK)&&do_block(a,e,c))return WM_DOINK_STEP_ACTION;
    ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
    if((a->but_val_cur&WM_BTN_ATTACK_MASK)==(WM_BTN_PUNCH|WM_BTN_KICK))ac=A_PUNCHKICK;
    switch(ac){case A_PUNCH:basic_punch(a,o,c);break;case A_BLOCK:(void)do_block(a,e,c);break;case A_SPUNCH:super_punch(a,o,c);break;case A_KICK:basic_kick(a,o,c);break;case A_PUNCHKICK:anim(a,"start_run_anim",c);break;case A_SKICK:super_kick(a,o,c);break;case A_GRABOH:dnk_skick_kick(a,c);break;default:break;}
    if(a->anim_mode&WM_MODE_UNINT)return WM_DOINK_STEP_ACTION;
    a->move_dir=a->stick_val_cur;
    if(c&&c->climb_turnbuckle&&c->climb_turnbuckle(a,c->user)){if(c->jump_rope_audio)c->jump_rope_audio(a,c->user);return WM_DOINK_STEP_EXTERNAL;}
    if(c&&c->execute_walk)c->execute_walk(a,c->user);
    return WM_DOINK_STEP_ACTION;
}
/* ---- the two mode_running tables ------------------------------- */

/*
 * DOINK.ASM:2471 #punch_clothesline. Two gates before the lunge, and
 * the source is emphatic about why: "Only allow clothesline if near
 * centre of ring, and running toward opponent."
 *
 * The first is position -- past RING_X_MID+70h running right, or short
 * of RING_X_MID-70h running left, and the press is simply dropped.
 *
 * The second is #mv_tbl, eleven longs indexed by NEW_FACING_DIR, whose
 * value is a BIT NUMBER tested against MOVE_DIR. Facing left
 * (UP_LEFT 5, DOWN_LEFT 6) stores MOVE_RIGHT_BIT and facing right
 * (UP_RIGHT 9, DOWN_RIGHT 10) stores MOVE_LEFT_BIT, so a wrestler
 * running AWAY from the way he faces is refused. Every other index
 * holds 0, which is MOVE_UP_BIT -- so for those facings the test asks
 * whether he is drifting up the screen. That is the table as written.
 */
static const uint8_t dnk_cline_mv_tbl[11] = {
    0, 0, 0, 0, 0, 3 /* MOVE_RIGHT_BIT */, 3,
    0, 0, 2 /* MOVE_LEFT_BIT */, 2
};

static wm_arcade_doink_step_result_t dnk_punch_clothesline(
        wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    unsigned idx;
    if(a->move_dir&WM_MOVE_LEFT){
        if(!(a->x_int>WM_RING_X_MID-0x70)) return WM_DOINK_STEP_IDLE;
    } else {
        if(!(a->x_int<WM_RING_X_MID+0x70)) return WM_DOINK_STEP_IDLE;
    }
    idx=(unsigned)(a->new_facing_dir&0xff);
    if(idx<sizeof dnk_cline_mv_tbl/sizeof dnk_cline_mv_tbl[0]&&
       (a->move_dir&(1<<dnk_cline_mv_tbl[idx])))
        return WM_DOINK_STEP_IDLE;
    anim(a,"dnk_fly_cline_anim",c);
    setmode(a,WM_PMODE_INAIR);
    a->run_time=0;
    snd(a,"FLYKICK",c);
    return WM_DOINK_STEP_ACTION;
}

/* :2530 #punch_lbowdrop / #punch_bellyflop -- two labels on one
   instruction, which is why the table's 176/176 split has the same
   body either side of it -- and :2622 #kick_runstomp, which is the
   same move again. */
static wm_arcade_doink_step_result_t dnk_run_belly(
        wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    anim(a,"dnk_belly_anim",c);
    setmode(a,WM_PMODE_INAIR);
    a->run_time=0;
    snd(a,"FLYKICK",c);
    return WM_DOINK_STEP_ACTION;
}

/* :2605 #kick_flyingkick. Note what it does NOT do: it is the one
   member of this group that leaves RUN_TIME alone. */
static wm_arcade_doink_step_result_t dnk_kick_flyingkick(
        wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    if(c&&c->ck_ignore&&c->ck_ignore(a,c->user)) return WM_DOINK_STEP_IDLE;
    anim(a,L.flykick,c);
    setmode(a,WM_PMODE_INAIR);
    snd(a,"FLYKICK",c);
    return WM_DOINK_STEP_ACTION;
}

static wm_arcade_doink_step_result_t run_punch(
        wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_doink_callbacks_t*c){
    const char*t=jjxm("mode_running","#punch",a,o);
    if(is(t,"#punch_clothesline")) return dnk_punch_clothesline(a,c);
    if(is(t,"#punch_bellyflop")||is(t,"#punch_lbowdrop")) return dnk_run_belly(a,c);
    return WM_DOINK_STEP_IDLE;
}
static wm_arcade_doink_step_result_t run_kick(
        wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_doink_callbacks_t*c){
    const char*t=jjxm("mode_running","#kick",a,o);
    if(is(t,"#kick_runstomp")) return dnk_run_belly(a,c);
    if(is(t,"#kick_flyingkick")) return dnk_kick_flyingkick(a,c);
    return WM_DOINK_STEP_IDLE;
}

static wm_arcade_doink_step_result_t mode_running(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_doink_env_t*e,const wm_arcade_doink_callbacks_t*c){
    int32_t v=0x00060000; a->run_time++; if(!a->usr_var1){if(c&&c->bounce_off_ropes)c->bounce_off_ropes(a,c->user);if(e&&e->hyper_speed_on>0&&e->hyper_speed_on<15)v<<=e->hyper_speed_on;if(!(a->move_dir&WM_MOVE_RIGHT))v=-v;a->x_vel=v;}
    if(a->stick_val_cur&WM_MOVE_UP)a->z_vel=-0x00020000;else if(a->stick_val_cur&WM_MOVE_DOWN)a->z_vel=0x00020000;else a->z_vel=0; if(a->getup_time||a->delay_butns)return WM_DOINK_STEP_IDLE;
    switch(action_table[a->but_val_down&WM_BTN_ATTACK_MASK]){
    case A_BLOCK:a->x_vel>>=1;setmode(a,WM_PMODE_NORMAL);(void)do_block(a,e,c);return WM_DOINK_STEP_ACTION;
    /* DOINK.ASM:2580's table, reached from #kick and #super_kick alike. */
    case A_KICK:case A_SKICK:return run_kick(a,o,c);
    /* :2444's table, reached from #punch, #super_punch, #punchkick and
       #graboh alike -- the clothesline group. */
    case A_PUNCH:case A_SPUNCH:case A_PUNCHKICK:case A_GRABOH:return run_punch(a,o,c);
    default:return WM_DOINK_STEP_IDLE;}
}
static wm_arcade_doink_step_result_t mode_bouncing(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){a->x_vel=0;a->z_vel=0;if(a->anim_mode&WM_MODE_END){a->move_dir^=(WM_MOVE_LEFT+WM_MOVE_RIGHT);a->facing_dir=(a->new_facing_dir&(WM_MOVE_UP+WM_MOVE_DOWN))|a->move_dir;anim(a,L.run,c);setmode(a,WM_PMODE_RUNNING);return WM_DOINK_STEP_ACTION;}return WM_DOINK_STEP_IDLE;}
static wm_arcade_doink_step_result_t mode_turn(wm_arcade_actor_t*a,const wm_arcade_doink_callbacks_t*c){
    uint8_t ac; if(a->stick_val_cur&WM_MOVE_DOWN){anim(a,L.climbdown,c);setmode(a,WM_PMODE_CLIMBTURNBKL);return WM_DOINK_STEP_ACTION;} ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];if(ac==A_NONE)return WM_DOINK_STEP_IDLE;setmode(a,WM_PMODE_INAIR);anim(a,(ac==A_KICK||ac==A_SKICK)?L.turn_kick:L.turn_punch,c);snd(a,"TURNDIVE",c);if(c&&c->jump_rope_audio)c->jump_rope_audio(a,c->user);return WM_DOINK_STEP_ACTION;
}
static wm_arcade_doink_step_result_t mode_block(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_doink_env_t*e,const wm_arcade_doink_callbacks_t*c){
    a->block_time++;if(a->block_time>=160&&o&&o->player_mode==WM_PMODE_BLOCK&&a->closest_xdist<0x61&&a->closest_xdist>=45&&a->closest_zdist<30){setmode(a,WM_PMODE_NORMAL);anim(a,L.push,c);snd(a,"PUSH",c);return WM_DOINK_STEP_ACTION;}
    if(!(a->but_val_down&WM_BTN_ATTACK_MASK))return WM_DOINK_STEP_IDLE;
    if(!(a->but_val_cur&WM_BTN_BLOCK)){setmode(a,WM_PMODE_NORMAL);return WM_DOINK_STEP_ACTION;}
    if((a->but_val_down&3)==1||(a->but_val_down&3)==3){setmode(a,WM_PMODE_NORMAL);anim(a,L.push,c);return WM_DOINK_STEP_ACTION;}
    (void)e;
    return WM_DOINK_STEP_IDLE;
}
static wm_arcade_doink_step_result_t mode_headhold(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_doink_env_t*e,const wm_arcade_doink_callbacks_t*c){
    /* "Bozo power move": `callr bozo_check / jrnc #fail`, and
       everything below this is #fail. Six of the eight
       dispatchers did not call it at all, so the move was
       missing rather than merely inert. */
    if(c&&c->bozo_check&&c->bozo_check(a,c->user)){
        snd(a,L.bozo_snd,c);
        anim(a,(e&&(e->pcnt&1))?L.bozo_b:L.bozo_a,c);
        return WM_DOINK_STEP_ACTION;
    }
    /*
     * `#fail`. The mode reads WHOIHIT, not the caller's opponent: this
     * is the man Doink has by the head, who need not be the closest
     * wrestler once a third one is in the ring.
     */
    wm_arcade_actor_t *v = a->who_i_hit ? a->who_i_hit : o;
    uint8_t ac;
    if(!v||v->player_mode!=WM_PMODE_HEADHELD){
        /*
         * `#exit`: drop six units of Z, then FACE the opponent --
         * MOVE_DOWN_RIGHT normally and MOVE_DOWN_LEFT when B_FLIPH is
         * set -- into BOTH FACING_DIR and NEW_FACING_DIR, and only
         * then SETMODE NORMAL. The facing write used to be missing,
         * so letting go of a head hold left Doink pointing wherever
         * the hold had put him.
         */
        int32_t f=(a->obj_control&WM_OBJ_FLIPH)?WM_MOVE_DOWN_LEFT
                                              :WM_MOVE_DOWN_RIGHT;
        a->z_fixed-=6<<16;
        a->facing_dir=f;
        a->new_facing_dir=f;
        setmode(a,WM_PMODE_NORMAL);
        return WM_DOINK_STEP_ACTION;
    }
    if(a->anim_mode&WM_MODE_UNINT)return WM_DOINK_STEP_IDLE;
    /*
     * mode_headhold's OWN #action_table (:3411). Same shape as
     * mode_normal's, and its #block, #graboh and #z all fall on one
     * `rets`, so three of the eight actions do nothing here.
     *
     * Every animation below is change_anim1a. That is not incidental:
     * #punch restarting from frame 0 on each press is what the
     * repeated-uppercut combo is, and USR_VAR2 -- which do_pile checks
     * -- is the flag that combo sets.
     */
    ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
    switch(ac){
    case A_PUNCH:                                   /* :3430 #punch */
        dnk_hdhold_punch(a,c);
        return WM_DOINK_STEP_ACTION;
    case A_SPUNCH:                                  /* :3451 #super_punch */
        dnk_do_pile(a,c);
        return WM_DOINK_STEP_ACTION;
    case A_KICK: case A_PUNCHKICK:                  /* :3467 #kick/#punchkick */
        if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);
        snd(a,"KICK",c);
        anim(a,"dnk_3_knee_to_head_anim",c);
        return WM_DOINK_STEP_ACTION;
    case A_SKICK:                                   /* :3477 #super_kick */
        /* Toward the opponent only; anything else falls on #z. */
        if(a->stick_val_cur!=(uint16_t)(a->new_facing_dir&0x0c))
            return WM_DOINK_STEP_IDLE;
        if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);
        snd(a,"KICK",c);
        anim(a,"dnk_3_knees_to_head_anim",c);
        return WM_DOINK_STEP_ACTION;
    default:                                        /* #block, #graboh, #z */
        return WM_DOINK_STEP_IDLE;
    }
}

wm_arcade_doink_step_result_t wm_arcade_move_doink(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_doink_env_t*e,const wm_arcade_doink_callbacks_t*c){
    if(!a)return WM_DOINK_STEP_IDLE;
    if(c&&c->check_secret_moves)c->check_secret_moves(a,secret_patterns,sizeof secret_patterns/sizeof secret_patterns[0],c->user);
    switch(a->player_mode){case WM_PMODE_NORMAL:case 18:case 22:case 23:return mode_normal(a,o,e,c);case WM_PMODE_RUNNING:return mode_running(a,o,e,c);case WM_PMODE_ATTACHED:if(c&&c->keep_attached)c->keep_attached(a,c->user);else(void)wm_arcade_keep_attached(a);if(!a->attach_proc){setmode(a,WM_PMODE_NORMAL);a->anim_mode=0;}return WM_DOINK_STEP_EXTERNAL;case WM_PMODE_BOUNCING:return mode_bouncing(a,c);case WM_PMODE_ONTURNBKL:return mode_turn(a,c);case WM_PMODE_BLOCK:return mode_block(a,o,e,c);case WM_PMODE_DEAD:if(c&&c->mode_dead)c->mode_dead(a,c->user);return WM_DOINK_STEP_EXTERNAL;case WM_PMODE_WAITANIM:if((a->anim_mode&WM_MODE_END)&&c&&c->code_addr)c->code_addr(a,(uint32_t)a->code_addr,c->user);return WM_DOINK_STEP_EXTERNAL;case WM_PMODE_MASTER:if(c&&c->master_keep_attached)c->master_keep_attached(a,c->user);else(void)wm_arcade_master_keep_attached(a);return WM_DOINK_STEP_EXTERNAL;case WM_PMODE_HEADHOLD:return mode_headhold(a,o,e,c);case WM_PMODE_HEADHELD:if((a->anim_mode&WM_MODE_NOGRAVITY)&&c&&c->mode_choking){c->mode_choking(a,c->user);return WM_DOINK_STEP_EXTERNAL;}if(c&&c->bozo_check&&c->bozo_check(a,c->user)){if(c->do_reversal)c->do_reversal(a,c->user);if(c->do_reversal_message)c->do_reversal_message(a,c->user);snd(a,L.bozo_snd,c);anim(a,(e&&(e->pcnt&1))?L.bozo_hh_b:L.bozo_a,c);return WM_DOINK_STEP_ACTION;}if(!a->attach_proc&&a->y_int<=a->ground_y)anim(a,L.headheld,c);return WM_DOINK_STEP_EXTERNAL;case WM_PMODE_PUPPET:if(c&&c->mode_puppet)c->mode_puppet(a,c->user);return WM_DOINK_STEP_EXTERNAL;case WM_PMODE_INAIR2:if(c&&c->mode_inair2)c->mode_inair2(a,c->user);return WM_DOINK_STEP_EXTERNAL;case WM_PMODE_CHOKING:if(c&&c->mode_choking)c->mode_choking(a,c->user);return WM_DOINK_STEP_EXTERNAL;default:return WM_DOINK_STEP_IDLE;}
}

static int reject_common(wm_arcade_actor_t*a,wm_arcade_actor_t*o){return !a||!o||(a->anim_mode&WM_MODE_UNINT)||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_HEADHELD||o->player_mode==WM_PMODE_ATTACHED;}
int wm_arcade_doink_release_charge(wm_arcade_actor_t*a,wm_arcade_actor_t*o,uint16_t ticks,const wm_arcade_doink_callbacks_t*c){if(!a||ticks<100)return 0;if(a->player_mode==WM_PMODE_HEADHELD||a->player_mode==WM_PMODE_HEADHOLD||(a->anim_mode&WM_MODE_UNINT))return 0;if(!o||o->player_mode==WM_PMODE_DEAD)return 0; if(a->player_mode==WM_PMODE_RUNNING||a->stick_val_cur==(uint16_t)(a->new_facing_dir&0x0c)) anim(a,face_label("dnk_2_buzz2_anim","dnk_4_buzz2_anim",a),c); else anim(a,face_label("dnk_2_buzz_anim","dnk_4_buzz_anim",a),c); snd(a,"HDBUTT_T1",c); return 1;}
int wm_arcade_doink_fire_secret(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_doink_secret_id_t id,uint32_t pcnt,const wm_arcade_doink_callbacks_t*c){
    switch(id){
    case WM_DOINK_SECRET_NECK_GRAB:
        if(reject_common(a,o)||groundish(o))return 0;
        if((uint32_t)(pcnt-a->last_headhold)<120) anim(a,"dnk_3_fake_hold_anim",c);
        else if(a->closest_xdist<=80) anim(a,L.headhold2,c); else anim(a,L.headhold,c);
        return 1; case WM_DOINK_SECRET_GRAB_FLING: if(reject_common(a,o))return 0; anim(a,face_label(L.headhold2,L.headhold,a),c); snd(a,"GRABFLING",c); return 1; case WM_DOINK_SECRET_GRAB_FLING2: if(reject_common(a,o))return 0; anim(a,face_label(L.headhold2,L.headhold,a),c); snd(a,"GRABFLING",c); return 1; case WM_DOINK_SECRET_HIP_TOSS: if(reject_common(a,o)||groundish(o))return 0; if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0; anim(a,face_label("dnk_2_hiptoss_anim","dnk_4_hiptoss_anim",a),c); snd(a,"HIPTOSS_PUNCH",c); return 1; case WM_DOINK_SECRET_HIP_TOSS2: if(reject_common(a,o)||groundish(o))return 0; if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0; anim(a,face_label("dnk_2_hiptoss_anim","dnk_4_hiptoss_anim",a),c); snd(a,"HIPTOSS_PUNCH",c); return 1;
    case WM_DOINK_SECRET_EARSLAP: if(reject_common(a,o))return 0; anim(a,face_label("dnk_2_earslap_anim","dnk_4_earslap_anim",a),c); return 1;
    case WM_DOINK_SECRET_HAMMER: if((a->anim_mode&WM_MODE_UNINT)||a->player_mode==WM_PMODE_ONTURNBKL)return 0; anim(a,"dnk_4_hammer_anim",c); return 1;
    case WM_DOINK_SECRET_BOXING_PNCH: if(reject_common(a,o)||a->combo_count)return 0; anim(a,face_label("dnk_2_box_anim","dnk_4_box_anim",a),c); return 1;
    default:return 0;}
}
int wm_arcade_doink_fire_monitor(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_doink_monitor_id_t id,const wm_arcade_doink_env_t*e,int opponent_attack_is_leaping,const wm_arcade_doink_callbacks_t*c){
    wm_arcade_actor_t*t;const char*s;(void)e;(void)opponent_attack_is_leaping;if(!a||(unsigned)id>=sizeof special_processes/sizeof special_processes[0])return 0;s=special_processes[(unsigned)id];
    if(strstr(s,"hdhold_")!=NULL){if(a->player_mode==WM_PMODE_HEADHELD){t=a->who_hit_me?a->who_hit_me:o;if(!t)return 0;a->smart_target=t;t->immobilize_time=15;if(c&&c->do_reversal)c->do_reversal(a,c->user);if(c&&c->do_reversal_message)c->do_reversal_message(a,c->user);}else{t=a->who_i_hit?a->who_i_hit:o;if(a->player_mode!=WM_PMODE_HEADHOLD||!t)return 0;a->smart_target=t;t->immobilize_time=15;}if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);}
    if(strstr(s,"combo")!=NULL&&c&&c->check_combo_go&&c->check_combo_go(a,c->user)<0)return 0;
    startsp(a,s,c);
    return 1;
}
