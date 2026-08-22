#include "wm_arcade_movement.h"

#include "wm_arcade_combat_defs.h"
#include "wmania_ring_geometry.h"

static bool valid_mutual_attach(const wm_arcade_actor_t *a)
{
    return a && a->attach_proc && a->attach_proc->attach_proc == a;
}

static int32_t fx_from_int(int32_t v)
{
    return v << 16;
}

static int32_t fx_int(int32_t v)
{
    return v >> 16;
}

static void sync_integer_positions(wm_arcade_actor_t *a)
{
    a->x_int = fx_int(a->x_fixed);
    a->y_int = fx_int(a->y_fixed);
    a->z_int = fx_int(a->z_fixed);
}

void wm_arcade_calc_ground_y(wm_arcade_actor_t *a, bool in_pregame2)
{
    int16_t z;
    int16_t x;
    int16_t rx;
    int16_t lx;
    const WmRingBoundarySeed *right;
    const WmRingBoundarySeed *left;

    if (!a) return;

    if (a->in_ring != 0)
        a->priority = (fx_int(a->z_fixed) > 0x05bd) ? 117 : 103;
    else
        a->priority = 112;

    z = (int16_t)fx_int(a->z_fixed);
    x = (int16_t)fx_int(a->x_fixed);
    right = wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_MAT);
    left = wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_MAT);
    rx = wm_ring_calc_line_x(right, z);
    if (x >= rx) goto outside;
    lx = wm_ring_calc_line_x(left, z);
    if (x <= lx) goto outside;

    if (a->climbing_thru) {
        a->ground_y = WM_MAT_Y;
        a->in_ring = 0;
    }
    return;

outside:
    a->ground_y = 0;
    if (!in_pregame2) a->in_ring = 1;
}

void wm_arcade_wrestler_veladd(wm_arcade_actor_t *a,
                               bool halt, bool in_pregame2)
{
    int32_t gy_before;
    int32_t rel_y;
    int32_t old_yvel;
    int32_t gy;
    bool mutual;

    if (!a || halt) return;

    a->x_fixed += a->x_vel;

    gy_before = fx_from_int(a->ground_y);
    rel_y = a->y_fixed - gy_before;
    old_yvel = a->y_vel;
    rel_y += old_yvel;

    if (rel_y < 0) {
        mutual = valid_mutual_attach(a);
        if (mutual && (a->anim_mode & WM_ARCADE_MODE_GHOST)) {
            /* Source deliberately allows this attached ghost below ground. */
        } else if (mutual && old_yvel < 0) {
            rel_y = 0;
        } else {
            if (a->anim_mode & WM_ARCADE_MODE_WAITHITOPP)
                a->ani_count = 1;
            a->y_vel = 0;
            rel_y = 0;
        }
    }
    a->y_fixed = rel_y + gy_before;

    wm_arcade_calc_ground_y(a, in_pregame2);

    gy = fx_from_int(a->ground_y);
    if (gy >= a->y_fixed &&
        !(valid_mutual_attach(a) && (a->anim_mode & WM_ARCADE_MODE_GHOST)))
        a->y_fixed = gy;

    a->z_fixed += a->z_vel;

    if (!(a->anim_mode & WM_ARCADE_MODE_NOGRAVITY) && a->y_fixed != gy) {
        int32_t gravity = a->gravity ? a->gravity : WM_ARCADE_GRAVITY;
        int32_t yv = a->y_vel - gravity;
        if (yv < WM_ARCADE_MAX_YVEL) yv = WM_ARCADE_MAX_YVEL;
        a->y_vel = yv;
    }

    sync_integer_positions(a);
}

void wm_arcade_wrestler_friction(wm_arcade_actor_t *a)
{
    int32_t v;
    int32_t f;
    if (!a || !(a->anim_mode & WM_ARCADE_MODE_FRICTION)) return;
    f = a->obj_friction;
    v = a->x_vel;
    if (v == 0) return;
    if (v < 0) {
        v += f;
        if (v >= 0) v = 0;
    } else {
        v -= f;
        if (v <= 0) v = 0;
    }
    a->x_vel = v;
}

static const uint8_t xflip_table[16] = {
    0x0,0x1,0x2,0x3,0x8,0x9,0xA,0xB,
    0x4,0x5,0x6,0x7,0xC,0xD,0xE,0xF
};

uint16_t wm_arcade_xflip_joy(uint16_t joy)
{
    return xflip_table[joy & 0x0fu];
}

uint16_t wm_arcade_stick_relative(uint16_t joy, bool facing_right)
{
    joy &= 0x0fu;
    return facing_right ? joy : wm_arcade_xflip_joy(joy);
}

uint16_t wm_arcade_stick_relative_new(uint16_t joy_cur,
                                      uint16_t joy_down,
                                      uint16_t joy_up,
                                      bool facing_right)
{
    if (((joy_down | joy_up) & 0x0fu) == 0u) return 0u;
    return wm_arcade_stick_relative(joy_cur, facing_right);
}
