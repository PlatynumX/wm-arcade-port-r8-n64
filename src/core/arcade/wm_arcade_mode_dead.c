#include "wm/arcade/wm_arcade_mode_dead.h"
#include "wm/arcade/wm_arcade_combo.h"
#include "wm/arcade/wm_arcade_lifebar.h"

#include <string.h>

/*
 * `#count_btns`: "count new presses this tick and add to BUCKOFF_COUNT".
 *
 * The source pops bits one at a time -- `lmo a0,a14 / jrz #end0 /
 * rl a14,a0 / sla 1,a0 / inc a1` -- which is a population count of
 * BUT_VAL_DOWN, the buttons that went down THIS tick. Mashing two
 * buttons at once really does count double.
 */
static int32_t new_presses(uint16_t but_val_down) {
    int32_t n = 0;
    uint16_t v = but_val_down;
    while (v) { v &= (uint16_t)(v - 1u); ++n; }
    return n;
}

/* DOINK.ASM:3093 `#dobuck` -- SUCCESS! Do the buckoff. */
static void do_buckoff(wm_arcade_actor_t *actor,
                       const wm_mode_dead_env_t *env,
                       wm_mode_dead_result_t *out) {
    size_t i;

    /* `calla clear_combo_meter`. */
    wm_arcade_clear_combo_meter(actor);

    /*
     * "If any opponent has pinned, send him to his buckoff anim." The
     * sweep takes the FIRST process carrying DID_PIN, whoever it is --
     * the commented-out code above it shows the author first tried
     * WHOPINNEDME and replaced it with this.
     */
    if (env && env->actors) {
        for (i = 0; i < env->actor_count; ++i) {
            wm_arcade_actor_t *p = env->actors[i];
            if (!p) continue;
            if (!(p->status_flags & WM_STATUS_DID_PIN)) continue;
            p->status_flags &= ~(uint32_t)WM_STATUS_DID_PIN;
            /*
             * "if his DID_RAISEARM bit is set, then it was probably
             * taker and he's no longer on top of us, so skip the
             * buckoff and just let him get his raisearm anim shut off
             * along with the other drones."
             */
            if (!(p->status_flags & WM_STATUS_DID_RAISEARM) && out)
                out->pinner_to_buck = p;
            break;
        }
    }

    /* "back to life..." */
    actor->player_mode = WM_PMODE_ONGROUND;
    actor->life += WM_BUCKOFF_HEALTH;           /* `movk 2,a0` */
    if (actor->life > WM_ARCADE_LIFE_MAX) actor->life = WM_ARCADE_LIFE_MAX;

    /* "no getup meter". */
    actor->getup_time = 0;
    actor->delay_meter = 0;
    /* "If died in a combo, allow buckoff to continue". */
    actor->i_will_die = 0;
    /* "reset my pal, just in case." */
    actor->obj_pal = actor->my_pal;

    if (out) out->second_wind_message = true;

    /* `ori M_DID_BUCKOFF|M_NEW_BUCKOFF / andni M_DO_BUCKOFF|M_PINNED|
       M_PINABLE` -- one read, one write, on the whole long. */
    actor->status_flags |= (uint32_t)(WM_STATUS_DID_BUCKOFF |
                                      WM_STATUS_NEW_BUCKOFF);
    actor->status_flags &= ~(uint32_t)(WM_STATUS_DO_BUCKOFF |
                                       WM_STATUS_PINNED |
                                       WM_STATUS_PINABLE);

    /* `FACETBL hitonground_tbl / calla change_anim1a`, then the
       nocollis bit so nothing can hit him mid-convulse. */
    if (out) out->convulse = true;
    actor->anim_mode |= (uint16_t)WM_MODE_NOCOLLIS;

    /*
     * "if anyone has done a raisearm, put 'em back in a stand. just
     * clear the DID_RAISEARM bit. Anim scripts will do the rest."
     */
    if (env && env->actors) {
        for (i = 0; i < env->actor_count; ++i) {
            wm_arcade_actor_t *p = env->actors[i];
            if (!p) continue;
            p->status_flags &= ~(uint32_t)WM_STATUS_DID_RAISEARM;
        }
    }

    /*
     * "If anyone has turned into a drone, turn 'em back" -- and note
     * that this is process_ptrs slots 0 and 1 SPECIFICALLY, the two
     * human PLYRNUMs, not a sweep. A buddy at PLYRNUM 2 stays a drone.
     */
    if (env && env->actors) {
        for (i = 0; i < env->actor_count; ++i) {
            wm_arcade_actor_t *p = env->actors[i];
            if (!p) continue;
            if (p->player_num == 0 || p->player_num == 1)
                p->plyr_type = WM_PTYPE_PLAYER;
        }
    }

    if (out) out->bucked_off = true;
}

