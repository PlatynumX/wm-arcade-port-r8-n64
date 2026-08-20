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
