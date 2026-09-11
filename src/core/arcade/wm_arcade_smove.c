/*
 * WRESTLE2.ASM:4058 init_smoves and the special-move watchdogs it
 * creates -- see wm/arcade/wm_arcade_smove.h.
 */
#include "wm/arcade/wm_arcade_smove.h"

#include <stddef.h>
#include <string.h>
#ifdef WM_SMOVE_DEBUG
#include <stdio.h>
#endif

#include "wm/arcade/wm_arcade_damage.h"
#include "wm/wrestler_anim_tables.h"

/* ------------------------------------------------------------------ */
/* MACROS.H:652 WAITSWITCH_DWN                                        */

uint16_t wm_smove_switch_word(uint16_t but_val_down, uint16_t stick_rel_new) {
    /* `move *a8(BUT_VAL_DOWN),a0 / sll 4,a0 / move *a8(STICK_REL_NEW),a1
       / or a1,a0`. GAME.EQU's B_* constants carry the same shift, so
       the comparison is against this word directly. */
    return (uint16_t)(((uint16_t)(but_val_down << 4)) | stick_rel_new);
}

wm_smove_tick_t wm_smove_waitswitch(int32_t *countdown,
                                    uintptr_t special_move_addr,
                                    uint16_t but_val_down,
                                    uint16_t stick_rel_new,
                                    uint16_t switches,
                                    uint16_t mask) {
    uint16_t word;

    /* `dec a11 / jrz FAILADDR` -- decremented BEFORE the test, so a
       countdown of 0 wraps to -1 and never reaches zero. That is how
       every monitor's opening input gets no deadline. */
    if (countdown) {
        *countdown -= 1;
        if (*countdown == 0) return WM_SMOVE_FAIL;
    }

    /* "already doing a special move" -- he cannot be starting one. */
    if (special_move_addr != 0) return WM_SMOVE_FAIL;

    word = wm_smove_switch_word(but_val_down, stick_rel_new);
    word = (uint16_t)(word & (uint16_t)~mask);   /* `andni MASK,a0` */
    if (word == 0) return WM_SMOVE_WAIT;         /* `jrz lp?` */

    /* `cmpi SWITCHES,a0 / jrne FAILADDR` -- the wrong input RESTARTS
       the sequence, unlike AWARD.ASM's PUPWAITSWITCH, which ignores
       it. The source's own note is beside this line: "subk doesn't
       work -- some out-of-range values in doink." */
    return (word == switches) ? WM_SMOVE_MATCH : WM_SMOVE_FAIL;
}

/* ------------------------------------------------------------------ */
/* The three monitors this port can complete                          */

/*
 * TAKER.ASM:608 und_finish_move1. UP, then DOWN, then PUNCH, and the
 * seven guards behind it are wm_arcade_und_finish_allowed's.
 */
static bool und_finish_fire(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                            wm_smove_fire_t *out) {
    wm_arcade_und_finish_env_t fe;
    const char *anim;

    if (!a || !env || !out) return false;

    memset(&fe, 0, sizeof fe);
    fe.my_pins = env->my_pins;
    fe.player_type = a->plyr_type;
    fe.ring_time = env->ring_time;
    fe.world_tlx = env->world_tlx;
    fe.world_tly = env->world_tly;

    anim = wm_arcade_und_finish_move1(a, env->victim, &fe, env->und_cb);
#ifdef WM_SMOVE_DEBUG
    fprintf(stderr, "guards: vmode=%u pins=%d ptype=%d rt=%d didpin=%d "
            "tx=%d vx=%d max=%d\n", env->victim?env->victim->player_mode:999,
            (int)env->my_pins, (int)a->plyr_type, (int)env->ring_time,
            (int)((a->status_flags & WM_STATUS_DID_PIN)!=0),
            (int)a->x_int, env->victim?(int)env->victim->x_int:-1,
            1174);
#endif
    if (!anim) return false;

    out->anim = anim;
    out->in_finish_move = true;
    return true;
}

/*
 * DOINK.ASM:1092 std_walk_fast. "One time per match", says the source
 * -- the gate is WALK_FAST already being zero, so once it is set the
 * monitor can never complete again even though it keeps running.
 */
static bool walk_fast_gate(const wm_arcade_actor_t *a,
                           const wm_smove_env_t *env) {
    (void)env;
    if (!a) return false;
    if (a->walk_fast != 0) return false;
    return a->player_mode == WM_PMODE_NORMAL;
}

