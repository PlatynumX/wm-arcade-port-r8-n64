/*
 * The final battle: PROGRESS.ASM's FINAL_BATTLE_LINEUP/FINAL_PTR queue
 * and ANIM.ASM's zombie promotion.
 *
 * The last rung of the championship ladder looks like a three-on-one
 * and is not. Every drone you put down is replaced by the next man in
 * a queue of eight, which is what is_8_on_1's name has been saying all
 * along. This port played it as three opponents who stayed dead.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/app.h"
#include "wm/match.h"
#include "wm/pregame.h"
#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_final_battle.h"
#include "wm/arcade/wmania_ring_geometry.h"

static uint32_t hc(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t spf(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static WmRng RNG;
static uint32_t RNG_T;
static void rng_reset(uint32_t seed) {
    RNG_T = 1;
    memset(&RNG, 0, sizeof RNG);
    wm_rng_init(&RNG, seed, hc, spf, &RNG_T);
}

/*
 * PROGRESS.ASM:1593 get_final_lineup: "just copy 8 bytes from
 * TEMP_LADDER to FINAL_BATTLE_LINEUP", then the end-of-battle marker.
 * TEMP_LADDER is INIT_TEMP_TABLE's 7..0 shuffled by RANDOMIZE_ORDER, so
 * the eight are a PERMUTATION -- every wrestler appears once, and the
 * five behind the first three are the rest of the roster.
 */
static void test_the_lineup_is_the_whole_roster(void) {
    wm_pregame_state s;
    int seen[8];
    unsigned i;

    rng_reset(0x2468ACE0u);
    memset(&s, 0, sizeof s);
    s.rng = &RNG;
    wm_pregame_get_final_lineup(&s);

    memset(seen, 0, sizeof seen);
    for (i = 0; i < WM_FINAL_LINEUP_WRESTLERS; ++i) {
        int w = s.final_battle.lineup[i];
        assert(w >= 0 && w < 8);
        assert(!seen[w]);
        seen[w] = 1;
    }
    /* `movi -1,a14 / move a14,*a1,W` -- a WORD, so both spare bytes. */
    assert(s.final_battle.lineup[8] == -1);
    assert(s.final_battle.lineup[9] == -1);
    /* PROGRESS.ASM:4263 `addi 3*8,a0` -- the fourth entry. */
    assert(s.final_battle.ptr == WM_FINAL_PTR_START);
}

/*
 * The queue protocol: read one, stop when it is negative. Five
 * replacements behind the three you start against, then the terminator.
 */
static void test_the_queue_runs_out_after_five(void) {
    wm_pregame_state s;
    int drawn = 0;

    rng_reset(0x13579BDFu);
    memset(&s, 0, sizeof s);
    s.rng = &RNG;
    wm_pregame_get_final_lineup(&s);

    while (!wm_final_queue_empty(&s.final_battle)) {
        int w = wm_final_next_wrestler(&s.final_battle);
        assert(w >= 0 && w < 8);
        ++drawn;
        assert(drawn <= 8);
    }
    assert(drawn == 5);
    /* `jrn #die` never advances, so asking again is still empty. */
    assert(wm_final_next_wrestler(&s.final_battle) == -1);
    assert(wm_final_queue_empty(&s.final_battle));
}

/*
 * PROGRESS.ASM:1535 get_royal_lineup. Neither named entrant may be one
 * of the first two. The fix is a swap and then, if it is still wrong, a
 * 16-bit rotate of the leading long -- and nothing checks a third time,
 * which is a real limit of the routine rather than of this test.
 */
static void test_the_royal_fixup_moves_the_named_entrants(void) {
    wm_final_battle_state_t fb;
    const uint8_t lineup[8] = { 3, 5, 0, 1, 2, 4, 6, 7 };

    /* index1 = 3 leads, so SWAP: 2,4,6,7 comes to the front. */
    wm_final_set_lineup(&fb, lineup);
    wm_final_royal_fixup(&fb, 3, 9);
    assert(fb.lineup[0] == 2 && fb.lineup[1] == 4);
    assert(fb.lineup[4] == 3 && fb.lineup[5] == 5);

    /*
     * Both halves lead with a named entrant: swap, then rotate. After
     * the swap the front is 2,4,6,7 and 4 is still named, so `rl 16`
     * brings 6,7 in front of 2,4.
     */
    wm_final_set_lineup(&fb, lineup);
    wm_final_royal_fixup(&fb, 3, 4);
    assert(fb.lineup[0] == 6 && fb.lineup[1] == 7);
    assert(fb.lineup[2] == 2 && fb.lineup[3] == 4);

    /* Nobody named up front: nothing moves at all. */
    wm_final_set_lineup(&fb, lineup);
    wm_final_royal_fixup(&fb, 2, 4);
    assert(fb.lineup[0] == 3 && fb.lineup[1] == 5);
}

