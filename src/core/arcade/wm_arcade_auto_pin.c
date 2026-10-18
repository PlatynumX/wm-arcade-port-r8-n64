#include "wm/arcade/wm_arcade_auto_pin.h"
#include "wm/arcade/wm_arcade_round_announce.h"

#include <string.h>

wm_auto_pin_result_t wm_arcade_auto_pin_check(wm_arcade_actor_t *actor,
                                              const wm_auto_pin_env_t *env) {
    wm_auto_pin_result_t out;

    memset(&out, 0, sizeof out);
    if (!actor || !env) return out;

    /* The three globals, in the source's order. */
    if (env->in_finish_move) return out;
    if (env->finish_completed) return out;
    if (env->royal_rumble) return out;

    /*
     * `calla get_opp_plyrmode / cmpi MODE_DEAD,a0 / jrne #alive`. The
     * opponent still standing is the reset case, not a pause -- and so
     * is having no opponent at all, which the source cannot express.
     */
    if (!env->opponent || env->opponent->player_mode != WM_PMODE_DEAD) {
        actor->auto_pin_cntdown = 0;                /* `#alive` */
        return out;
    }

    /* "skip it if we've already pinned" -- and note this RETURNS rather
       than resetting, so the count is held, not lost. */
    if (actor->status_flags & WM_STATUS_DID_PIN) return out;

    /*
     * `callr get_opp_process / btst B_ZOMBIE / jrnz #alive`. A zombie
     * is MODE_DEAD but not beaten: he is on his way back up, so this
     * resets rather than holding.
     */
    if (env->opponent->status_flags & WM_STATUS_ZOMBIE) {
        actor->auto_pin_cntdown = 0;
        return out;
    }

    /* Somebody mid-buckoff holds the count too. */
    if (wm_arcade_anyone_bucking(env->actors, env->actor_count,
                                 env->royal_rumble) != NULL)
        return out;

    /* "all opponents are dead. increment AUTO_PIN_CNTDOWN and turn into
       a drone if the total is >= TSEC*4" -- the code says TSEC*3. */
    actor->auto_pin_cntdown = (uint16_t)(actor->auto_pin_cntdown + 1u);
    out.counted = true;
    if (actor->auto_pin_cntdown < WM_AUTO_PIN_TICKS) return out;

    /* The last guard, and the reason the count is held rather than
       reset by the three above: he waits here for his own animation to
       become interruptible. */
    if (actor->anim_mode & WM_MODE_UNINT) return out;

    /* "become a drone" */
    actor->plyr_type = WM_PTYPE_DRONE;
    out.became_drone = true;
    return out;
}

bool wm_arcade_drone_change_back(wm_arcade_actor_t *actor) {
    if (!actor) return false;
    /* "don't check real drones" -- PLYRNUM 0 and 1 are the two credit
       slots; 2 and up were never human to begin with. */
    if (actor->player_num >= 2) return false;
    actor->plyr_type = WM_PTYPE_PLAYER;
    return true;
}

void wm_arcade_set_raisearm_bit(wm_arcade_actor_t *actor) {
    if (!actor) return;
    actor->status_flags |= WM_STATUS_DID_RAISEARM;
}

bool wm_arcade_raisearm_check(const wm_arcade_actor_t *actor,
                              wm_arcade_actor_t *const *actors,
                              size_t actor_count,
                              bool royal_rumble,
                              bool final_queue_empty) {
    size_t i;
    int opp_inside = 0;                 /* the source's a4 */

    if (!actor) return false;

    /*
     * The royal-rumble hack, and the source's own comment on it: "if a13
     * is human and this is a royal rumble, make sure @FINAL_PTR is FFh.
     * If it isn't, don't raise arm." A drone (nonzero PLYR_TYPE) jumps
     * straight past it.
     */
    if (royal_rumble && actor->plyr_type == WM_PTYPE_PLAYER &&
        !final_queue_empty)
        return false;

    /* `movk NUM_WRES,a1 / dsj a1,#lp` -- a fixed sweep of the process
       table, which this port walks as the actor list. */
    for (i = 0; i < actor_count; ++i) {
        const wm_arcade_actor_t *o = actors ? actors[i] : NULL;
        if (!o) continue;                       /* `jrz #nxt` */
        /* One comparison skips himself AND his teammates. */
        if (o->player_side == actor->player_side) continue;
        if (o->player_mode != WM_PMODE_DEAD) return false;
        if (o->status_flags & WM_STATUS_ZOMBIE) return false;
        /* INRING is 1 for OUTSIDE in the source; in_ring is the plain
           boolean here, so an opponent still in the ring counts. */
        if (!o->in_ring) continue;              /* `jrnz #nxt` */
        ++opp_inside;
    }

    /* "okay, all our opponents are dead and staying that way. now check
       the inside/outside stuff." Outside the ring he may pose whatever
       is on the mat; inside, a body beside him stops it. */
    if (!actor->in_ring) return true;
    return opp_inside == 0;
}
