#include "wm/arcade/wm_arcade_confine.h"
#include "wm/arcade/wmania_ring_geometry.h"

/* WRESTLE.ASM:3074-3425, in-ring branch only -- see wm_arcade_confine.h
   for exactly what is and isn't translated. Returns this single pass's
   CAN_MOVE_DIR bits and clamps actor->{x,z}_int/_fixed in place. */
static int32_t confine_once(wm_arcade_actor_t *actor) {
    int32_t bits = 0;
    int32_t z = actor->z_int;
    int32_t collx1, collx2, rope_x;

    /* WRESTLE.ASM:3091-3129: top/bottom rope Z bounds. */
    if (z <= WM_RING_TOP) {
        if (z < WM_RING_TOP) {
            actor->z_int = WM_RING_TOP;
            actor->z_fixed = (int32_t)WM_RING_TOP << 16;
        }
        bits |= WM_MOVE_UP;
    } else if (z >= WM_RING_BOT) {
        if (z > WM_RING_BOT) {
            actor->z_int = WM_RING_BOT;
            actor->z_fixed = (int32_t)WM_RING_BOT << 16;
        }
        bits |= WM_MOVE_DOWN;
    }

    /* WRESTLE.ASM:3132-3274: left rope, using OBJ_COLLX1 (hurt_box.x1). */
    collx1 = actor->hurt_box.x1;
    rope_x = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_ROPE), actor->z_int);
    if (collx1 <= rope_x) {
        if (collx1 < rope_x) {
            int32_t delta = rope_x - collx1;
            actor->x_int += delta;
            actor->x_fixed += delta << 16;
        }
        return bits | WM_MOVE_LEFT;
    }

    /* WRESTLE.ASM:3277-3421: right rope, using OBJ_COLLX2 (hurt_box.x2). */
    collx2 = actor->hurt_box.x2;
    rope_x = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE), actor->z_int);
    if (collx2 >= rope_x) {
        if (collx2 > rope_x) {
            int32_t delta = collx2 - rope_x;
            actor->x_int -= delta;
            actor->x_fixed -= delta << 16;
        }
        return bits | WM_MOVE_RIGHT;
    }

    return bits;
}

void wm_arcade_confine_wrestler(wm_arcade_actor_t *actor) {
    if (!actor) return;

    /* WRESTLE.ASM:3078-3084 #no_confine early-out: MODE_NOCONFINE or
       PLYRMODE==ATTACHED both skip straight to storing CAN_MOVE_DIR=0
       (the a7 accumulator's untouched initial value). */
    if ((actor->anim_mode & WM_MODE_NOCONFINE) ||
        actor->player_mode == WM_PMODE_ATTACHED) {
        actor->can_move_dir = 0;
        return;
    }

    actor->can_move_dir = confine_once(actor);
}
