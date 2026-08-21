#ifndef WMANIA_ATTRACT_SECRET_H
#define WMANIA_ATTRACT_SECRET_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WM_ATTRACT_SECRET_KICK = 0,
    WM_ATTRACT_SECRET_BLOCK,
    WM_ATTRACT_SECRET_POWER_PUNCH,
    WM_ATTRACT_SECRET_PUNCH,
    WM_ATTRACT_SECRET_POWER_KICK,
    WM_ATTRACT_SECRET_OTHER
} WmAttractSecretButton;

typedef struct {
    uint8_t progress;
    uint32_t overall_ticks_left;
    uint32_t per_button_ticks_left;
    uint32_t tsec_ticks;
    bool active;
    bool succeeded;
} WmAttractSecretState;

/*
 * Source octopus_page: total timeout 10*TSEC and each of six required
 * button states must arrive within TSEC/3.
 */
void wm_attract_secret_begin(
    WmAttractSecretState *state,
    uint32_t tsec_ticks);

bool wm_attract_secret_tick(
    WmAttractSecretState *state,
    bool has_button,
    WmAttractSecretButton button);

#ifdef __cplusplus
}
#endif

#endif
