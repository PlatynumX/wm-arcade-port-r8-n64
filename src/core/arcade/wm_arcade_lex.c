#include "wm/arcade/wm_arcade_lex.h"
#include "wm/arcade/wm_arcade_attach_anim.h"
#include "wm/arcade/wm_arcade_jjxm.h"
#include <string.h>

/*
 * Dedicated direct C translation boundary for LEX.ASM.
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

static const wm_arcade_input_pattern_t secret_patterns[]={
    {"charge_clobber",NULL,0,0},
    {"neck_grab",s_neck30,3,30},
    {"grab_fling",s_grab_fling,3,32},
    {"hip_toss",s_hip_toss,3,32},
    {"grab_fling2",s_grab_fling2,1,10},
    {"hip_toss2",s_hip_toss2,1,10},
    {"sliding_elbow",s_toward_punch,3,30},
    {"hammer",s_toward_skick,3,32}
};
/* WRESTLE2.ASM's lex_smove_table, as the assembler built it.
   The finishing-move entries every one of these tables carries sit
   inside `.if NUM_LEX_FINISHES`, and GAME.EQU:585 sets that switch to
   0 -- so none of them was assembled. Generated as
   wm_wrestler_smoves[] (wm/wrestler_anim_tables.h); a source-tool test
   holds this copy to it. */
static const char *const special_processes[]={
    "lex_hdhold_pile",
    "lex_hdhold_elbow_face",
    "lex_hdhold_graboh",
    "lex_grab_toss_air",
    "lex_hdhold_combo1",
    "lex_hdhold_combo2",
    "std_walk_fast",
    "std_taunt"
};

