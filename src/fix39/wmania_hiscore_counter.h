#ifndef WMANIA_HISCORE_COUNTER_H
#define WMANIA_HISCORE_COUNTER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WM_HS_RESET_MINIMUM 750u

typedef struct {
    uint32_t value;
    uint32_t verifier;
} WmHsResetCounter;

/* INIT_HSR / PUT_HSC */
void wm_hs_counter_init(WmHsResetCounter *counter, uint32_t adjusted_value);
void wm_hs_counter_put(WmHsResetCounter *counter, uint32_t value);

/* GET_HSC: invalid complement resets to adjusted_value. */
uint32_t wm_hs_counter_get(
    WmHsResetCounter *counter,
    uint32_t adjusted_value,
    bool *was_repaired);

/* DEC_HSR: called on each start/continue; never decrements below zero. */
uint32_t wm_hs_counter_decrement(
    WmHsResetCounter *counter,
    uint32_t adjusted_value);

/* DELAY_HSRESET: force remaining count to at least HS_MIN (750). */
uint32_t wm_hs_counter_delay(
    WmHsResetCounter *counter,
    uint32_t adjusted_value);

#ifdef __cplusplus
}
#endif

#endif