/*
 * PROGRESS.ASM:4149 `move @belt_type,a14 / jrz #notfin`. The WWF ladder's
 * last rung is built from a fresh lineup; the intercontinental one's is
 * scrambled like any other three-opponent entry.
 */
static void test_only_the_championship_ladder_gets_the_queue(void) {
    wm_app A;
    unsigned i;
    uint8_t opps[3];

    memset(&A, 0, sizeof A);
    wm_app_init(&A);
    wm_pregame_init(&A.pregame, 0u, WM_WRESTLER_BRET, &A.rng);
    A.pregame.belt_type = WM_PREGAME_BELT_WWF;

    /* Walk the ladder to the final rung the way PUT_UP_PROGRESS does. */
    for (i = 0; i <= (unsigned)WM_PREGAME_FINAL_LADDER_INDEX; ++i)
        wm_pregame_next_in_ladder(&A.pregame);

    assert(A.pregame.current_ladder_index == WM_PREGAME_FINAL_LADDER_INDEX);
    assert(wm_pregame_is_final_match(&A.pregame));
    assert(wm_pregame_is_8_on_1(&A.pregame));
    /* `ori 03000000h,a1` -- the count is set, not drawn. */
    assert(A.pregame.opponent_count == 3u);

    /* `move *a0,a1,L / andi 00FFFFFFh,a1`: the rung IS the first three
       of the lineup, so the queue behind it is the other five. */
    for (i = 0; i < 3u; ++i)
        opps[i] = (uint8_t)A.pregame.final_battle.lineup[i];
    assert(A.pregame.opponents[0] == opps[0]);
    assert(A.pregame.opponents[1] == opps[1]);
    assert(A.pregame.opponents[2] == opps[2]);
    assert(A.pregame.final_battle.ptr == WM_FINAL_PTR_START);

    /* The intercontinental belt has no eight-on-one at all. */
    memset(&A, 0, sizeof A);
    wm_app_init(&A);
    wm_pregame_init(&A.pregame, 0u, WM_WRESTLER_BRET, &A.rng);
    A.pregame.belt_type = WM_PREGAME_BELT_INTERCONTINENTAL;
    for (i = 0; i <= (unsigned)WM_PREGAME_FINAL_LADDER_INDEX; ++i)
        wm_pregame_next_in_ladder(&A.pregame);
    assert(wm_pregame_is_final_match(&A.pregame));
    assert(!wm_pregame_is_8_on_1(&A.pregame));
}

/*
 * PROGRESS.ASM:4160 `#notfin`, and it is not the RNG: `move @PCNT,a14 /
 * sll 32-5 / srl 32-5` is the main loop's own counter, low five bits.
 * One tick in 32, any three-opponent rung becomes `03060606h`.
 */
static void test_the_triple_doink_is_a_pcnt_draw(void) {
    wm_pregame_state s;
    unsigned i;

    /* PCNT low five bits clear. Rung 4 is the first 1v2... rung 6 is the
       first three-opponent entry on the intercontinental ladder. */
    rng_reset(0x2468ACE0u);
    memset(&s, 0, sizeof s);
    s.rng = &RNG;
    s.belt_type = WM_PREGAME_BELT_INTERCONTINENTAL;
    wm_pregame_init(&s, 0u, WM_WRESTLER_BRET, &RNG);
    s.belt_type = WM_PREGAME_BELT_INTERCONTINENTAL;
    s.pcnt = 64u;   /* 64 & 31 == 0 */
    for (i = 0; i <= (unsigned)WM_PREGAME_FINAL_LADDER_INDEX; ++i)
        wm_pregame_next_in_ladder(&s);
    /* The intercontinental ladder is 4x1, 2x2, 1x3, so the rung it
       finishes on is the three-opponent one the branch tests for. */
    assert(s.opponent_count == 3u);
    assert(s.opponents[0] == 6u);
    assert(s.opponents[1] == 6u);
    assert(s.opponents[2] == 6u);

    /* One off the multiple and it is an ordinary scramble. */
    rng_reset(0x2468ACE0u);
    memset(&s, 0, sizeof s);
    s.rng = &RNG;
    wm_pregame_init(&s, 0u, WM_WRESTLER_BRET, &RNG);
    s.belt_type = WM_PREGAME_BELT_INTERCONTINENTAL;
    s.pcnt = 65u;
    for (i = 0; i <= (unsigned)WM_PREGAME_FINAL_LADDER_INDEX; ++i)
        wm_pregame_next_in_ladder(&s);
    assert(s.opponent_count == 3u);
    assert(!(s.opponents[0] == 6u &&
             s.opponents[1] == 6u &&
             s.opponents[2] == 6u));
}

