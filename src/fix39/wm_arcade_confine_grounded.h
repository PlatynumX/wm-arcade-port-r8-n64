#ifndef WM_ARCADE_CONFINE_GROUNDED_H
#define WM_ARCADE_CONFINE_GROUNDED_H

#include <stdbool.h>
#include <stdint.h>

#include "wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Source-safe slice of WRESTLE.ASM::confine_wrestler.
 *
 * This helper deliberately runs only in the state where the omitted source
 * side effects are provably unreachable: grounded, unattached, non-zombie,
 * in-ring wrestler with all opponents also inside the ring. In that state the
 * WRESTLE2 climb-out calls return immediately and the rope-bounce airborne
 * branch is not entered, so the boundary correction below is the literal
 * source result rather than an approximation.
 *
 * Returns true when the source-safe preconditions were met. CAN_MOVE_DIR is
 * written to actor->can_move_dir exactly as the source does.
 */
bool wm_arcade_confine_grounded_inring(wm_arcade_actor_t *actor,
                                        bool any_opponent_outside);

#ifdef __cplusplus
}
#endif

#endif
