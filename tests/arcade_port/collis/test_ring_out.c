/*
 * WRESTLE.ASM:3429 confine_wrestler's #outring branch, and the climb
 * checks that are the only way to reach it.
 *
 * ck_climb_out_top/bot/side have been fully translated in
 * wm/arcade/wmania_ring_climb.h for a long time and had NO CALLER --
 * their real call site is inside confine_wrestler itself. Climbing out
 * is the only thing in the game that clears INRING, so the entire
 * out-of-ring half of the routine was dead by construction, and the
 * header said so while giving the wrong reason for it.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_confine.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm/arcade/wmania_ring_climb.h"

/* A wrestler as the match places him: in the ring, box 30 either side. */
static void place(wm_arcade_actor_t *a, int32_t x, int32_t z) {
    memset(a, 0, sizeof *a);
    a->active = 1;
    a->in_ring = 1;
    a->x_int = x; a->x_fixed = x << 16;
    a->z_int = z; a->z_fixed = z << 16;
    a->hurt_box.x1 = x - 30;
    a->hurt_box.x2 = x + 30;
}

/*
 * The dispatch itself, and the polarity that makes it easy to get
 * backwards: PLYR.EQU's INRING is 1 for OUTSIDE (`jrnz #outring`) while
 * this port's field is the ordinary boolean.
 */
static void test_inring_chooses_the_branch(void) {
    wm_arcade_actor_t a;

    /* Inside, hard against the top ropes: the RING bound, not the arena. */
    place(&a, WM_RING_X_CENTER, WM_RING_TOP - 50);
    wm_arcade_confine_wrestler(&a);
    assert(a.z_int == WM_RING_TOP);
    assert(a.can_move_dir & WM_MOVE_UP);

    /*
     * The same Z with in_ring cleared, at an X clear of both the fence
     * and the apron, is well inside the ARENA -- so nothing clamps it
     * at all. That is the whole point of the branch and would be
     * invisible if the dispatch were ignored.
     *
     * The X matters and is not arbitrary. At ring-centre X an
     * "outside" wrestler is standing deep in the apron, and #cont_x
     * correctly ejects him to the mat edge; 700 is left of the mat and
     * right of the fence, which is where a man who has climbed out
     * actually stands.
     */
    place(&a, 700, WM_RING_TOP - 50);
    a.in_ring = 0;
    wm_arcade_confine_wrestler(&a);
    assert(a.z_int == WM_RING_TOP - 50);
    assert(a.can_move_dir == 0);
}

/* `#outring`'s Z: ARENA_TOP and ARENA_BOT stand in for the ropes. */
static void test_the_arena_bounds_the_outside(void) {
    wm_arcade_actor_t a;

    place(&a, WM_RING_X_CENTER, (int32_t)WM_ARENA_TOP - 20);
    a.in_ring = 0;
    wm_arcade_confine_wrestler(&a);
    assert(a.z_int == (int32_t)WM_ARENA_TOP);
    assert(a.z_fixed == ((int32_t)WM_ARENA_TOP << 16));
    assert(a.z_bound == (int32_t)WM_ARENA_TOP);
    assert(a.can_move_dir & WM_MOVE_UP);

    place(&a, WM_RING_X_CENTER, (int32_t)WM_ARENA_BOT + 20);
    a.in_ring = 0;
    wm_arcade_confine_wrestler(&a);
    assert(a.z_int == (int32_t)WM_ARENA_BOT);
    assert(a.can_move_dir & WM_MOVE_DOWN);

    /*
     * And the asymmetry worth recording: the IN-ring branch clears
     * Z_BOUND when the wrestler is comfortably between the ropes
     * (`#zd_ok`); `#outring` has no such clear, so Z_BOUND survives.
     */
    place(&a, 650, WM_RING_Z_CENTER);   /* clear of fence and apron */
    a.in_ring = 0;
    a.z_bound = 12345;
    wm_arcade_confine_wrestler(&a);
    assert(a.z_bound == 12345);

    place(&a, WM_RING_X_CENTER, WM_RING_Z_CENTER);
    a.z_bound = 12345;
    wm_arcade_confine_wrestler(&a);
    assert(a.z_bound == 0);
}

