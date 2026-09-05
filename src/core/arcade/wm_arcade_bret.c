#include "wm/arcade/wm_arcade_bret.h"
#include "wm/arcade/wm_arcade_damage.h"
#include "wm/arcade/wm_arcade_attach_anim.h"

#include <stddef.h>

/* Literal secret-move records from BRET.ASM. */
static const wm_arcade_bret_sequence_step_t seq_neck[] = {
    { WM_B_SPUNCH, WM_J_ALL }, { WM_J_TOWARD, WM_J_REAL_LR }, { WM_J_TOWARD, WM_J_REAL_LR }
};
static const wm_arcade_bret_sequence_step_t seq_grab_fling[] = {
    { WM_B_SPUNCH, WM_J_ALL }, { WM_J_AWAY, WM_J_REAL_LR }, { WM_J_AWAY, WM_J_REAL_LR }
};
static const wm_arcade_bret_sequence_step_t seq_hip_toss[] = {
    { WM_B_PUNCH, WM_J_ALL }, { WM_J_AWAY, WM_J_REAL_LR }, { WM_J_AWAY, WM_J_REAL_LR }
};
static const wm_arcade_bret_sequence_step_t seq_grab_fling2[] = {
    { (uint16_t)(WM_B_SPUNCH | WM_J_AWAY), (uint16_t)(WM_J_REAL_LR | WM_J_UP | WM_J_DOWN) }
};
static const wm_arcade_bret_sequence_step_t seq_hip_toss2[] = {
    { (uint16_t)(WM_B_PUNCH | WM_J_AWAY), (uint16_t)(WM_J_REAL_LR | WM_J_UP | WM_J_DOWN) }
};
static const wm_arcade_bret_sequence_step_t seq_face_rake[] = {
    { WM_B_PUNCH, WM_J_ALL }, { WM_J_TOWARD, WM_J_REAL_LR },
    { WM_J_DOWN_TOWARD, WM_J_REAL_LR }, { WM_J_DOWN, WM_J_REAL_LR }
};
static const wm_arcade_bret_sequence_step_t seq_jump_kick[] = {
    { WM_B_SKICK, WM_J_ALL },
    { WM_J_AWAY, (uint16_t)(WM_J_REAL_LR | WM_J_UP | WM_J_DOWN) },
    { WM_J_AWAY, (uint16_t)(WM_J_REAL_LR | WM_J_UP | WM_J_DOWN) }
};
static const wm_arcade_bret_sequence_step_t seq_supercut[] = {
    { WM_B_PUNCH, WM_J_ALL }, { WM_J_DOWN, WM_J_REAL_LR }, { WM_J_DOWN, WM_J_REAL_LR }
};
/* charge_ddt is executable probe code in the source, not a data sequence record. */
const wm_arcade_bret_secret_pattern_t wm_arcade_bret_secret_patterns[8] = {
    { WM_BRET_SECRET_NECK_GRAB, seq_neck, 3, 32 },
    { WM_BRET_SECRET_GRAB_FLING, seq_grab_fling, 3, 32 },
    { WM_BRET_SECRET_HIP_TOSS, seq_hip_toss, 3, 32 },
    { WM_BRET_SECRET_GRAB_FLING2, seq_grab_fling2, 1, 10 },
    { WM_BRET_SECRET_HIP_TOSS2, seq_hip_toss2, 1, 10 },
    { WM_BRET_SECRET_FACE_RAKE, seq_face_rake, 4, 30 },
    { WM_BRET_SECRET_JUMP_KICK, seq_jump_kick, 3, 32 },
    { WM_BRET_SECRET_SUPERCUT, seq_supercut, 3, 16 }
};


/* Persistent hrt_smove_table input monitors. TSEC is 60 ticks in this game. */
static const wm_arcade_bret_sequence_step_t mon_roll_uppercut[] = {
    { WM_J_DOWN, 0 }, { WM_J_TOWARD, (uint16_t)(WM_J_UP | WM_J_DOWN) },
    { WM_B_SPUNCH, WM_J_ALL }
};
static const wm_arcade_bret_sequence_step_t mon_combo1[] = {
    { WM_J_TOWARD, 0 }, { WM_J_TOWARD, 0 }, { WM_B_PUNCH, WM_J_ALL }
};
static const wm_arcade_bret_sequence_step_t mon_combo2[] = {
    { WM_J_TOWARD, 0 }, { WM_J_TOWARD, 0 },
    { WM_B_SKICK, (uint16_t)(WM_J_DOWN_TOWARD | WM_J_UP_TOWARD) }
};
static const wm_arcade_bret_sequence_step_t mon_pile[] = {
    { WM_J_TOWARD, 0 }, { WM_J_TOWARD, 0 }, { WM_B_SPUNCH, WM_J_ALL }
};
static const wm_arcade_bret_sequence_step_t mon_ddt[] = {
    { WM_J_DOWN, 0 }, { WM_J_DOWN, 0 }, { WM_B_SKICK, WM_J_ALL }
};
static const wm_arcade_bret_sequence_step_t mon_faceslam[] = {
    { WM_J_DOWN, 0 }, { WM_J_TOWARD, (uint16_t)(WM_J_DOWN | WM_J_UP) },
    { WM_B_PUNCH, WM_J_ALL }
};
static const wm_arcade_bret_sequence_step_t mon_grab_air[] = {
    { WM_J_AWAY, 0 }, { WM_J_AWAY, 0 }, { WM_B_PUNCH, WM_J_ALL }
};
static const wm_arcade_bret_sequence_step_t mon_finish1[] = {
    { WM_J_UP, 0 }, { WM_J_DOWN, 0 },
    { WM_J_TOWARD, (uint16_t)(WM_J_DOWN | WM_J_UP) },
    { WM_J_TOWARD, (uint16_t)(WM_J_DOWN | WM_J_UP) }, { WM_B_PUNCH, WM_J_ALL }
};
static const wm_arcade_bret_sequence_step_t mon_finish2[] = {
    { WM_J_UP, 0 }, { WM_J_UP, 0 }, { WM_J_RIGHT, WM_J_UP },
    { WM_J_RIGHT, WM_J_UP }, { WM_B_SPUNCH, WM_J_ALL }
};

