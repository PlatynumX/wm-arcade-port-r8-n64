/*
 * CODE_ADDR and MODE_WAITANIM -- the deferred half of a climb.
 *
 * Three of the climb checks do not climb on the spot when the wrestler
 * reaches the rope facing the wrong way. They turn him first and defer
 * the climb itself (WRESTLE2.ASM:196-202, :573-580, :697-704):
 *
 *      move    a2,*a13(NEW_FACING_DIR)
 *      calla   set_rotate_anim
 *      calla   change_anim1a
 *      movi    #climb,a0
 *      move    a0,*a13(CODE_ADDR),L    ;when the rotate anim
 *      SETMODE WAITANIM                ;finishes
 *
 * and each wrestler's own mode_waitanim (BRET.ASM:2550 and its seven
 * copies) `call`s CODE_ADDR once that animation ends.
 *
 * Every piece of this existed separately and none of it was joined up.
 * wm_ring_climb_continue was written and had no caller;
 * wm_confine_result_t reported only the checks that started an
 * animation outright, so WM_RING_CLIMB_ACTION_ROTATE_THEN_CONTINUE was
 * dropped; climb_to_actor copied four fields back and not PLYRMODE, so
 * the WAITANIM the module wrote never reached the wrestler; and
 * wm_arcade_actor_t::code_addr was read by all eight dispatchers and
 * written by nobody, so WM_PMODE_WAITANIM was a state a wrestler could
 * be put into and never leave.
 *
 * The consequence was not a missing animation. CLIMBING_THRU is set on
 * this path too, and only the climbthru animation's own ANI_INRING/
 * ANI_NOTINRING clears it (ANIM.ASM:405, :4471) -- so a wrestler who
 * walked into the side ropes facing away came out flagged as climbing
 * through with nothing running to unflag him, and ck_climb_in_side's
 * leading `if (climbing_thru) return` then refused him the ropes for
 * the rest of the match.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_confine.h"
#include "wm/arcade/wmania_ring_climb.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm/wrestler_backend.h"
#include "wm/bret_backend.h"

/* Same placement the ring-out tests use: box 30 either side of him. */
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
 * A wrestler on the apron, walking into the ring, at the X the mat edge
 * makes the cheaper correction -- `#no_r3`/`#no_l3`, which is the one
 * place confine_wrestler calls ck_climb_in_side (WRESTLE.ASM:3604).
 *
 * He is on the LEFT of the ring, so the box edge that overlaps the mat
 * line is OBJ_COLLX2 and the way in is MOVE_RIGHT, and ck_climb_in_side
 * wants him facing MOVE_DOWN_RIGHT to do it.
 */
static void place_at_left_mat_edge(wm_arcade_actor_t *a, int32_t *line_x_out) {
    const WmRingBoundarySeed *mat2 =
        wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_MAT2);
    int32_t line_x = wm_ring_calc_line_x(mat2, WM_RING_Z_CENTER);
    assert(line_x != 0);
    /* Box right edge five past the line: an X overlap of 5 against a Z
       overlap of over two hundred, so X is the smaller correction. */
    place(a, line_x - 25, WM_RING_Z_CENTER);
    a->in_ring = 0;
    a->move_dir = WM_MOVE_RIGHT;      /* he is walking into the ring */
    a->but_val_cur = 1u;              /* and pressing something */
    if (line_x_out) *line_x_out = line_x;
}

/*
 * The deferral itself: facing the wrong way, confine reports a
 * continuation and the wrestler is really parked in WAITANIM.
 */
static void test_the_wrong_facing_defers_the_climb(void) {
    wm_arcade_actor_t a;
    wm_arcade_actor_t *roster[1];
    wm_confine_result_t r;

    place_at_left_mat_edge(&a, NULL);
    a.facing_dir = WM_MOVE_DOWN_LEFT;      /* away from the ring */
    roster[0] = &a;

    wm_arcade_confine_wrestler_ex(&a, roster, 1u, 100u, &r);

    /* No animation started -- the turn comes first. */
    assert(r.climb_anim == NULL);
    /* The continuation the source would have put in CODE_ADDR. */
    assert(r.climb_rotate_then == WM_RING_CLIMB_CONT_IN_SIDE);
    assert(r.climb_facing == WM_MOVE_DOWN_RIGHT);
    /* `SETMODE WAITANIM`, on the wrestler and not just in the check. */
    assert(a.player_mode == WM_PMODE_WAITANIM);
    /* `movk 1,a0 / move a0,*a13(CLIMBING_THRU)`. */
    assert(a.climbing_thru == 1);
}