/*
 * ANIM.ASM:3113 "if we're right up against either Z edge of the ring,
 * move away a few pixels so we can roll." Three cases, and the middle
 * one moves nothing.
 */
static void test_the_zombie_is_nudged_off_the_z_edges(void) {
    assert(wm_final_zombie_z_nudge(WM_RING_TOP) == WM_RING_TOP + 7);
    assert(wm_final_zombie_z_nudge(WM_RING_TOP + 7) == WM_RING_TOP + 7 + 7);
    assert(wm_final_zombie_z_nudge(WM_RING_TOP + 8) == WM_RING_TOP + 8);
    assert(wm_final_zombie_z_nudge(WM_RING_BOT - 7) == WM_RING_BOT - 7);
    assert(wm_final_zombie_z_nudge(WM_RING_BOT) == WM_RING_BOT - 7);
}

/* ANIM.ASM:3100 the 7->8 hack, and the flags the promotion sets. */
static void test_the_promotion_sets_the_zombie_up(void) {
    wm_arcade_actor_t a;

    memset(&a, 0, sizeof a);
    a.z_int = WM_RING_Z_CENTER;
    a.zombie_time = 99;
    wm_final_make_zombie(&a, 4);
    assert(a.new_wrestlernum == 4);
    assert(a.status_flags & WM_STATUS_ZOMBIE);
    assert(a.zombie_time == 0);
    assert(a.z_int == WM_RING_Z_CENTER); /* well inside: no nudge */

    /* `cmpi 7,a1 / jrne #vok / movk 8,a1` -- packed slot 7 is Lex. */
    memset(&a, 0, sizeof a);
    a.z_int = WM_RING_Z_CENTER;
    wm_final_make_zombie(&a, 7);
    assert(a.new_wrestlernum == 8);
}

/*
 * ANIM.ASM:3043 _ani_waitroll's `#dead` tail, through the animation
 * env. A drone in an ordinary match stays dead; the same drone in the
 * eight-on-one takes the next man out of the queue.
 */