const wm_arcade_bret_monitor_pattern_t wm_arcade_bret_monitor_patterns[9] = {
    { WM_BRET_MON_ROLL_UPPERCUT, mon_roll_uppercut, 3, 60 },
    { WM_BRET_MON_HEADHOLD_COMBO1, mon_combo1, 3, 60 },
    { WM_BRET_MON_HEADHOLD_COMBO2, mon_combo2, 3, 60 },
    { WM_BRET_MON_HEADHOLD_PILE, mon_pile, 3, 60 },
    { WM_BRET_MON_HEADHOLD_DDT, mon_ddt, 3, 60 },
    { WM_BRET_MON_HEADHOLD_FACESLAM, mon_faceslam, 3, 60 },
    { WM_BRET_MON_GRAB_TOSS_AIR, mon_grab_air, 3, 40 },
    { WM_BRET_MON_FINISH1, mon_finish1, 5, 60 },
    { WM_BRET_MON_FINISH2, mon_finish2, 5, 60 }
};

static void anim(wm_arcade_actor_t *a, wm_arcade_bret_anim_id_t id,
                 const wm_arcade_bret_callbacks_t *cb)
{ if (cb && cb->change_anim) cb->change_anim(a,id,cb->user); }
static void snd(wm_arcade_actor_t *a, wm_arcade_bret_sound_id_t id,
                const wm_arcade_bret_callbacks_t *cb)
{ if (cb && cb->sound) cb->sound(a,id,cb->user); }

static int setmode(wm_arcade_actor_t *a, uint16_t mode)
{
    if (!a || a->player_mode == WM_PMODE_DEAD) return 0;
    a->player_mode = mode;
    return 1;
}

static int face_is_2(const wm_arcade_actor_t *a) { return a && (a->facing_dir & WM_MOVE_UP); }
static wm_arcade_bret_anim_id_t f24(const wm_arcade_actor_t *a,
                                     wm_arcade_bret_anim_id_t id2,
                                     wm_arcade_bret_anim_id_t id4)
{ return face_is_2(a) ? id2 : id4; }
static int near(const wm_arcade_actor_t *a,int x,int z)
{ return a && a->closest_xdist <= x && a->closest_zdist <= z; }
static int opp_groundish(const wm_arcade_actor_t *o)
{ return o && (o->player_mode==WM_PMODE_ONGROUND || o->player_mode==WM_PMODE_DEAD); }
static int opp_mode_for_close(const wm_arcade_actor_t *o)
{
    if (!o) return WM_PMODE_NORMAL;
    return o->player_mode;
}

static const wm_arcade_bret_action_id_t action_table[32] = {
    WM_BRET_ACT_NONE,WM_BRET_ACT_PUNCH,WM_BRET_ACT_BLOCK,WM_BRET_ACT_BLOCK,
    WM_BRET_ACT_SUPER_PUNCH,WM_BRET_ACT_SUPER_PUNCH,WM_BRET_ACT_BLOCK,WM_BRET_ACT_BLOCK,
    WM_BRET_ACT_KICK,WM_BRET_ACT_PUNCHKICK,WM_BRET_ACT_BLOCK,WM_BRET_ACT_BLOCK,
    WM_BRET_ACT_SUPER_PUNCH,WM_BRET_ACT_PUNCHKICK,WM_BRET_ACT_BLOCK,WM_BRET_ACT_BLOCK,
    WM_BRET_ACT_SUPER_KICK,WM_BRET_ACT_SUPER_KICK,WM_BRET_ACT_BLOCK,WM_BRET_ACT_BLOCK,
    WM_BRET_ACT_GRABOH,WM_BRET_ACT_GRABOH,WM_BRET_ACT_BLOCK,WM_BRET_ACT_BLOCK,
    WM_BRET_ACT_SUPER_KICK,WM_BRET_ACT_PUNCHKICK,WM_BRET_ACT_BLOCK,WM_BRET_ACT_BLOCK,
    WM_BRET_ACT_GRABOH,WM_BRET_ACT_GRABOH,WM_BRET_ACT_BLOCK,WM_BRET_ACT_BLOCK
};

static int do_block(wm_arcade_actor_t *a,const wm_arcade_bret_env_t *e,const wm_arcade_bret_callbacks_t *cb)
{
    if (e && e->blocking_off) return 0;
    if (cb && cb->round_award_block) cb->round_award_block(a,cb->user);
    anim(a,WM_BRET_ANIM_BLOCK4,cb); snd(a,WM_BRET_SND_BLOCK_WOOSH,cb);
    a->block_time=0; return 1;
}

static void do_punch(wm_arcade_actor_t *a,wm_arcade_actor_t *o,const wm_arcade_bret_callbacks_t *cb)
{
    int m=opp_mode_for_close(o);
    if (m==WM_PMODE_CLIMBTURNBKL) {
        anim(a,f24(a,WM_BRET_ANIM_PUNCH2,WM_BRET_ANIM_PUNCH4),cb); snd(a,WM_BRET_SND_PUNCH,cb); return;
    }
    if (opp_groundish(o)) {
        if (near(a,160,140)) { anim(a,f24(a,WM_BRET_ANIM_GROUND_PUNCH2,WM_BRET_ANIM_GROUND_PUNCH4),cb); snd(a,WM_BRET_SND_LBOWDROP,cb); }
        else { anim(a,f24(a,WM_BRET_ANIM_PUNCH2,WM_BRET_ANIM_PUNCH4),cb); snd(a,WM_BRET_SND_PUNCH,cb); }
        return;
    }
    if (near(a,50,45)) { anim(a,f24(a,WM_BRET_ANIM_BUTT2,WM_BRET_ANIM_BUTT4),cb); snd(a,WM_BRET_SND_HDBUTT,cb); }
    else { anim(a,f24(a,WM_BRET_ANIM_PUNCH2,WM_BRET_ANIM_PUNCH4),cb); snd(a,WM_BRET_SND_PUNCH,cb); }
}