/*
 * set_rotate_anim (WRESTLE.ASM:5062), which the deferral calls before
 * change_anim1a: it picks the turn from (old facing, new facing) and
 * copies NEW_FACING_DIR into FACING_DIR on the way out, so the turn is
 * cosmetic -- he is already facing the ring while it plays.
 */
static void test_the_turn_animation_and_the_facing_copy(void) {
    wm_arcade_actor_t a;
    const char *turn;

    place(&a, 0, WM_RING_Z_CENTER);
    a.facing_dir = WM_MOVE_DOWN_LEFT;      /* convert_facing 5, >>1 = 2 */
    a.new_facing_dir = WM_MOVE_DOWN_RIGHT; /* convert_facing 3, >>1 = 1 */

    turn = wm_wrestler_set_rotate_anim(&a, 0 /* Bret */, a.facing_dir);
    assert(turn != NULL);
    assert(strcmp(turn, "hrt_6_to_4_turn_anim") == 0);
    /* WRESTLE.ASM:5082-5083, unconditional. */
    assert(a.facing_dir == WM_MOVE_DOWN_RIGHT);

    /* A wrestler with no table of his own (the referee's row is a real
       0 in #wres_rotate_anims) still gets the facing copy. */
    a.facing_dir = WM_MOVE_DOWN_LEFT;
    turn = wm_wrestler_set_rotate_anim(&a, 9 /* referee */, a.facing_dir);
    assert(turn == NULL);
    assert(a.facing_dir == WM_MOVE_DOWN_RIGHT);
}

/*
 * mode_waitanim's `call a0`: the continuation runs the climb the
 * deferral promised, and takes him back out of WAITANIM.
 */
static void test_the_continuation_finishes_the_climb(void) {
    wm_arcade_actor_t a;
    wm_arcade_actor_t *roster[1];
    wm_confine_result_t r;
    const char *label;

    place_at_left_mat_edge(&a, NULL);
    a.facing_dir = WM_MOVE_DOWN_LEFT;
    roster[0] = &a;
    wm_arcade_confine_wrestler_ex(&a, roster, 1u, 100u, &r);
    assert(r.climb_rotate_then == WM_RING_CLIMB_CONT_IN_SIDE);

    label = wm_arcade_climb_continue(&a, r.climb_rotate_then);
    assert(label != NULL);
    assert(strcmp(label, "hrt_climbin_side_anim") == 0);
    /* `SETMODE NORMAL` -- he is out of WAITANIM, which is the whole
       point: nothing else in the game clears that mode. */
    assert(a.player_mode == WM_PMODE_NORMAL);
    assert(a.climbing_thru == 1);

    /* WM_RING_CLIMB_CONT_NONE is not a continuation and does nothing. */
    a.player_mode = WM_PMODE_WAITANIM;
    assert(wm_arcade_climb_continue(&a, WM_RING_CLIMB_CONT_NONE) == NULL);
    assert(a.player_mode == WM_PMODE_WAITANIM);
    assert(wm_arcade_climb_continue(NULL, WM_RING_CLIMB_CONT_IN_SIDE) == NULL);
}

/*
 * The latch this fixes, stated as the behaviour that used to happen:
 * the check set CLIMBING_THRU and nothing ever started an animation, so
 * a second pass found `if (climbing_thru) return` and refused. Now the
 * continuation is what the second pass finds him having done.
 */
static void test_the_flag_does_not_latch_without_a_climb(void) {
    wm_arcade_actor_t a;
    wm_arcade_actor_t *roster[1];
    wm_confine_result_t r;

    place_at_left_mat_edge(&a, NULL);
    a.facing_dir = WM_MOVE_DOWN_LEFT;
    roster[0] = &a;
    wm_arcade_confine_wrestler_ex(&a, roster, 1u, 100u, &r);
    assert(a.climbing_thru == 1);
    assert(a.player_mode == WM_PMODE_WAITANIM);

    /*
     * A second confine pass while he is still in WAITANIM reports
     * nothing new -- ck_climb_in_side's own `if (climbing_thru) return`
     * -- and, critically, does not take him back out of the mode the
     * continuation is waiting on.
     */
    memset(&r, 0, sizeof r);
    wm_arcade_confine_wrestler_ex(&a, roster, 1u, 101u, &r);
    assert(r.climb_anim == NULL);
    assert(r.climb_rotate_then == WM_RING_CLIMB_CONT_NONE);
    assert(a.player_mode == WM_PMODE_WAITANIM);

    /* And the continuation still resolves it, whenever it arrives. */
    assert(wm_arcade_climb_continue(&a, WM_RING_CLIMB_CONT_IN_SIDE) != NULL);
    assert(a.player_mode == WM_PMODE_NORMAL);
}

