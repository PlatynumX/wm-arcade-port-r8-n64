/*
 * WRESTLE2.ASM's roster predicates -- the questions a wrestler asks
 * about everyone else in the ring.
 *
 * Four of these are the same shape: walk process_ptrs, skip empty
 * slots, skip yourself, skip the other side, then test one thing. The
 * source writes each one out separately and they differ only in that
 * last test, so they are four functions here too rather than one with
 * a mode argument -- the difference between them is the point.
 *
 * `ck_live_teammates` is what wm_arcade_combat.h's
 * `victim_has_live_teammates` seam has been asking for.
 */
#ifndef WM_ARCADE_TEAMMATES_H
#define WM_ARCADE_TEAMMATES_H

#include <stdbool.h>
#include <stddef.h>

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GAME.EQU:438 -- "number of wrestlers in game (inc ref.)". Every one
 * of these loops is `movk NUM_WRES` slots long. */
#define WM_NUM_WRES 7

/*
 * ck_teammate_pin (WRESTLE2.ASM:3253): has one of my teammates pinned
 * someone this round? Reads STATUS_FLAGS' B_DID_PIN.
 */
bool wm_ck_teammate_pin(const wm_arcade_actor_t *self,
                        wm_arcade_actor_t *const *actors, size_t actor_count);

/*
 * ck_live_teammates (WRESTLE2.ASM:3601): do I have a teammate who is
 * not in MODE_DEAD?
 */
bool wm_ck_live_teammates(const wm_arcade_actor_t *self,
                          wm_arcade_actor_t *const *actors,
                          size_t actor_count);

/*
 * ck_any_teammates (WRESTLE2.ASM:3636): do I have a teammate at all,
 * living or dead? Identical to the above minus the mode test.
 */
bool wm_ck_any_teammates(const wm_arcade_actor_t *self,
                         wm_arcade_actor_t *const *actors,
                         size_t actor_count);

/*
 * is_a14_behind (WRESTLE2.ASM:4591): is `other` behind me?
 *
 * The source subtracts and branches on sign, so an opponent at exactly
 * my X counts as being on my LEFT (`jrn` is taken only for a strictly
 * negative difference). The two halves then test opposite bits of
 * FACING_DIR -- MOVE_LEFT_BIT on the left, MOVE_RIGHT_BIT on the
 * right -- which is what makes a diagonal facing answer sensibly: a
 * wrestler facing up-left has MOVE_LEFT set and so is facing an
 * opponent on his left.
 */
bool wm_is_behind(const wm_arcade_actor_t *self,
                  const wm_arcade_actor_t *other);

/*
 * clr_climb (WRESTLE2.ASM:3236): stop climbing through the ropes.
 * SAFE_TIME is set to 1, not zero -- one tick of grace, which is what
 * keeps the wrestler from being immediately re-caught by whatever he
 * just climbed out of.
 */
void wm_clr_climb(wm_arcade_actor_t *self);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_TEAMMATES_H */