/*
 * `#check_x2`: the fences, which are much wider than the ropes. At the
 * arena's own top Z the left fence is at 618 and the right at 1736.
 */
static void test_the_fences_bound_the_outside(void) {
    wm_arcade_actor_t a;
    int32_t fence;

    /*
     * At the arena's own top Z the mat is out of range (calc_line_x
     * returns its zero there), so these two only exercise the fences.
     */
    fence = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_FENCE), WM_ARENA_TOP);
    assert(fence > 0);

    /* Walk into the left fence: pushed right until the box edge is on it. */
    place(&a, 400, (int32_t)WM_ARENA_TOP);
    a.in_ring = 0;
    wm_arcade_confine_wrestler(&a);
    assert(a.can_move_dir & WM_MOVE_LEFT);
    assert(a.x_bound == fence);
    /* The COLLISION BOX edge lands on the line, not the wrestler's X --
       OBJ_COLLX1 is what the source compares. */
    assert(a.x_int - 30 == fence);

    /* And the right one, against OBJ_COLLX2. */
    fence = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_FENCE), WM_ARENA_TOP);
    place(&a, 2100, (int32_t)WM_ARENA_TOP);
    a.in_ring = 0;
    wm_arcade_confine_wrestler(&a);
    assert(a.can_move_dir & WM_MOVE_RIGHT);
    assert(a.x_bound == fence);
    assert(a.x_int + 30 == fence);

    /*
     * "if we're attached, move our opponent too" -- and the FENCE
     * clamp writes X_BOUND onto the partner as well, where the
     * mat-edge clamp below moves only his position.
     */
    {
        wm_arcade_actor_t partner;
        int32_t before;
        place(&partner, 900, (int32_t)WM_ARENA_TOP);
        place(&a, 400, (int32_t)WM_ARENA_TOP);
        a.in_ring = 0;
        a.attach_proc = &partner;
        before = partner.x_int;
        fence = wm_ring_calc_line_x(
            wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_FENCE), WM_ARENA_TOP);
        wm_arcade_confine_wrestler(&a);
        assert(partner.x_int == before + (fence - (400 - 30)));
        assert(partner.x_bound == fence);
    }
}

/*
 * `#cont_x`: the mat edge. He is outside, walking into the apron, and
 * the routine takes whichever correction is smaller -- push him back
 * along X, or stand him on the mat edge along Z and offer the climb in.
 */
static void test_the_mat_edge_pushes_the_smaller_way(void) {
    wm_arcade_actor_t a;
    const WmRingBoundarySeed *mat2 =
        wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_MAT2);
    int32_t line_x;

    /*
     * Deep in X but only just past the mat's top edge: the Z overlap is
     * the small one, so he is stood on the edge and gets MOVE_DOWN.
     */
    line_x = wm_ring_calc_line_x(mat2, (int32_t)mat2->top_z + 2);
    assert(line_x != 0);
    /* On the LEFT of the ring, overlapping the apron means the box's
       RIGHT edge (OBJ_COLLX2) is past the mat line. */
    place(&a, line_x + 100, (int32_t)mat2->top_z + 2);
    a.in_ring = 0;
    wm_arcade_confine_wrestler(&a);
    assert(a.can_move_dir & WM_MOVE_DOWN);
    assert(a.z_int == (int32_t)mat2->top_z);
    assert(a.z_bound == (int32_t)mat2->top_z);

    /*
     * Mid-mat in Z but barely overlapping in X: now X is the small
     * correction, so he is pushed back off the apron instead and the
     * bit is MOVE_RIGHT -- he is on the LEFT of the ring, so the way he
     * cannot go is further in.
     */
    line_x = wm_ring_calc_line_x(mat2, WM_RING_Z_CENTER);
    assert(line_x != 0);
    /* Box right edge five past the line: xov 5 against a zov of over
       two hundred, so X is the cheaper correction. */
    place(&a, line_x - 25, WM_RING_Z_CENTER);
    a.in_ring = 0;
    wm_arcade_confine_wrestler(&a);
    assert(a.can_move_dir & WM_MOVE_RIGHT);
    assert(a.x_int == line_x - 25 - 5);
    assert(a.z_int == WM_RING_Z_CENTER);   /* Z untouched on this path */

    /*
     * Above the mat's own top_z, calc_line_x returns its out-of-range
     * zero and `jrz #done2` leaves him alone.
     */
    place(&a, 700, (int32_t)mat2->top_z - 20);
    a.in_ring = 0;
    wm_arcade_confine_wrestler(&a);
    assert(a.z_int == (int32_t)mat2->top_z - 20);
    assert(a.can_move_dir == 0);
}