static bool walk_fast_fire(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                           wm_smove_fire_t *out) {
    (void)env;
    if (!a || !out) return false;
    /* The mode is tested a second time after the rotation completes:
       `move *a8(PLYRMODE),a0 / cmpi MODE_NORMAL,a0 / jrnz #lp0`. */
    if (a->player_mode != WM_PMODE_NORMAL) return false;
    out->walk_fast = WM_SMOVE_WALK_FAST_TIME;
    /*
     * What is NOT here, and is not a simplification but a deliberate
     * stop: the rest of std_walk_fast is presentation. Doink alone
     * gets a `movi 30000h,a0 / move a0,*a8(OBJ_YVEL)` hop; then
     * ADD_VOICE (211h for Doink, 17eh for everyone else), then three
     * OBJ_CONST/M_CONNON flashes three ticks apart, then Doink's three
     * honks, then a wait on WALK_FAST running out and two more
     * flashes. All of it is object control and audio, which this port
     * routes through its own seams rather than from inside a monitor.
     */
    return true;
}

/*
 * DOINK.ASM:1216 std_taunt. The same eight-way rotation, starting at
 * UP instead of AWAY and with BLOCK held throughout -- the mask is
 * B_BLOCK, which keeps the held button from failing each step.
 */
static bool taunt_gate(const wm_arcade_actor_t *a,
                       const wm_smove_env_t *env) {
    (void)env;
    return a && a->player_mode == WM_PMODE_BLOCK;
}

/* "move *a8(BUT_VAL_CUR),a14 / btst PLAYER_BLOCK_BIT,a14 / jrz #lp0",
   between the opening UP and the timeout. */
static bool taunt_mid_gate(const wm_arcade_actor_t *a,
                           const wm_smove_env_t *env) {
    (void)env;
    return a && (a->but_val_cur & WM_BTN_BLOCK) != 0;
}

static bool taunt_fire(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                       wm_smove_fire_t *out) {
    const char *anim;

    if (!a || !env || !out) return false;

    /* The same two tests again after the rotation, then one more. */
    if (!(a->but_val_cur & WM_BTN_BLOCK)) return false;
    if (a->player_mode != WM_PMODE_BLOCK) return false;
    /* "no taunts if all opponents are dead." */
    if (env->victim && env->victim->player_mode == WM_PMODE_DEAD)
        return false;

    /* `FACETBL #taunt_tbl / calla change_anim1a` -- the same table
       do_taunt reads, which the port already carries. */
    anim = (a->wrestler_num >= 0 &&
            a->wrestler_num < WM_WRESTLER_ANIM_SLOTS)
         ? wm_wrestler_taunt_anims[a->wrestler_num] : NULL;
    if (!anim) return false;

    out->anim = anim;
    out->risk = (uint16_t)WM_SMOVE_TAUNT_RISK;
    return true;
}

/* ------------------------------------------------------------------ */
/* The guard lists                                                    */

bool wm_smove_guards_pass(const wm_smove_guard_t *guard, size_t count,
                          const wm_arcade_actor_t *a,
                          const wm_arcade_actor_t *opp) {
    size_t i;
    if (!a) return false;
    for (i = 0; i < count; ++i) {
        switch (guard[i].kind) {
        case WM_SMOVE_G_SELF_MODE:
            if (a->player_mode == guard[i].mode) return false;
            break;
        case WM_SMOVE_G_OPP_MODE:
            /* `move *a0(PLYRMODE),a0` on a null process pointer is
               not something the arcade can do -- there is always a
               closest wrestler -- so a missing one refuses nothing
               rather than inventing a mode. */
            if (opp && opp->player_mode == guard[i].mode) return false;
            break;
        case WM_SMOVE_G_UNINT:
            if (a->anim_mode & WM_MODE_UNINT) return false;
            break;
        case WM_SMOVE_G_GETUP:
            if (a->getup_time != 0) return false;
            break;
        case WM_SMOVE_G_IMMOBILIZE:
            if (a->immobilize_time != 0) return false;
            break;
        case WM_SMOVE_G_I_WILL_DIE:
            if (a->i_will_die != 0) return false;
            break;
        case WM_SMOVE_G_CK_IGNORE:
            if (wm_arcade_ck_ignore(a)) return false;
            break;
        default:
            return false;
        }
    }
    return true;
}

/*
 * FACE24 (MACROS.H:51) loads the `_2_` form and keeps it when
 * MOVE_UP_BIT is set in FACING_DIR, replacing it with the `_4_` form
 * when it is clear. Every generated animation pair in this file is
 * resolved through here so the one reading of that macro is in one
 * place.
 */
static const char *face24(const wm_arcade_actor_t *a,
                          const char *up, const char *down) {
    if (!down) return up;
    return (a->facing_dir & WM_MOVE_UP) ? up : down;
}

/* SETMODE, where a row has one. -1 means the routine does not change
   the mode, which is most of them. */
static void apply_set_mode(wm_arcade_actor_t *a, int32_t mode) {
    if (mode >= 0) a->player_mode = (uint16_t)mode;
}