static void do_super_punch(wm_arcade_actor_t *a,wm_arcade_actor_t *o,const wm_arcade_bret_callbacks_t *cb)
{
    int m=opp_mode_for_close(o);
    if (m==WM_PMODE_HEADHELD) return;
    if (m==WM_PMODE_ONTURNBKL) {
        anim(a,f24(a,WM_BRET_ANIM_SUPER_PUNCH2_2,WM_BRET_ANIM_SUPER_PUNCH2_4),cb); snd(a,WM_BRET_SND_PUNCH,cb); return;
    }
    if (m==WM_PMODE_CLIMBTURNBKL) { do_punch(a,o,cb); return; }
    if (opp_groundish(o)) {
        if (!near(a,160,140)) { do_punch(a,o,cb); return; }
        if (o && o->player_mode!=WM_PMODE_DEAD) {
            int32_t dx=a->x_fixed-o->x_fixed; if(dx<0) dx=-dx; dx >>= 16;
            if (dx>=0x30) {
                if ((a->obj_control & WM_OBJ_FLIPH)!=(o->obj_control & WM_OBJ_FLIPH)) {
                    anim(a,f24(a,WM_BRET_ANIM_HAIR_PICKUP2,WM_BRET_ANIM_HAIR_PICKUP4),cb); snd(a,WM_BRET_SND_LBOWDROP,cb); return;
                }
                if (dx>=0x40) { anim(a,f24(a,WM_BRET_ANIM_SHOOTER2,WM_BRET_ANIM_SHOOTER4),cb); snd(a,WM_BRET_SND_LBOWDROP,cb); return; }
            }
        }
        anim(a,f24(a,WM_BRET_ANIM_GROUND_PUNCH2,WM_BRET_ANIM_GROUND_PUNCH4),cb); snd(a,WM_BRET_SND_LBOWDROP,cb); return;
    }
    if (near(a,70,45)) {
        if (a->stick_val_cur & WM_MOVE_DOWN) { anim(a,WM_BRET_ANIM_UPPERCUT4,cb); snd(a,WM_BRET_SND_UPRCUT,cb); return; }
        if (a->closest_xdist>55) { do_punch(a,o,cb); return; }
        anim(a,f24(a,WM_BRET_ANIM_BUTTS2,WM_BRET_ANIM_BUTTS4),cb); snd(a,WM_BRET_SND_HDBUTT,cb); return;
    }
    anim(a,f24(a,WM_BRET_ANIM_SUPER_PUNCH2_2,WM_BRET_ANIM_SUPER_PUNCH2_4),cb); snd(a,WM_BRET_SND_PUNCH,cb);
}

static void do_kick(wm_arcade_actor_t *a,wm_arcade_actor_t *o,const wm_arcade_bret_callbacks_t *cb)
{
    int m=opp_mode_for_close(o);
    if (m==WM_PMODE_INAIR2) { anim(a,WM_BRET_ANIM_KICK_TB,cb); snd(a,WM_BRET_SND_KICK,cb); return; }
    if (m==WM_PMODE_ONTURNBKL || m==WM_PMODE_CLIMBTURNBKL) {
        anim(a,f24(a,WM_BRET_ANIM_KICK2,WM_BRET_ANIM_KICK4),cb); snd(a,WM_BRET_SND_KICK,cb); return;
    }
    if (opp_groundish(o)) {
        if (near(a,160,140)) anim(a,f24(a,WM_BRET_ANIM_STOMP2,WM_BRET_ANIM_STOMP4),cb);
        else anim(a,f24(a,WM_BRET_ANIM_KICK2,WM_BRET_ANIM_KICK4),cb);
        snd(a,WM_BRET_SND_KICK,cb); return;
    }
    if (near(a,50,92)) anim(a,f24(a,WM_BRET_ANIM_KNEE2,WM_BRET_ANIM_KNEE4),cb);
    else anim(a,f24(a,WM_BRET_ANIM_KICK2,WM_BRET_ANIM_KICK4),cb);
    snd(a,WM_BRET_SND_KICK,cb);
}

static void do_super_kick(wm_arcade_actor_t *a,wm_arcade_actor_t *o,const wm_arcade_bret_callbacks_t *cb)
{
    int m=opp_mode_for_close(o);
    if (m==WM_PMODE_INAIR2) { do_kick(a,o,cb); return; }
    if (m==WM_PMODE_HEADHELD) { do_kick(a,o,cb); return; }
    if (opp_groundish(o)) { do_kick(a,o,cb); return; }
    if ((m==WM_PMODE_ONTURNBKL||m==WM_PMODE_CLIMBTURNBKL) && !near(a,60,96)) { do_kick(a,o,cb); return; }
    {
        int zclose = 60;
        if (m==WM_PMODE_WAITANIM || m==WM_PMODE_GRAPPLE || m==WM_PMODE_MASTER ||
            m==WM_PMODE_SLAVE || m==WM_PMODE_HEADHOLD || m==WM_PMODE_PUPPET2 ||
            m==WM_PMODE_PUPPET || m==WM_PMODE_CHOKEHOLD) zclose = 62;
        if (m!=WM_PMODE_ONTURNBKL && m!=WM_PMODE_CLIMBTURNBKL && !near(a,60,zclose)) {
            anim(a,f24(a,WM_BRET_ANIM_SUPER_KICK2,WM_BRET_ANIM_SUPER_KICK4),cb);
            snd(a,WM_BRET_SND_FLYKICK,cb);
            return;
        }
    }
    if (m==WM_PMODE_ONTURNBKL || m==WM_PMODE_CLIMBTURNBKL) {
        anim(a,f24(a,WM_BRET_ANIM_SUPER_KICK2,WM_BRET_ANIM_SUPER_KICK4),cb); snd(a,WM_BRET_SND_FLYKICK,cb); return;
    }
    if (a->stick_val_cur == (uint16_t)(a->new_facing_dir & 0x0c)) {
        anim(a,WM_BRET_ANIM_KNEE_FALL4,cb); snd(a,WM_BRET_SND_KICK,cb);
    } else {
        anim(a,f24(a,WM_BRET_ANIM_KNEE2,WM_BRET_ANIM_KNEE4),cb); snd(a,WM_BRET_SND_KICK,cb);
    }
}