const wm_arcade_wrestler_profile_t wm_arcade_profile_lex={
    WM_ROSTER_LEX,"Lex Luger","LEX.ASM",2749,"lex",WM_BTN_PUNCH,100,
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
    "lex_stand2_anim",
    "lex_stand4_anim",
    "lex_torso2_anim",
    "lex_torso4_anim",
    "lex_fall_back_anim",
    "lex_4_block_anim",
    "lex_4_push_anim",
    "lex_run2_anim",
    "lex_climb_down_anim",
    "lex_2_pin_anim",
    "lex_4_pin_anim",
    "lex_2_raise_arm_anim",
    "lex_4_raise_arm_anim",
    "lex_2_punch_anim",
    "lex_4_punch_anim",
    "lex_2_butt_anim",
    "lex_4_butt_anim",
    "lex_2_ground_punch_anim",
    "lex_4_ground_punch_anim",
    "lex_2_kick_anim",
    "lex_4_kick_anim",
    "lex_4_knee_anim",
    "lex_4_knee_anim",
    "lex_2_stomp_anim",
    "lex_4_stomp_anim",
    "lex_flying_kick_anim",
    "lex_buckle_leap_anim",
    "lex_buckle_leap_anim",
    "lex_3_head_hold2_anim",
    "lex_3_head_hold_anim",
    "lex_3_head_held_stand_anim",
    "lex_vsuplex_anim",
    "lex_4_graboh_anim",
    "lex_vsuplex_anim",
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
static void anim(wm_arcade_actor_t*a,const char*l,const wm_arcade_lex_callbacks_t*c){if(c&&c->change_anim_label&&l)c->change_anim_label(a,l,c->user);}
static void snd(wm_arcade_actor_t*a,const char*l,const wm_arcade_lex_callbacks_t*c){if(c&&c->sound_label&&l)c->sound_label(a,l,c->user);}
static void startsp(wm_arcade_actor_t*a,const char*l,const wm_arcade_lex_callbacks_t*c){if(!a||!l)return;if(c&&c->resolve_label_token)a->special_move_addr=c->resolve_label_token(l,c->user);if(c&&c->start_special_label)c->start_special_label(a,l,c->user);}

static int do_block(wm_arcade_actor_t*a,const wm_arcade_lex_env_t*e,const wm_arcade_lex_callbacks_t*c){
    if(e&&e->blocking_off)return 0;
    anim(a,L.block,c);
    a->block_time=0;
    snd(a,"BLOCK_WOOSH",c);
    setmode(a,WM_PMODE_BLOCK);
    return 1;
}
/*
 * LEX.ASM's six JJXM tables (JJXM.H; wm/arcade/wm_arcade_jjxm.h), the
 * same replacement Doink got. What stood here approximated the two
 * common rows of each 22-row table and lost the rest; super_kick was
 * not an approximation at all, it called basic_kick, so Lex's super
 * kick was his light kick.
 */
static const char *jjxm(const char*section,const char*entry,
                        const wm_arcade_actor_t*a,const wm_arcade_actor_t*o){
    return wm_jjxm_pick("LEX",section,entry,a,o);
}
#define is(t,n) wm_jjxm_is((t),(n))

/* LEX.ASM:1353 #punch_punch, alias std_punch. */
static void lex_punch_punch(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    anim(a,face_label(L.punch2,L.punch4,a),c); snd(a,"PUNCH",c);
}
/* :1361 #punch_hdbutt */
static void lex_punch_hdbutt(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    anim(a,face_label(L.close2,L.close4,a),c); snd(a,"HDBUTT",c);
}
/* :1368 #punch_lbowdrop -- Lex's is the ground PUNCH, not an elbow. */
static void lex_punch_lbowdrop(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    anim(a,face_label(L.ground2,L.ground4,a),c); snd(a,"LBOWDROP",c);
}
/* :1447 #spunch_slap -- and it is the clobber, not a slap. */
static void lex_spunch_slap(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    anim(a,"lex_3_clobber_anim",c); snd(a,"PUNCH",c);
}
/* :1455 #spunch_special. One threshold, and it is 55 -- the commented
   -out 65 above it in the source is not what ships. */
static void lex_spunch_special(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    if(a->closest_xdist>55){ lex_punch_punch(a,c); return; }
    anim(a,face_label("lex_2_butts_anim","lex_4_butts_anim",a),c); snd(a,"HDBUTT",c);
}
/* :1475 #spunch_lbowdrop -- the hair grab, the same three tests Doink's
   has: not DEAD, at least 20h whole pixels along X, and the two
   sprites' M_FLIPH bits DIFFERING. */
static void lex_spunch_lbowdrop(wm_arcade_actor_t*a,wm_arcade_actor_t*o,
                                const wm_arcade_lex_callbacks_t*c){
    int hair=0;
    if(o&&o->player_mode!=WM_PMODE_DEAD){
        int32_t dx=a->x_fixed-o->x_fixed;
        if(dx<0)dx=-dx;
        if((dx>>16)>=0x20&&
           ((a->obj_control&WM_OBJ_FLIPH)!=(o->obj_control&WM_OBJ_FLIPH)))
            hair=1;
    }
    if(hair) anim(a,face_label("lex_2_hair_pickup_anim","lex_4_hair_pickup_anim",a),c);
    else     anim(a,face_label(L.ground2,L.ground4,a),c);
    snd(a,"LBOWDROP",c);
}
/* :1569 #kick_kick, alias std_kick. */
static void lex_kick_kick(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    anim(a,face_label(L.kick2,L.kick4,a),c); snd(a,"KICK",c);
}
/* :1579 #kick_knee, alias std_knee. Note it is NOT a FACE24 -- Lex has
   one knee animation and the source names it outright. */
static void lex_kick_knee(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    anim(a,"lex_4_knee_anim",c); snd(a,"KICK",c);
}
/* :1589 #kick_stomp, alias std_stomp. */
static void lex_kick_stomp(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    anim(a,face_label(L.stomp2,L.stomp4,a),c); snd(a,"KICK",c);
}
/* :1560 #kick_TB */
static void lex_kick_tb(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    anim(a,"lex_kick_TB_anim",c); snd(a,"KICK",c);
}
/* :1665 #skick_kick */
static void lex_skick_kick(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    anim(a,"lex_super_kick_anim",c); snd(a,"FLYKICK",c);
}
/* :1674 #skick_special -- stick held toward him is the knee-fall. */
static void lex_skick_special(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    if(a->stick_val_cur==(uint16_t)(a->new_facing_dir&0x0c)){
        anim(a,"lex_4_knee_fall_anim",c); snd(a,"GRABHOLD",c); return;
    }
    anim(a,"lex_4_knee_anim",c); snd(a,"KICK",c);
}
/* :1696 #skick_bigboot */
static void lex_skick_bigboot(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    anim(a,"lex_4_bigboot_anim",c); snd(a,"FLYKICK",c);
}
/* :1706 #graboh, which unlike Doink's is a routine of its own rather
   than an alias of the spin kick. */
static void lex_graboh(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    anim(a,"lex_4_graboh_anim",c); snd(a,"GRABHOLD",c);
}

static void basic_punch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_lex_callbacks_t*c){
    const char*t=jjxm("mode_normal","#punch",a,o);
    if(is(t,"#punch_hdbutt"))        lex_punch_hdbutt(a,c);
    else if(is(t,"#punch_lbowdrop")) lex_punch_lbowdrop(a,c);
    else if(is(t,"#punch_punch"))    lex_punch_punch(a,c);
}
static void basic_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_lex_callbacks_t*c){
    const char*t=jjxm("mode_normal","#kick",a,o);
    if(is(t,"#kick_knee"))       lex_kick_knee(a,c);
    else if(is(t,"#kick_stomp")) lex_kick_stomp(a,c);
    else if(is(t,"#kick_TB"))    lex_kick_tb(a,c);
    else if(is(t,"#kick_kick"))  lex_kick_kick(a,c);
}
static void super_punch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_lex_callbacks_t*c){
    const char*t=jjxm("mode_normal","#super_punch",a,o);
    if(is(t,"#spunch_special"))       lex_spunch_special(a,c);
    else if(is(t,"#spunch_lbowdrop")) lex_spunch_lbowdrop(a,o,c);
    else if(is(t,"#spunch_slap"))     lex_spunch_slap(a,c);
    else if(is(t,"std_punch"))        lex_punch_punch(a,c);
}
static void super_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_lex_callbacks_t*c){
    const char*t=jjxm("mode_normal","#super_kick",a,o);
    if(is(t,"#skick_special"))      lex_skick_special(a,c);
    else if(is(t,"#skick_kick"))    lex_skick_kick(a,c);
    else if(is(t,"#skick_bigboot")) lex_skick_bigboot(a,c);
    else if(is(t,"#kick_TB"))       lex_kick_tb(a,c);
    else if(is(t,"std_stomp"))      lex_kick_stomp(a,c);
    else if(is(t,"std_kick"))       lex_kick_kick(a,c);
}