/* ------------------------------------------------------------------ */
/* The charge family                                                  */

bool wm_smove_charge_tick(const wm_smove_charge_t *row, int32_t *held,
                          wm_arcade_actor_t *a, const wm_smove_env_t *env,
                          wm_smove_fire_t *out) {
    if (!row || !held || !a || !env) return false;

    /*
     * `#loop1 SLEEPK 1 / ++#CHARGE_TIME / move *a8(BUT_VAL_CUR),a0 /
     * btst PLAYER_x_BIT,a0 / jrz #p1 / jruc #loop1`. The count goes up
     * FIRST and the button is tested after, so a press that is
     * released on the same tick still counts that tick.
     */
    *held += 1;
    if (a->but_val_cur & row->button) return false;

    /* `cmpi 100,a14 / jrlt #start_over` -- a short press is no press,
       and the counter starts again from zero either way. */
    if (*held < row->threshold) { *held = 0; return false; }
    *held = 0;

    if (!wm_smove_guards_pass(row->guard, row->guards, a, env->closest))
        return false;

    if (out) {
        memset(out, 0, sizeof(*out));
        out->bonus = -1;
        /*
         * `movi <anim>,a14 / cmpi MODE_RUNNING,a0 / jrnz #cont / movi
         * <anim>_run,a14` -- Shawn's suplex and Bam Bam's neckbreaker
         * have a second animation for a wrestler already running.
         */
        out->anim = (row->anim_running && a->player_mode == WM_PMODE_RUNNING)
                        ? row->anim_running : row->anim;
    }
    apply_set_mode(a, row->set_mode);
    return true;
}

/* ------------------------------------------------------------------ */
/* The grab_toss_air family                                           */

static bool grab_fire(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                      wm_smove_fire_t *out) {
    const wm_smove_grab_t *row = (const wm_smove_grab_t *)env->hdhold_row;
    const wm_arcade_actor_t *opp = env->closest;
    bool air;

    if (!row) return false;

    /* `move *a8(ANIMODE),a0 / btst MODE_UNINT_BIT,a0 / jrnz #lp`. */
    if (a->anim_mode & WM_MODE_UNINT) return false;
    /* `cmpi MODE_HEADHOLD,a0 / jreq #lp0` -- not while holding a head;
       the head-hold monitors own that case. */
    if (a->player_mode == WM_PMODE_HEADHOLD) return false;

    /* Unlike a guard list, this one cannot shrug off a missing
       opponent: his mode is what CHOOSES between the two animations,
       so there is no answer to give without him. */
    if (!opp) return false;
    if (opp->player_mode == WM_PMODE_ONGROUND ||
        opp->player_mode == WM_PMODE_DEAD)
        return false;

    /*
     * The two-way choice. An opponent already off the ground -- in
     * either air mode, or in the middle of a leaping attack -- is
     * caught wherever he is; anyone else has to be close enough.
     */
    air = (opp->player_mode == WM_PMODE_INAIR ||
           opp->player_mode == WM_PMODE_INAIR2 ||
           opp->attack_type == WM_AT_LEAPING);
    if (!air) {
        /* `move *a8(CLOSEST_DIST),a0 / cmpi 68h,a0 / jrgt #lp`. */
        if (a->closest_dist > row->near_dist) return false;
    }

    if (out) {
        memset(out, 0, sizeof(*out));
        out->bonus = -1;
        out->anim = air ? face24(a, row->air_anim, row->air_anim_flipped)
                        : face24(a, row->near_anim, row->near_anim_flipped);
    }
    /* The Undertaker alone lets go of whatever he was attached to. */
    if (row->clear_attach) a->attach_proc = NULL;
    apply_set_mode(a, row->set_mode);
    return true;
}

/* ------------------------------------------------------------------ */
/* The free-move family                                               */

static bool free_gate(const wm_arcade_actor_t *a,
                      const wm_smove_env_t *env) {
    const wm_smove_free_t *row = (const wm_smove_free_t *)env->hdhold_row;
    if (!a || !row) return false;
    return wm_smove_guards_pass(row->gate, row->gates, a, env->closest);
}

static bool free_fire(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                      wm_smove_fire_t *out) {
    const wm_smove_free_t *row = (const wm_smove_free_t *)env->hdhold_row;
    if (!row) return false;
    if (!wm_smove_guards_pass(row->guard, row->guards, a, env->closest))
        return false;
    if (out) {
        memset(out, 0, sizeof(*out));
        out->bonus = -1;
        out->anim = row->anim;
    }
    /* `clr a0 / move a0,*a8(RUN_TIME)` -- the move stops a run dead. */
    if (row->clear_run_time) a->run_time = 0;
    apply_set_mode(a, row->set_mode);
    return true;
}

