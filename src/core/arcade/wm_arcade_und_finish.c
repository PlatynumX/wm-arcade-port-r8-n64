/*
 * TAKER.ASM's finishing move -- see wm/arcade/wm_arcade_und_finish.h
 * for why it is the only one in the game.
 */
#include "wm/arcade/wm_arcade_und_finish.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wmania_ring_geometry.h"

#include <stddef.h>

bool wm_arcade_und_finish_allowed(const wm_arcade_actor_t *taker,
                                  const wm_arcade_actor_t *victim,
                                  const wm_arcade_und_finish_env_t *env) {
    if (!taker || !victim || !env) return false;

    /* `calla get_opp_plyrmode / cmpi MODE_DEAD,a0 / jrnz #reset` */
    if (victim->player_mode != WM_PMODE_DEAD) return false;

    /* "Check to make sure this is my 2nd pin attempt": `cmpi 2,a0 /
       jrlt #reset`, so two or more, not exactly two. */
    if (env->my_pins < 2) return false;

    /* "Don't allow the move if I've been turned into a drone by the
       autopin goop": `move *a8(PLYR_TYPE),a14 / jrnz #fi1_exit`. */
    if (env->player_type != 0) return false;

    /* "Don't allow the move if I'm outside the ring": `jrn` is the
       sign test, so a NEGATIVE RING_TIME refuses. */
    if (env->ring_time < 0) return false;

    /* "Don't allow the move if I've already pinned the guy". */
    if (taker->status_flags & WM_STATUS_DID_PIN) return false;

    /* Neither the victim nor the Undertaker may be right of centre plus
       100 -- `cmpi RING_X_CENTER+100,a14 / jrgt`, a SIGNED compare, and
       strictly greater, so exactly the boundary is allowed. The coffin
       comes in from the left, which is why the half matters. */
    if (victim->x_int > WM_UND_FINISH_MAX_X) return false;
    if (taker->x_int > WM_UND_FINISH_MAX_X) return false;

    return true;
}

void wm_arcade_und_shake_world(
        int32_t base_tlx, int32_t base_tly,
        const wm_arcade_und_finish_callbacks_t *cb) {
    int32_t jx, jy;
    uint32_t r;

    if (!cb) return;

    /* `movk 4,a0 / calla RNDRNG0 / movk 2,a11 / sub a0,a11 / sll 16,a11`
       -- RNDRNG0(4) gives 0..4, so 2 minus it is +2..-2, shifted into
       the 16.16 fixed point the world origin is kept in. Both axes
       offset from the ORIGINAL origin the loop captured in a8/a9, not
       from the last jittered one, so it shakes about a fixed point
       instead of wandering. */
    r = cb->rng ? wm_rng_rndrng0(cb->rng, 4) : 2u;
    jx = (2 - (int32_t)r) << 16;
    r = cb->rng ? wm_rng_rndrng0(cb->rng, 4) : 2u;
    jy = (2 - (int32_t)r) << 16;

    if (cb->set_world_origin)
        cb->set_world_origin(base_tlx + jx, base_tly + jy, cb->user);
}

int wm_arcade_und_adjust_view(const wm_arcade_actor_t *taker,
                              const wm_arcade_und_finish_env_t *env,
                              const wm_arcade_und_finish_callbacks_t *cb) {
    int32_t tlx, step, mid;
    int i;

    if (!taker || !env || !cb) return 0;

    /* "If we are positioned within 100 pixels to the left of the right
       edge of the ring don't bother scrolling any further." The compare
       is against WORLDTLX >> 16, the integer scroll position. */
    tlx = env->world_tlx;
    if ((tlx >> 16) >= WM_UND_VIEW_STOP_X) {
        if (cb->start_shake) cb->start_shake(cb->user);
        return 0;
    }

    /* "Get the midpoint between taker and the right edge of ring", then
       the difference from the Undertaker, then `sra 5` -- an arithmetic
       shift, so a negative difference keeps its sign and rounds toward
       negative infinity exactly as the source does. Everything here is
       16.16: OBJ_XPOS, not OBJ_XPOSINT. */
    mid = ((taker->x_fixed + (WM_UND_VIEW_TARGET_X << 16)) >> 1)
        - taker->x_fixed;
    step = mid >> 5;

    /* `movi 32,a9 / #mv_loop ... dsjs a9,#mv_loop` -- 32 iterations,
       one tick apart, each adding the step and refreshing the
       background. The sleep and BGND_UD1 are the caller's; this hands
       out the positions in order. */
    for (i = 0; i < 32; ++i) {
        tlx += step;
        if (cb->set_world_origin)
            cb->set_world_origin(tlx, env->world_tly, cb->user);
    }

    /* `#av_exit: CREATE FIREWRK_PID,shake_world` -- reached whether or
       not the scroll ran, which is why the early exit above starts it
       too. */
    if (cb->start_shake) cb->start_shake(cb->user);
    return 32;
}

const char *wm_arcade_und_finish_move1(
        wm_arcade_actor_t *taker, wm_arcade_actor_t *victim,
        const wm_arcade_und_finish_env_t *env,
        const wm_arcade_und_finish_callbacks_t *cb) {
    if (!wm_arcade_und_finish_allowed(taker, victim, env)) return NULL;

    /* "tell all who care we are going to do a finishing move. This also
       shuts down the scroller." */
    if (cb && cb->set_in_finish_move) cb->set_in_finish_move(1, cb->user);

    /* "clear victim's DO_BUCKOFF bit and set his NO_BUCKOFF bit" -- he
       is not getting out of this one. */
    victim->status_flags &= (uint32_t)~WM_STATUS_DO_BUCKOFF;
    victim->status_flags |= (uint32_t)WM_STATUS_NO_BUCKOFF;

    wm_arcade_und_adjust_view(taker, env, cb);

    /* SLEEPK 15 ("let the dust settle") then SPECIAL_MOVE_ADDR. The
       sleep is the caller's; the animation is the answer. */
    return WM_UND_FINISH_ANIM;
}