static wm_arcade_lex_step_result_t mode_normal(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_lex_env_t*e,const wm_arcade_lex_callbacks_t*c){
    uint8_t ac;
    if(a->anim_mode&WM_MODE_UNINT)return WM_LEX_STEP_IDLE;
    if(a->i_will_die&&!a->immobilize_time){anim(a,L.fall,c);if(c&&c->adjust_health)c->adjust_health(a,-10,c->user);setmode(a,WM_PMODE_DEAD);a->i_will_die=0;return WM_LEX_STEP_ACTION;}
    if(o&&o->player_mode==WM_PMODE_DEAD&&!(o->status_flags&WM_STATUS_ZOMBIE)){
        int reciprocal=a->attach_proc&&a->attach_proc->attach_proc==a;
        if(!reciprocal){
            if((c&&c->teammate_pin&&c->teammate_pin(a,c->user))||(c&&c->raisearm_check&&c->raisearm_check(a,c->user))){anim(a,face_label(L.raise2,L.raise4,a),c);if(c->set_raisearm_bit)c->set_raisearm_bit(a,c->user);if(c->drone_change_back)c->drone_change_back(a,c->user);return WM_LEX_STEP_ACTION;}
            if(a->but_val_cur&&c&&c->can_pin&&c->can_pin(a,o,c->user)){
            anim(a,face_label(L.pin2,L.pin4,a),c); a->status_flags |= WM_STATUS_DID_PIN;
                if(c->drone_change_back)c->drone_change_back(a,c->user);
                return WM_LEX_STEP_ACTION;
            }
        }
    }
    if(a->immobilize_time){a->move_dir=0;if(c&&c->execute_walk)c->execute_walk(a,c->user);return WM_LEX_STEP_EXTERNAL;}
    if((a->but_val_cur&WM_BTN_BLOCK)&&do_block(a,e,c))return WM_LEX_STEP_ACTION;
    ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
    if((a->but_val_cur&WM_BTN_ATTACK_MASK)==(WM_BTN_PUNCH|WM_BTN_KICK))ac=A_PUNCHKICK;
    switch(ac){case A_PUNCH:basic_punch(a,o,c);break;case A_BLOCK:(void)do_block(a,e,c);break;case A_SPUNCH:super_punch(a,o,c);break;case A_KICK:basic_kick(a,o,c);break;case A_PUNCHKICK:anim(a,"start_run_anim",c);break;case A_SKICK:super_kick(a,o,c);break;case A_GRABOH:lex_graboh(a,c);break;default:break;}
    if(a->anim_mode&WM_MODE_UNINT)return WM_LEX_STEP_ACTION;
    a->move_dir=a->stick_val_cur;
    if(c&&c->climb_turnbuckle&&c->climb_turnbuckle(a,c->user)){if(c->jump_rope_audio)c->jump_rope_audio(a,c->user);return WM_LEX_STEP_EXTERNAL;}
    if(c&&c->execute_walk)c->execute_walk(a,c->user);
    return WM_LEX_STEP_ACTION;
}
/* ---- the two mode_running tables ------------------------------- */