static void normal_action(wm_arcade_actor_t *a,wm_arcade_actor_t *o,wm_arcade_bret_action_id_t ac,const wm_arcade_bret_env_t *e,const wm_arcade_bret_callbacks_t *cb)
{
    switch(ac){
    case WM_BRET_ACT_PUNCH: do_punch(a,o,cb); break;
    case WM_BRET_ACT_BLOCK: (void)do_block(a,e,cb); break;
    case WM_BRET_ACT_SUPER_PUNCH: do_super_punch(a,o,cb); break;
    case WM_BRET_ACT_KICK: do_kick(a,o,cb); break;
    case WM_BRET_ACT_PUNCHKICK: anim(a,WM_BRET_ANIM_START_RUN,cb); break;
    case WM_BRET_ACT_SUPER_KICK: do_super_kick(a,o,cb); break;
    case WM_BRET_ACT_GRABOH: do_super_kick(a,o,cb); break; /* source shares #graboh/#skick_kick */
    default: break;
    }
}

void wm_arcade_bret_ani_init(wm_arcade_actor_t *a,const wm_arcade_bret_callbacks_t *cb)
{
    if (!a) return;
    if (a->facing_dir & WM_MOVE_RIGHT) {
        anim(a,WM_BRET_ANIM_STAND2,cb);
        if(cb&&cb->change_torso_anim)cb->change_torso_anim(a,WM_BRET_ANIM_TORSO2,cb->user);
    } else {
        anim(a,WM_BRET_ANIM_STAND4,cb);
        if(cb&&cb->change_torso_anim)cb->change_torso_anim(a,WM_BRET_ANIM_TORSO4,cb->user);
        /* BRET.ASM creates the taunt process only on this branch; process creation stays native. */
    }
}

static wm_arcade_bret_step_result_t mode_normal(wm_arcade_actor_t *a,wm_arcade_actor_t *o,const wm_arcade_bret_env_t *e,const wm_arcade_bret_callbacks_t *cb)
{
    if (a->anim_mode & WM_MODE_UNINT) return WM_BRET_STEP_IDLE;
    if (a->i_will_die && !a->immobilize_time) {
        anim(a,WM_BRET_ANIM_FALL_BACK,cb); if(cb&&cb->adjust_health)cb->adjust_health(a,-10,cb->user);
        setmode(a,WM_PMODE_DEAD); a->i_will_die=0; return WM_BRET_STEP_ACTION;
    }
    if (o && o->player_mode==WM_PMODE_DEAD && !(o->status_flags & WM_STATUS_ZOMBIE)) {
        int reciprocal=a->attach_proc && a->attach_proc->attach_proc==a;
        if (!reciprocal) {
            if ((cb&&cb->teammate_pin&&cb->teammate_pin(a,cb->user)) || (cb&&cb->raisearm_check&&cb->raisearm_check(a,cb->user))) {
                anim(a,f24(a,WM_BRET_ANIM_RAISE_ARM2,WM_BRET_ANIM_RAISE_ARM4),cb);
                if(cb&&cb->set_raisearm_bit)cb->set_raisearm_bit(a,cb->user);
                if(cb&&cb->drone_change_back)cb->drone_change_back(a,cb->user);
                return WM_BRET_STEP_ACTION;
            }
            if (a->but_val_cur && cb&&cb->can_pin&&cb->can_pin(a,o,cb->user)) {
                anim(a,f24(a,WM_BRET_ANIM_PIN2,WM_BRET_ANIM_PIN4),cb); a->status_flags|=WM_STATUS_DID_PIN;
                if (cb->drone_change_back) cb->drone_change_back(a,cb->user);
                return WM_BRET_STEP_ACTION;
            }
        }
    }
    if (a->immobilize_time) { a->move_dir=0; if(cb&&cb->execute_walk)cb->execute_walk(a,cb->user); return WM_BRET_STEP_EXTERNAL; }
    if (a->but_val_cur & WM_BTN_BLOCK) {
        if (do_block(a,e,cb)) { a->attack_type=0; return WM_BRET_STEP_ACTION; }
    }
    if ((a->but_val_cur & WM_BTN_ATTACK_MASK)==(WM_BTN_PUNCH|WM_BTN_KICK)) {
        normal_action(a,o,WM_BRET_ACT_PUNCHKICK,e,cb); return WM_BRET_STEP_ACTION;
    }
    normal_action(a,o,action_table[a->but_val_down & WM_BTN_ATTACK_MASK],e,cb);
    if (a->anim_mode & WM_MODE_UNINT) return WM_BRET_STEP_ACTION;
    a->move_dir=a->stick_val_cur;
    if (cb&&cb->climb_turnbuckle&&cb->climb_turnbuckle(a,cb->user)) { if(cb->jump_rope_audio)cb->jump_rope_audio(a,cb->user); return WM_BRET_STEP_EXTERNAL; }
    if(cb&&cb->execute_walk)cb->execute_walk(a,cb->user);
    return WM_BRET_STEP_ACTION;
}

