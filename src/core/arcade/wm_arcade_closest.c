#include "wm/arcade/wm_arcade_closest.h"

static int32_t iabs32(int32_t v) {
    return v < 0 ? -v : v;
}

/* Standard binary integer square root (largest r such that r*r <= v). */
static int32_t isqrt32(uint32_t v) {
    uint32_t res = 0;
    uint32_t bit = 1u << 30;

    while (bit > v) bit >>= 2;
    while (bit != 0) {
        if (v >= res + bit) {
            v -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return (int32_t)res;
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
