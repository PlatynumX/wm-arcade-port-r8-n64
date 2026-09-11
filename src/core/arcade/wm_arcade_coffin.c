/*
 * FINISEQ.ASM's Undertaker coffin sequence -- the decision-making half.
 * See include/wm/arcade/wm_arcade_coffin.h for what is here and what is
 * deliberately left to the display layer.
 */
#include "wm/arcade/wm_arcade_coffin.h"

#include <stddef.h>

void wm_coffin_reset(wm_coffin_state_t *st) {
    if (!st) return;
    st->close_the_door = 0;
    st->close_the_floor = 0;
    st->guy_in = false;
    st->finish_completed = 0;
    st->dead_wrestler = NULL;
}

/*
 * TAKER.ASM:661 latches @dead_wrestler from WHOIHIT before the finish
 * starts, so the two are the same wrestler by construction; the latch
 * is preferred because the source reads the latch.
 */
wm_arcade_actor_t *wm_coffin_dead_wrestler(const wm_coffin_state_t *st,
                                           wm_arcade_actor_t *asker) {
    if (st && st->dead_wrestler) return st->dead_wrestler;
    return asker ? asker->who_i_hit : NULL;
}

/* FINISEQ.ASM:915 close_door: `movk 2,a14 / move a14,@close_the_door`. */
void wm_coffin_close_door(wm_coffin_state_t *st) {
    if (!st) return;
    st->close_the_door = 2;
}

/* FINISEQ.ASM:1550 guy_is_in. */
void wm_coffin_guy_is_in(wm_coffin_state_t *st) {
    if (!st) return;
    st->guy_in = true;
}

/* FINISEQ.ASM:924 is_door_open -- `jrz`, so 1, 2 and 3 all mean open. */
bool wm_coffin_is_door_open(const wm_coffin_state_t *st) {
    return st && st->close_the_door != 0;
}

/* FINISEQ.ASM:956 is_he_in. */
bool wm_coffin_is_he_in(const wm_coffin_state_t *st) {
    return st && st->guy_in;
}

/* FINISEQ.ASM:940 is_guy_up, reading what anim_code's guy_is_up set. */
bool wm_coffin_is_guy_up(const wm_arcade_actor_t *dead) {
    return dead && (dead->status_flags & WM_STATUS_GUY_UP) != 0;
}

/*
 * FINISEQ.ASM:985 make_wres_disappear. `cmpi 3,a14 / jrnz #not_time`:
 * an equality, so the poll answers no again if the state ever moved
 * past 3 -- it cannot here, 3 is terminal for @close_the_door, but the
 * test is transcribed as written rather than relaxed to >=.
 */
bool wm_coffin_make_wres_disappear(const wm_coffin_state_t *st,
                                   const char **anim) {
    if (anim) *anim = NULL;
    if (!st || st->close_the_door != 3) return false;
    if (anim) *anim = WM_COFFIN_DISAPPEAR_ANIM;
    return true;
}

/*
 * FINISEQ.ASM:1560 set_speeds.
 *
 * Both halves are `sub / sll 16 / divs TIME_FOR_MOVE` on an ODD
 * register (a1), which on the TMS34010 is a 32-bit signed divide
 * rather than the 64-bit one an even register would take. The shift
 * happens BEFORE the divide, so the quotient lands directly in 16.16
 * with no second scaling.
 */
void wm_coffin_set_speeds(wm_arcade_actor_t *dead) {
    int32_t delta;

    if (!dead) return;

    delta = (int32_t)(WM_COFFIN_XPOS - dead->x_int);
    dead->x_vel = (int32_t)((uint32_t)delta << 16) / WM_COFFIN_TIME_FOR_MOVE;

    /* `cmpi WRES_Z,a2 / jrge #is_in_front` -- the clamp runs only when
       he is BEHIND WRES_Z, and it rewrites the position he is about to
       be measured from, which is what makes the resulting velocity 0. */
    if (dead->z_int < WM_COFFIN_WRES_Z) {
        dead->z_int = WM_COFFIN_WRES_Z;
        dead->z_fixed = (int32_t)WM_COFFIN_WRES_Z << 16;
    }
    delta = (int32_t)(WM_COFFIN_WRES_Z - dead->z_int);
    dead->z_vel = (int32_t)((uint32_t)delta << 16) / WM_COFFIN_TIME_FOR_MOVE;
}

/*
 * do_up_coffin `#mv_up_lp` (FINISEQ.ASM:726). Equality against 136, as
 * the source writes it: `cmpi 136,a14 / jrz #open_lp`.
 */
bool wm_coffin_rise_step(int32_t *sizey, int32_t *ypos) {
    if (!sizey || !ypos) return false;
    if (*sizey == WM_COFFIN_FULL_SIZEY) return false;
    *sizey += WM_COFFIN_VEL;
    *ypos -= WM_COFFIN_VEL;
    return true;
}

/*
 * do_up_coffin `#mv_dn_lp` (FINISEQ.ASM:838). `cmpi 1,a14 / jrle`, so
 * the test is <= 1 and not == 0; from 136 in steps of 4 it stops on 0
 * either way, but a size that is not a multiple of 4 would stop one
 * step earlier here and never at all in the rise above.
 */
