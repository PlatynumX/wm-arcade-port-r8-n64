#ifndef WMANIA_ROPE_SPAWN_H
#define WMANIA_ROPE_SPAWN_H

#include "wmania_rope_command.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WM_FIX39_ROPE_CHANNEL_RED = 0,
    WM_FIX39_ROPE_CHANNEL_WHITE = 1,
    WM_FIX39_ROPE_CHANNEL_BLUE = 2,
    WM_FIX39_ROPE_CHANNEL_SHADOW = 3,
    WM_FIX39_ROPE_CHANNEL_COUNT = 4
} WmRopeChannel;

typedef enum {
    WM_FIX39_ROPE_HALF_FIRST = 0,
    WM_FIX39_ROPE_HALF_SECOND = 1
} WmRopeHalf;

/*
 * Portable form of one LWWWWW object seed from the live ROPES.ASM
 * front/back/left/right position tables.
 *
 * raw_x/raw_y are the literal position-table values after arithmetic in the
 * source expression.  BEGINOBJ receives:
 *   x = (raw_x + 104) << 16
 *   y = (raw_y - 258) << 16
 */
typedef struct {
    const char *source_image_symbol;
    bool exists;
    bool flip_horizontal;
    bool source_horizontal_rope;
    bool source_side_rope;
    int16_t raw_x;
    int16_t raw_y;
    uint16_t raw_z;
} WmRopeObjectSeed;

typedef struct {
    WmRopeBank bank;
    bool horizontal_bank;
    bool has_shadow_channel;
    WmRopeObjectSeed object[WM_FIX39_ROPE_CHANNEL_COUNT][2];
} WmRopeBankSeed;

extern const WmRopeBankSeed wm_rope_bank_seeds[4];

const WmRopeBankSeed *wm_rope_bank_seed(WmRopeBank bank);
const WmRopeObjectSeed *wm_rope_object_seed(
    WmRopeBank bank,
    WmRopeChannel channel,
    WmRopeHalf half);

int32_t wm_rope_spawn_x_fp16(const WmRopeObjectSeed *seed);
int32_t wm_rope_spawn_y_fp16(const WmRopeObjectSeed *seed);

/*
 * ROPES.ASM creates the objects first, sleeps two ticks, then if reduce_bog
 * is nonzero kills only front/back rope PROCESSES and clears front_rproc /
 * back_rproc.  The already-created rope objects remain represented here.
 */
bool wm_rope_process_survives_reduce_bog(
    WmRopeBank bank,
    bool reduce_bog);

#ifdef __cplusplus
}
#endif

#endif
