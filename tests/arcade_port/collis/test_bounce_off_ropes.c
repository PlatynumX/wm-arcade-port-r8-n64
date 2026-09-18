/*
 * WRESTLE.ASM:5115 bounce_off_ropes -- the rope rebound, and the
 * high-risk window nobody could open.
 *
 * Every wrestler's mode_running calls this (BRET.ASM:1935,
 * BAM.ASM:1880, DNK.ASM:1970, DOINK.ASM:2337, ...) and all eight of
 * this port's dispatchers already had the callback seam with nothing
 * behind it, so a running wrestler crossed the ropes and kept going.
 *
 * The animation is the visible half. The half that matters is RISK: a
 * sixty-tick window in which the next hit counts as a high-risk move,
 * whose READER was already translated and waiting in
 * src/core/arcade/wm_arcade_react.c.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_bounce.h"
#include "wm/arcade/wm_arcade_react.h"
#include "wm/arcade/wm_arcade_roster_anims.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm/wrestler_backend.h"
#include "wm/bret_backend.h"

/* At the ring's mid Z the rope lines sit at 830 and 1322. */
#define LEFT_ROPE   830
#define RIGHT_ROPE 1322

/*
 * A running wrestler at X, heading `dir`, box thirty either side. He is
 * Bret unless the caller says otherwise -- the wrestler number is set
 * after the call rather than passed, because every one of the offset
 * cases below varies it and a fourth positional argument is one too
 * many to keep straight.
 */
static void runner(wm_arcade_actor_t *a, int32_t x, int32_t dir) {
    memset(a, 0, sizeof *a);
    a->active = 1;
    a->in_ring = 1;
    a->wrestler_num = 0;
    a->player_mode = WM_PMODE_RUNNING;
    a->move_dir = dir;
    a->x_int = x; a->x_fixed = x << 16;
    a->z_int = WM_RING_Z_CENTER;
    a->z_fixed = (int32_t)WM_RING_Z_CENTER << 16;
    a->hurt_box.x1 = x - 30;
    a->hurt_box.x2 = x + 30;
}

/* The geometry this test is written against, asserted rather than
   assumed -- if the rope lines move, the numbers below are wrong. */
static void test_the_rope_lines_are_where_we_think(void) {
    assert(wm_ring_calc_line_x(
               wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_ROPE),
               WM_RING_Z_CENTER) == LEFT_ROPE);
    assert(wm_ring_calc_line_x(
               wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE),
               WM_RING_Z_CENTER) == RIGHT_ROPE);
}

/*
 * The offsets, and the thing about them that reads backwards: the
 * source subtracts on the left and adds on the right with the SAME
 * value, so a negative offset pulls the trigger line inward on both
 * sides.
 */
static void test_the_offsets_pull_the_line_inward(void) {
    wm_arcade_actor_t a;
    wm_bounce_result_t r;

    /* Bret, -20: the left line is effectively 850, not 830. */
    assert(wm_bounce_xoffsets[0] == -20);
    runner(&a, 880 + 30, WM_MOVE_LEFT);    /* box x1 = 880 */
    r = wm_arcade_bounce_off_ropes(&a);
    assert(!r.bounced);                       /* 880 > 850 */
    runner(&a, 850 + 30, WM_MOVE_LEFT);
    r = wm_arcade_bounce_off_ropes(&a);
    assert(r.bounced);                        /* 850 <= 850, exactly */

    /* Shawn's row is 0 -- he turns AT the rope, twenty pixels later
       than everyone else. */
    assert(wm_bounce_xoffsets[4] == 0);
    runner(&a, 850 + 30, WM_MOVE_LEFT);
    a.wrestler_num = 4;
    r = wm_arcade_bounce_off_ropes(&a);
    assert(!r.bounced);                       /* 850 > 830 */
    runner(&a, LEFT_ROPE + 30, WM_MOVE_LEFT);
    a.wrestler_num = 4;
    r = wm_arcade_bounce_off_ropes(&a);
    assert(r.bounced);

    /* Bam Bam's is -30, the deepest on the roster: he turns earliest. */
    assert(wm_bounce_xoffsets[5] == -30);
    runner(&a, 860 + 30, WM_MOVE_LEFT);
    a.wrestler_num = 5;
    r = wm_arcade_bounce_off_ropes(&a);
    assert(r.bounced);                        /* 860 <= 860 */
    runner(&a, 860 + 30, WM_MOVE_LEFT);
    a.wrestler_num = 0;                       /* the same X, as Bret */
    r = wm_arcade_bounce_off_ropes(&a);
    assert(!r.bounced);                       /* 860 > 850 */

    /* And the whole table, since it is transcribed by hand. */
    {
        static const int16_t expect[WM_BOUNCE_SLOTS] = {
            -20, -20, -20, -20, 0, -30, -20, -20, -20, 0
        };
        int i;
        for (i = 0; i < WM_BOUNCE_SLOTS; ++i)
            assert(wm_bounce_xoffsets[i] == expect[i]);
    }
}