bool wm_coffin_fall_step(int32_t *sizey, int32_t *ypos) {
    if (!sizey || !ypos) return false;
    if (*sizey <= 1) return false;
    *sizey -= WM_COFFIN_VEL;
    *ypos += WM_COFFIN_VEL;
    return true;
}

/*
 * hover_coffin `#do_agin` (FINISEQ.ASM:883). One iteration; the caller
 * supplies the 7-tick sleep.
 */
int wm_coffin_hover_step(int32_t *delta, int32_t *back_y, int32_t *front_y,
                         int32_t front_start_y) {
    int32_t off;

    if (!delta || !back_y || !front_y) return 0;

    *back_y += *delta;
    *front_y += *delta;

    /* `sub a11,a14 / abs a14 / cmpi 2,a14 / jrle #do_agin` -- measured
       on the front object only, and the turn needs strictly more
       than 2. */
    off = *front_y - front_start_y;
    if (off < 0) off = -off;
    if (off <= 2) return 0;

    /* `move a8,a8 / jrn #reset_a8` -- a negative delta goes back to the
       +1 entry point and makes no smoke. */
    if (*delta < 0) {
        *delta = WM_COFFIN_HOVER_DOWN;
        return 0;
    }
    *delta = WM_COFFIN_HOVER_UP;
    return WM_COFFIN_HOVER_PUFFS;
}

/*
 * do_up_coffin `#shk_lp` (FINISEQ.ASM:817). The draws are in the
 * source's order, X first, because they share one RNDRNG0 stream.
 */
void wm_coffin_shake_offset(WmRng *rng, int32_t base_x, int32_t base_y,
                            int32_t *out_x, int32_t *out_y) {
    if (out_x) {
        int32_t r = (int32_t)wm_rng_rndrng0(rng, 4);
        *out_x = base_x + (r - 2);
    }
    if (out_y) {
        /* RNDRNG0(2), not (4): the Y jitter is -2..0. */
        int32_t r = (int32_t)wm_rng_rndrng0(rng, 2);
        *out_y = base_y + (r - 2);
    }
}

/* FINISEQ.ASM:474 ltl_exp's six draws, in order. */
void wm_coffin_ltl_exp_params(WmRng *rng, wm_coffin_puff_t *out) {
    if (!out) return;
    out->delay = (int32_t)wm_rng_rndrng0(rng, 10) + 1;
    out->x     = (int32_t)wm_rng_rndrng0(rng, 80) - 40 + WM_COFFIN_HOLE_XPOS;
    out->y     = (int32_t)wm_rng_rndrng0(rng, 30) - 25 + WM_COFFIN_HOLE_YPOS;
    out->z     = (int32_t)wm_rng_rndrng0(rng, 10) + WM_COFFIN_EXP_Z;
    out->vel   = (int32_t)wm_rng_rndrng0(rng, 7) + 2;
    /* `movk 1,a0 / RNDRNG0 / sll 5,a0 / add a0,a9` -- the shift is the
       32-bit stride of a `.long` table entry, so the draw is the
       index: 0 or 1. */
    out->which = (int32_t)wm_rng_rndrng0(rng, 1);
}

const char *const wm_coffin_mat_frames[] = {
    "MATCOF01", "MATCOF02", "MATCOF03", "MATCOF04",
};
const size_t wm_coffin_mat_frame_count =
    sizeof(wm_coffin_mat_frames) / sizeof(wm_coffin_mat_frames[0]);

const char *const wm_coffin_door_frames[] = {
    "COFFIN02", "COFFIN03", "COFFIN04", "COFFIN05",
};
const size_t wm_coffin_door_frame_count =
    sizeof(wm_coffin_door_frames) / sizeof(wm_coffin_door_frames[0]);

const char *const wm_coffin_tstone_frames[] = {
    "TMBSTN02", "TMBSTN03", "TMBSTN04", "TMBSTN05",
    "TMBSTN06", "TMBSTN07", "TMBSTN08",
};
const size_t wm_coffin_tstone_frame_count =
    sizeof(wm_coffin_tstone_frames) / sizeof(wm_coffin_tstone_frames[0]);

const char *const wm_coffin_exp1_frames[] = {
    "SMOKE01", "SMOKE02", "SMOKE03", "SMOKE04", "SMOKE05",
    "SMOKE06", "SMOKE07", "SMOKE08", "SMOKE09", "SMOKE10",
};
const char *const wm_coffin_exp2_frames[] = {
    "SMOKEB01", "SMOKEB02", "SMOKEB03", "SMOKEB04", "SMOKEB05",
    "SMOKEB06", "SMOKEB07", "SMOKEB08", "SMOKEB09", "SMOKEB10",
};
const size_t wm_coffin_exp_frame_count =
    sizeof(wm_coffin_exp1_frames) / sizeof(wm_coffin_exp1_frames[0]);

/* `move *-a9,a0,L` from the terminator label back to the list head. */
const char *wm_coffin_close_frame(const char *const *frames, size_t count,
                                  size_t i) {
    if (!frames || i >= count) return NULL;
    return frames[count - 1 - i];
}
