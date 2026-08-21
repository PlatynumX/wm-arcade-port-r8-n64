#ifndef WM_ARCADE_REACT8_CORE_H
#define WM_ARCADE_REACT8_CORE_H

#include "wm_arcade_react7_core.h"

#ifdef __cplusplus
extern "C" {
#endif

int wm_arcade_react8_supports(wm_arcade_reaction_id_t reaction);
int wm_arcade_react8_apply(wm_arcade_actor_t *attacker,
                           wm_arcade_actor_t *victim,
                           wm_arcade_reaction_id_t reaction,
                           int16_t *hit_damage_pending,
                           int16_t *new_victim_movedir,
                           wm_arcade_react1_context_t *ctx);

/*
 * REACT8.ASM contains a legacy hit_flyelbow body labelled as attack 39,
 * but the current PLYR.EQU/REACT1 hit table routes attack 39 to
 * hit_combo_uprcut.  Keep the body available for source parity but do not
 * install it in the live attack table.
 */
void wm_arcade_react8_legacy_flyelbow(wm_arcade_actor_t *attacker,
                                      wm_arcade_actor_t *victim,
                                      wm_arcade_react1_context_t *ctx);

void wm_arcade_react12345678_reaction_callback(wm_arcade_actor_t *attacker,
                                               wm_arcade_actor_t *victim,
                                               wm_arcade_reaction_id_t reaction,
                                               int16_t *hit_damage_pending,
                                               int16_t *new_victim_movedir,
                                               void *user);

#ifdef __cplusplus
}
#endif
#endif