static wm_arcade_bret_step_result_t mode_running(wm_arcade_actor_t *a,wm_arcade_actor_t *o,const wm_arcade_bret_env_t *e,const wm_arcade_bret_callbacks_t *cb)
{
    a->run_time++;
    if (!a->usr_var1) {
        int32_t v=a->walk_fast?WM_BRET_XRUN2:WM_BRET_XRUN;
        if(cb&&cb->bounce_off_ropes)cb->bounce_off_ropes(a,cb->user);
        if(e && e->hyper_speed_on>0 && e->hyper_speed_on<15) v <<= e->hyper_speed_on;
        if (!(a->move_dir & WM_MOVE_RIGHT)) v=-v;
        a->x_vel=v;
    }
    if (a->getup_time) return WM_BRET_STEP_IDLE;
    if (!(a->anim_mode & WM_MODE_UNINT)) {
        if (((a->stick_val_cur | a->move_dir)&(WM_MOVE_LEFT|WM_MOVE_RIGHT))==(WM_MOVE_LEFT|WM_MOVE_RIGHT)) setmode(a,WM_PMODE_NORMAL);
    }
    if (a->stick_val_cur & WM_MOVE_UP) a->z_vel=-WM_BRET_ZDRIFT;
    else if (a->stick_val_cur & WM_MOVE_DOWN) a->z_vel=WM_BRET_ZDRIFT;
    else a->z_vel=0;
    if (a->delay_butns) return WM_BRET_STEP_IDLE;
    wm_arcade_bret_action_id_t ac=action_table[a->but_val_down & WM_BTN_ATTACK_MASK];
    if (ac==WM_BRET_ACT_BLOCK) { a->x_vel >>= 1; setmode(a,WM_PMODE_NORMAL); (void)do_block(a,e,cb); return WM_BRET_STEP_ACTION; }
    if (ac==WM_BRET_ACT_KICK || ac==WM_BRET_ACT_SUPER_KICK) {
        if (cb&&cb->ck_ignore&&cb->ck_ignore(a,cb->user)) return WM_BRET_STEP_IDLE;
        anim(a,WM_BRET_ANIM_FLYING_KICK,cb); setmode(a,WM_PMODE_INAIR); snd(a,WM_BRET_SND_FLYKICK,cb); return WM_BRET_STEP_ACTION;
    }
    if (ac==WM_BRET_ACT_PUNCH||ac==WM_BRET_ACT_SUPER_PUNCH||ac==WM_BRET_ACT_PUNCHKICK||ac==WM_BRET_ACT_GRABOH) {
        if ((a->facing_dir & a->new_facing_dir & (WM_MOVE_LEFT|WM_MOVE_RIGHT))==0) return WM_BRET_STEP_IDLE;
        if (opp_groundish(o)) anim(a,WM_BRET_ANIM_RUNNING_GROUND_PUNCH,cb); else anim(a,WM_BRET_ANIM_RUNNING_DDT,cb);
        snd(a,WM_BRET_SND_FLYKICK,cb); return WM_BRET_STEP_ACTION;
    }
    return WM_BRET_STEP_IDLE;
}

static wm_arcade_bret_step_result_t mode_bouncing(wm_arcade_actor_t*a,const wm_arcade_bret_callbacks_t*cb)
{
    a->x_vel=0;a->z_vel=0;
    if(a->anim_mode&WM_MODE_END){a->move_dir^=(WM_MOVE_LEFT+WM_MOVE_RIGHT);a->facing_dir=(a->new_facing_dir&(WM_MOVE_UP+WM_MOVE_DOWN))|a->move_dir;anim(a,WM_BRET_ANIM_RUN2,cb);setmode(a,WM_PMODE_RUNNING);return WM_BRET_STEP_ACTION;}
    return WM_BRET_STEP_IDLE;
}
static wm_arcade_bret_step_result_t mode_turnbuckle(wm_arcade_actor_t*a,const wm_arcade_bret_callbacks_t*cb)
{
    if(a->stick_val_cur&WM_MOVE_DOWN){anim(a,WM_BRET_ANIM_CLIMB_DOWN,cb);setmode(a,WM_PMODE_CLIMBTURNBKL);return WM_BRET_STEP_ACTION;}
    if((a->but_val_down&WM_BTN_ATTACK_MASK)==0)return WM_BRET_STEP_IDLE;
    wm_arcade_bret_action_id_t ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
    if(ac==WM_BRET_ACT_NONE)return WM_BRET_STEP_IDLE;
    setmode(a,WM_PMODE_INAIR);snd(a,WM_BRET_SND_TURNDIVE,cb);anim(a,WM_BRET_ANIM_TBUKL_LEAP,cb);if(cb&&cb->jump_rope_audio)cb->jump_rope_audio(a,cb->user);return WM_BRET_STEP_ACTION;
}
static wm_arcade_bret_step_result_t mode_block(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bret_env_t*e,const wm_arcade_bret_callbacks_t*cb)
{
    a->block_time++;
    if(a->block_time>=160 && o && o->player_mode==WM_PMODE_BLOCK && a->closest_xdist<0x61 && a->closest_xdist>=45 && a->closest_zdist<30){setmode(a,WM_PMODE_NORMAL);anim(a,WM_BRET_ANIM_PUSH4,cb);snd(a,WM_BRET_SND_PUSH,cb);return WM_BRET_STEP_ACTION;}
    uint16_t down=a->but_val_down&WM_BTN_ATTACK_MASK;if(!down)return WM_BRET_STEP_IDLE;
    if(!(a->but_val_cur&WM_BTN_BLOCK)){setmode(a,WM_PMODE_NORMAL);return mode_normal(a,o,e,cb);}
    if(down==1||down==3){setmode(a,WM_PMODE_NORMAL);anim(a,WM_BRET_ANIM_PUSH4,cb);snd(a,WM_BRET_SND_PUSH,cb);return WM_BRET_STEP_ACTION;}
    return WM_BRET_STEP_IDLE;
}
static wm_arcade_bret_step_result_t mode_headhold(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bret_env_t*e,const wm_arcade_bret_callbacks_t*cb)
{
    if(cb&&cb->bozo_check&&cb->bozo_check(a,cb->user)){snd(a,WM_BRET_SND_GRABFLING,cb);anim(a,(e&&((e->pcnt)&1))?WM_BRET_ANIM_HH_DDT2:WM_BRET_ANIM_PILE_DRIVER3,cb);return WM_BRET_STEP_ACTION;}
    if(!o||o->player_mode!=WM_PMODE_HEADHELD){a->z_fixed-=6<<16;a->facing_dir=a->new_facing_dir=(a->obj_control&WM_OBJ_FLIPH)?WM_MOVE_DOWN_LEFT:WM_MOVE_DOWN_RIGHT;setmode(a,WM_PMODE_NORMAL);return WM_BRET_STEP_ACTION;}
    if(a->anim_mode&WM_MODE_UNINT)return WM_BRET_STEP_IDLE;
    wm_arcade_bret_action_id_t ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
    if(ac==WM_BRET_ACT_PUNCH||ac==WM_BRET_ACT_KICK||ac==WM_BRET_ACT_PUNCHKICK||ac==WM_BRET_ACT_SUPER_PUNCH){if(cb&&cb->find_and_kill_endless)cb->find_and_kill_endless(a,cb->user);}
    if(ac==WM_BRET_ACT_PUNCH){if(a->stick_val_cur==(uint16_t)(a->new_facing_dir&0x0c)){anim(a,WM_BRET_ANIM_UPPERCUTS_TO_HEAD,cb);snd(a,WM_BRET_SND_UPRCUT,cb);}else{anim(a,WM_BRET_ANIM_KNEE_TO_HEAD4,cb);snd(a,WM_BRET_SND_KICK,cb);}return WM_BRET_STEP_ACTION;}
    if(ac==WM_BRET_ACT_SUPER_PUNCH){if(a->stick_val_cur&WM_MOVE_DOWN){a->special_damage_time=(e?e->pcnt:0)+15;a->next_damage=WM_D_UPRCUT/2;anim(a,WM_BRET_ANIM_UPPERCUT4,cb);snd(a,WM_BRET_SND_UPRCUT,cb);return WM_BRET_STEP_ACTION;}return WM_BRET_STEP_IDLE;}
    if(ac==WM_BRET_ACT_KICK){if(a->stick_val_cur==(uint16_t)(a->new_facing_dir&0x0c)){anim(a,WM_BRET_ANIM_KNEES_TO_HEAD,cb);snd(a,WM_BRET_SND_UPRCUT,cb);}else{anim(a,WM_BRET_ANIM_KNEE_TO_HEAD4,cb);snd(a,WM_BRET_SND_KICK,cb);}return WM_BRET_STEP_ACTION;}
    if(ac==WM_BRET_ACT_PUNCHKICK){anim(a,WM_BRET_ANIM_KNEE_TO_HEAD4,cb);snd(a,WM_BRET_SND_KICK,cb);return WM_BRET_STEP_ACTION;}
    return WM_BRET_STEP_IDLE;
}
static wm_arcade_bret_step_result_t mode_headheld(wm_arcade_actor_t*a,const wm_arcade_bret_env_t*e,const wm_arcade_bret_callbacks_t*cb)
{
    if(a->anim_mode&WM_MODE_NOGRAVITY){if(cb&&cb->mode_choking)cb->mode_choking(a,cb->user);return WM_BRET_STEP_EXTERNAL;}
    if(cb&&cb->bozo_check&&cb->bozo_check(a,cb->user)){if(cb->do_reversal)cb->do_reversal(a,cb->user);if(cb->do_reversal_message)cb->do_reversal_message(a,cb->user);snd(a,WM_BRET_SND_GRABFLING,cb);anim(a,(e&&((e->pcnt)&1))?WM_BRET_ANIM_HH_DDT2:WM_BRET_ANIM_PILE_DRIVER3,cb);return WM_BRET_STEP_ACTION;}
    if(!a->attach_proc && a->y_int<=a->ground_y){anim(a,WM_BRET_ANIM_HEAD_HELD_STAND3,cb);return WM_BRET_STEP_ACTION;}return WM_BRET_STEP_IDLE;
}