void wm_arcade_mode_dead_ex(wm_arcade_actor_t *actor,
                            const wm_mode_dead_env_t *env,
                            wm_mode_dead_result_t *out) {
    int32_t lost;

    if (out) memset(out, 0, sizeof *out);
    if (!actor) return;

    /*
     * `btst B_ZOMBIE / jrnz #zmb`. The zombie tail itself lives in
     * wm/arcade/wm_arcade_final_battle.h and is driven from the match;
     * here it is simply not this routine's business any more.
     */
    if (actor->status_flags & WM_STATUS_ZOMBIE) return;

    /* One buckoff per match, and one check per round. */
    if (actor->status_flags & (WM_STATUS_DID_BUCKOFF |
                               WM_STATUS_NO_BUCKOFF)) return;

    /* `btst B_DO_BUCKOFF / jrnz #count_btns` -- already counting. */
    if (actor->status_flags & WM_STATUS_DO_BUCKOFF) {
        actor->buckoff_count += new_presses(actor->but_val_down);
        if (actor->buckoff_count >= WM_BUCKOFF_TARGET)
            do_buckoff(actor, env, out);
        return;
    }

    /* `move @royal_rumble,a14 / jrnz #done` -- no flag is set either, so
       a rumble re-asks every tick and never gets anywhere. */
    if (env && env->royal_rumble) return;

    if (!env) goto no_buckoff;

    /*
     * "Is this the second round we've lost? (skip this test if we're in
     * the 8-on-1 match)". The count read is the OTHER side's -- a wrestler
     * on side 0 reads p2rounds -- because pXrounds counts rounds that
     * player has WON, so the opposite side's total is what he has lost.
     */
    if (env->eight_on_one) {
        /* `#ck81`: "it's 8-on-1. only the player is allowed to buckoff." */
        if (actor->player_num >= 2) goto no_buckoff;
    } else {
        lost = env->rounds[actor->player_side == 0 ? 1 : 0];
        if (lost == 0) goto no_buckoff;
    }

    /* `#tcombo`: "is our combo meter lit?" `jrlt #nobuck`. */
    if (wm_arcade_check_combo_go(actor, env->instant_combos_on) < 0)
        goto no_buckoff;

    /* "are we inside the ring?" -- `jrnz #nobuck` on INRING, whose
       source polarity is 1 for OUTSIDE, so this port tests the inverse. */
    if (!actor->in_ring) goto no_buckoff;

    /* "Buckoff is NOT allowed if undertaker started his finish move or
       has completed his finish move!!!!" */
    if (env->in_finish_move || env->finish_completed) goto no_buckoff;

    /* "possible buckoff. Zero BUCKOFF_COUNT and set DO_BUCKOFF bit",
       then straight into the count on this same tick. */
    actor->buckoff_count = 0;
    actor->status_flags |= (uint32_t)WM_STATUS_DO_BUCKOFF;
    actor->buckoff_count += new_presses(actor->but_val_down);
    if (actor->buckoff_count >= WM_BUCKOFF_TARGET)
        do_buckoff(actor, env, out);
    return;

no_buckoff:
    /* `#nobuck`: set NO_BUCKOFF so the whole chain is not re-run until
       the next round resets it. */
    actor->status_flags |= (uint32_t)WM_STATUS_NO_BUCKOFF;
}

void wm_arcade_mode_dead(wm_arcade_actor_t *actor) {
    wm_arcade_mode_dead_ex(actor, NULL, NULL);
}
