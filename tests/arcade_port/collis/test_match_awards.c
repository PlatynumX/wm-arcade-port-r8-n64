/*
 * The MATCH half of AWARD.ASM, which this port computed and never
 * granted.
 *
 * Every routine here was already translated. What did not exist was any
 * caller: wm_award_match_award was reached from nowhere, so TWO_RND_AWD,
 * PERFECT_AWD and FIVE_WINS_AWD could not be earned; arm_winstreak_award
 * was never called, so the winstreak award nothing armed could never
 * fire even though the routine that grants it ran; and adjust_perfects
 * and total_icons were never called, so a player's accumulated icon
 * total stayed zero and the bonus the select screen shows between
 * matches was always nothing.
 *
 * The three source call sites:
 *   LIFEBAR.ASM:2815  MATCH_AWARD a10,TWO_RND_AWD
 *   LIFEBAR.ASM:2823  MATCH_AWARD a10,PERFECT_AWD
 *   AWARD.ASM:1160    MATCH_AWARD a0,FIVE_WINS_AWD
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_match_end.h"
#include "wm/arcade/wm_arcade_lifebar.h"
#include "wm/award.h"
#include "wm/match.h"
#include "wm_arcade_roster.h"

/* ---- the two-round gate ------------------------------------------ */

static bool two_round(int32_t p1, int32_t p2, bool eight)
{
    wm_arcade_match_score_t sc;
    memset(&sc, 0, sizeof sc);
    sc.p1rounds = p1;
    sc.p2rounds = p2;
    return wm_match_two_round_victory(&sc, eight);
}

/*
 * `or a0,a3 / cmpi 2,a3` -- a bitwise OR of the two round counts, not a
 * sum and not a test on the winner's own count. It happens to mean "a
 * straight-rounds sweep", but by a route that has to be read carefully.
 */
static void test_two_round_victory_is_an_or(void)
{
    /* A 2-0 sweep, either side. */
    assert(two_round(2, 0, false));
    assert(two_round(0, 2, false));
    /* 2-1 is 3 and misses -- winning the match is not enough. */
    assert(!two_round(2, 1, false));
    assert(!two_round(1, 2, false));
    /* Nothing finished yet. */
    assert(!two_round(1, 1, false));
    assert(!two_round(0, 0, false));
    assert(!two_round(1, 0, false));
    /*
     * And the arithmetic's own edge: 2|2 is 2, so the source would award
     * it. The match cannot reach that state -- it ends the moment either
     * side hits two -- but the test is what the source wrote, so this is
     * pinned as written rather than tidied into a winner check.
     */
    assert(two_round(2, 2, false));

    /* `calla is_8_on_1 / jrc #no_2_rnd_victory` -- never in a gauntlet. */
    assert(!two_round(2, 0, true));
    assert(!wm_match_two_round_victory(NULL, false));
}

/* ---- is_perfect --------------------------------------------------- */

static void mk(wm_arcade_actor_t *a, int side, int32_t life)
{
    memset(a, 0, sizeof *a);
    a->active = 1;
    a->in_ring = 1;
    a->player_side = side;
    a->life = life;
}

static void test_is_perfect(void)
{
    wm_arcade_actor_t a[3];
    wm_arcade_actor_t *ptr[3] = { &a[0], &a[1], &a[2] };

    /* Winner's side untouched: perfect. */
    mk(&a[0], 0, WM_ARCADE_LIFE_MAX);
    mk(&a[1], 1, 0);
    mk(&a[2], 1, 7);
    assert(wm_match_is_perfect(ptr, 3, 0, false));
    /* The enemy being hurt is the whole idea -- it cannot cost the
       award, and the loser's own side is plainly not perfect. */
    assert(!wm_match_is_perfect(ptr, 3, 1, false));

    /* One point of damage on the winner loses it. */
    a[0].life = WM_ARCADE_LIFE_MAX - 1;
    assert(!wm_match_is_perfect(ptr, 3, 0, false));

    /*
     * It is the winning SIDE, not the winning wrestler: a partner who
     * got hit costs the award even if the winner never was.
     */
    mk(&a[0], 0, WM_ARCADE_LIFE_MAX);
    mk(&a[1], 0, WM_ARCADE_LIFE_MAX - 30);
    mk(&a[2], 1, 0);
    assert(!wm_match_is_perfect(ptr, 3, 0, false));

    /* An inactive slot is skipped, the way `jrz #nxt` skips an empty
       process_ptrs entry -- so a hurt partner who is not in the ring
       does not count. */
    a[1].active = 0;
    assert(wm_match_is_perfect(ptr, 3, 0, false));

    /* An eight-on-one is never perfect, however it was won. */
    a[1].active = 1;
    a[1].life = WM_ARCADE_LIFE_MAX;
    assert(wm_match_is_perfect(ptr, 3, 0, false));
    assert(!wm_match_is_perfect(ptr, 3, 0, true));

    assert(!wm_match_is_perfect(NULL, 3, 0, false));
}