/*
 * The right rope, which the source spells with the opposite jump target
 * for the same comparison -- `jrle #no_bounce` where the left has
 * `jrle #bounce`. Both mean "the box edge has reached the line".
 */
static void test_the_right_rope_is_the_mirror(void) {
    wm_arcade_actor_t a;
    wm_bounce_result_t r;

    /* Bret, -20: the right line is effectively 1302. */
    runner(&a, 1302 - 30, WM_MOVE_RIGHT);  /* box x2 = 1302 */
    r = wm_arcade_bounce_off_ropes(&a);
    assert(!r.bounced);                       /* 1302 is NOT > 1302 */
    runner(&a, 1303 - 30, WM_MOVE_RIGHT);
    r = wm_arcade_bounce_off_ropes(&a);
    assert(r.bounced);

    /*
     * Which rope is chosen by MOVE_DIR and not by which half of the
     * ring he stands in: a wrestler deep on the right, running LEFT, is
     * tested against the LEFT rope and does not bounce.
     */
    runner(&a, 1290, WM_MOVE_LEFT);
    r = wm_arcade_bounce_off_ropes(&a);
    assert(!r.bounced);
    runner(&a, 1290, WM_MOVE_RIGHT);
    r = wm_arcade_bounce_off_ropes(&a);
    assert(r.bounced);                        /* 1320 > 1302 */

    /* MOVE_DIR without the RIGHT bit takes the left branch, so a
       diagonal counts as right only when the bit is set. */
    runner(&a, 1290, WM_MOVE_DOWN_RIGHT);
    r = wm_arcade_bounce_off_ropes(&a);
    assert(r.bounced);
    runner(&a, 1290, WM_MOVE_UP);
    r = wm_arcade_bounce_off_ropes(&a);
    assert(!r.bounced);
}

/* What the bounce does: MODE_BOUNCING, his own animation, no snap. */
static void test_the_bounce_itself(void) {
    wm_arcade_actor_t a;
    wm_bounce_result_t r;
    int32_t x0;

    runner(&a, 840 + 30, WM_MOVE_LEFT);
    x0 = a.x_int;
    r = wm_arcade_bounce_off_ropes(&a);
    assert(r.bounced);
    assert(a.player_mode == WM_PMODE_BOUNCING);
    assert(r.bounce_anim != NULL);
    assert(strcmp(r.bounce_anim, "hrt_bounce_anim") == 0);
    /* The source's `;;; move a0,*a13(OBJ_XPOSINT)` is commented out --
       he keeps whatever overshoot the run gave him. */
    assert(a.x_int == x0);
    assert(a.x_fixed == x0 << 16);

    /* Each wrestler gets his own, out of the real extracted table. */
    runner(&a, 840 + 30, WM_MOVE_LEFT);
    a.wrestler_num = 2;
    r = wm_arcade_bounce_off_ropes(&a);
    assert(strcmp(r.bounce_anim, "und_bounce_anim") == 0);
    /* Adam Bomb and the Referee both carry Doink's, which is what the
       source's own table says rather than a gap. */
    runner(&a, 840 + 30, WM_MOVE_LEFT);
    a.wrestler_num = WM_ROSTER_ANIM_ADAM_BOMB;
    r = wm_arcade_bounce_off_ropes(&a);
    assert(strcmp(r.bounce_anim, "dnk_bounce_anim") == 0);

    /* Outside the ring there are no ropes: INRING is 1 for OUTSIDE in
       the source, and this port's in_ring is the plain boolean. */
    runner(&a, 840 + 30, WM_MOVE_LEFT);
    a.in_ring = 0;
    r = wm_arcade_bounce_off_ropes(&a);
    assert(!r.bounced);
    assert(a.player_mode == WM_PMODE_RUNNING);
    assert(wm_arcade_bounce_off_ropes(NULL).bounced == false);
}

