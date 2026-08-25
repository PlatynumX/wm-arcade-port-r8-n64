#include "wm_arcade_confine_full.h"
#include "wmania_ring_geometry.h"

#include <stdint.h>

#define WM_GATE_CRASH_DAMAGE 20
#define WM_TSEC 53u

static void set_full_x(wm_arcade_actor_t *a, int32_t x)
{
    a->x_int = x;
    a->x_fixed = x << 16;
}

static void set_full_z(wm_arcade_actor_t *a, int32_t z)
{
    a->z_int = z;
    a->z_fixed = z << 16;
}

static void add_x_int_preserve_fraction(wm_arcade_actor_t *a, int32_t dx)
{
    a->x_int += dx;
    a->x_fixed += dx << 16;
}

static void source_pair_left_bounce(wm_arcade_actor_t *a,
                                    wm_arcade_actor_t *o)
{
    int set = 0;
    if (!a || !o) return;

    if (a->x_vel == 0x00040000) set = 1;
    else if (o->x_vel != 0 && o->x_vel != 0x00040000) set = 1;

    if (set) {
        a->x_vel = 0x00040000;
        o->x_vel = 0x00040000;
        if (a->y_vel <= 0x00030000) a->y_vel = 0x00030000;
        if (o->y_vel <= 0x00030000) o->y_vel = 0x00030000;
    }
}

static void source_pair_right_bounce(wm_arcade_actor_t *a,
                                     wm_arcade_actor_t *o)
{
    int set = 0;
    if (!a || !o) return;

    if (a->x_vel == -0x00040000) set = 1;
    else if (o->x_vel != 0 && o->x_vel != -0x00040000) set = 1;

    if (set) {
        a->x_vel = -0x00040000;
        o->x_vel = -0x00040000;
        if (a->y_vel <= 0x00030000) a->y_vel = 0x00030000;
        if (o->y_vel <= 0x00030000) o->y_vel = 0x00030000;
    }
}

static void rope_wobble(wm_arcade_actor_t *a,
                        unsigned bank,
                        const wm_arcade_confine_callbacks_t *cb)
{
    if (cb && cb->rope_bounce_io)
        cb->rope_bounce_io(a, bank, 1u, cb->user);
    if (cb && cb->sound)
        cb->sound(a, 0x003cu, cb->user);
}

static void climb_call(wm_arcade_actor_t *a,
                       wm_arcade_confine_climb_request_t request,
                       int16_t line_x,
                       const wm_arcade_confine_callbacks_t *cb)
{
    if (cb && cb->climb)
        cb->climb(a, request, line_x, cb->user);
}

static void confine_inside(wm_arcade_actor_t *a,
                           const wm_arcade_confine_callbacks_t *cb)
{
    uint16_t can = 0u;
    int16_t line;
    int32_t penetration;
    wm_arcade_actor_t *o;

    if (a->z_int < WM_RING_TOP) {
        set_full_z(a, WM_RING_TOP);
        a->z_bound = WM_RING_TOP;
        can |= WM_MOVE_UP;
        climb_call(a, WM_CONFINE_CLIMB_OUT_TOP, 0, cb);
    } else if (a->z_int > WM_RING_BOT) {
        set_full_z(a, WM_RING_BOT);
        a->z_bound = WM_RING_BOT;
        can |= WM_MOVE_DOWN;
        climb_call(a, WM_CONFINE_CLIMB_OUT_BOTTOM, 0, cb);
    } else {
        a->z_bound = 0;
    }

    line = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_ROPE),
        (int16_t)a->z_int);
    if (line != 0 && a->hurt_box.x1 <= line) {
        penetration = (int32_t)line - a->hurt_box.x1;
        o = a->attach_proc;
        if (o) {
            add_x_int_preserve_fraction(a, penetration);
            add_x_int_preserve_fraction(o, penetration);

            if (a->y_int != a->ground_y) {
                source_pair_left_bounce(a, o);
                rope_wobble(a, WM_CONFINE_ROPE_LEFT, cb);
            }
        } else {
            int32_t new_x = (int32_t)line + (a->x_int - a->hurt_box.x1);
            set_full_x(a, new_x);
            a->x_bound = line;

            climb_call(a, WM_CONFINE_CLIMB_OUT_SIDE, line, cb);

            if (a->move_dir == 0 && a->x_vel != 0 &&
                a->y_int != a->ground_y) {
                uint16_t old_frac = (uint16_t)a->x_vel;
                a->x_vel = 0x00030001;
                /* Source reuses a7 for this velocity literal and CAN_MOVE_DIR. */
                can = 1u;
                if (old_frac != 1u)
                    rope_wobble(a, WM_CONFINE_ROPE_LEFT, cb);
            }
        }
        can |= WM_MOVE_LEFT;
        a->can_move_dir = can;
        return;
    }

    line = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE),
        (int16_t)a->z_int);
    if (line != 0 && a->hurt_box.x2 >= line) {
        penetration = a->hurt_box.x2 - (int32_t)line;
        o = a->attach_proc;
        if (o) {
            add_x_int_preserve_fraction(a, -penetration);
            add_x_int_preserve_fraction(o, -penetration);

            if (a->y_int != a->ground_y) {
                source_pair_right_bounce(a, o);
                rope_wobble(a, WM_CONFINE_ROPE_RIGHT, cb);
            }
        } else {
            int32_t new_x = (int32_t)line - (a->hurt_box.x2 - a->x_int);
            set_full_x(a, new_x);
            a->x_bound = line;

            climb_call(a, WM_CONFINE_CLIMB_OUT_SIDE, line, cb);

            if (a->move_dir == 0 && a->x_vel != 0 &&
                a->y_int != a->ground_y) {
                uint16_t old_frac = (uint16_t)a->x_vel;
                a->x_vel = (int32_t)0xfffd0001;
                can = 1u;
                if (old_frac != 1u)
                    rope_wobble(a, WM_CONFINE_ROPE_RIGHT, cb);
            }
        }
        can |= WM_MOVE_RIGHT;
        a->can_move_dir = can;
        return;
    }

    a->x_bound = 0;
    a->can_move_dir = can;
}

