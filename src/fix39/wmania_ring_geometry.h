#ifndef WMANIA_RING_GEOMETRY_H
#define WMANIA_RING_GEOMETRY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Direct translation of the ACTIVE constants in RING.EQU.
 *
 * NOTE: RING.ASM is explicitly marked "This entire ASM file is no longer
 * required."  Do not use its old pregenerated line tables as authority.
 */

#define WM_RING_Y_SCALE_MULTIPLIER 0x3566
#define WM_RING_X_CENTER (0x0400 + 50)
#define WM_RING_Z_CENTER 0x04a0

#define WM_ARENA_BOT_LEFT   (383 - 54)
#define WM_ARENA_TOP_LEFT   (668 - 50)
#define WM_ARENA_LEFT_WIDTH (WM_ARENA_TOP_LEFT - WM_ARENA_BOT_LEFT)
#define WM_ARENA_TOP_RIGHT  (1686 + 50)
#define WM_ARENA_BOT_RIGHT  (1939 + 50)
#define WM_ARENA_RIGHT_WIDTH (WM_ARENA_BOT_RIGHT - WM_ARENA_TOP_RIGHT)
#define WM_ARENA_TOP 0x0250
#define WM_ARENA_BOT 0x0768
#define WM_ARENA_DEPTH (WM_ARENA_BOT - WM_ARENA_TOP)

#define WM_RING_BOT_LEFT   (805 - 10 + 10)
#define WM_RING_TOP_LEFT   (856 - 10 + 10)
#define WM_RING_LEFT_WIDTH (WM_RING_TOP_LEFT - WM_RING_BOT_LEFT)
#define WM_RING_TOP_RIGHT  (1292 + 5)
#define WM_RING_BOT_RIGHT  (1343 + 5)
#define WM_RING_RIGHT_WIDTH (WM_RING_BOT_RIGHT - WM_RING_TOP_RIGHT)
#define WM_RING_TOP 1023
#define WM_RING_BOT 1345
#define WM_RING_DEPTH (WM_RING_BOT - WM_RING_TOP)

#define WM_MAT_BOT_LEFT   (709 - 10)
#define WM_MAT_TOP_LEFT   (798 - 10)
#define WM_MAT_LEFT_WIDTH (WM_MAT_TOP_LEFT - WM_MAT_BOT_LEFT)
#define WM_MAT_TOP_RIGHT  (1350 + 7)
#define WM_MAT_BOT_RIGHT  (1439 + 7)
#define WM_MAT_RIGHT_WIDTH (WM_MAT_BOT_RIGHT - WM_MAT_TOP_RIGHT)
#define WM_MAT_TOP 0x03c7
#define WM_MAT_BOT 0x05f1
#define WM_MAT_DEPTH (WM_MAT_BOT - WM_MAT_TOP)

#define WM_MAT2_BOT_LEFT   (668 + 10)
#define WM_MAT2_TOP_LEFT   (757 + 15)
#define WM_MAT2_LEFT_WIDTH (WM_MAT2_TOP_LEFT - WM_MAT2_BOT_LEFT)
#define WM_MAT2_TOP_RIGHT  (1396 - 15)
#define WM_MAT2_BOT_RIGHT  (1485 - 10)
#define WM_MAT2_RIGHT_WIDTH (WM_MAT2_BOT_RIGHT - WM_MAT2_TOP_RIGHT)
#define WM_MAT2_TOP (WM_MAT_TOP - 5)
#define WM_MAT2_BOT (WM_MAT_BOT + 5)
#define WM_MAT2_DEPTH (WM_MAT2_BOT - WM_MAT2_TOP)

#define WM_MAT_Y 62

typedef enum {
    WM_RING_BOUNDARY_RIGHT_ROPE = 0,
    WM_RING_BOUNDARY_LEFT_ROPE,
    WM_RING_BOUNDARY_RIGHT_MAT,
    WM_RING_BOUNDARY_LEFT_MAT,
    WM_RING_BOUNDARY_RIGHT_MAT2,
    WM_RING_BOUNDARY_LEFT_MAT2,
    WM_RING_BOUNDARY_RIGHT_FENCE,
    WM_RING_BOUNDARY_LEFT_FENCE,
    WM_RING_BOUNDARY_COUNT
} WmRingBoundaryId;

/*
 * Direct C form of the eight *_r descriptors initialized in WRESTLE.ASM:
 *
 *   .WORD top_x, top_z, bottom_x, bottom_z
 *   .WORD depth, width
 *
 * The later ARE_WE_IN_RING/climb/keep_onscreen routines are NOT guessed in
 * this chunk.  This structure intentionally exposes only the verified seed.
 */
typedef struct {
    int16_t top_x;
    int16_t top_z;
    int16_t bottom_x;
    int16_t bottom_z;
    int16_t depth;
    int16_t width;
} WmRingBoundarySeed;

extern const WmRingBoundarySeed
    wm_ring_boundary_seeds[WM_RING_BOUNDARY_COUNT];

const WmRingBoundarySeed *wm_ring_boundary_seed(WmRingBoundaryId id);

/* Sanity check for the literal translated source descriptors only. */
bool wm_ring_boundary_seed_consistent(const WmRingBoundarySeed *seed);


/* Exact WRESTLE.ASM calc_line_x / ARE_WE_IN_RING services. */
int16_t wm_ring_calc_line_x(const WmRingBoundarySeed *line, int16_t z);
bool wm_ring_inside_trapezoid(int16_t x, int16_t z,
                              const WmRingBoundarySeed *left,
                              const WmRingBoundarySeed *right);
bool wm_ring_are_we_in_ring(int16_t x, int16_t z);
/* Arcade INRING convention: 0 = in the ring, 1 = outside. */
int16_t wm_ring_inring_field(int16_t x, int16_t z);

#ifdef __cplusplus
}
#endif

#endif
