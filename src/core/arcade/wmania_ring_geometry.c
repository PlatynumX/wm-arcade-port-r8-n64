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

/*
 * WRESTLE.ASM:5789 get_rope_x. `cmpi RING_X_CENTER,a0 / jrgt #right`,
 * so the centre line itself belongs to the left rope.
 */
int32_t wm_ring_get_rope_x(int32_t xposint, int32_t zposint)
{
    WmRingBoundaryId id = (xposint > WM_RING_X_CENTER)
        ? WM_RING_BOUNDARY_RIGHT_ROPE : WM_RING_BOUNDARY_LEFT_ROPE;
    return wm_ring_calc_line_x(wm_ring_boundary_seed(id), zposint);
}

/*
 * WRESTLE.ASM:5704 get_box_overlap. See the header for the tie rule
 * and for why "outside" and "out of range in Z" look the same.
 */
wm_ring_pushout wm_ring_box_overlap(const WmRingBoundarySeed *left,
                                    const WmRingBoundarySeed *right,
                                    int32_t xposint, int32_t zposint)
{
    wm_ring_pushout out;
    int32_t lx, rx;
    int32_t dl, dr, dt, db;

    out.xoff = 0;
    out.zoff = 0;
    if (!left || !right) return out;

    /* The right line first, then the left -- and a6 keeps the LEFT
       seed, which is whose top and bottom Z are used below. */
    rx = wm_ring_calc_line_x(right, zposint);
    if (rx == 0) return out;                  /* `jrz #outrange` */
    lx = wm_ring_calc_line_x(left, zposint);
    if (lx == 0) return out;

    dl = lx - xposint;
    if (dl > 0) return out;                   /* left of the box */
    dl = -dl;                                 /* distance inside, from the left */

    dr = rx - xposint;
    if (dr < 0) return out;                   /* right of the box */

    dt = zposint - (int32_t)left->top_z;
    if (dt < 0) return out;                   /* above */

    db = zposint - (int32_t)left->bottom_z;
    if (db > 0) return out;                   /* below */
    db = -db;

    /*
     * `cmp a0,a1 / jrlt #right_min` picks the nearer side first, then
     * each arm checks the two Z edges. Every test is strict, so an
     * exact tie keeps the horizontal push.
     */
    {
        int32_t h = (dr < dl) ? dr : dl;
        int use_right = (dr < dl);

        if (h > dt || h > db) {
            /* One of the Z edges is nearer. `#top_min` and `#bot_min`
               each defer to the other, and the conditions are strict
               opposites, so the pair settles rather than looping. */
            if (dt > db) {
                out.zoff = db;
            } else {
                out.zoff = -dt;
            }
            out.xoff = 0;
            return out;
        }
        out.xoff = use_right ? dr : -dl;
        out.zoff = 0;
    }
    return out;
}
