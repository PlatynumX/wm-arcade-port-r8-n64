#include "wm/arcade/wm_arcade_modes.h"
#include "wm/arcade/wm_arcade_roster_anims.h"

#include <string.h>

/*
 * The source's own mutual-attachment test, which mode_puppet and
 * mode_choking both open with and which reads backwards at a glance:
 *
 *      move    *a13(ATTACH_PROC),a0,L
 *      jrz     <not attached>
 *      move    *a0(ATTACH_PROC),a1,L
 *      cmp     a1,a13
 *      jreq    <attached, and mutual>
 *
 * A NULL ATTACH_PROC is NOT the sound case -- it falls through to the
 * count in mode_puppet and to the bail-out in mode_choking. The sound
 * case is the narrow one: attached, AND the other man attached back.
 */
static bool mutually_attached(const wm_arcade_actor_t *a) {
    return a->attach_proc != NULL && a->attach_proc->attach_proc == a;
}

wm_mode_puppet_result_t wm_arcade_mode_puppet(wm_arcade_actor_t *actor,
                                              uint32_t pcnt) {
    wm_mode_puppet_result_t out;
    uint16_t last;
    const wm_roster_anim_table *stand;

    memset(&out, 0, sizeof out);
    if (!actor) return out;

    /* "if we're attached, don't check anything." */
    if (mutually_attached(actor)) return out;

    /* `#check`: PUPPET_TIME is updated either way, before the branch. */
    last = actor->puppet_time;
    actor->puppet_time = (uint16_t)pcnt;

    if ((uint16_t)((uint16_t)pcnt - last) != 1u) {
        /* `#new_arrival` -- the run starts here, at one tick. */
        actor->puppet_ticks = 1u;
        return out;
    }

    /* "been here awhile" */
    actor->puppet_ticks = (uint16_t)(actor->puppet_ticks + 1u);
    if (actor->puppet_ticks < WM_PUPPET_TIMEOUT) return out;

    /*
     * "bark! Been here too long. glitch to a stand or something."
     *
     * The `.if DEBUG / LOCKUP / .endif` above this is a development
     * assertion compiled out of the shipped ROM, so there is nothing to
     * translate there -- but it is worth knowing the authors treated
     * reaching this point as a bug in the animation that stranded him,
     * not as ordinary play.
     */
    actor->player_mode = WM_PMODE_NORMAL;
    out.glitched_to_stand = true;

    stand = wm_roster_anim_find("#stand_tbl");
    if (stand)
        out.stand_anim = wm_roster_anim_facing(
            stand, (int)actor->wrestler_num,
            (actor->facing_dir & (int32_t)WM_MOVE_UP) != 0);
    return out;
}

void wm_arcade_mode_inair2(wm_arcade_actor_t *actor) {
    uint16_t stick;
    int32_t drift;

    if (!actor) return;
    stick = actor->stick_val_cur;

    /* Z first, up before down, and neither is an else-if in the source:
       each `btst` falls through to the next when clear. */
    drift = 0;
    if (stick & WM_MOVE_UP)         drift = -WM_INAIR2_ZDRIFT;
    else if (stick & WM_MOVE_DOWN)  drift = WM_INAIR2_ZDRIFT;
    actor->z_fixed += drift;
    actor->z_int = actor->z_fixed >> 16;

    drift = 0;
    if (stick & WM_MOVE_LEFT)        drift = -WM_INAIR2_XDRIFT;
    else if (stick & WM_MOVE_RIGHT)  drift = WM_INAIR2_XDRIFT;
    actor->x_fixed += drift;
    actor->x_int = actor->x_fixed >> 16;
}

wm_mode_choking_result_t wm_arcade_mode_choking(wm_arcade_actor_t *actor) {
    wm_mode_choking_result_t out;

    memset(&out, 0, sizeof out);
    if (!actor) return out;

    /* Held: the choker's animation owns him and this does nothing. */
    if (mutually_attached(actor)) return out;

    /* `#fall_out` */
    actor->attach_proc = NULL;
    actor->player_mode = WM_PMODE_NORMAL;
    /* `movi MODE_NORMAL,a0 / move a0,*a13(ANIMODE)` -- the whole word,
       so every animation flag goes with it. */
    actor->anim_mode = 0;
    out.fell_out = true;
    out.kill_endless_sound = true;
    return out;
}