/* ---- the sequence's new steps ------------------------------------- */

static int ARM_CALLS;
static int32_t ARMED[WM_AWARD_PLAYER_COUNT];
static int ADJUST_CALLS, TOTAL_CALLS, WINSTREAK_CALLS;

static void cb_arm(void *user, unsigned player, int32_t streak)
{
    (void)user;
    ++ARM_CALLS;
    if (player < WM_AWARD_PLAYER_COUNT) ARMED[player] = streak;
}
static void cb_adjust(void *user) { (void)user; ++ADJUST_CALLS; }
static void cb_total(void *user)  { (void)user; ++TOTAL_CALLS; }
static void cb_winstreak(void *user) { (void)user; ++WINSTREAK_CALLS; }

static void run_sequence(bool royal_rumble, int polling_ticks)
{
    wm_match_end_t st;
    wm_match_end_ctx_t ctx;
    wm_arcade_match_score_t sc;
    wm_match_streaks_t streaks;
    int i;

    memset(&sc, 0, sizeof sc);
    sc.p1rounds = 2; sc.p2rounds = 0; sc.match_winner = 1;
    memset(&streaks, 0, sizeof streaks);
    streaks.p1winstreak = 4;          /* a win makes it 5 -- armable */
    streaks.p2winstreak = 2;

    memset(&ctx, 0, sizeof ctx);
    ctx.score = &sc;
    ctx.streaks = &streaks;
    ctx.pstatus = 3;
    ctx.match_cnt = 7;
    ctx.winner_wrestler_num = WM_ROSTER_TAKER;
    ctx.awards_done = polling_ticks == 0;
    ctx.royal_rumble = royal_rumble;
    ctx.arm_winstreak = cb_arm;
    ctx.adjust_perfects = cb_adjust;
    ctx.total_icons = cb_total;
    ctx.run_winstreak_award = cb_winstreak;

    ARM_CALLS = 0; ADJUST_CALLS = 0; TOTAL_CALLS = 0; WINSTREAK_CALLS = 0;
    ARMED[0] = ARMED[1] = -1;

    wm_match_end_init(&st);
    wm_match_end_start(&st);
    for (i = 0; i < polling_ticks; ++i) (void)wm_match_end_tick(&st, &ctx);
    ctx.awards_done = true;
    for (i = 0; i < 4000 && st.phase != (uint8_t)WM_MEND_DONE; ++i)
        (void)wm_match_end_tick(&st, &ctx);
    assert(st.phase == (uint8_t)WM_MEND_DONE);
}

/*
 * arm_winstreak_award is called for BOTH players with the value
 * increment_wincount just wrote -- a loss writes zero, which disarms.
 * The header has described this call since it was written; nothing made
 * it, so nothing was ever armed.
 */
static void test_arm_winstreak_runs_for_both_players(void)
{
    run_sequence(false, 0);
    assert(ARM_CALLS == 2);
    /* Player 1 won (match_winner 1, pstatus 3): 4 -> 5. */
    assert(ARMED[0] == 5);
    /* Player 2 lost, so his streak was reset rather than stepped down,
       and he is armed with that zero -- which disarms him. */
    assert(ARMED[1] == 0);
}

/*
 * create_end_rnd_awards runs adjust_perfects once when the process
 * starts and total_icons once when the bars are done. The phase that
 * holds them is re-entered on every polling tick, so "once" has to be
 * real rather than incidental -- and so does check_for_award_for_
 * winstreak, which sits in the same phase.
 */
