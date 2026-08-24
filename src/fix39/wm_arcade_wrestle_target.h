#ifndef WM_ARCADE_WRESTLE_TARGET_H
#define WM_ARCADE_WRESTLE_TARGET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wm_arcade_closest_world {
    wm_arcade_actor_t **actors;
    size_t actor_count;
    uint32_t pcnt;
} wm_arcade_closest_world_t;

/* Direct translation of WRESTLE.ASM::calc_closest. */
bool wm_arcade_calc_closest(wm_arcade_actor_t *self,
                            const wm_arcade_closest_world_t *world);

/* Direct translation of WRESTLE.ASM::calc_closest2 scheduling gate. */
bool wm_arcade_calc_closest2(wm_arcade_actor_t *self,
                             const wm_arcade_closest_world_t *world);

#ifdef __cplusplus
}
#endif

#endif