/*
 * The direct path, for the PLYRMODE half of the same fix: facing the
 * right way already, ck_climb_in_side starts the climb outright AND
 * drops him to MODE_NORMAL (WRESTLE2.ASM:716). A runner climbing in is
 * the case where that is observable -- it used to be discarded, so he
 * stayed MODE_RUNNING through a climb.
 */
static void test_the_direct_climb_carries_the_mode_back(void) {
    wm_arcade_actor_t a;
    wm_arcade_actor_t *roster[1];
    wm_confine_result_t r;

    place_at_left_mat_edge(&a, NULL);
    a.facing_dir = WM_MOVE_DOWN_RIGHT;     /* already facing the ring */
    a.player_mode = WM_PMODE_RUNNING;      /* the running arm of the check */
    a.but_val_cur = 0u;                    /* which needs no button */
    a.getup_time = 0;
    roster[0] = &a;

    wm_arcade_confine_wrestler_ex(&a, roster, 1u, 100u, &r);

    assert(r.climb_rotate_then == WM_RING_CLIMB_CONT_NONE);
    assert(r.climb_anim != NULL);
    assert(strcmp(r.climb_anim, "hrt_climbin_side_anim") == 0);
    assert(a.player_mode == WM_PMODE_NORMAL);
    assert(a.climbing_thru == 1);
}

/*
 * The other end of the same wire: the callback each dispatcher's
 * WM_PMODE_WAITANIM case calls. All eight read CODE_ADDR and handed it
 * to this seam already -- there was simply nothing behind it, and no
 * caller set the field, so the mode was a dead end for every wrestler.
 */
static void test_the_waitanim_callback_is_wired(void) {
    wm_wrestler_backend_actor st;
    wm_bret_backend_actor bva;
    wm_arcade_roster_callbacks_t roster_cb;
    wm_arcade_razor_callbacks_t razor_cb;
    wm_arcade_bret_callbacks_t bret_cb;
    wm_arcade_actor_t a;

    memset(&st, 0, sizeof st);
    st.wrestler_num = 2;                   /* the Undertaker */
    roster_cb = wm_wrestler_roster_callbacks(&st);
    razor_cb = wm_wrestler_razor_callbacks(&st);
    assert(roster_cb.code_addr != NULL);
    assert(razor_cb.code_addr != NULL);

    wm_bret_backend_init(&bva);
    bret_cb = wm_bret_backend_callbacks(&bva);
    assert(bret_cb.code_addr != NULL);

    /* A wrestler parked in WAITANIM with a continuation in CODE_ADDR,
       exactly as the deferral above leaves him. */
    place(&a, 0, WM_RING_Z_CENTER);
    a.wrestler_num = 2;
    a.player_mode = WM_PMODE_WAITANIM;
    a.climbing_thru = 1;
    a.code_addr = (uintptr_t)WM_RING_CLIMB_CONT_IN_SIDE;

    roster_cb.code_addr(&a, (uint32_t)a.code_addr, roster_cb.user);

    assert(a.player_mode == WM_PMODE_NORMAL);
    assert(a.code_addr == 0);
    /* His own animation, started on his own program channel. */
    assert(st.current_label != NULL);
    assert(strcmp(st.current_label, "und_climbin_side_anim") == 0);

    /*
     * A token that is not a continuation is ignored rather than called.
     * The source's `call a0` would jump to whatever was in the field;
     * there is no C equivalent of that worth reproducing.
     */
    a.player_mode = WM_PMODE_WAITANIM;
    a.code_addr = 0x5eed;
    roster_cb.code_addr(&a, (uint32_t)a.code_addr, roster_cb.user);
    assert(a.player_mode == WM_PMODE_WAITANIM);
}

/*
 * climb_turnbuckle (WRESTLE2.ASM:103) -- the fourth climb check, the
 * third and last writer of CODE_ADDR, and the one whose callback seam
 * existed in all eight dispatchers with nothing behind it.
 *
 * The left turnbuckle sits at the left rope's X (855) and the ring
 * centre is 1074, so a wrestler at 880 has his box's left edge inside
 * the source's five pixels of leeway.
 */
static void place_at_left_turnbuckle(wm_arcade_actor_t *a, int32_t z) {
    place(a, 880, z);
    a->stick_val_cur = WM_MOVE_UP_LEFT;   /* the source wants it exactly */
}