wm_arcade_bret_step_result_t wm_arcade_move_bret(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_bret_env_t*e,const wm_arcade_bret_callbacks_t*cb)
{
    if(!a)return WM_BRET_STEP_IDLE;
    if(cb&&cb->check_secret_moves)cb->check_secret_moves(a,wm_arcade_bret_secret_patterns,8,cb->user);
    switch(a->player_mode){
    case WM_PMODE_NORMAL: case 18: case 22: case 23:return mode_normal(a,o,e,cb);
    case WM_PMODE_RUNNING:return mode_running(a,o,e,cb);
    case WM_PMODE_INAIR: case WM_PMODE_ONGROUND: case WM_PMODE_DIZZY: case WM_PMODE_OPPOVERHEAD: case WM_PMODE_CLIMBTURNBKL: case WM_PMODE_GRAPPLE: case WM_PMODE_SLAVE: case WM_PMODE_PUPPET2: case WM_PMODE_CHOKEHOLD:return WM_BRET_STEP_IDLE;
    case WM_PMODE_ATTACHED:if(cb&&cb->keep_attached)cb->keep_attached(a,cb->user);else (void)wm_arcade_keep_attached(a);if(!a->attach_proc){setmode(a,WM_PMODE_NORMAL);a->anim_mode=0;}return WM_BRET_STEP_EXTERNAL;
    case WM_PMODE_BOUNCING:return mode_bouncing(a,cb);
    case WM_PMODE_ONTURNBKL:return mode_turnbuckle(a,cb);
    case WM_PMODE_BLOCK:return mode_block(a,o,e,cb);
    case WM_PMODE_DEAD:if(cb&&cb->mode_dead)cb->mode_dead(a,cb->user);return WM_BRET_STEP_EXTERNAL;
    case WM_PMODE_WAITANIM:if((a->anim_mode&WM_MODE_END)&&cb&&cb->code_addr)cb->code_addr(a,a->code_addr,cb->user);return WM_BRET_STEP_EXTERNAL;
    case WM_PMODE_MASTER:if(cb&&cb->master_keep_attached)cb->master_keep_attached(a,cb->user);else (void)wm_arcade_master_keep_attached(a);return WM_BRET_STEP_EXTERNAL;
    case WM_PMODE_HEADHOLD:return mode_headhold(a,o,e,cb);
    case WM_PMODE_HEADHELD:return mode_headheld(a,e,cb);
    case WM_PMODE_PUPPET:if(cb&&cb->mode_puppet)cb->mode_puppet(a,cb->user);return WM_BRET_STEP_EXTERNAL;
    case WM_PMODE_INAIR2:if(cb&&cb->mode_inair2)cb->mode_inair2(a,cb->user);return WM_BRET_STEP_EXTERNAL;
    case WM_PMODE_CHOKING:if(cb&&cb->mode_choking)cb->mode_choking(a,cb->user);return WM_BRET_STEP_EXTERNAL;
    default:return WM_BRET_STEP_IDLE;
    }
}