/*
 * The gate crash, WRESTLE.ASM:3664. A runner who hits the fence bounces
 * off it, loses D_GATE_CRASH health, and -- if he hit the same gate
 * inside three seconds -- falls back instead of bouncing.
 */
static void test_the_gate_crash(void) {
    wm_arcade_actor_t a;
    wm_confine_result_t r;

    /* Running left into the left fence, facing left. */
    place(&a, 500, (int32_t)WM_ARENA_TOP);
    a.in_ring = 0;
    a.player_mode = WM_PMODE_RUNNING;
    a.facing_dir = WM_MOVE_LEFT;
    a.life = 100;
    a.run_time = 40;
    wm_arcade_confine_wrestler_ex(&a, NULL, 0, 10000u, &r);
    assert(r.gate_crash);
    assert(!r.gate_fall_back);          /* hit_gate_time 0 is long ago */
    assert(a.x_vel == (3 << 16));       /* bounced back to the right */
    assert(a.y_vel == (3 << 16));
    assert(a.player_mode == WM_PMODE_NORMAL);
    assert(a.run_time == 0);
    assert(a.life == 100 - WM_CONFINE_GATE_DAMAGE);
    assert(a.hit_gate_time == 10000u);

    /* Hit again a second later: fall back rather than bounce. */
    place(&a, 500, (int32_t)WM_ARENA_TOP);
    a.in_ring = 0;
    a.player_mode = WM_PMODE_RUNNING;
    a.facing_dir = WM_MOVE_LEFT;
    a.life = 100;
    a.hit_gate_time = 10000u;
    wm_arcade_confine_wrestler_ex(&a, NULL, 0, 10000u + 53u, &r);
    assert(r.gate_crash);
    assert(r.gate_fall_back);

    /*
     * "blow this off if we're running AWAY from the gate we've hit":
     * facing RIGHT while stopped on the LEFT fence is not a crash.
     */
    place(&a, 500, (int32_t)WM_ARENA_TOP);
    a.in_ring = 0;
    a.player_mode = WM_PMODE_RUNNING;
    a.facing_dir = WM_MOVE_RIGHT;
    a.life = 100;
    wm_arcade_confine_wrestler_ex(&a, NULL, 0, 10000u, &r);
    assert(!r.gate_crash);
    assert(a.life == 100);

    /* Not running at all: nothing. */
    place(&a, 500, (int32_t)WM_ARENA_TOP);
    a.in_ring = 0;
    a.facing_dir = WM_MOVE_LEFT;
    a.life = 100;
    wm_arcade_confine_wrestler_ex(&a, NULL, 0, 10000u, &r);
    assert(!r.gate_crash);
    assert(a.life == 100);
}

/*
 * WRESTLE.ASM:3718 `#dead`: a zombie who reaches the arena edge
 * transforms there, which is the faster of the two ways out of zombie
 * mode. All three conditions have to hold.
 */