/* ------------------------------------------------------------------ */
/* The two three-way monitors                                         */

/*
 * SHAWN.ASM:777 shn_flipslam -- "Can do from head hold also!", says
 * the source's own comment on the line under the label, and that is
 * exactly what makes it neither a head-hold monitor nor a free move.
 * The same three inputs do three different things:
 *
 *   MODE_HEADHELD  a reversal, targeting WHOHITME
 *   MODE_HEADHOLD  the slam, bonus message 39, targeting WHOIHIT
 *   anything else  `#slam2` -- the same animation, no target, no
 *                  bonus, and nobody immobilised
 *
 * so there is no shared body to parametrise and it is read by hand.
 * Its timeout is 30 where the rest of Shawn's are 60.
 */
static bool flipslam_gate(const wm_arcade_actor_t *a,
                          const wm_smove_env_t *env) {
    (void)env;
    /* `move *a8(GETUP_TIME),a0 / jrnz #lp0`. */
    return a && a->getup_time == 0;
}

static bool flipslam_fire(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                          wm_smove_fire_t *out) {
    wm_arcade_actor_t *target = NULL;
    int32_t bonus = -1;

    (void)env;
    if (!a || !out) return false;
    memset(out, 0, sizeof(*out));
    out->bonus = -1;

    /* `move *a8(ANIMODE),a0 / btst MODE_UNINT_BIT,a0 / jrnz #lp`. */
    if (a->anim_mode & WM_MODE_UNINT) return false;

    if (a->player_mode == WM_PMODE_HEADHELD) {
        if (a->i_will_die) return false;
        if (a->immobilize_time) return false;
        /* CALLA DO_REVERSAL / DO_REVERSAL_MESS / SMRTTGT a8,WHOHITME */
        target = a->who_hit_me;
    } else if (a->player_mode == WM_PMODE_HEADHOLD) {
        if (a->immobilize_time) return false;
        bonus = 39;                       /* `movi 39,a10` */
        target = a->who_i_hit;
    }
    /* Otherwise `#slam2`: straight to the animation with none of the
       above. He is not holding anyone, so there is nobody to pin. */

    if (target) {
        a->smart_target = target;
        out->victim = target;
        out->victim_immobilize = 15;      /* `movk 15,a14` */
    }
    out->bonus = bonus;
    out->anim = "shn_flipslam_anim";
    return true;
}

/*
 * SHAWN.ASM:570 shn_swirl_speedkick, the other one, and its three
 * paths diverge further than flipslam's: holding a head turns the
 * same input into a flying knee with its own animation, its own
 * damage window and a lift into the air, while the other two produce
 * the speed kick.
 *
 * Worth noting about the reversal path, because it looks like a bug
 * and is not: `jruc #is_reversal` after the DO_REVERSAL block is
 * COMMENTED OUT, so a wrestler who reverses falls straight through
 * into `#slam` and performs the speed kick. The bonus message and
 * the immobilise on that path are commented out too. Reading the
 * live text is the only way to get this one right.
 */
static bool swirl_gate(const wm_arcade_actor_t *a,
                       const wm_smove_env_t *env) {
    (void)env;
    return a && a->getup_time == 0;
}

static bool swirl_fire(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                       wm_smove_fire_t *out) {
    if (!a || !out || !env) return false;
    memset(out, 0, sizeof(*out));
    out->bonus = -1;

    if (a->anim_mode & WM_MODE_UNINT) return false;
    if (a->player_mode == WM_PMODE_DEAD) return false;

    if (a->player_mode == WM_PMODE_HEADHOLD) {
        /* `#knee`. */
        wm_arcade_actor_t *victim = a->who_i_hit;
        if (a->immobilize_time) return false;
        if (victim) {
            /* `movi 6*60,a14 / move a14,*a0(DELAY_METER)`. */
            victim->delay_meter = 6 * 60;
        }
        /* MACROS.H:714 SPCDMG D_FLYKICK/2,20 -- DAMAGE.EQU:16 makes
           D_FLYKICK 28, so fourteen points for the next twenty
           ticks. */
        a->special_damage_time = env->pcnt + 20u;
        a->next_damage = 28 / 2;
        out->anim = "shn_flying_kick_anim";
        a->player_mode = WM_PMODE_INAIR;   /* SETMODE INAIR */
        return true;
    }

    if (a->player_mode == WM_PMODE_HEADHELD) {
        if (a->i_will_die) return false;
        if (a->immobilize_time) return false;
        /* DO_REVERSAL / DO_REVERSAL_MESS / SMRTTGT a8,WHOHITME, and
           then FALLS THROUGH into #slam rather than jumping past it. */
        if (a->who_hit_me) a->smart_target = a->who_hit_me;
    }
    /* `#slam`. Its bonus, its target and its immobilise are all
       commented out, so what is left is the animation. */
    if (a->immobilize_time) return false;
    out->anim = "shn_super_speedkick_anim";
    return true;
}

