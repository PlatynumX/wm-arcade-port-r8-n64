/*
 * WRESTLE2.ASM:2282 wrestler_veladd, :2385 calc_ground_y, and
 * WRESTLE.ASM:2456 wrestler_friction -- see wm/arcade/wm_arcade_veladd.h
 * for the sign convention and the main-loop placement.
 */
#include "wm/arcade/wm_arcade_veladd.h"
#include "wm/arcade/wmania_ring_geometry.h"

/* The source works in OBJ_XPOS/YPOS/ZPOS, 16.16 LONGs whose top word IS
   OBJ_*POSINT. This port keeps the two halves in separate fields, so every
   write to a fixed position refreshes its integer beside it. */
static void set_x(wm_arcade_actor_t *a, int32_t fixed) {
    a->x_fixed = fixed;
    a->x_int = fixed >> 16;
}

static void set_y(wm_arcade_actor_t *a, int32_t fixed) {
    a->y_fixed = fixed;
    a->y_int = fixed >> 16;
}

static void set_z(wm_arcade_actor_t *a, int32_t fixed) {
    a->z_fixed = fixed;
    a->z_int = fixed >> 16;
}

void wm_wrestler_calc_ground_y(wm_arcade_actor_t *a, int in_pregame2) {
    int32_t priority;
    int32_t edge;

    if (!a) return;

    /*
     * `move *a13(INRING),a0 / jrz #inring` -- the source's INRING is TRUE
     * when the wrestler is OUTSIDE. This port's field is the ordinary
     * boolean, so the test is flipped.
     */
    if (!a->in_ring) {
        priority = 117;
        if (a->z_int <= 0x5bd) priority = 103;
    } else {
        priority = 112;
    }
    a->obj_priority = priority;

    /* Between the two mat edges at this Z? calc_line_x is the real one. */
    edge = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_MAT), a->z_int);
    if (a->x_int >= edge) goto outside;
    edge = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_MAT), a->z_int);
    if (a->x_int <= edge) goto outside;

    /*
     * Inside the mat edges. The source leaves GROUND_Y and INRING alone
     * here -- a wrestler standing in the ring keeps the MAT_Y he was
     * created with (WRESTLE.ASM:2727) -- unless he is climbing through the
     * ropes, in which case he is landing IN the ring and gets put on the
     * mat. The source's own comments at this branch record two failed
     * attempts to fix a throw-into-ring bug with it.
     */
    if (a->climbing_thru) {
        a->ground_y = WM_MAT_Y;
        a->in_ring = 1;         /* source writes INRING = 0 */
    }
    return;

outside:
    a->ground_y = 0;            /* the floor outside the ring */
    if (in_pregame2) return;    /* GAMSTATE == INPREGAME2: leave INRING */
    a->in_ring = 0;             /* source writes INRING = 1 */
}

void wm_wrestler_friction(wm_arcade_actor_t *a) {
    int32_t f, v;

    if (!a) return;
    if (!(a->anim_mode & WM_MODE_FRICTION)) return;

    f = a->friction;
    v = a->x_vel;
    if (v == 0) return;

    if (v > 0) {
        v -= f;
        if (v < 0) v = 0;       /* `jrp #ok1 / clr a1`: never cross zero */
    } else {
        v += f;
        if (v > 0) v = 0;       /* `jrn #ok2 / clr a1` */
    }
    a->x_vel = v;
}

/* The mutual link ANIM.ASM checks before one wrestler may act on another. */
static int linked_both_ways(const wm_arcade_actor_t *a) {
    return a->attach_proc && a->attach_proc->attach_proc == a;
}

void wm_wrestler_veladd(wm_arcade_actor_t *a, wm_anim_exec *exec,
                        int in_pregame2) {
    int32_t ground_fp, height, yvel;

    if (!a) return;
    /* `move @HALT,a0 / jrnz #x` -- this port has no HALT global; a paused
       match simply does not tick. */

    set_x(a, a->x_fixed + a->x_vel);

    ground_fp = a->ground_y << 16;
    height = a->y_fixed - ground_fp;
    yvel = a->y_vel;
    height += yvel;

    if (height < 0) {
        /* Under the floor. */
        int through_floor = 0;
        if (linked_both_ways(a)) {
            if (a->anim_mode & WM_MODE_GHOST) {
                /* "may fall through floor if attached" -- keep going. */
                through_floor = 1;
            } else if (yvel >= 0) {
                /* Rising or still: put him at ground level but KEEP the
                   velocity, so he carries on up next tick. */
                height = 0;
                through_floor = 1;
            }
        }
        if (!through_floor) {
            /*
             * He has hit the ground. If MODE_WAITHITOPP is set, stuff a 1
             * in ANICNT -- ANIM.ASM:2300 ANI_WAITHITOPP is an ordinary
             * frame hold that landing cuts short.
             */
            if ((a->anim_mode & WM_MODE_WAITHITOPP) && exec)
                exec->ticks_left = 1;
            height = 0;
            a->y_vel = 0;
        }
    }
    set_y(a, height + ground_fp);

    wm_wrestler_calc_ground_y(a, in_pregame2);

    /*
     * Second clamp, against the ground calc_ground_y just chose. Note this
     * one does NOT check that the link is mutual, unlike the first -- the
     * source tests only that both pointers are non-null. Kept as written.
     */
    ground_fp = a->ground_y << 16;
    if (a->y_fixed <= ground_fp) {
        int ghost = a->attach_proc && a->attach_proc->attach_proc &&
                    (a->anim_mode & WM_MODE_GHOST);
        if (!ghost) set_y(a, ground_fp);
    }

    set_z(a, a->z_fixed + a->z_vel);

    if (a->anim_mode & WM_MODE_NOGRAVITY) return;
    /* Standing exactly on the ground: nothing to pull. */
    if ((a->ground_y << 16) == a->y_fixed) return;

    yvel = a->y_vel - a->gravity;
    if (yvel < WM_MAX_YVEL) yvel = WM_MAX_YVEL;
    a->y_vel = yvel;
}
