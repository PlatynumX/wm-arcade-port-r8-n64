#include "wm/arcade/wmania_attract_secret.h"

#include <string.h>

static const WmAttractSecretButton sequence[6] = {
    WM_ATTRACT_SECRET_KICK,
    WM_ATTRACT_SECRET_BLOCK,
    WM_ATTRACT_SECRET_POWER_PUNCH,
    WM_ATTRACT_SECRET_PUNCH,
    WM_ATTRACT_SECRET_BLOCK,
    WM_ATTRACT_SECRET_POWER_KICK
};

void wm_attract_secret_begin(
    WmAttractSecretState *state,
    uint32_t tsec_ticks)
{
    memset(state, 0, sizeof(*state));
    state->tsec_ticks = tsec_ticks;
    state->overall_ticks_left = tsec_ticks * 10u;
    state->per_button_ticks_left = tsec_ticks / 3u;
    if (state->per_button_ticks_left == 0u) {
        state->per_button_ticks_left = 1u;
    }
    state->active = true;
}

bool wm_attract_secret_tick(
    WmAttractSecretState *state,
    bool has_button,
    WmAttractSecretButton button)
{
    if (state == 0 || !state->active || state->succeeded) {
        return state != 0 && state->succeeded;
    }

    if (state->overall_ticks_left == 0u) {
        state->active = false;
        return false;
    }
    --state->overall_ticks_left;

    if (has_button && button == sequence[state->progress]) {
        ++state->progress;
        if (state->progress >= 6u) {
            state->succeeded = true;
            state->active = false;
            return true;
        }

        state->per_button_ticks_left = state->tsec_ticks / 3u;
        if (state->per_button_ticks_left == 0u) {
            state->per_button_ticks_left = 1u;
        }
        return false;
    }

    if (state->per_button_ticks_left > 0u) {
        --state->per_button_ticks_left;
    }

    /*
     * Source wait_but does not restart the sequence on wrong input; it
     * continues sampling until the per-button timeout expires.
     */
    if (state->per_button_ticks_left == 0u) {
        state->active = false;
    }

    return false;
}
