#include "wm/arcade/wmania_ring_geometry.h"

#include <stdlib.h>

const WmRingBoundarySeed
wm_ring_boundary_seeds[WM_RING_BOUNDARY_COUNT] = {
    /* WRESTLE.ASM vln_right_rope_r */
    { WM_RING_TOP_RIGHT, WM_RING_TOP,
      WM_RING_BOT_RIGHT, WM_RING_BOT,
      WM_RING_DEPTH, WM_RING_RIGHT_WIDTH },

    /* WRESTLE.ASM vln_left_rope_r */
    { WM_RING_TOP_LEFT, WM_RING_TOP,
      WM_RING_BOT_LEFT, WM_RING_BOT,
      WM_RING_DEPTH, WM_RING_LEFT_WIDTH },

    /* WRESTLE.ASM vln_right_matedge_r */
    { WM_MAT_TOP_RIGHT, WM_MAT_TOP,
      WM_MAT_BOT_RIGHT, WM_MAT_BOT,
      WM_MAT_DEPTH, WM_MAT_RIGHT_WIDTH },

    /* WRESTLE.ASM vln_left_matedge_r */
    { WM_MAT_TOP_LEFT, WM_MAT_TOP,
      WM_MAT_BOT_LEFT, WM_MAT_BOT,
      WM_MAT_DEPTH, WM_MAT_LEFT_WIDTH },

    /* WRESTLE.ASM vln_right_matedge2_r */
    { WM_MAT2_TOP_RIGHT, WM_MAT2_TOP,
      WM_MAT2_BOT_RIGHT, WM_MAT2_BOT,
      WM_MAT2_DEPTH, WM_MAT2_RIGHT_WIDTH },

    /* WRESTLE.ASM vln_left_matedge2_r */
    { WM_MAT2_TOP_LEFT, WM_MAT2_TOP,
      WM_MAT2_BOT_LEFT, WM_MAT2_BOT,
      WM_MAT2_DEPTH, WM_MAT2_LEFT_WIDTH },

    /* WRESTLE.ASM vln_right_fence_r */
    { WM_ARENA_TOP_RIGHT, WM_ARENA_TOP,
      WM_ARENA_BOT_RIGHT, WM_ARENA_BOT,
      WM_ARENA_DEPTH, WM_ARENA_RIGHT_WIDTH },

    /* WRESTLE.ASM vln_left_fence_r */
    { WM_ARENA_TOP_LEFT, WM_ARENA_TOP,
      WM_ARENA_BOT_LEFT, WM_ARENA_BOT,
      WM_ARENA_DEPTH, WM_ARENA_LEFT_WIDTH }
};

const WmRingBoundarySeed *wm_ring_boundary_seed(WmRingBoundaryId id)
{
    if ((unsigned)id >= WM_RING_BOUNDARY_COUNT) {
        return 0;
    }
    return &wm_ring_boundary_seeds[id];
}

bool wm_ring_boundary_seed_consistent(const WmRingBoundarySeed *seed)
{
    int depth;
    int width;

    if (seed == 0) {
        return false;
    }

    depth = (int)seed->bottom_z - (int)seed->top_z;
    width = abs((int)seed->bottom_x - (int)seed->top_x);

    return depth == seed->depth && width == seed->width;
}

int32_t wm_ring_calc_line_x(const WmRingBoundarySeed *seed, int32_t zpos)
{
    int32_t i, count;
    int64_t top_x_fixed, delta_fixed, value_fixed;

    if (!seed) return 0;
    if (zpos > seed->bottom_z) return 0;
    i = zpos - seed->top_z;
    if (i < 0) return 0;

    count = (int32_t)seed->depth + 1;
    top_x_fixed = (int64_t)seed->top_x << 16;
    delta_fixed = ((int64_t)seed->width << 16) / count;

    if (seed->top_x > seed->bottom_x)
        value_fixed = top_x_fixed - (int64_t)(i + 1) * delta_fixed;
    else
        value_fixed = top_x_fixed + (int64_t)(i + 1) * delta_fixed;

    return (int32_t)(value_fixed >> 16);
}