static void outside_gate_crash(wm_arcade_actor_t *a,
                               uint16_t can,
                               uint32_t pcnt,
                               const wm_arcade_confine_callbacks_t *cb)
{
    int32_t vx;

    if (a->player_mode == WM_PMODE_DEAD) {
        if ((can & (WM_MOVE_LEFT | WM_MOVE_RIGHT)) != 0u &&
            (a->status_flags & WM_STATUS_ZOMBIE) != 0u &&
            (a->status_flags & WM_STATUS_CAN_XFORM) != 0u) {
            if (cb && cb->zombie_transform)
                cb->zombie_transform(a, cb->user);
        }
        return;
    }

    if (a->player_mode != WM_PMODE_RUNNING)
        return;

    if ((can & WM_MOVE_LEFT) != 0u)
        vx = 0x00030000;
    else if ((can & WM_MOVE_RIGHT) != 0u)
        vx = -0x00030000;
    else
        return;

    if ((can & (uint16_t)a->facing_dir &
         (WM_MOVE_LEFT | WM_MOVE_RIGHT)) == 0u)
        return;

    a->x_vel = vx;
    if (a->player_mode != WM_PMODE_DEAD)
        a->player_mode = WM_PMODE_NORMAL;
    a->y_vel = 0x00030000;
    a->run_time = 0;

    if (cb && cb->gate_anim) {
        wm_arcade_confine_gate_anim_t anim =
            (uint32_t)(pcnt - a->hit_gate_time) < (WM_TSEC * 3u) ?
            WM_CONFINE_GATE_FALL_BACK : WM_CONFINE_GATE_BOUNCE;
        cb->gate_anim(a, anim, cb->user);
    }

    if (cb && cb->sound)
        cb->sound(a, 0x00c5u, cb->user);

    a->hit_gate_time = pcnt;

    if (cb && cb->adjust_health)
        cb->adjust_health(a, -WM_GATE_CRASH_DAMAGE, cb->user);
    if (cb && cb->ditch_getup_meter)
        cb->ditch_getup_meter(a, cb->user);
}