/*
 * `SLEEPK 5 / movi 40000h,a0 / move a0,*a8(OBJ_YVEL),L / SLEEPK 20`.
 * The lift comes five ticks into the twenty-five the routine sleeps
 * for, not at the moment the animation starts, and only on the knee.
 */
static void swirl_tail(wm_arcade_actor_t *a, int32_t remaining) {
    if (!a) return;
    if (remaining == 20 && a->player_mode == WM_PMODE_INAIR)
        a->y_vel = 0x40000;
}

/*
 * The head-hold family's fire(), shared by all 41 rows. Which row is
 * running is carried on the run itself, because the monitor
 * descriptor is built from the generated table rather than written
 * out here.
 */
static bool hdhold_fire(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                        wm_smove_fire_t *out) {
    const wm_smove_hdhold_t *row = (const wm_smove_hdhold_t *)env->hdhold_row;
    wm_arcade_actor_t *victim = NULL;
    if (!row) return false;
    return wm_smove_hdhold_fire(row, a, &victim, out) != WM_SMOVE_HH_NOTHING;
}

/*
 * The gate: live only while somebody has a head hold, either way
 * round. `move *a8(PLYRMODE),a0 / cmpi MODE_HEADHOLD,a0 / jrz #cont /
 * cmpi MODE_HEADHELD,a0 / jrnz #lp0`.
 */
static bool hdhold_gate(const wm_arcade_actor_t *a,
                        const wm_smove_env_t *env) {
    const wm_smove_hdhold_t *row;
    if (!a || !env) return false;
    row = (const wm_smove_hdhold_t *)env->hdhold_row;
    if (!row) {
        return a->player_mode == WM_PMODE_HEADHOLD ||
               a->player_mode == WM_PMODE_HEADHELD;
    }
    /*
     * Not both modes on every row. Fifteen combo moves are gated
     * `cmpi MODE_HEADHOLD,a0 / jrnz #lp0` and belong to the man doing
     * the holding alone.
     */
    if (a->player_mode == WM_PMODE_HEADHOLD) {
        if (!row->gate_headhold) return false;
    } else if (a->player_mode == WM_PMODE_HEADHELD) {
        if (!row->gate_headheld) return false;
    } else {
        return false;
    }
    /* `move *a8(GETUP_TIME),a0 / jrnz #lp0`. */
    if (row->needs_getup_clear && a->getup_time != 0) return false;
    /* `calla CHECK_COMBO_GO / jrlt #lp0` -- a signed less-than, so a
       negative answer refuses and zero does not. The combo state is
       the actor's own combo_count, which is what CHECK_COMBO_GO
       reads. */
    if (row->needs_combo && a->combo_count <= 0) return false;
    return true;
}

/*
 * One monitor descriptor per generated row, built once. They are not
 * written out because they differ only in the columns the extractor
 * read; building them here keeps the source data and the runtime
 * shape in one place.
 */
#define WM_SMOVE_GENERATED_MAX 96
static wm_smove_monitor_t GENERATED[WM_SMOVE_GENERATED_MAX];
static size_t generated_built;

static wm_smove_monitor_t *next_generated(const char *name,
                                          const char *file) {
    wm_smove_monitor_t *m;
    if (generated_built >= WM_SMOVE_GENERATED_MAX) return NULL;
    m = &GENERATED[generated_built++];
    memset(m, 0, sizeof(*m));
    m->name = name;
    m->file = file;
    return m;
}