/* LEX.ASM:1936 #kick_flyingkick. */
static wm_arcade_lex_step_result_t lex_kick_flyingkick(
        wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    if(c&&c->ck_ignore&&c->ck_ignore(a,c->user)) return WM_LEX_STEP_IDLE;
    setmode(a,WM_PMODE_INAIR);
    anim(a,L.flykick,c);
    snd(a,"FLYKICK",c);
    return WM_LEX_STEP_ACTION;
}
/* :1869 #punch_bellyflop, alias attack_bellyflop. Unlike Doink's it
   sets no mode and does not clear RUN_TIME. */
static wm_arcade_lex_step_result_t lex_attack_bellyflop(
        wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    anim(a,"lex_flying_ground_punch_anim",c);
    snd(a,"FLYKICK",c);
    return WM_LEX_STEP_ACTION;
}

static wm_arcade_lex_step_result_t lex_run_punch(
        wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_lex_callbacks_t*c){
    const char*t=jjxm("mode_running","#punch",a,o);
    /* :1861 #punch_clothesline is `;TODO - fix this / ;HACK!!! /
       jruc #kick_flyingkick` -- Lex has no clothesline in the shipped
       game, the label just falls into the flying kick. Kept as the
       source has it rather than tidied into one target, because the
       table still names both. */
    if(is(t,"#punch_clothesline")) return lex_kick_flyingkick(a,c);
    if(is(t,"#punch_bellyflop"))   return lex_attack_bellyflop(a,c);
    if(is(t,"#punch_rets"))        return WM_LEX_STEP_IDLE;   /* `rets` */
    return WM_LEX_STEP_IDLE;
}
static wm_arcade_lex_step_result_t lex_run_kick(
        wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_lex_callbacks_t*c){
    const char*t=jjxm("mode_running","#kick",a,o);
    if(is(t,"#kick_flyingkick")) return lex_kick_flyingkick(a,c);
    if(is(t,"attack_bellyflop")) return lex_attack_bellyflop(a,c);
    if(is(t,"#kick_rets"))       return WM_LEX_STEP_IDLE;     /* `rets` */
    return WM_LEX_STEP_IDLE;
}