static void confine_outside(wm_arcade_actor_t *a,
                            uint32_t pcnt,
                            const wm_arcade_confine_callbacks_t *cb)
{
    uint16_t can = 0u;
    int16_t line;
    int32_t delta;
    wm_arcade_actor_t *o = a->attach_proc;

    if (a->z_int < WM_ARENA_TOP) {
        set_full_z(a, WM_ARENA_TOP);
        a->z_bound = WM_ARENA_TOP;
        can |= WM_MOVE_UP;
    } else if (a->z_int > WM_ARENA_BOT) {
        set_full_z(a, WM_ARENA_BOT);
        a->z_bound = WM_ARENA_BOT;
        can |= WM_MOVE_DOWN;
    } else {
        a->z_bound = 0;
    }

    line = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_FENCE),
        (int16_t)a->z_int);
    if (line != 0 && a->hurt_box.x1 < line) {
        delta = (int32_t)line - a->hurt_box.x1;
        add_x_int_preserve_fraction(a, delta);
        a->x_bound = line;
        can |= WM_MOVE_LEFT;
        if (o) {
            add_x_int_preserve_fraction(o, delta);
            o->x_bound = line;
        }
    } else {
        line = wm_ring_calc_line_x(
            wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_FENCE),
            (int16_t)a->z_int);
        if (line != 0 && a->hurt_box.x2 > line) {
            delta = a->hurt_box.x2 - (int32_t)line;
            add_x_int_preserve_fraction(a, -delta);
            a->x_bound = line;
            can |= WM_MOVE_RIGHT;
            if (o) {
                add_x_int_preserve_fraction(o, -delta);
                o->x_bound = line;
            }
        } else {
            a->x_bound = 0;
        }
    }

    if (a->x_int <= WM_RING_X_CENTER) {
        int32_t xover;
        int32_t zover;
        line = wm_ring_calc_line_x(
            wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_MAT2),
            (int16_t)a->z_int);
        if (line != 0) {
            xover = a->hurt_box.x2 - (int32_t)line;
            if (xover >= 0) {
                if (a->z_int <= WM_RING_Z_CENTER) {
                    zover = a->z_int - WM_MAT2_TOP;
                    if (zover <= xover) {
                        set_full_z(a, a->z_int - zover);
                        can |= WM_MOVE_DOWN;
                        climb_call(a, WM_CONFINE_CLIMB_IN_TOP, line, cb);
                        goto done;
                    }
                } else {
                    zover = WM_MAT2_BOT - a->z_int;
                    if (zover <= xover) {
                        set_full_z(a, a->z_int + zover);
                        can |= WM_MOVE_UP;
                        climb_call(a, WM_CONFINE_CLIMB_IN_BOTTOM, line, cb);
                        goto done;
                    }
                }

                add_x_int_preserve_fraction(a, -xover);
                a->x_bound = line;
                can |= WM_MOVE_RIGHT;
                if (o) {
                    add_x_int_preserve_fraction(o, -xover);
                    o->x_bound = line;
                }
                climb_call(a, WM_CONFINE_CLIMB_IN_SIDE, line, cb);
            }
        }
    } else {
        int32_t xover;
        int32_t zover;
        line = wm_ring_calc_line_x(
            wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_MAT2),
            (int16_t)a->z_int);
        if (line != 0) {
            xover = (int32_t)line - a->hurt_box.x1;
            if (xover >= 0) {
                if (a->z_int <= WM_RING_Z_CENTER) {
                    zover = a->z_int - WM_MAT2_TOP;
                    if (zover <= xover) {
                        set_full_z(a, a->z_int - zover);
                        can |= WM_MOVE_DOWN;
                        climb_call(a, WM_CONFINE_CLIMB_IN_TOP, line, cb);
                        goto done;
                    }
                } else {
                    zover = WM_MAT2_BOT - a->z_int;
                    if (zover <= xover) {
                        set_full_z(a, a->z_int + zover);
                        can |= WM_MOVE_UP;
                        climb_call(a, WM_CONFINE_CLIMB_IN_BOTTOM, line, cb);
                        goto done;
                    }
                }

                add_x_int_preserve_fraction(a, xover);
                a->x_bound = line;
                can |= WM_MOVE_LEFT;
                if (o) {
                    add_x_int_preserve_fraction(o, xover);
                    o->x_bound = line;
                }
                climb_call(a, WM_CONFINE_CLIMB_IN_SIDE, line, cb);
            }
        }
    }

done:
    a->can_move_dir = can;
    outside_gate_crash(a, can, pcnt, cb);
}

void wm_arcade_confine_wrestler(wm_arcade_actor_t *a,
                                uint32_t pcnt,
                                const wm_arcade_confine_callbacks_t *cb)
{
    if (!a) return;

    a->can_move_dir = 0u;
    if ((a->anim_mode & WM_ARCADE_MODE_NOCONFINE) != 0u) return;
    if (a->player_mode == WM_PMODE_ATTACHED) return;

    if (a->in_ring == 0) confine_inside(a, cb);
    else confine_outside(a, pcnt, cb);
}

void wm_arcade_confine_fix1(wm_arcade_actor_t *a)
{
    if (a) a->can_move_temp = a->can_move_dir;
}

void wm_arcade_confine_fix2(wm_arcade_actor_t *a)
{
    if (a)
        a->can_move_dir = (uint16_t)(a->can_move_dir | a->can_move_temp);
}
