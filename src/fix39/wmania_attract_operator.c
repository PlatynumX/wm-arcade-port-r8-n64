#include "wmania_attract_operator.h"

#include <string.h>

bool wm_attract_operator_has_message(
    const WmAttractOperatorMessage *message)
{
    uint16_t line;
    uint16_t c;

    if (message == 0 || message->bytes == 0 ||
        message->line_count == 0u || message->line_size == 0u) {
        return false;
    }

    for (line = 0u; line < message->line_count; ++line) {
        const uint8_t *src =
            message->bytes + (size_t)line * message->line_size;

        /*
         * show_operatormsg tests the first byte of each CMOS line to decide
         * whether any operator message exists.
         */
        if (src[0] != 0u) {
            return true;
        }

        /*
         * This loop intentionally does not infer printable content deeper
         * in a line if byte zero is NUL; the source cmlp only calls RC_BYTEI
         * once per line before advancing CMESS_LINE_SIZE.
         */
        for (c = 1u; c < message->line_size; ++c) {
            (void)src[c];
        }
    }

    return false;
}

void wm_attract_operator_init_balls(
    WmAttractBall balls[WM_ATTRACT_OPERATOR_BALL_COUNT],
    WmRng *rng)
{
    size_t i;

    memset(balls, 0,
           sizeof(WmAttractBall) * WM_ATTRACT_OPERATOR_BALL_COUNT);

    for (i = 0; i < WM_ATTRACT_OPERATOR_BALL_COUNT; ++i) {
        /*
         * Exact ONE_BALL source calls:
         *
         * movi 80000h,a0 / RNDRNG0 / subi 40000h -> XVEL
         * movi 60000h,a0 / RNDRNG0 / subi 30000h -> YVEL
         * movi [255,0],a0 / RNDRNG0             -> YPOS (16.16)
         * movi [400,0],a0 / RNDRNG0             -> XPOS (16.16)
         *
         * RNDRNG0 is inclusive, so positive velocity endpoints are legal.
         * [255,0] and [400,0] are fixed-point maxima; the source does NOT
         * choose integer pixels and shift them afterward.
         */
        balls[i].vx_fp =
            (int32_t)wm_rng_rndrng0(rng, 0x00080000u) - 0x00040000;
        balls[i].vy_fp =
            (int32_t)wm_rng_rndrng0(rng, 0x00060000u) - 0x00030000;
        balls[i].y_fp =
            (int32_t)wm_rng_rndrng0(rng, 255u << 16);
        balls[i].x_fp =
            (int32_t)wm_rng_rndrng0(rng, 400u << 16);
        balls[i].active = true;
    }
}

void wm_attract_operator_tick_balls(
    WmAttractBall balls[WM_ATTRACT_OPERATOR_BALL_COUNT])
{
    size_t i;
    const int32_t xmax = WM_ATTRACT_OPERATOR_BALL_X_LIMIT << 16;
    const int32_t ymax = WM_ATTRACT_OPERATOR_BALL_Y_LIMIT << 16;

    for (i = 0; i < WM_ATTRACT_OPERATOR_BALL_COUNT; ++i) {
        WmAttractBall *b = &balls[i];

        if (!b->active) {
            continue;
        }

        b->x_fp += b->vx_fp;
        b->y_fp += b->vy_fp;

        if (b->x_fp < 0) {
            b->x_fp = 0;
            if (b->vx_fp < 0) b->vx_fp = -b->vx_fp;
        } else if (b->x_fp >= xmax) {
            b->x_fp = xmax - 1;
            if (b->vx_fp > 0) b->vx_fp = -b->vx_fp;
        }

        if (b->y_fp < 0) {
            b->y_fp = 0;
            if (b->vy_fp < 0) b->vy_fp = -b->vy_fp;
        } else if (b->y_fp >= ymax) {
            b->y_fp = ymax - 1;
            if (b->vy_fp > 0) b->vy_fp = -b->vy_fp;
        }
    }
}