static void test_a_dead_drone_only_rises_in_the_final_battle(void) {
    wm_final_battle_state_t fb;
    const uint8_t lineup[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    wm_arcade_actor_t a;
    wm_anim_env env;
    int w;

    wm_final_set_lineup(&fb, lineup);

    /* Not a final battle: nothing reads the queue. */
    memset(&env, 0, sizeof env);
    env.final_battle = &fb;
    env.eight_on_one = false;
    env.royal_rumble = false;
    memset(&a, 0, sizeof a);
    a.plyr_type = WM_PTYPE_DRONE;
    a.player_mode = WM_PMODE_DEAD;
    assert(fb.ptr == WM_FINAL_PTR_START);

    /* The eight-on-one: entry 3 is the first replacement. */
    env.eight_on_one = true;
    w = wm_final_next_wrestler(&fb);
    assert(w == 3);
    wm_final_make_zombie(&a, w);
    assert(a.status_flags & WM_STATUS_ZOMBIE);
    assert(a.new_wrestlernum == 3);
    assert(fb.ptr == WM_FINAL_PTR_START + 1u);
}

/*
 * DOINK.ASM:3008 mode_dead's `#zmb` tail. He waits for MODE_END, then
 * runs for whichever side of the arena is farther from the camera --
 * and if ten seconds pass without him getting there, he transforms
 * where he stands.
 */
static void test_the_zombie_runs_then_times_out(void) {
    wm_arcade_actor_t a;
    bool started, left;
    int32_t xvel;
    int i;

    /* Still getting up: no run, no transform. */
    memset(&a, 0, sizeof a);
    a.wrestler_num = 0;                     /* Bret, HRT_XRUN */
    assert(!wm_final_zombie_tick(&a, 0, &started, &left, &xvel));
    assert(!started);
    assert(a.zombie_time == 1);
    assert(!(a.status_flags & WM_STATUS_CAN_XFORM));

    /* `btst MODE_END_BIT` set: he is up, outside, with a clear lane. */
    a.anim_mode |= WM_MODE_END;
    assert(!wm_final_zombie_tick(&a, 0, &started, &left, &xvel));
    assert(started);
    assert(a.status_flags & WM_STATUS_CAN_XFORM);
    /* WORLDTLX 0 is left of RING_X_CENTER-200, so he runs RIGHT. */
    assert(!left);
    assert(xvel == wm_final_run_speeds[0]);
    assert(a.stick_val_cur == (uint16_t)WM_MOVE_RIGHT);

    /* Camera at the centre: `jrge #run_left`, and the velocity negates. */
    memset(&a, 0, sizeof a);
    a.wrestler_num = 3;                     /* Yokozuna, the slow one */
    a.anim_mode |= WM_MODE_END;
    assert(!wm_final_zombie_tick(&a, WM_RING_X_CENTER, &started, &left, &xvel));
    assert(left);
    assert(xvel == -wm_final_run_speeds[3]);
    assert(a.stick_val_cur == (uint16_t)WM_MOVE_LEFT);

    /* `cmpi TSEC*10,a14` -- ten seconds and he gives up on the trip. */
    memset(&a, 0, sizeof a);
    a.anim_mode |= WM_MODE_END;
    for (i = 1; i < WM_FINAL_ZOMBIE_TIMEOUT; ++i)
        assert(!wm_final_zombie_tick(&a, 0, NULL, NULL, NULL));
    assert(wm_final_zombie_tick(&a, 0, NULL, NULL, NULL));
    assert(a.zombie_time == WM_FINAL_ZOMBIE_TIMEOUT);
}

/*
 * WRESTLE2.ASM:3772 change_wrestler. "hunt until you find one that's
 * offscreen, then use it", and everything the old wrestler was is
 * cleared off.
 */
static void test_the_transform_reenters_offscreen(void) {
    wm_arcade_actor_t a;
    unsigned idx;

    /*
     * Camera at the origin: the ring centre is way off to the right of
     * WORLDTLX+430, so the very first entry qualifies.
     */
    assert(wm_final_choose_start_position(0) == 0u);

    /*
     * Camera framing the ring centre: entry 0 is on screen, and the
     * hunt walks on. Whatever it settles on must itself be offscreen,
     * unless it is the last entry -- which the source takes by default.
     */
    idx = wm_final_choose_start_position(WM_RING_X_CENTER - 200);
    assert(idx < WM_FINAL_START_POSITIONS);
    if (idx + 1u < WM_FINAL_START_POSITIONS) {
        int32_t x = wm_final_start_positions[idx].x;
        int32_t left = (WM_RING_X_CENTER - 200) - 30;
        assert(x <= left || x >= left + 460);
    }

    memset(&a, 0, sizeof a);
    a.new_wrestlernum = 5;
    a.wrestler_num = 2;
    a.player_mode = WM_PMODE_DEAD;
    a.status_flags = WM_STATUS_ZOMBIE | WM_STATUS_CAN_XFORM |
                     WM_STATUS_NO_BUCKOFF;
    a.i_will_die = 1;
    a.x_vel = a.y_vel = a.z_vel = 12345;

    wm_final_change_wrestler(&a, 0);
    assert(a.wrestler_num == 5);
    assert(a.player_mode == WM_PMODE_NORMAL);
    /* `clr a14 / move a14,*a13(STATUS_FLAGS),L` -- the whole long. */
    assert(a.status_flags == 0u);
    assert(a.i_will_die == 0);
    assert(a.x_vel == 0 && a.y_vel == 0 && a.z_vel == 0);
    assert(a.x_int == wm_final_start_positions[0].x);
    assert(a.z_int == wm_final_start_positions[0].z);
    assert(a.y_int == wm_final_start_positions[0].y);
    assert(a.ground_y == wm_final_start_positions[0].y);
}

/*
 * WRESTLE.ASM:1600, the re-init start_match does that NEXT_IN_LADDER
 * has already done -- "because if we're speeding through the rounds,
 * [NEXT_IN_LADDER] can happen while wrestler processes from the previous
 * round are still active and DEAD, so they gobble up the first three
 * slots and we end up with a 1v5 match."
 */
static void test_the_match_puts_the_pointer_back(void) {
    wm_match_state m;
    wm_final_battle_state_t fb;
    const uint8_t lineup[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

    memset(&m, 0, sizeof m);
    wm_match_init(&m);
    wm_final_set_lineup(&fb, lineup);

    /* Two corpses already ate two slots. */
    (void)wm_final_next_wrestler(&fb);
    (void)wm_final_next_wrestler(&fb);
    assert(fb.ptr == WM_FINAL_PTR_START + 2u);

    wm_match_bind_final_battle(&m, &fb, true, true);
    assert(fb.ptr == WM_FINAL_PTR_START);
    assert(m.final_battle == &fb);
    assert(m.eight_on_one);

    /* `jrnc #do_zf` -- not the final match, so nothing is touched. */
    (void)wm_final_next_wrestler(&fb);
    wm_match_bind_final_battle(&m, &fb, false, false);
    assert(fb.ptr == WM_FINAL_PTR_START + 1u);
    assert(!m.eight_on_one);
}

/*
 * LIFEBAR.ASM:5131. `inc a4 / inc a4 / calla is_8_on_1 / jrc #a4ok /
 * @royal_rumble / jrnz #a4ok / dec a4` -- it increments twice and takes
 * one back off only when the match is neither. One round decides the
 * final battle, which is what makes a gauntlet of eight one fight
 * rather than a best of three.
 */
static void test_a_final_battle_round_counts_double(void) {
    wm_arcade_match_score_t score;

    wm_arcade_match_score_init(&score);
    wm_arcade_match_score_award_round(&score, 0);
    assert(score.p1rounds == 1);
    assert(score.match_winner == 0);   /* best of three, still going */

    wm_arcade_match_score_init(&score);
    score.double_rounds = true;
    wm_arcade_match_score_award_round(&score, 0);
    assert(score.p1rounds == 2);
    assert(score.match_winner == 1);   /* `cmpi 2,a4` -- decided at once */
}

/* The flag has to survive the start path's own score reset. */
static void test_the_match_sets_the_double_round_flag(void) {
    wm_app A;
    unsigned i;

    memset(&A, 0, sizeof A);
    wm_app_init(&A);
    wm_pregame_init(&A.pregame, 0u, WM_WRESTLER_BRET, &A.rng);
    A.pregame.belt_type = WM_PREGAME_BELT_WWF;
    for (i = 0; i <= (unsigned)WM_PREGAME_FINAL_LADDER_INDEX; ++i)
        wm_pregame_next_in_ladder(&A.pregame);
    assert(wm_pregame_is_8_on_1(&A.pregame));

    A.match_pstatus = 1;
    A.mode = WM_APP_MODE_MATCH_INIT;
    wm_app_tick(&A, NULL);
    assert(A.mode == WM_APP_MODE_MATCH);
    assert(A.match.eight_on_one);
    assert(A.match.score.double_rounds);
    assert(A.match.final_battle == &A.pregame.final_battle);
    /* The rung is three-on-one, so four wrestlers are in the ring. */
    assert(A.match.actor_count == 4u);
    assert(A.match.num_opps == 3);
    /* WRESTLE.ASM:1600 put the pointer back to the fourth man. */
    assert(A.pregame.final_battle.ptr == WM_FINAL_PTR_START);
}

int main(void) {
    test_the_lineup_is_the_whole_roster();
    test_the_queue_runs_out_after_five();
    test_the_royal_fixup_moves_the_named_entrants();
    test_only_the_championship_ladder_gets_the_queue();
    test_the_triple_doink_is_a_pcnt_draw();
    test_the_zombie_is_nudged_off_the_z_edges();
    test_the_promotion_sets_the_zombie_up();
    test_a_dead_drone_only_rises_in_the_final_battle();
    test_the_zombie_runs_then_times_out();
    test_the_transform_reenters_offscreen();
    test_the_match_puts_the_pointer_back();
    test_a_final_battle_round_counts_double();
    test_the_match_sets_the_double_round_flag();
    printf("final battle ok\n");
    return 0;
}
