#ifndef WM_ARCADE_REACT5_CORE_H
#define WM_ARCADE_REACT5_CORE_H

#include "wm_arcade_react4_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Direct REACT5.ASM good_run_hit translation. */
int wm_arcade_react5_good_run_hit(const wm_arcade_actor_t *attacker,
                                  const wm_arcade_actor_t *victim);
int wm_arcade_react5_good_run_hit_callback(wm_arcade_actor_t *attacker,
                                           wm_arcade_actor_t *victim,
                                           void *user);

int wm_arcade_react5_supports(wm_arcade_reaction_id_t reaction);
int wm_arcade_react5_apply(wm_arcade_actor_t *attacker,
                           wm_arcade_actor_t *victim,
                           wm_arcade_reaction_id_t reaction,
                           int16_t *hit_damage_pending,
                           int16_t *new_victim_movedir,
                           wm_arcade_react1_context_t *ctx);

/* Cumulative REACT1..REACT5 callback (Stages 3..7). */
void wm_arcade_react12345_reaction_callback(wm_arcade_actor_t *attacker,
                                            wm_arcade_actor_t *victim,
                                            wm_arcade_reaction_id_t reaction,
                                            int16_t *hit_damage_pending,
                                            int16_t *new_victim_movedir,
                                            void *user);

#ifdef __cplusplus
}
#endif
#endif
