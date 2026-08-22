#include "wmania_hiscore_counter.h"

void wm_hs_counter_put(WmHsResetCounter *counter, uint32_t value)
{
    counter->value = value;
    counter->verifier = ~value;
}

void wm_hs_counter_init(WmHsResetCounter *counter, uint32_t adjusted_value)
{
    wm_hs_counter_put(counter, adjusted_value);
}

uint32_t wm_hs_counter_get(
    WmHsResetCounter *counter,
    uint32_t adjusted_value,
    bool *was_repaired)
{
    bool bad = counter->verifier != ~counter->value;

    if (bad) {
        wm_hs_counter_put(counter, adjusted_value);
    }

    if (was_repaired != 0) {
        *was_repaired = bad;
    }

    return counter->value;
}

uint32_t wm_hs_counter_decrement(
    WmHsResetCounter *counter,
    uint32_t adjusted_value)
{
    uint32_t value = wm_hs_counter_get(counter, adjusted_value, 0);

    if (value > 0u) {
        --value;
        wm_hs_counter_put(counter, value);
    }

    return value;
}

uint32_t wm_hs_counter_delay(
    WmHsResetCounter *counter,
    uint32_t adjusted_value)
{
    uint32_t value = wm_hs_counter_get(counter, adjusted_value, 0);

    if (value < WM_HS_RESET_MINIMUM) {
        value = WM_HS_RESET_MINIMUM;
        wm_hs_counter_put(counter, value);
    }

    return value;
}