static void test_the_zombie_transforms_at_the_edge(void) {
    wm_arcade_actor_t a;
    wm_confine_result_t r;

    place(&a, 500, (int32_t)WM_ARENA_TOP);
    a.in_ring = 0;
    a.player_mode = WM_PMODE_DEAD;
    a.status_flags = WM_STATUS_ZOMBIE | WM_STATUS_CAN_XFORM;
    wm_arcade_confine_wrestler_ex(&a, NULL, 0, 0u, &r);
    assert(a.can_move_dir & WM_MOVE_LEFT);
    assert(r.zombie_transform);

    /* CAN_XFORM not set yet -- mode_dead has not started his run. */
    place(&a, 500, (int32_t)WM_ARENA_TOP);
    a.in_ring = 0;
    a.player_mode = WM_PMODE_DEAD;
    a.status_flags = WM_STATUS_ZOMBIE;
    wm_arcade_confine_wrestler_ex(&a, NULL, 0, 0u, &r);
    assert(!r.zombie_transform);

    /* Dead but not a zombie: just a corpse against the fence. */
    place(&a, 500, (int32_t)WM_ARENA_TOP);
    a.in_ring = 0;
    a.player_mode = WM_PMODE_DEAD;
    a.status_flags = WM_STATUS_CAN_XFORM;
    wm_arcade_confine_wrestler_ex(&a, NULL, 0, 0u, &r);
    assert(!r.zombie_transform);

    /* Nowhere near a fence: not stopped in X, so no cue. */
    place(&a, WM_RING_X_CENTER, (int32_t)WM_ARENA_TOP);
    a.in_ring = 0;
    a.player_mode = WM_PMODE_DEAD;
    a.status_flags = WM_STATUS_ZOMBIE | WM_STATUS_CAN_XFORM;
    wm_arcade_confine_wrestler_ex(&a, NULL, 0, 0u, &r);
    assert(!(a.can_move_dir & (WM_MOVE_LEFT | WM_MOVE_RIGHT)));
    assert(!r.zombie_transform);
}

/*
 * The point of the whole change: confine_wrestler now CALLS the climb
 * checks, so a wrestler pinned against the bottom ropes with an
 * opponent already outside can start the climbthru animation that
 * clears INRING.
 */
static void test_the_ropes_offer_a_way_out(void) {
    wm_arcade_actor_t me, opp;
    wm_arcade_actor_t *roster[2];
    wm_confine_result_t r;

    place(&me, WM_RING_X_CENTER, WM_RING_BOT + 10);
    me.player_side = 0;
    me.but_val_cur = 1u;              /* he is pressing something */
    place(&opp, WM_RING_X_CENTER, WM_RING_Z_CENTER);
    opp.player_side = 1;
    opp.in_ring = 0;                  /* already outside -- any_opp_outside */
    roster[0] = &me;
    roster[1] = &opp;

    wm_arcade_confine_wrestler_ex(&me, roster, 2u, 100u, &r);

    /* The clamp still happened. */
    assert(me.z_int == WM_RING_BOT);
    assert(me.can_move_dir & WM_MOVE_DOWN);
    /* And the climb was offered: a real per-wrestler animation label. */
    assert(r.climb_anim != NULL);
    assert(me.climbing_thru == 2);

    /*
     * With no opponent outside, any_opp_outside refuses and nothing is
     * offered -- the clamp alone is what the routine used to do.
     */
    place(&me, WM_RING_X_CENTER, WM_RING_BOT + 10);
    me.but_val_cur = 1u;
    place(&opp, WM_RING_X_CENTER, WM_RING_Z_CENTER);
    opp.player_side = 1;
    wm_arcade_confine_wrestler_ex(&me, roster, 2u, 100u, &r);
    assert(me.z_int == WM_RING_BOT);
    assert(r.climb_anim == NULL);
    assert(me.climbing_thru == 0);

    /* No roster at all is the old entry point's behaviour, unchanged. */
    place(&me, WM_RING_X_CENTER, WM_RING_BOT + 10);
    wm_arcade_confine_wrestler(&me);
    assert(me.z_int == WM_RING_BOT);
    assert(me.climbing_thru == 0);
}

int main(void) {
    test_inring_chooses_the_branch();
    test_the_arena_bounds_the_outside();
    test_the_fences_bound_the_outside();
    test_the_mat_edge_pushes_the_smaller_way();
    test_the_gate_crash();
    test_the_zombie_transforms_at_the_edge();
    test_the_ropes_offer_a_way_out();
    printf("ring out ok\n");
    return 0;
}
