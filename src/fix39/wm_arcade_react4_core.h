#ifndef WM_ARCADE_REACT4_CORE_H
#define WM_ARCADE_REACT4_CORE_H

#include "wm_arcade_react3_core.h"

#ifdef __cplusplus
extern "C" {
#endif

int wm_arcade_react4_supports(wm_arcade_reaction_id_t reaction);
int wm_arcade_react4_apply(wm_arcade_actor_t *attacker,
                           wm_arcade_actor_t *victim,
                           wm_arcade_reaction_id_t reaction,
                           int16_t *hit_damage_pending,
                           int16_t *new_victim_movedir,
                           wm_arcade_react1_context_t *ctx);

/* Cumulative REACT1..REACT4 callback (Stages 3..6). */
void wm_arcade_react1234_reaction_callback(wm_arcade_actor_t *attacker,
                                           wm_arcade_actor_t *victim,
                                           wm_arcade_reaction_id_t reaction,
                                           int16_t *hit_damage_pending,
                                           int16_t *new_victim_movedir,
                                           void *user);

#ifdef __cplusplus
}
#endif
#endif
