#ifndef WM_ARCADE_REACT7_CORE_H
#define WM_ARCADE_REACT7_CORE_H

#include "wm_arcade_react6_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* REACT7.ASM contains only five legacy bare-RETS stubs. */
void wm_arcade_react7_att30_stub(void);
void wm_arcade_react7_att31_stub(void);
void wm_arcade_react7_att32_stub(void);
void wm_arcade_react7_att33_stub(void);
void wm_arcade_react7_att34_stub(void);

/* Current live hit_table routing remains REACT1..REACT5. */
void wm_arcade_react1234567_reaction_callback(wm_arcade_actor_t *attacker,
                                              wm_arcade_actor_t *victim,
                                              wm_arcade_reaction_id_t reaction,
                                              int16_t *hit_damage_pending,
                                              int16_t *new_victim_movedir,
                                              void *user);

#ifdef __cplusplus
}
#endif
#endif