static void build_generated(void) {
    size_t i;
    if (generated_built) return;

    for (i = 0; i < wm_smove_hdhold_count; ++i) {
        const wm_smove_hdhold_t *r = &wm_smove_hdhold[i];
        wm_smove_monitor_t *m = next_generated(r->name, r->file);
        if (!m) return;
        m->step[0] = r->step[0];
        m->step[1] = r->step[1];
        m->step[2] = r->step[2];
        m->steps = 3;
        m->timeout = r->timeout;
        m->gate = hdhold_gate;
        m->fire = hdhold_fire;
        /* `SLEEPK 20 / jruc #lp` -- it goes round again rather than
           dying, so a head hold can be turned into move after move. */
        m->one_shot = false;
        m->cooldown = WM_SMOVE_HH_COOLDOWN;
        m->hdhold = r;
    }

    for (i = 0; i < wm_smove_charge_count; ++i) {
        const wm_smove_charge_t *r = &wm_smove_charge[i];
        wm_smove_monitor_t *m = next_generated(r->name, r->file);
        if (!m) return;
        /* No steps at all: wm_smove_tick routes a charge monitor
           straight to wm_smove_charge_tick. */
        m->cooldown = r->cooldown;
        m->charge = r;
    }

    for (i = 0; i < wm_smove_grab_count; ++i) {
        const wm_smove_grab_t *r = &wm_smove_grab[i];
        wm_smove_monitor_t *m = next_generated(r->name, r->file);
        if (!m) return;
        m->step[0] = r->step[0];
        m->step[1] = r->step[1];
        m->step[2] = r->step[2];
        m->steps = 3;
        m->timeout = r->timeout;
        m->fire = grab_fire;
        m->cooldown = r->cooldown;
        m->grab = r;
    }

    for (i = 0; i < wm_smove_free_count; ++i) {
        const wm_smove_free_t *r = &wm_smove_free[i];
        wm_smove_monitor_t *m = next_generated(r->name, r->file);
        if (!m) return;
        m->step[0] = r->step[0];
        m->step[1] = r->step[1];
        m->step[2] = r->step[2];
        m->steps = 3;
        m->timeout = r->timeout;
        /* Only the Undertaker's two spirit moves have anything in
           their gate, but an empty list passes, so this is uniform. */
        m->gate = free_gate;
        m->fire = free_fire;
        m->cooldown = r->cooldown;
        m->freemove = r;
    }
}

/* The generated row this descriptor carries, whichever family it is
   from; the runtime hands it to gate() and fire() through the env. */
static const void *generated_row(const wm_smove_monitor_t *m) {
    if (m->hdhold) return m->hdhold;
    if (m->charge) return m->charge;
    if (m->grab) return m->grab;
    if (m->freemove) return m->freemove;
    return NULL;
}

/*
 * The registry. A monitor is here because its whole body is
 * translated; the entries that resolve to NULL are what makes the
 * remaining gap countable.
 */
static const wm_smove_monitor_t MONITORS[] = {
    {
        .name = "und_finish_move1", .file = "TAKER.ASM",
        .step = { { WM_J_UP, 0 }, { WM_J_DOWN, 0 },
                  { WM_B_PUNCH, WM_J_ALL } },
        .steps = 3, .timeout = WM_SMOVE_TIMEOUT_FINISH,
        .fire = und_finish_fire,
        .one_shot = true,          /* `#fi1_exit DIE` */
    },
    {
        .name = "std_walk_fast", .file = "DOINK.ASM",
        .step = { { WM_J_AWAY, 0 }, { WM_J_DOWN_AWAY, 0 }, { WM_J_DOWN, 0 },
                  { WM_J_DOWN_TOWARD, 0 }, { WM_J_TOWARD, 0 },
                  { WM_J_UP_TOWARD, 0 }, { WM_J_UP, 0 },
                  { WM_J_UP_AWAY, 0 } },
        .steps = 8, .timeout = WM_SMOVE_TIMEOUT_DOINK,
        .gate = walk_fast_gate, .fire = walk_fast_fire,
        .one_shot = true,          /* the tail ends in DIE */
    },
    {
        .name = "std_taunt", .file = "DOINK.ASM",
        .step = { { WM_J_UP, WM_B_BLOCK }, { WM_J_UP_TOWARD, WM_B_BLOCK },
                  { WM_J_TOWARD, WM_B_BLOCK },
                  { WM_J_DOWN_TOWARD, WM_B_BLOCK },
                  { WM_J_DOWN, WM_B_BLOCK }, { WM_J_DOWN_AWAY, WM_B_BLOCK },
                  { WM_J_AWAY, WM_B_BLOCK }, { WM_J_UP_AWAY, WM_B_BLOCK } },
        .steps = 8, .timeout = WM_SMOVE_TIMEOUT_DOINK,
        .humans_only = true,       /* `PLYR_TYPE != 0 -> SUCIDE` */
        .gate = taunt_gate, .mid_gate = taunt_mid_gate, .fire = taunt_fire,
        .one_shot = true,          /* DIE after setting RISK */
    },
    {
        .name = "shn_flipslam", .file = "SHAWN.ASM",
        .step = { { WM_J_UP, 0 }, { WM_J_TOWARD, 0 },
                  { WM_B_SPUNCH, WM_J_ALL } },
        /* `#TIMEOUT .equ 30` -- half of what the rest of Shawn's get. */
        .steps = 3, .timeout = 30,
        .gate = flipslam_gate, .fire = flipslam_fire,
        .cooldown = 20,            /* `SLEEPK 20 / jruc #lp` */
    },
    {
        .name = "shn_swirl_speedkick", .file = "SHAWN.ASM",
        .step = { { WM_J_DOWN, 0 },
                  { WM_J_TOWARD, WM_J_DOWN | WM_J_UP },
                  { WM_B_KICK, WM_J_ALL } },
        .steps = 3, .timeout = 60,
        .gate = swirl_gate, .fire = swirl_fire,
        /* Both paths sleep twenty-five ticks in total; the knee's
           OBJ_YVEL write happens five ticks in. */
        .cooldown = 25, .tail = swirl_tail,
    },
};

