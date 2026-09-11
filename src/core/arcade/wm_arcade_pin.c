/*
 * The pin -- WRESTLE2.ASM:3925 can_pin and AWARD.ASM:1768 pin_prompt's
 * decision half. See wm/arcade/wm_arcade_pin.h.
 */
#include "wm/arcade/wm_arcade_pin.h"
#include "wm/arcade/wm_arcade_combat_defs.h"

#include <stddef.h>

bool wm_arcade_can_pin(wm_arcade_actor_t *pinner, wm_arcade_actor_t *victim,
                       wm_arcade_actor_t *const *actors, size_t count) {
    size_t i;

    if (!pinner || !victim) return false;

    /* "no pin if there are any zombies or live wrestler on other team".
       The sweep skips an inactive slot and skips teammates by
       PLYR_SIDE; anything left that is not a clean corpse refuses. */
    for (i = 0; i < count; ++i) {
        const wm_arcade_actor_t *o = actors ? actors[i] : NULL;
        if (!o || !o->active) continue;              /* `jrz #nxt0` */
        if (o->player_side == pinner->player_side) continue;
        if (o->player_mode != WM_PMODE_DEAD) return false;
        if (o->status_flags & WM_STATUS_ZOMBIE) return false;
    }

    /* "check range" -- both `jrgt`, so the boundary itself passes. */
    if (pinner->closest_dist > WM_PIN_MAX_DIST) return false;
    if (pinner->closest_zdist > WM_PIN_MAX_ZDIST) return false;

    /* xxx_dead_anim's #set_pinable_bit is what puts this there. */
    if (!(victim->status_flags & WM_STATUS_PINABLE)) return false;

    /* #setc -- "just to be safe, set the PINNED bit on the guy". */
    victim->status_flags |= WM_STATUS_PINNED;
    victim->who_pinned_me = pinner;
    victim->x_vel = 0;
    victim->y_vel = 0;
    victim->z_vel = 0;
    /* "set his PTIME to one and clear his KOD bit, 'cuz he's probably
       been KO'd if he's a drone." PTIME is the process scheduler's
       wake delay and belongs to whatever runs him; the KOD clear is
       the part that is state on the wrestler. */
    victim->status_flags &= (uint32_t)~WM_STATUS_KOD;

    return true;
}

wm_arcade_actor_t *wm_arcade_pin_prompt(wm_arcade_actor_t *const *actors,
                                        size_t count, int dead_side) {
    wm_arcade_actor_t *pinner = NULL;
    bool found_in_ring = false;
    size_t i;

    if (!actors) return NULL;

    /* "look for a live human from the winning team inside the ring.
       quit if there isn't one." `movi 2,a1 ;only check humans` -- the
       sweep is over the two human slots, not all NUM_WRES. */
    for (i = 0; i < count && i < 2; ++i) {
        wm_arcade_actor_t *o = actors[i];
        if (!o || !o->active) continue;
        if (o->player_side == dead_side) continue;
        if (o->player_mode == WM_PMODE_DEAD) continue;
        /* INRING is ZERO inside the ring in this source; the port's
           `in_ring` is an ordinary boolean, so the sense flips. */
        if (!o->in_ring) continue;
        pinner = o;
        break;
    }
    if (!pinner) return NULL;

    /*
     * "Make sure everyone on the dead team is actually dead and not a
     * zombie. Further, make sure at least one is inside the ring."
     * A live one or a zombie kills the prompt; one merely outside the
     * ring is skipped, and only the in-ring flag is collected.
     */
    for (i = 0; i < count; ++i) {
        const wm_arcade_actor_t *o = actors[i];
        if (!o || !o->active) continue;
        if (o->player_side != dead_side) continue;
        if (!o->in_ring) continue;                   /* `jrnz #nxt1` */
        if (o->player_mode != WM_PMODE_DEAD) return NULL;
        if (o->status_flags & WM_STATUS_ZOMBIE) return NULL;
        found_in_ring = true;                        /* `inc a10` */
    }

    /* `#fp_dn TEST a10 / jrz #die`. */
    if (!found_in_ring) return NULL;

    return pinner;
}

void wm_arcade_pins_clear(wm_arcade_pins_t *p) {
    if (!p) return;
    p->p1pins = 0;
    p->p2pins = 0;
}

void wm_arcade_pins_award(wm_arcade_pins_t *p, int plyr_side) {
    if (!p) return;
    /* `move *a8(PLYR_SIDE),a14 / jrnz #onrt` -- side 0 is p1. */
    if (plyr_side == 0) p->p1pins += 1;
    else p->p2pins += 1;
}

int32_t wm_arcade_pins_for(const wm_arcade_pins_t *p, int plyr_side) {
    if (!p) return 0;
    return (plyr_side == 0) ? p->p1pins : p->p2pins;
}