int wm_arcade_bret_try_charge_ddt(wm_arcade_actor_t*a,wm_arcade_actor_t*o,uint16_t powerp_dtime,const wm_arcade_bret_callbacks_t*cb)
{
    if(!a || !(a->but_val_up & WM_BTN_SPUNCH) || powerp_dtime < 100) return 0;
    return wm_arcade_bret_fire_secret(a,o,WM_BRET_SECRET_CHARGE_DDT,0,cb);
}

int wm_arcade_bret_fire_secret(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_bret_secret_id_t id,uint32_t pcnt,const wm_arcade_bret_callbacks_t*cb)
{
    if(!a)return 0;
    switch(id){
    case WM_BRET_SECRET_SUPERCUT:
        /* BRET.ASM #scrt_cut: "movi hrt_4_super_punch_anim,a0" -- the exact
           same real label WM_BRET_ANIM_SUPER_PUNCH2_4 already maps to
           (wm/bret_backend.h's own comment), not the separate, permanently-
           unmapped WM_BRET_ANIM_SUPER_PUNCH4 id this used to dispatch to.
           Reusing the already-wired id also means this now gets real frame
           data, its existing attack window, and real WM_MODE_UNINT/
           ANI_SETFACING protection for free. */
        if((a->anim_mode&WM_MODE_UNINT)||!o||o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_HEADHELD||o->player_mode==WM_PMODE_ATTACHED)return 0;
        anim(a,WM_BRET_ANIM_SUPER_PUNCH2_4,cb);snd(a,WM_BRET_SND_PUNCH,cb);return 1;
    case WM_BRET_SECRET_JUMP_KICK:
        if((a->anim_mode&WM_MODE_UNINT)||!o||o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_HEADHELD||o->player_mode==WM_PMODE_ATTACHED)return 0;
        anim(a,WM_BRET_ANIM_JUMP_KICK4,cb);snd(a,WM_BRET_SND_FLYKICK,cb);return 1;
    case WM_BRET_SECRET_CHARGE_DDT:
        if(a->player_mode==WM_PMODE_HEADHELD||a->player_mode==WM_PMODE_HEADHOLD||(a->anim_mode&WM_MODE_UNINT))return 0;
        if(a->player_mode==WM_PMODE_RUNNING||a->stick_val_cur==(uint16_t)(a->new_facing_dir&0x0c))anim(a,WM_BRET_ANIM_RUNNING_DDT,cb);else anim(a,WM_BRET_ANIM_HH_DDT2,cb);snd(a,WM_BRET_SND_FLYKICK,cb);return 1;
    case WM_BRET_SECRET_NECK_GRAB:
        if((a->anim_mode&WM_MODE_UNINT)||!o||o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_HEADHELD)return 0;
        if((uint32_t)(pcnt-a->last_headhold)<120)anim(a,WM_BRET_ANIM_FAKE_HOLD3,cb);else if(a->closest_xdist<=90)anim(a,WM_BRET_ANIM_HEAD_HOLD2_3,cb);else anim(a,WM_BRET_ANIM_HEAD_HOLD3,cb);return 1;
    case WM_BRET_SECRET_GRAB_FLING: case WM_BRET_SECRET_GRAB_FLING2:
        if((a->anim_mode&WM_MODE_UNINT)||!o)return 0;
        if(o->player_mode==WM_PMODE_BOUNCING||o->player_mode==WM_PMODE_RUNNING){anim(a,WM_BRET_ANIM_HIPTOSS,cb);snd(a,WM_BRET_SND_GRABFLING,cb);return 1;}
        if(id==WM_BRET_SECRET_GRAB_FLING2)return 0;
        if(o->player_mode==WM_PMODE_HEADHELD||o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD)return 0;
        anim(a,WM_BRET_ANIM_GRABFLING_FACE24,cb);snd(a,WM_BRET_SND_GRABFLING,cb);return 1;
    case WM_BRET_SECRET_HIP_TOSS: case WM_BRET_SECRET_HIP_TOSS2:
        if((a->anim_mode&WM_MODE_UNINT)||!o)return 0;
        if(id==WM_BRET_SECRET_HIP_TOSS2 && !(o->player_mode==WM_PMODE_BOUNCING||o->player_mode==WM_PMODE_RUNNING))return 0;
        if(o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_HEADHELD)return 0;
        if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0;
        anim(a,WM_BRET_ANIM_HIPTOSS,cb);snd(a,WM_BRET_SND_HIPTOSS,cb);return 1;
    case WM_BRET_SECRET_FACE_RAKE:
        if((a->anim_mode&WM_MODE_UNINT)||a->player_mode==WM_PMODE_ONTURNBKL||!o||o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_HEADHELD)return 0;
        anim(a,WM_BRET_ANIM_RAKE_FACE,cb);snd(a,WM_BRET_SND_UPRCUT,cb);return 1;
    default:return 0;
    }
}

static int headhold_move_common(wm_arcade_actor_t *a, wm_arcade_actor_t *o,
                                wm_arcade_bret_anim_id_t move, int bonus,
                                int play_grab_sound,
                                const wm_arcade_bret_callbacks_t *cb)
{
    wm_arcade_actor_t *target;
    if (!a || !o) return 0;
    if (a->player_mode != WM_PMODE_HEADHOLD && a->player_mode != WM_PMODE_HEADHELD) return 0;
    if (a->immobilize_time) return 0;
    if (a->player_mode == WM_PMODE_HEADHELD) {
        if (a->i_will_die) return 0;
        if (cb && cb->do_reversal) cb->do_reversal(a, cb->user);
        if (cb && cb->do_reversal_message) cb->do_reversal_message(a, cb->user);
        a->smart_target = a->who_hit_me;
        target = a->who_hit_me;
    } else {
        if (bonus && cb && cb->bonus_message) cb->bonus_message(a, bonus, cb->user);
        a->smart_target = a->who_i_hit;
        target = a->who_i_hit;
    }
    if (!target) target = o;
    target->immobilize_time = 15;
    if (cb && cb->find_and_kill_endless) cb->find_and_kill_endless(a, cb->user);
    if (play_grab_sound) snd(a, WM_BRET_SND_GRABFLING, cb);
    a->special_move_addr = (uintptr_t)move;
    return 1;
}