static void test_the_turnbuckle_climb_and_its_deferral(void) {
    wm_arcade_actor_t a, other;
    wm_arcade_actor_t *roster[2];
    wm_climb_turnbuckle_result_t r;

    /*
     * Bret's row in #face_turnbuckle is 1 -- "back to turnbuckle" --
     * so the facing he wants is UP_LEFT's opposite, DOWN_RIGHT. Facing
     * it already, he climbs outright.
     */
    place_at_left_turnbuckle(&a, WM_RING_TOP);
    a.wrestler_num = 0;
    a.facing_dir = WM_MOVE_DOWN_RIGHT;
    roster[0] = &a;

    r = wm_arcade_climb_turnbuckle(&a, roster, 1u);
    assert(r.handled);
    assert(r.rotate_then == WM_RING_CLIMB_CONT_NONE);
    assert(r.anim != NULL);
    assert(strcmp(r.anim, "hrt_climb_up_anim") == 0);
    assert(r.facing == WM_MOVE_DOWN_RIGHT);
    assert(a.player_mode == WM_PMODE_CLIMBTURNBKL);

    /* Facing anywhere else, the turn comes first and the climb waits in
       CODE_ADDR -- the continuation this file's first test proved the
       rope climbs need, now with a producer. */
    place_at_left_turnbuckle(&a, WM_RING_TOP);
    a.wrestler_num = 0;
    a.facing_dir = WM_MOVE_UP_LEFT;
    r = wm_arcade_climb_turnbuckle(&a, roster, 1u);
    assert(r.handled);
    assert(r.anim == NULL);
    assert(r.rotate_then == WM_RING_CLIMB_CONT_TURNBUCKLE);
    assert(a.player_mode == WM_PMODE_WAITANIM);
    assert(a.new_facing_dir == WM_MOVE_DOWN_RIGHT);

    /* And it resolves the same way, into MODE_CLIMBTURNBKL. */
    assert(strcmp(wm_arcade_climb_continue(&a, r.rotate_then),
                  "hrt_climb_up_anim") == 0);
    assert(a.player_mode == WM_PMODE_CLIMBTURNBKL);

    /*
     * Yokozuna's row is 0 -- he climbs FACING the corner -- so the same
     * input wants the opposite facing of Bret's, untouched by the flip.
     */
    place_at_left_turnbuckle(&a, WM_RING_TOP);
    a.wrestler_num = 3;
    a.facing_dir = WM_MOVE_UP_LEFT;
    r = wm_arcade_climb_turnbuckle(&a, roster, 1u);
    assert(r.handled);
    assert(r.rotate_then == WM_RING_CLIMB_CONT_NONE);
    assert(r.anim != NULL);
    assert(strcmp(r.anim, "yok_climb_up_anim") == 0);
    assert(r.facing == WM_MOVE_UP_LEFT);

    /*
     * "give 'em 5 pixels of leeway": within five past RING_TOP he still
     * climbs, and the check glitches him exactly onto the line, which
     * has to reach the actor and not just the module's own row.
     */
    place_at_left_turnbuckle(&a, WM_RING_TOP + 4);
    a.wrestler_num = 0;
    a.facing_dir = WM_MOVE_DOWN_RIGHT;
    r = wm_arcade_climb_turnbuckle(&a, roster, 1u);
    assert(r.handled);
    assert(a.z_int == WM_RING_TOP);
    assert(a.z_fixed == ((int32_t)WM_RING_TOP << 16));

    /* Six past, and #not_top refuses -- nothing is reported and nothing
       on the actor moves. */
    place_at_left_turnbuckle(&a, WM_RING_TOP + 6);
    a.wrestler_num = 0;
    a.facing_dir = WM_MOVE_DOWN_RIGHT;
    r = wm_arcade_climb_turnbuckle(&a, roster, 1u);
    assert(!r.handled);
    assert(a.z_int == WM_RING_TOP + 6);
    assert(a.player_mode == WM_PMODE_NORMAL);

    /*
     * #climbit's own sweep: somebody already on the SAME turnbuckle
     * refuses the climb. The left one, because both are left of centre.
     */
    place_at_left_turnbuckle(&a, WM_RING_TOP);
    a.wrestler_num = 0;
    a.facing_dir = WM_MOVE_DOWN_RIGHT;
    place(&other, 880, WM_RING_TOP);
    other.player_mode = WM_PMODE_ONTURNBKL;
    roster[0] = &a;
    roster[1] = &other;
    r = wm_arcade_climb_turnbuckle(&a, roster, 2u);
    assert(!r.handled);

    /* On the OTHER turnbuckle he is no obstacle. */
    other.x_int = WM_RING_X_CENTER + 100;
    r = wm_arcade_climb_turnbuckle(&a, roster, 2u);
    assert(r.handled);
    assert(r.anim != NULL);

    /* The stick has to be exactly UP_LEFT, not merely include UP. */
    place_at_left_turnbuckle(&a, WM_RING_TOP);
    a.wrestler_num = 0;
    a.facing_dir = WM_MOVE_DOWN_RIGHT;
    a.stick_val_cur = WM_MOVE_UP;
    roster[0] = &a;
    assert(!wm_arcade_climb_turnbuckle(&a, roster, 1u).handled);

    /* And a wrestler already outside the ring cannot climb in from
       there: the source's `move *a13(INRING),a0 / jrnz #no_climb`. */
    place_at_left_turnbuckle(&a, WM_RING_TOP);
    a.wrestler_num = 0;
    a.facing_dir = WM_MOVE_DOWN_RIGHT;
    a.in_ring = 0;
    assert(!wm_arcade_climb_turnbuckle(&a, roster, 1u).handled);
}