/*
 * RISK -- the point of the routine, and the state whose reader was
 * written long before anything could produce it.
 */
static void test_the_high_risk_window(void) {
    wm_arcade_actor_t a;
    wm_bounce_result_t r;

    runner(&a, 840 + 30, WM_MOVE_LEFT);
    r = wm_arcade_bounce_off_ropes(&a);
    assert(r.bounced);
    assert(r.opened_risk_window);
    assert(a.risk == WM_BOUNCE_RISK_TICKS);
    assert(a.risk == 60);
    /* A plain 60: the high bit is NOT set, so a rope-bounce hit counts
       as a hit without the taunt path's damage multiplier. */
    assert((a.risk & WM_ARCADE_RISK_HIGH_BIT) == 0);

    /* `MOVE *A13(GETUP_TIME),A0 / JRNZ ALREADY_DONE_RISK_MESS` -- and
       note where that label is: past the store, BEFORE the animation.
       He still bounces, he just gets no window. */
    runner(&a, 840 + 30, WM_MOVE_LEFT);
    a.getup_time = 5;
    r = wm_arcade_bounce_off_ropes(&a);
    assert(r.bounced);
    assert(!r.opened_risk_window);
    assert(a.risk == 0);
    assert(a.player_mode == WM_PMODE_BOUNCING);
    assert(r.bounce_anim != NULL);

    /* A window already open is not re-opened, and not extended. */
    runner(&a, 840 + 30, WM_MOVE_LEFT);
    a.risk = 12;
    r = wm_arcade_bounce_off_ropes(&a);
    assert(r.bounced);
    assert(!r.opened_risk_window);
    assert(a.risk == 12);
}

/* The seam, on both backends. */
static void test_the_seam_is_wired(void) {
    wm_wrestler_backend_actor st;
    wm_bret_backend_actor bva;
    wm_arcade_roster_callbacks_t roster_cb;
    wm_arcade_razor_callbacks_t razor_cb;
    wm_arcade_bret_callbacks_t bret_cb;
    wm_arcade_actor_t a;

    memset(&st, 0, sizeof st);
    st.wrestler_num = 2;
    roster_cb = wm_wrestler_roster_callbacks(&st);
    razor_cb = wm_wrestler_razor_callbacks(&st);
    wm_bret_backend_init(&bva);
    bret_cb = wm_bret_backend_callbacks(&bva);
    assert(roster_cb.bounce_off_ropes != NULL);
    assert(razor_cb.bounce_off_ropes != NULL);
    assert(bret_cb.bounce_off_ropes != NULL);

    runner(&a, 840 + 30, WM_MOVE_LEFT);
    a.wrestler_num = 2;                       /* the Undertaker */
    roster_cb.bounce_off_ropes(&a, roster_cb.user);
    assert(a.player_mode == WM_PMODE_BOUNCING);
    assert(a.risk == WM_BOUNCE_RISK_TICKS);
    assert(st.current_label != NULL);
    assert(strcmp(st.current_label, "und_bounce_anim") == 0);

    /* No bounce, no animation, no mode change -- the seam is called
       every running tick, so the common case has to be quiet. */
    memset(&st, 0, sizeof st);
    st.wrestler_num = 2;
    roster_cb = wm_wrestler_roster_callbacks(&st);
    runner(&a, WM_RING_X_CENTER, WM_MOVE_LEFT);
    a.wrestler_num = 2;
    roster_cb.bounce_off_ropes(&a, roster_cb.user);
    assert(a.player_mode == WM_PMODE_RUNNING);
    assert(a.risk == 0);
    assert(st.current_label == NULL);

    runner(&a, 840 + 30, WM_MOVE_LEFT);
    bret_cb.bounce_off_ropes(&a, bret_cb.user);
    assert(a.player_mode == WM_PMODE_BOUNCING);
    assert(a.risk == WM_BOUNCE_RISK_TICKS);
}

int main(void) {
    test_the_rope_lines_are_where_we_think();
    test_the_offsets_pull_the_line_inward();
    test_the_right_rope_is_the_mirror();
    test_the_bounce_itself();
    test_the_high_risk_window();
    test_the_seam_is_wired();
    printf("bounce off ropes ok\n");
    return 0;
}