int wm_arcade_bret_fire_monitor(wm_arcade_actor_t *a, wm_arcade_actor_t *o,
                                wm_arcade_bret_monitor_id_t id,
                                const wm_arcade_bret_env_t *env,
                                int opponent_attack_is_leaping,
                                const wm_arcade_bret_callbacks_t *cb)
{
    if (!a) return 0;
    switch (id) {
    case WM_BRET_MON_ROLL_UPPERCUT:
        if ((a->anim_mode & WM_MODE_UNINT) || a->immobilize_time ||
            a->player_mode == WM_PMODE_ONTURNBKL ||
            a->player_mode == WM_PMODE_HEADHOLD || a->player_mode == WM_PMODE_HEADHELD) return 0;
        snd(a, WM_BRET_SND_GRABFLING, cb);
        a->special_move_addr = (uintptr_t)WM_BRET_ANIM_ROLL_UPPERCUT;
        a->run_time = 0;
        return 1;
    case WM_BRET_MON_HEADHOLD_COMBO1:
    case WM_BRET_MON_HEADHOLD_COMBO2:
        if (a->player_mode != WM_PMODE_HEADHOLD || a->immobilize_time) return 0;
        if (!cb || !cb->check_combo_go || cb->check_combo_go(a, cb->user) < 0) return 0;
        a->smart_target = a->who_i_hit;
        if (cb && cb->find_and_kill_endless) cb->find_and_kill_endless(a, cb->user);
        a->special_move_addr = (uintptr_t)(id == WM_BRET_MON_HEADHOLD_COMBO1 ?
            WM_BRET_ANIM_COMBO_PUNCH : WM_BRET_ANIM_COMBO_KICK);
        return 1;
    case WM_BRET_MON_HEADHOLD_PILE:
        return headhold_move_common(a,o,WM_BRET_ANIM_PILE_DRIVER3,35,1,cb);
    case WM_BRET_MON_HEADHOLD_DDT:
        return headhold_move_common(a,o,WM_BRET_ANIM_HH_DDT2,16,0,cb);
    case WM_BRET_MON_HEADHOLD_FACESLAM:
        return headhold_move_common(a,o,WM_BRET_ANIM_FACE_DRIVER2_3,20,0,cb);
    case WM_BRET_MON_GRAB_TOSS_AIR:
        if (!o || (a->anim_mode & WM_MODE_UNINT) || a->player_mode == WM_PMODE_HEADHOLD ||
            o->player_mode == WM_PMODE_ONGROUND || o->player_mode == WM_PMODE_DEAD) return 0;
        if (o->player_mode == WM_PMODE_INAIR || o->player_mode == WM_PMODE_INAIR2 ||
            opponent_attack_is_leaping) {
            a->special_move_addr = (uintptr_t)WM_BRET_ANIM_HIPTOSS2;
        } else {
            if (a->closest_dist > 0x70) return 0;
            a->special_move_addr = (uintptr_t)WM_BRET_ANIM_HIPTOSS;
        }
        snd(a, WM_BRET_SND_HIPTOSS, cb);
        return 1;
    case WM_BRET_MON_FINISH1:
    case WM_BRET_MON_FINISH2:
        if (!env || ((env->p1rounds | env->p2rounds) < 2)) return 0;
        a->special_move_addr = (uintptr_t)(id == WM_BRET_MON_FINISH1 ?
            WM_BRET_ANIM_FINISH1 : WM_BRET_ANIM_FINISH2);
        return 1;
    default:
        return 0;
    }
}

int wm_arcade_bret_release_charge_flying_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,uint16_t charge,const wm_arcade_bret_callbacks_t*cb)
{
    if(!a||charge<100||a->getup_time||a->player_mode==WM_PMODE_HEADHELD||a->player_mode==WM_PMODE_HEADHOLD||a->player_mode==WM_PMODE_ONGROUND||a->player_mode==WM_PMODE_DEAD||(a->anim_mode&WM_MODE_UNINT)||!o||o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD)return 0;
    if(cb&&cb->ck_ignore&&cb->ck_ignore(a,cb->user))return 0;
    a->special_move_addr=(uintptr_t)WM_BRET_ANIM_FLYING_KICK;setmode(a,WM_PMODE_INAIR);snd(a,WM_BRET_SND_FLYKICK,cb);return 1;
}
int wm_arcade_bret_release_charge_face_rake(wm_arcade_actor_t*a,uint16_t charge,const wm_arcade_bret_callbacks_t*cb)
{
    if(!a||charge<100||a->getup_time||a->player_mode==WM_PMODE_HEADHELD||a->player_mode==WM_PMODE_HEADHOLD||a->player_mode==WM_PMODE_ONGROUND||a->player_mode==WM_PMODE_DEAD||(a->anim_mode&WM_MODE_UNINT))return 0;
    a->special_move_addr=(uintptr_t)WM_BRET_ANIM_RAKE_FACE;snd(a,WM_BRET_SND_UPRCUT,cb);return 1;
}

void wm_arcade_bret_velocity_for_dir(unsigned d,int32_t*x,int32_t*z)
{
    static const int32_t t[8][2]={{0,-WM_BRET_WALK_VEL},{WM_BRET_WALK_DVEL,-WM_BRET_WALK_DVEL},{WM_BRET_WALK_VEL,0},{WM_BRET_WALK_DVEL,WM_BRET_WALK_DVEL},{0,WM_BRET_WALK_VEL},{-WM_BRET_WALK_DVEL,WM_BRET_WALK_DVEL},{-WM_BRET_WALK_VEL,0},{-WM_BRET_WALK_DVEL,-WM_BRET_WALK_DVEL}};
    d&=7;if(x)*x=t[d][0];if(z)*z=t[d][1];
}