static wm_arcade_lex_step_result_t mode_running(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_lex_env_t*e,const wm_arcade_lex_callbacks_t*c){
    (void)o;
    int32_t v=0x00060000; a->run_time++; if(!a->usr_var1){if(c&&c->bounce_off_ropes)c->bounce_off_ropes(a,c->user);if(e&&e->hyper_speed_on>0&&e->hyper_speed_on<15)v<<=e->hyper_speed_on;if(!(a->move_dir&WM_MOVE_RIGHT))v=-v;a->x_vel=v;}
    if(a->stick_val_cur&WM_MOVE_UP)a->z_vel=-0x00020000;else if(a->stick_val_cur&WM_MOVE_DOWN)a->z_vel=0x00020000;else a->z_vel=0; if(a->getup_time||a->delay_butns)return WM_LEX_STEP_IDLE;
    switch(action_table[a->but_val_down&WM_BTN_ATTACK_MASK]){
    case A_BLOCK:a->x_vel>>=1;setmode(a,WM_PMODE_NORMAL);(void)do_block(a,e,c);return WM_LEX_STEP_ACTION;
    case A_KICK:case A_SKICK:return lex_run_kick(a,o,c);
    case A_PUNCH:case A_SPUNCH:case A_PUNCHKICK:case A_GRABOH:return lex_run_punch(a,o,c);
    default:return WM_LEX_STEP_IDLE;}
}
static wm_arcade_lex_step_result_t mode_bouncing(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){a->x_vel=0;a->z_vel=0;if(a->anim_mode&WM_MODE_END){a->move_dir^=(WM_MOVE_LEFT+WM_MOVE_RIGHT);a->facing_dir=(a->new_facing_dir&(WM_MOVE_UP+WM_MOVE_DOWN))|a->move_dir;anim(a,L.run,c);setmode(a,WM_PMODE_RUNNING);return WM_LEX_STEP_ACTION;}return WM_LEX_STEP_IDLE;}
static wm_arcade_lex_step_result_t mode_turn(wm_arcade_actor_t*a,const wm_arcade_lex_callbacks_t*c){
    uint8_t ac; if(a->stick_val_cur&WM_MOVE_DOWN){anim(a,L.climbdown,c);setmode(a,WM_PMODE_CLIMBTURNBKL);return WM_LEX_STEP_ACTION;} ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];if(ac==A_NONE)return WM_LEX_STEP_IDLE;setmode(a,WM_PMODE_INAIR);anim(a,L.turn_punch,c);snd(a,"TURNDIVE",c);if(c&&c->jump_rope_audio)c->jump_rope_audio(a,c->user);return WM_LEX_STEP_ACTION;
}
static wm_arcade_lex_step_result_t mode_block(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_lex_env_t*e,const wm_arcade_lex_callbacks_t*c){
    a->block_time++;if(a->block_time>=160&&o&&o->player_mode==WM_PMODE_BLOCK&&a->closest_xdist<0x61&&a->closest_xdist>=45&&a->closest_zdist<30){setmode(a,WM_PMODE_NORMAL);anim(a,L.push,c);snd(a,"PUSH",c);return WM_LEX_STEP_ACTION;}
    if(!(a->but_val_down&WM_BTN_ATTACK_MASK))return WM_LEX_STEP_IDLE;
    if(!(a->but_val_cur&WM_BTN_BLOCK)){setmode(a,WM_PMODE_NORMAL);return WM_LEX_STEP_ACTION;}
    if((a->but_val_down&3)==1||(a->but_val_down&3)==3){setmode(a,WM_PMODE_NORMAL);anim(a,L.push,c);return WM_LEX_STEP_ACTION;}
    (void)e;
    return WM_LEX_STEP_IDLE;
}
static wm_arcade_lex_step_result_t mode_headhold(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_lex_env_t*e,const wm_arcade_lex_callbacks_t*c){
    /* "Bozo power move": `callr bozo_check / jrnc #fail`, and
       everything below this is #fail. Six of the eight
       dispatchers did not call it at all, so the move was
       missing rather than merely inert. */
    if(c&&c->bozo_check&&c->bozo_check(a,c->user)){
        snd(a,L.bozo_snd,c);
        anim(a,(e&&(e->pcnt&1))?L.bozo_b:L.bozo_a,c);
        return WM_LEX_STEP_ACTION;
    }
    uint8_t ac;if(!o||o->player_mode!=WM_PMODE_HEADHELD){a->z_fixed-=6<<16;setmode(a,WM_PMODE_NORMAL);return WM_LEX_STEP_ACTION;}if(a->anim_mode&WM_MODE_UNINT)return WM_LEX_STEP_IDLE;ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
    if(ac==A_PUNCH||ac==A_SPUNCH||ac==A_KICK||ac==A_PUNCHKICK){if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);}

    if(ac==A_PUNCH||ac==A_KICK||ac==A_PUNCHKICK){anim(a,L.close4,c);return WM_LEX_STEP_ACTION;}return WM_LEX_STEP_IDLE;
}