/*
 * The turnbuckle callback, on both backends: the seam all eight
 * dispatchers call from mode_normal and neither backend filled.
 */
static void test_the_turnbuckle_callback_is_wired(void) {
    wm_wrestler_backend_actor st;
    wm_bret_backend_actor bva;
    wm_arcade_roster_callbacks_t roster_cb;
    wm_arcade_razor_callbacks_t razor_cb;
    wm_arcade_bret_callbacks_t bret_cb;
    wm_arcade_actor_t a;
    wm_arcade_actor_t *roster[1];

    memset(&st, 0, sizeof st);
    st.wrestler_num = 3;                   /* Yokozuna */
    roster_cb = wm_wrestler_roster_callbacks(&st);
    razor_cb = wm_wrestler_razor_callbacks(&st);
    wm_bret_backend_init(&bva);
    bret_cb = wm_bret_backend_callbacks(&bva);
    assert(roster_cb.climb_turnbuckle != NULL);
    assert(razor_cb.climb_turnbuckle != NULL);
    assert(bret_cb.climb_turnbuckle != NULL);

    place_at_left_turnbuckle(&a, WM_RING_TOP);
    a.wrestler_num = 3;
    a.facing_dir = WM_MOVE_UP_LEFT;        /* Yokozuna faces the corner */
    roster[0] = &a;
    st.all_actors = roster;
    st.all_actor_count = 1u;

    assert(roster_cb.climb_turnbuckle(&a, roster_cb.user) == 1);
    assert(a.player_mode == WM_PMODE_CLIMBTURNBKL);
    assert(st.current_label != NULL);
    assert(strcmp(st.current_label, "yok_climb_up_anim") == 0);

    /* The deferral through the same callback: wrong facing, so the turn
       plays and CODE_ADDR carries the climb. */
    place_at_left_turnbuckle(&a, WM_RING_TOP);
    a.wrestler_num = 3;
    a.facing_dir = WM_MOVE_DOWN_RIGHT;
    assert(roster_cb.climb_turnbuckle(&a, roster_cb.user) == 1);
    assert(a.player_mode == WM_PMODE_WAITANIM);
    assert(a.code_addr == (uintptr_t)WM_RING_CLIMB_CONT_TURNBUCKLE);
    /* set_rotate_anim ran: facing already caught up to the corner. */
    assert(a.facing_dir == WM_MOVE_UP_LEFT);

    /* Which mode_waitanim then resolves, through the other callback. */
    roster_cb.code_addr(&a, (uint32_t)a.code_addr, roster_cb.user);
    assert(a.player_mode == WM_PMODE_CLIMBTURNBKL);
    assert(a.code_addr == 0);
    assert(strcmp(st.current_label, "yok_climb_up_anim") == 0);

    /* A refusal is a refusal on the callback too -- the dispatcher has
       to keep processing the tick. */
    place_at_left_turnbuckle(&a, WM_RING_TOP + 6);
    a.wrestler_num = 3;
    assert(roster_cb.climb_turnbuckle(&a, roster_cb.user) == 0);
}

int main(void) {
    test_the_wrong_facing_defers_the_climb();
    test_the_turn_animation_and_the_facing_copy();
    test_the_continuation_finishes_the_climb();
    test_the_flag_does_not_latch_without_a_climb();
    test_the_direct_climb_carries_the_mode_back();
    test_the_waitanim_callback_is_wired();
    test_the_turnbuckle_climb_and_its_deferral();
    test_the_turnbuckle_callback_is_wired();
    printf("climb waitanim ok\n");
    return 0;
}
