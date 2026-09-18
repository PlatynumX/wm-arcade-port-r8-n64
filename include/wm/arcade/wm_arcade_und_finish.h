#ifndef WM_ARCADE_UND_FINISH_H
#define WM_ARCADE_UND_FINISH_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wmania_rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * TAKER.ASM:527 und_finish_move1 -- the only finishing move in the game.
 *
 * Every wrestler's smove table carries finishing-move entries, and
 * GAME.EQU:580-587 switches seven of the eight off: NUM_TAKER_FINISHES
 * is 1 and the rest are 0, so the assembler skipped fifteen of the
 * sixteen *_finish_move routines along with their table entries. This
 * one it built, and with it the two helpers that live inside the same
 * `.if` block -- adjust_view (:715) and shake_world (:527).
 *
 * The routine is a monitor process: init_smoves spawns it at match
 * start and it sits waiting for UP, then DOWN, then PUNCH, restarting
 * its wait whenever a guard fails. Seven guards, all of them read off
 * the source rather than summarised:
 *
 *   the opponent must be MODE_DEAD;
 *   this must be the wrestler's SECOND pin attempt (p1pins/p2pins >= 2);
 *   the wrestler must not have been turned into a drone by the autopin;
 *   he must not be outside the ring (RING_TIME negative);
 *   he must not have pinned this opponent already (B_DID_PIN);
 *   and NEITHER wrestler may be right of RING_X_CENTER+100 -- the
 *   coffin is off the left of the ring, so the animation only works
 *   from that half.
 *
 * When they all pass it raises in_finish_move (which also shuts the
 * scroller down), takes the victim's buckoff away, scrolls the view to
 * the midpoint, waits 15 ticks and starts und_2_raise_dead_anim.
 */

/* What the routine reads that does not live on the actor. */
typedef struct {
    /* @p1pins / @p2pins, selected by the wrestler's PLYR_SIDE. */
    int32_t my_pins;
    /* *a8(PLYR_TYPE) -- non-zero once the autopin has made him a drone. */
    int32_t player_type;
    /* *a8(RING_TIME) -- negative means outside the ring. */
    int32_t ring_time;
    /* @WORLDTLX / @WORLDTLY, the scroll origin, in 16.16. */
    int32_t world_tlx;
    int32_t world_tly;
    /* @finish_completed, raised by the animation when it is done. */
    bool finish_completed;
} wm_arcade_und_finish_env_t;

typedef struct {
    /* @in_finish_move. Raised before the animation and cleared after;
       the round announcer already reads this (wm_arcade_round_announce). */
    void (*set_in_finish_move)(int on, void *user);
    /* @WORLDTLX / @WORLDTLY writes, each followed by BGND_UD1. */
    void (*set_world_origin)(int32_t tlx, int32_t tly, void *user);
    /* CREATE FIREWRK_PID,shake_world and the KIL1C that ends it. */
    void (*start_shake)(void *user);
    void (*kill_shake)(void *user);
    /* UTIL.ASM RNDRNG0, for shake_world's jitter. */
    WmRng *rng;
    void *user;
} wm_arcade_und_finish_callbacks_t;

/*
 * The seven guards, as one predicate. True when the move may start.
 * `victim` is *a8(WHOIHIT), the opponent he is standing over.
 */
bool wm_arcade_und_finish_allowed(const wm_arcade_actor_t *taker,
                                  const wm_arcade_actor_t *victim,
                                  const wm_arcade_und_finish_env_t *env);

/*
 * The body after the guards pass: raise in_finish_move, take the
 * victim's buckoff away, scroll, and hand back the animation label to
 * start (NULL when the guards refuse).
 *
 * Does NOT sleep: the 15-tick settle and the #fdone_wait poll belong to
 * whatever drives this, the same way every other translated monitor in
 * this port hands its animation back rather than running a scheduler.
 */
const char *wm_arcade_und_finish_move1(
    wm_arcade_actor_t *taker, wm_arcade_actor_t *victim,
    const wm_arcade_und_finish_env_t *env,
    const wm_arcade_und_finish_callbacks_t *cb);

/* TAKER.ASM:715 adjust_view: 32 steps toward the midpoint between the
   Undertaker and the ring's right edge, then start the shaker. Returns
   the number of steps actually taken -- 0 when the view is already
   within 100 pixels of the right edge and it exits early. */
int wm_arcade_und_adjust_view(const wm_arcade_actor_t *taker,
                              const wm_arcade_und_finish_env_t *env,
                              const wm_arcade_und_finish_callbacks_t *cb);

/* TAKER.ASM:527 shake_world, one iteration: jitter the world origin by
   RNDRNG0(4)-2 on each axis about the ORIGINAL origin, not the jittered
   one -- the source keeps the base in a8/a9 for the whole loop. */
void wm_arcade_und_shake_world(int32_t base_tlx, int32_t base_tly,
                                    const wm_arcade_und_finish_callbacks_t *cb);

/* The animation und_finish_move1 starts, TAKER.ASM's own label. */
#define WM_UND_FINISH_ANIM "und_2_raise_dead_anim"

/* TAKER.ASM:718's own constant: the ring's right edge, less 100. */
#define WM_UND_VIEW_STOP_X (1322 - 100)
/* The midpoint the view scrolls to is halfway between the Undertaker
   and x=1200 (TAKER.ASM:726 `addi [1200,0],a14`). */
#define WM_UND_VIEW_TARGET_X 1200
/* Neither wrestler may be right of this (TAKER.ASM:544, :549). */
#define WM_UND_FINISH_MAX_X (WM_RING_X_CENTER + 100)

#ifdef __cplusplus
}
#endif
#endif