wm_arcade_lex_step_result_t wm_arcade_move_lex(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_lex_env_t*e,const wm_arcade_lex_callbacks_t*c){
    if(!a)return WM_LEX_STEP_IDLE;
    if(c&&c->check_secret_moves)c->check_secret_moves(a,secret_patterns,sizeof secret_patterns/sizeof secret_patterns[0],c->user);
    switch(a->player_mode){case WM_PMODE_NORMAL:case 18:case 22:case 23:return mode_normal(a,o,e,c);case WM_PMODE_RUNNING:return mode_running(a,o,e,c);case WM_PMODE_ATTACHED:if(c&&c->keep_attached)c->keep_attached(a,c->user);else(void)wm_arcade_keep_attached(a);if(!a->attach_proc){setmode(a,WM_PMODE_NORMAL);a->anim_mode=0;}return WM_LEX_STEP_EXTERNAL;case WM_PMODE_BOUNCING:return mode_bouncing(a,c);case WM_PMODE_ONTURNBKL:return mode_turn(a,c);case WM_PMODE_BLOCK:return mode_block(a,o,e,c);case WM_PMODE_DEAD:if(c&&c->mode_dead)c->mode_dead(a,c->user);return WM_LEX_STEP_EXTERNAL;case WM_PMODE_WAITANIM:if((a->anim_mode&WM_MODE_END)&&c&&c->code_addr)c->code_addr(a,(uint32_t)a->code_addr,c->user);return WM_LEX_STEP_EXTERNAL;case WM_PMODE_MASTER:if(c&&c->master_keep_attached)c->master_keep_attached(a,c->user);else(void)wm_arcade_master_keep_attached(a);return WM_LEX_STEP_EXTERNAL;case WM_PMODE_HEADHOLD:return mode_headhold(a,o,e,c);case WM_PMODE_HEADHELD:if((a->anim_mode&WM_MODE_NOGRAVITY)&&c&&c->mode_choking){c->mode_choking(a,c->user);return WM_LEX_STEP_EXTERNAL;}if(c&&c->bozo_check&&c->bozo_check(a,c->user)){if(c->do_reversal)c->do_reversal(a,c->user);if(c->do_reversal_message)c->do_reversal_message(a,c->user);snd(a,L.bozo_snd,c);anim(a,(e&&(e->pcnt&1))?L.bozo_hh_b:L.bozo_a,c);return WM_LEX_STEP_ACTION;}if(!a->attach_proc&&a->y_int<=a->ground_y)anim(a,L.headheld,c);return WM_LEX_STEP_EXTERNAL;case WM_PMODE_PUPPET:if(c&&c->mode_puppet)c->mode_puppet(a,c->user);return WM_LEX_STEP_EXTERNAL;case WM_PMODE_INAIR2:if(c&&c->mode_inair2)c->mode_inair2(a,c->user);return WM_LEX_STEP_EXTERNAL;case WM_PMODE_CHOKING:if(c&&c->mode_choking)c->mode_choking(a,c->user);return WM_LEX_STEP_EXTERNAL;default:return WM_LEX_STEP_IDLE;}
}