static void test_the_bar_work_happens_exactly_once(void)
{
    run_sequence(false, 0);
    assert(ADJUST_CALLS == 1);
    assert(TOTAL_CALLS == 1);
    assert(WINSTREAK_CALLS == 1);

    /* ...and still exactly once when the bar really does hold. */
    run_sequence(false, 200);
    assert(ADJUST_CALLS == 1);
    assert(TOTAL_CALLS == 1);
    assert(WINSTREAK_CALLS == 1);
}

/* `move @royal_rumble,a14 / jrz #cera_ok / DIE` -- in a rumble the
   whole award-bar process dies before doing either. */
static void test_a_rumble_does_neither(void)
{
    run_sequence(true, 0);
    assert(ADJUST_CALLS == 0);
    assert(TOTAL_CALLS == 0);
    /* The winstreak award is NOT inside create_end_rnd_awards -- it is
       `#end`'s own line, above it -- so a rumble does not skip it. */
    assert(WINSTREAK_CALLS == 1);
    /* Arming is in increment_wincount and is likewise unaffected. */
    assert(ARM_CALLS == 2);
}

/* ---- the whole thing, in a real match ----------------------------- */

static uint32_t hc(void *u) { return *(uint32_t *)u; }
static uint32_t spf(void *u) { (void)u; return 0u; }

static wm_match_state M;

/*
 * A real one-player match, won two rounds to nothing without the human
 * taking a hit. Every one of these was zero before this change.
 */
static void test_a_live_match_grants_them(void)
{
    WmRng r;
    uint32_t t = 1;
    wm_input_state in;
    int i;

    wm_rng_init(&r, 0x12345678u, hc, spf, &t);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_match_start_selected(&M, &r, (uint8_t)WM_ROSTER_TAKER);
    memset(&in, 0, sizeof in);

    for (i = 0; i < 6000 && M.match_end.match_over == 0; ++i) {
        unsigned k;
        for (k = 0; k < M.actor_count; ++k) {
            if (M.actors[k].player_side == 1) {
                M.actors[k].player_mode = (uint16_t)WM_PMODE_DEAD;
                M.actors[k].life = 0;
            } else {
                /* He is never touched, so the match is a perfect one. */
                M.actors[k].life = WM_ARCADE_LIFE_MAX;
            }
        }
        wm_match_tick(&M, NULL, &in);
    }

    assert(M.match_end.match_over == 2);
    assert(M.score.p1rounds == 2);
    assert(M.score.match_winner == 1);

    /* LIFEBAR.ASM:2815 -- two rounds to nothing. */
    assert(M.awards.match[0][WM_AWARD_TWO_ROUND] != 0u);
    /* LIFEBAR.ASM:2823 -- and without being hit. */
    assert(M.awards.match[0][WM_AWARD_PERFECT] != 0u);
    /* The loser gets neither, and cannot: he is a drone, and
       match_award refuses anybody from process slot 2 up anyway. */
    assert(M.awards.match[1][WM_AWARD_TWO_ROUND] == 0u);
    assert(M.awards.match[1][WM_AWARD_PERFECT] == 0u);

    /* total_icons ran, so there is a bonus to show between matches. */
    assert(M.awards.icon_total[0] != 0u);

    /* And the award subsystem's own copy of the win streak is written,
       which is what stops show_bonus_icons wiping that total. */
    assert(M.streaks.p1winstreak == 1);
    assert(M.awards.win_streak[0] == 1u);
    assert(wm_award_select_total(&M.awards, 0) == M.awards.icon_total[0]);
    /* One win is not five, so the winstreak award stays unarmed. */
    assert(M.awards.match[0][WM_AWARD_FIVE_WINS] == 0u);
}

int main(void)
{
    test_two_round_victory_is_an_or();
    test_is_perfect();
    test_arm_winstreak_runs_for_both_players();
    test_the_bar_work_happens_exactly_once();
    test_a_rumble_does_neither();
    test_a_live_match_grants_them();
    printf("match award tests passed\n");
    return 0;
}
