#include "wm/arcade/wm_arcade_bounce.h"
#include "wm/arcade/wm_arcade_roster_anims.h"
#include "wm/arcade/wmania_ring_geometry.h"

#include <string.h>

/* WRESTLE.ASM:5185, transcribed -- see the header. */
const int16_t wm_bounce_xoffsets[WM_BOUNCE_SLOTS] = {
    -20,   /* 0 Bret Hart */
    -20,   /* 1 Razor Ramon */
    -20,   /* 2 Undertaker */
    -20,   /* 3 Yokozuna */
      0,   /* 4 Shawn Michaels */
    -30,   /* 5 Bam Bam */
    -20,   /* 6 Doink */
    -20,   /* 7 spare (Adam Bomb) */
    -20,   /* 8 Lex Luger */
      0    /* 9 Referee */
};

wm_bounce_result_t wm_arcade_bounce_off_ropes(wm_arcade_actor_t *actor) {
    wm_bounce_result_t out;
    int32_t xoff, line_x;
    bool bounce;
    const wm_roster_anim_table *anims;

    memset(&out, 0, sizeof out);
    if (!actor) return out;

    /* `move *a13(INRING),a0 / jrnz #outside`. */
    if (!actor->in_ring) return out;

    xoff = (actor->wrestler_num >= 0 && actor->wrestler_num < WM_BOUNCE_SLOTS)
               ? wm_bounce_xoffsets[actor->wrestler_num] : 0;

    /*
     * Which rope is decided by MOVE_DIR, not by which side of the ring
     * he is on: `btst PLAYER_RIGHT_BIT,a0 / jrnz #right`. A wrestler
     * running right is tested against the right rope wherever he is.
     *
     * calc_line_x answers 0 when his Z is past either end of that rope,
     * and the source does not check for it, so neither does this. The
     * comparison then runs against a line at 0 -- which on the right
     * would fall through into the bounce. It is not reachable from
     * here: a Z outside the rope's range puts him outside the ring, and
     * INRING has already sent him home. Reproduced rather than guarded,
     * because a guard would be an invention.
     */
    if (actor->move_dir & WM_MOVE_RIGHT) {
        line_x = wm_ring_calc_line_x(
            wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE),
            actor->z_int);
        line_x += xoff;                 /* `add a14,a0` */
        /* `cmp a0,a1 / jrle #no_bounce` -- a1 is OBJ_COLLX2, and the
           test is on a1 - a0, so collx2 <= line means no bounce. */
        bounce = actor->hurt_box.x2 > line_x;
    } else {
        line_x = wm_ring_calc_line_x(
            wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_ROPE),
            actor->z_int);
        line_x -= xoff;                 /* `sub a14,a0` */
        /* `cmp a0,a1 / jrle #bounce` -- the opposite jump target for
           the same test, so collx1 <= line means bounce. */
        bounce = actor->hurt_box.x1 <= line_x;
    }
    if (!bounce) return out;

    /*
     * `#bounce`. The source's own `;;; move a0,*a13(OBJ_XPOSINT)` sits
     * here commented out: he is NOT snapped onto the line, so he keeps
     * whatever overshoot the run gave him and the bounce animation
     * carries him back.
     */
    if (actor->getup_time == 0 && actor->risk == 0) {
        actor->risk = WM_BOUNCE_RISK_TICKS;
        out.opened_risk_window = true;
    }

    anims = wm_roster_anim_find("#bounce_anims");
    if (anims)
        out.bounce_anim = wm_roster_anim_for(anims, (int)actor->wrestler_num);
    actor->player_mode = WM_PMODE_BOUNCING;
    out.bounced = true;
    return out;
}