static int reject_common(wm_arcade_actor_t*a,wm_arcade_actor_t*o){return !a||!o||(a->anim_mode&WM_MODE_UNINT)||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_HEADHELD||o->player_mode==WM_PMODE_ATTACHED;}
int wm_arcade_lex_release_charge(wm_arcade_actor_t*a,wm_arcade_actor_t*o,uint16_t ticks,const wm_arcade_lex_callbacks_t*c){(void)o;if(!a||ticks<100)return 0;if(a->player_mode==WM_PMODE_HEADHELD||a->player_mode==WM_PMODE_HEADHOLD||(a->anim_mode&WM_MODE_UNINT))return 0;anim(a,face_label("lex_2_clobber_anim","lex_4_clobber_anim",a),c); snd(a,"HDBUTT",c); return 1;}
int wm_arcade_lex_fire_secret(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_lex_secret_id_t id,uint32_t pcnt,const wm_arcade_lex_callbacks_t*c){
    switch(id){
    case WM_LEX_SECRET_NECK_GRAB:
        if(reject_common(a,o)||groundish(o))return 0;
        if((uint32_t)(pcnt-a->last_headhold)<120) anim(a,"lex_3_fake_hold_anim",c);
        else if(a->closest_xdist<=80) anim(a,L.headhold2,c); else anim(a,L.headhold,c);
        return 1; case WM_LEX_SECRET_GRAB_FLING: if(reject_common(a,o))return 0; anim(a,face_label("lex_2_grabfling_anim","lex_4_grabfling_anim",a),c); snd(a,"GRABFLING",c); return 1; case WM_LEX_SECRET_GRAB_FLING2: if(reject_common(a,o))return 0; anim(a,face_label("lex_2_grabfling_anim","lex_4_grabfling_anim",a),c); snd(a,"GRABFLING",c); return 1; case WM_LEX_SECRET_HIP_TOSS: if(reject_common(a,o)||groundish(o))return 0; if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0; anim(a,"lex_hiptoss_anim",c); return 1; case WM_LEX_SECRET_HIP_TOSS2: if(reject_common(a,o)||groundish(o))return 0; if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0; anim(a,"lex_hiptoss_anim",c); return 1;
    case WM_LEX_SECRET_SLIDING_ELBOW: if(reject_common(a,o)||groundish(o))return 0; anim(a,"lex_sliding_elbow_anim",c); return 1;
    case WM_LEX_SECRET_HAMMER: if((a->anim_mode&WM_MODE_UNINT)||a->player_mode==WM_PMODE_OPPOVERHEAD)return 0; anim(a,"lex_hammer_anim",c); return 1;
    default:return 0;}
}
int wm_arcade_lex_fire_monitor(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_lex_monitor_id_t id,const wm_arcade_lex_env_t*e,int opponent_attack_is_leaping,const wm_arcade_lex_callbacks_t*c){
    wm_arcade_actor_t*t;const char*s;(void)e;(void)opponent_attack_is_leaping;if(!a||(unsigned)id>=sizeof special_processes/sizeof special_processes[0])return 0;s=special_processes[(unsigned)id];
    if(strstr(s,"hdhold_")!=NULL){if(a->player_mode==WM_PMODE_HEADHELD){t=a->who_hit_me?a->who_hit_me:o;if(!t)return 0;a->smart_target=t;t->immobilize_time=15;if(c&&c->do_reversal)c->do_reversal(a,c->user);if(c&&c->do_reversal_message)c->do_reversal_message(a,c->user);}else{t=a->who_i_hit?a->who_i_hit:o;if(a->player_mode!=WM_PMODE_HEADHOLD||!t)return 0;a->smart_target=t;t->immobilize_time=15;}if(c&&c->find_and_kill_endless)c->find_and_kill_endless(a,c->user);}
    if(strstr(s,"combo")!=NULL&&c&&c->check_combo_go&&c->check_combo_go(a,c->user)<0)return 0;
    startsp(a,s,c);
    return 1;
}
