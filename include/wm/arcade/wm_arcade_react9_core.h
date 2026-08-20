#ifndef WM_ARCADE_REACT9_CORE_H
#define WM_ARCADE_REACT9_CORE_H

#include "wm/arcade/wm_arcade_react8_core.h"

#ifdef __cplusplus
extern "C" {
#endif

int wm_arcade_react9_supports(wm_arcade_reaction_id_t reaction);
int wm_arcade_react9_apply(wm_arcade_actor_t *attacker,
                           wm_arcade_actor_t *victim,
                           wm_arcade_reaction_id_t reaction,
                           int16_t *hit_damage_pending,
                           int16_t *new_victim_movedir,
                           wm_arcade_react1_context_t *ctx);

void wm_arcade_react123456789_reaction_callback(wm_arcade_actor_t *attacker,
                                                wm_arcade_actor_t *victim,
                                                wm_arcade_reaction_id_t reaction,
                                                int16_t *hit_damage_pending,
                                                int16_t *new_victim_movedir,
                                                void *user);

#ifdef __cplusplus
}
#endif
#endif
