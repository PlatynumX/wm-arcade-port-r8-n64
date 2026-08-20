#ifndef WMANIA_ATTRACT_OPERATOR_H
#define WMANIA_ATTRACT_OPERATOR_H

#include "wmania_attract_data.h"
#include "wmania_rng.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /*
     * Source CUSTOM_MESSAGE buffer plus external CMESS_LINES and
     * CMESS_LINE_SIZE. These constants belong to the shared operator system,
     * not ATTRACT.ASM, so the merge supplies them instead of guessing.
     */
    const uint8_t *bytes;
    uint16_t line_count;
    uint16_t line_size;
} WmAttractOperatorMessage;

typedef struct {
    int32_t x_fp;
    int32_t y_fp;
    int32_t vx_fp;
    int32_t vy_fp;
    bool active;
} WmAttractBall;

/* Returns true if any source custom-message line contains a nonzero byte. */
bool wm_attract_operator_has_message(
    const WmAttractOperatorMessage *message);

/*
 * Source dan_test creates 32 BALLD05A objects, one per tick. The exact
 * RNDRNG0 implementation is supplied by the shared WmRng state.
 */

void wm_attract_operator_init_balls(
    WmAttractBall balls[WM_ATTRACT_OPERATOR_BALL_COUNT],
    WmRng *rng);

/*
 * Advance one portable simulation tick. Positions use 16.16 fixed point,
 * matching the source velocity scale. Bounce thresholds are 400x255.
 */
void wm_attract_operator_tick_balls(
    WmAttractBall balls[WM_ATTRACT_OPERATOR_BALL_COUNT]);

#ifdef __cplusplus
}
#endif

#endif
