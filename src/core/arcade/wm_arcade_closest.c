#include "wm/arcade/wm_arcade_closest.h"
#include "wm/arcade/wm_arcade_target.h"

static int32_t iabs32(int32_t v) {
    return v < 0 ? -v : v;
}

/*
 * calc_closest ends its distance computation with `calla square_root`
 * (WRESTLE.ASM:4183), and that routine is SQUARE.ASM:40 -- translated,
 * table and all, as wm_arcade_square_root. This file used to use a
 * textbook binary integer square root instead, on the argument that the
 * difference could not matter because the callers that gate behaviour
 * read only closest_xdist/closest_zdist. That is no longer true:
 * wm_arcade_drone.c compares closest_dist against 200 and
 * wm_arcade_doink.c against 0x70, so the quantisation the arcade's own
 * table introduces is load-bearing. Use the source's function.
 */
static int32_t isqrt32(uint32_t v) {
    return wm_arcade_square_root(v);
}

void wm_arcade_calc_closest(wm_arcade_actor_t *a, const wm_arcade_actor_t *o) {
    int32_t dx, dy, dz;
    uint32_t sumsq;

    if (!a || !o) return;

    dx = iabs32(a->x_int - o->x_int);
    dz = iabs32(a->z_int - o->z_int);
    dy = iabs32(a->y_int - o->y_int);

    a->closest_xdist = dx;
    a->closest_zdist = dz;
    a->closest_ydist = dy;

    sumsq = (uint32_t)dx * (uint32_t)dx +
            (uint32_t)dz * (uint32_t)dz +
            (uint32_t)dy * (uint32_t)dy;
    a->closest_dist = isqrt32(sumsq);
}

void wm_arcade_update_newfacing(wm_arcade_actor_t *a, const wm_arcade_actor_t *o) {
    int32_t facing;

    if (!a || !o) return;

    facing = (o->x_int > a->x_int) ? WM_MOVE_RIGHT : WM_MOVE_LEFT;
    facing |= (o->z_int > a->z_int) ? WM_MOVE_DOWN : WM_MOVE_UP;

    a->new_facing_dir = facing;
}
