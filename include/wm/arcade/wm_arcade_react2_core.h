#ifndef WM_ARCADE_REACT2_CORE_H
#define WM_ARCADE_REACT2_CORE_H

#include "wm/arcade/wm_arcade_react1_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Direct C translation boundary for REACT2.ASM.  This chunk intentionally
 * reuses the Stage 3 callback/context adapter because REACT2 calls the
 * REACT1 block handlers and shares several animation tables/sounds.
 */
int wm_arcade_react2_supports(wm_arcade_reaction_id_t reaction);

int wm_arcade_react2_apply(wm_arcade_actor_t *attacker,
                           wm_arcade_actor_t *victim,
                           wm_arcade_reaction_id_t reaction,
                           int16_t *hit_damage_pending,
                           int16_t *new_victim_movedir,
                           wm_arcade_react1_context_t *ctx);

/*
 * Cumulative Stage 4 callback for wm_arcade_react_callbacks.reaction.
 * REACT2-owned reactions are handled first, then the Stage 3 REACT1 core.
 */
void wm_arcade_react12_reaction_callback(wm_arcade_actor_t *attacker,
                                         wm_arcade_actor_t *victim,
                                         wm_arcade_reaction_id_t reaction,
                                         int16_t *hit_damage_pending,
                                         int16_t *new_victim_movedir,
                                         void *user);

#ifdef __cplusplus
}
#endif

#endif