#define MONITOR_COUNT (sizeof MONITORS / sizeof MONITORS[0])

const wm_smove_monitor_t *wm_smove_monitor_find(const char *name) {
    size_t i;
    if (!name) return NULL;
    for (i = 0; i < MONITOR_COUNT; ++i)
        if (strcmp(MONITORS[i].name, name) == 0) return &MONITORS[i];
    build_generated();
    for (i = 0; i < generated_built; ++i)
        if (strcmp(GENERATED[i].name, name) == 0) return &GENERATED[i];
    return NULL;
}

size_t wm_smove_monitor_count(void) {
    build_generated();
    return MONITOR_COUNT + generated_built;
}

const wm_smove_monitor_t *wm_smove_monitor_at(size_t i) {
    if (i < MONITOR_COUNT) return &MONITORS[i];
    build_generated();
    i -= MONITOR_COUNT;
    return (i < generated_built) ? &GENERATED[i] : NULL;
}

/* ------------------------------------------------------------------ */
/* init_smoves / reset_smoves / kill_smove_procs                      */

size_t wm_smove_init(int32_t wrestler_num, bool is_drone,
                     wm_smove_run_t *runs, size_t capacity,
                     size_t *unported) {
    const wm_wrestler_smove_table *tbl;
    size_t made = 0, missing = 0, i;

    if (unported) *unported = 0;
    if (!runs || capacity == 0) return 0;

    /* `move *a13(WRESTLERNUM),a2,W / X32 a2 / addi #special_moves,a2 /
       move *a2,a2,L / jrz #done` -- a zero table pointer means this
       wrestler has no watchdogs at all. */
    if (wrestler_num < 0 || wrestler_num >= WM_WRESTLER_ANIM_SLOTS)
        return 0;
    tbl = &wm_wrestler_smoves[wrestler_num];
    if (!tbl->labels || tbl->count <= 0) return 0;

    for (i = 0; i < (size_t)tbl->count && made < capacity; ++i) {
        const wm_smove_monitor_t *m = wm_smove_monitor_find(tbl->labels[i]);
        if (!m) { ++missing; continue; }
        /* std_taunt's first two instructions: a drone kills itself
           rather than watching for the code. The process IS created --
           it just does not survive its own first tick -- so this skips
           it here and counts it as made-and-gone rather than absent. */
        if (m->humans_only && is_drone) continue;
        memset(&runs[made], 0, sizeof(runs[made]));
        runs[made].monitor = m;
        ++made;
    }

    if (unported) *unported = missing;
    return made;
}

void wm_smove_reset(wm_smove_run_t *runs, size_t count) {
    size_t i;
    if (!runs) return;
    /* "writing their SM_RESET_ADDRESSes to their PWAKEs, and setting
       their PTIMEs to 1" -- back to the routine's entry, awake next
       tick. A monitor that DIEd is gone from ACTIVE and is NOT
       revived, so `dead` survives a reset. */
    for (i = 0; i < count; ++i) {
        if (runs[i].dead) continue;
        runs[i].at = 0;
        runs[i].countdown = 0;
        runs[i].armed = false;
        runs[i].cooldown = 0;
    }
}

void wm_smove_kill(wm_smove_run_t *runs, size_t count) {
    size_t i;
    if (!runs) return;
    for (i = 0; i < count; ++i) runs[i].dead = true;
}

