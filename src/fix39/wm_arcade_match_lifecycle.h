#ifndef WM_ARCADE_MATCH_LIFECYCLE_H
#define WM_ARCADE_MATCH_LIFECYCLE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wm_arcade_combat.h"
#define WM_ARCADE_TSEC 53u
typedef struct {
    uint8_t tens, ones;
    uint16_t fraction;
    uint16_t step;
    uint16_t startup_delay;
    uint16_t pin_timeout;
    uint32_t last_dead_pcnt;
    bool halt;
    bool attract_wrap;
    bool round_award_pending;
} wm_arcade_match_lifecycle;
void wm_arcade_match_lifecycle_init(wm_arcade_match_lifecycle *m, bool attract_wrap);
void wm_arcade_match_lifecycle_tick(wm_arcade_match_lifecycle *m, wm_arcade_actor_t **actors, size_t actor_count, uint32_t pcnt);
bool wm_arcade_match_clock_zero(const wm_arcade_match_lifecycle *m);
#endif
