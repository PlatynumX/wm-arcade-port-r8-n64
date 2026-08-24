#include "wm_arcade_confine_grounded.h"
#include "wmania_ring_geometry.h"

static void set_x(wm_arcade_actor_t *a, int32_t x)
{
    a->x_int = x;
    a->x_fixed = x << 16;
}

static void set_z(wm_arcade_actor_t *a, int32_t z)
{
    a->z_int = z;
    a->z_fixed = z << 16;
}

bool wm_arcade_confine_grounded_inring(wm_arcade_actor_t *a,
                                        bool any_opponent_outside)
{
    uint16_t blocked = 0u;
    int16_t line;
    int32_t delta;

    /* Combat2EL parity correction: WRESTLE.ASM does NOT gate ordinary
       in-ring X/Z confinement on Y==GROUND_Y, zombie state, or whether an
       opponent is already outside.  Those EK safety guards disabled the
       routine at match start because reset_start seeds OBJ_YPOS=0 while
       GROUND_Y=MAT_Y.  Keep only the unported attached-pair branch guarded. */
    (void)any_opponent_outside;
    if (!a) return false;

    if (a->anim_mode & WM_ARCADE_MODE_NOCONFINE) return false;
    if (a->player_mode == WM_PMODE_ATTACHED) return false;
    if (a->in_ring != 0) return false;
    if (a->attach_proc != 0) return false;

    /* WRESTLE.ASM inside-ring Z confinement. Equality blocks movement but
       does not rewrite the coordinate. */
    if (a->z_int <= WM_RING_TOP) {
        if (a->z_int < WM_RING_TOP) set_z(a, WM_RING_TOP);
        blocked |= WM_MOVE_UP;
    } else if (a->z_int >= WM_RING_BOT) {
        if (a->z_int > WM_RING_BOT) set_z(a, WM_RING_BOT);
        blocked |= WM_MOVE_DOWN;
    }

    /* Left rope: calc_line_x(vln_left_rope), compare OBJ_COLLX1. */
    line = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_ROPE),
        (int16_t)a->z_int);
    if (a->hurt_box.x1 <= line) {
        if (a->hurt_box.x1 < line) {
            delta = (int32_t)line - a->hurt_box.x1;
            set_x(a, a->x_int + delta);
            a->hurt_box.x1 += delta;
            a->hurt_box.x2 += delta;
        }
        blocked |= WM_MOVE_LEFT;
    } else {
        /* Right rope: calc_line_x(vln_right_rope), compare OBJ_COLLX2. */
        line = wm_ring_calc_line_x(
            wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE),
            (int16_t)a->z_int);
        if (a->hurt_box.x2 >= line) {
            if (a->hurt_box.x2 > line) {
                delta = a->hurt_box.x2 - (int32_t)line;
                set_x(a, a->x_int - delta);
                a->hurt_box.x1 -= delta;
                a->hurt_box.x2 -= delta;
            }
            blocked |= WM_MOVE_RIGHT;
        }
    }

    a->can_move_dir = blocked;
    return true;
}