bool wm_smove_tick(wm_smove_run_t *run, wm_arcade_actor_t *a,
                   const wm_smove_env_t *env, wm_smove_fire_t *out) {
    const wm_smove_monitor_t *m;
    wm_smove_env_t local;
    wm_smove_tick_t r;

    if (out) {
        memset(out, 0, sizeof(*out));
        out->bonus = -1;
    }
    if (!run || !run->monitor || run->dead || !a || !env) return false;
    m = run->monitor;

    local = *env;
    local.hdhold_row = generated_row(m);

    /*
     * The `SLEEPK 20` (or `SLEEP 120`, or Bam Bam's `SLEEP TSEC*3`)
     * every one of these routines runs before looping back to #lp.
     * Without it a monitor whose inputs are still satisfied fires
     * again on the very next tick.
     */
    if (run->cooldown > 0) {
        run->cooldown -= 1;
        if (m->tail) m->tail(a, run->cooldown);
        return false;
    }

    /*
     * A charge monitor has no switch sequence at all -- it counts a
     * held button -- so it does not go through the state machine
     * below. `countdown` carries its #CHARGE_TIME.
     */
    if (m->charge) {
        if (!wm_smove_charge_tick(m->charge, &run->countdown, a, &local,
                                  out))
            return false;
        run->cooldown = m->cooldown;
        if (m->one_shot) run->dead = true;
        return true;
    }

    /*
     * `#lp0: SLEEPK 1 / #lp: <gate> / jrnz #lp0`. The gate is re-tested
     * every pass before the sequence starts, and a failure inside the
     * sequence lands back on `#lp`, which re-tests it too.
     */
    if (!run->armed) {
        if (m->gate && !m->gate(a, &local)) return false;
        run->at = 0;
        run->countdown = 0;      /* `clr a11` -- no deadline on step 0 */
        run->armed = true;
        return false;            /* the SLEEPK before the first wait */
    }

    r = wm_smove_waitswitch(&run->countdown, a->special_move_addr,
                            a->but_val_down, a->stick_rel_new,
                            m->step[run->at].switches,
                            m->step[run->at].mask);
    if (r == WM_SMOVE_WAIT) return false;
    if (r == WM_SMOVE_FAIL) { run->armed = false; return false; }

    run->at += 1;

    /* The extra test between step 0 and the timeout, and then the one
       `movi #TIMEOUT,a11` the whole rest of the sequence shares. */
    if (run->at == 1) {
        if (m->mid_gate && !m->mid_gate(a, &local)) {
            run->armed = false;
            return false;
        }
        run->countdown = m->timeout;
    }

    if (run->at < m->steps) return false;

    run->armed = false;
    if (!m->fire) return false;
    if (!m->fire(a, &local, out)) return false;
    run->cooldown = m->cooldown;
    if (m->one_shot) run->dead = true;
    return true;
}

/* ------------------------------------------------------------------ */
/* The head-hold family's shared tail                                 */

wm_smove_hh_result_t wm_smove_hdhold_fire(const wm_smove_hdhold_t *row,
                                          wm_arcade_actor_t *a,
                                          wm_arcade_actor_t **victim,
                                          wm_smove_fire_t *out) {
    wm_arcade_actor_t *target;
    wm_smove_hh_result_t result;

    if (victim) *victim = NULL;
    if (out) {
        memset(out, 0, sizeof(*out));
        out->bonus = -1;
    }
    if (!row || !a) return WM_SMOVE_HH_NOTHING;

    /*
     * `move *a8(PLYRMODE),a0 / cmpi MODE_HEADHOLD,a0 / jrz #slam /
     * cmpi MODE_HEADHELD,a0 / jrnz #lp0`. Which mode he is in at THIS
     * moment decides whose move it is -- not who started the hold --
     * and the row says what each mode does, because the answer is
     * not the same for all forty.
     */
    if (a->player_mode == WM_PMODE_HEADHOLD) {
        result = (wm_smove_hh_result_t)WM_SMOVE_HH_SLAM;
    } else if (a->player_mode == WM_PMODE_HEADHELD) {
        result = (wm_smove_hh_result_t)row->headheld;
        /* `move *a8(I_WILL_DIE),A14 / jrnz #lp0` -- a man already on
           his way out cannot reverse. The test is on the reversal
           side only; the slam path has none. */
        if (result == WM_SMOVE_HH_REVERSAL && a->i_will_die)
            return WM_SMOVE_HH_NOTHING;
    } else {
        return WM_SMOVE_HH_NOTHING;
    }
    if (result == WM_SMOVE_HH_NOTHING) return WM_SMOVE_HH_NOTHING;

    /* `move *a8(IMMOBILIZE_TIME),a14 / jrnz #lp0 ;ignore` -- on both
       paths, and the source's own comment is that one word. */
    if (a->immobilize_time != 0) return WM_SMOVE_HH_NOTHING;

    /* "target WHOHITME -- don't hit anyone else" on the reversal,
       WHOIHIT on the slam. SMRTTGT is the smart-target write, and two
       of them do not make it at all. */
    target = (result == WM_SMOVE_HH_REVERSAL) ? a->who_hit_me : a->who_i_hit;
    if (!row->smart_target) target = NULL;
    if (victim) *victim = target;
    if (target) a->smart_target = target;

    if (out) {
        out->anim = face24(a, row->anim, row->anim_flipped);
        /* The slam's bonus message; a reversal gets DO_REVERSAL_MESS
           instead, which is a different message and not this one. */
        out->bonus = (result == WM_SMOVE_HH_REVERSAL) ? -1 : row->bonus;
        out->victim_immobilize = row->victim_immobilize;
        out->victim = target;
    }

    return result;
}
