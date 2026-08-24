#ifndef WM_ARCADE_WRESTLE_INPUT_H
#define WM_ARCADE_WRESTLE_INPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm_arcade_roster.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WRESTLE.ASM dtime banks -- source explicitly says KEEP THIS ORDER. */
typedef enum wm_arcade_joy_dtime_index {
    WM_DTIME_UP = 0,
    WM_DTIME_DOWN,
    WM_DTIME_LEFT,
    WM_DTIME_RIGHT,
    WM_DTIME_PUNCH,
    WM_DTIME_BLOCK,
    WM_DTIME_SPUNCH,
    WM_DTIME_KICK,
    WM_DTIME_SKICK,
    WM_DTIME_COUNT
} wm_arcade_joy_dtime_index_t;

void wm_arcade_wrestle_input_init(wm_arcade_actor_t *actor);
void wm_arcade_update_joystat(wm_arcade_actor_t *actor,
                              uint16_t round_tick,
                              bool halt);
void wm_arcade_update_joy_dtime(wm_arcade_actor_t *actor);

uint16_t wm_arcade_get_joy_dtime(const wm_arcade_actor_t *actor,
                                 wm_arcade_joy_dtime_index_t which);
uint16_t wm_arcade_get_button_dtime(const wm_arcade_actor_t *actor,
                                    uint16_t source_button_bit);

bool wm_arcade_wrestle_pattern_match(
    const wm_arcade_actor_t *actor,
    const wm_arcade_input_step_t *steps,
    size_t step_count,
    uint16_t max_ticks,
    uint16_t round_tick);

#ifdef __cplusplus
}
#endif
#endif
