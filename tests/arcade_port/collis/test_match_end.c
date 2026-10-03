/*
 * LIFEBAR.ASM:2852 DO_WAIT -- what happens once somebody has two
 * rounds. A finished match used to be a dead end the same way a
 * decided round was.
 */
#include <assert.h>
#include <string.h>

#include "wm_arcade_match_end.h"
#include "wm/match.h"
#include "wm_arcade_roster.h"

/* ---- increment_wincount ------------------------------------------ */

static void test_increment_wincount(void) {
    wm_match_streaks_t st;
    unsigned cleared;

    memset(&st, 0, sizeof st);
    st.p1winstreak = 4;
    st.p2winstreak = 7;

    /* Two humans, player 1 won. */
    cleared = wm_match_increment_wincount(&st, 1, 3);

    /* The OLD values are saved first -- loser_snd reads them to tell
       a broken streak from no streak, so if they were saved after the
       update the sound would never play. */
    assert(st.p1oldwinstreak == 4);
    assert(st.p2oldwinstreak == 7);

    /* A win steps up; a loss resets to zero rather than stepping
       down, because the increment is computed and then discarded. */
    assert(st.p1winstreak == 5);
    assert(st.p2winstreak == 0);
    assert(cleared == 2u);          /* only player 2's was cleared */

    /* `and @PSTATUS,a0`: a player who is not in the game cannot have
       won, whatever match_winner says. With only player 1 in, a
       match_winner of 2 clears BOTH. */
    memset(&st, 0, sizeof st);
    st.p1winstreak = 3;
    st.p2winstreak = 3;
    cleared = wm_match_increment_wincount(&st, 2, 1);
    assert(st.p1winstreak == 0);
    assert(st.p2winstreak == 0);
    assert(cleared == 3u);

    /* And player 1 alone, winning, keeps his. */
    memset(&st, 0, sizeof st);
    st.p1winstreak = 3;
    cleared = wm_match_increment_wincount(&st, 1, 1);
    assert(st.p1winstreak == 4);
    assert(cleared == 2u);          /* p2 was never on one */

    assert(wm_match_increment_wincount(NULL, 1, 3) == 0);
}

/* ---- the end-of-round graphic index ------------------------------ */

static void test_round_index(void) {
    wm_arcade_match_score_t sc;

    memset(&sc, 0, sizeof sc);
    /* The rounds played so far, less one: the first is 0, the second
       is 1 -- it is the round NUMBER, not the winner. */
    sc.p1rounds = 1; sc.p2rounds = 0;
    assert(wm_match_end_round_index(&sc) == 0);
    sc.p1rounds = 1; sc.p2rounds = 1;
    assert(wm_match_end_round_index(&sc) == 1);
    /* Two of anything is the match, and that is always index 2. */
    sc.p1rounds = 2; sc.p2rounds = 0;
    assert(wm_match_end_round_index(&sc) == 2);
    sc.p1rounds = 0; sc.p2rounds = 2;
    assert(wm_match_end_round_index(&sc) == 2);
    sc.p1rounds = 2; sc.p2rounds = 1;
    assert(wm_match_end_round_index(&sc) == 2);

    assert(!wm_match_end_reached(NULL));
    sc.p1rounds = 1; sc.p2rounds = 1;
    assert(!wm_match_end_reached(&sc));
    sc.p2rounds = 2;
    assert(wm_match_end_reached(&sc));
}

/* ---- the tip rule ------------------------------------------------ */

static void test_tip_rule(void) {
    /* Every 25 consecutive wins. */
    assert(wm_match_tip_due(25, 0, 1));
    assert(wm_match_tip_due(50, 0, 1));
    assert(!wm_match_tip_due(24, 0, 1));

    /*
     * The structure is not the obvious one: p2's streak is looked at
     * ONLY when p1's is zero. With both on streaks, p1's 24 is tested
     * and p2's 25 never is.
     */
    assert(wm_match_tip_due(0, 25, 1));
    assert(!wm_match_tip_due(24, 25, 1));

    /* A streak that misses falls THROUGH to the match count rather
       than giving up. */
    assert(wm_match_tip_due(24, 0, 100));
    assert(wm_match_tip_due(0, 0, 200));
    assert(!wm_match_tip_due(0, 0, 99));

    /* Match 0 is a multiple of 100 -- the source does not guard it,
       and neither does this. */
    assert(wm_match_tip_due(0, 0, 0));
}

/* ---- the music table --------------------------------------------- */

static void test_tunes(void) {
    /* `#wrestler_tunes .word 5,2,1,7,6,4,8,0,3`, by WRESTLERNUM. */
    assert(wm_match_wrestler_tune(WM_ROSTER_BRET) == 5);
    assert(wm_match_wrestler_tune(WM_ROSTER_RAZOR) == 2);
    assert(wm_match_wrestler_tune(WM_ROSTER_TAKER) == 1);
    assert(wm_match_wrestler_tune(3) == 7);
    assert(wm_match_wrestler_tune(4) == 6);
    assert(wm_match_wrestler_tune(5) == 4);
    assert(wm_match_wrestler_tune(6) == 8);
    /* Slot 7 is Adam Bomb, cut: its 0 is the table's own content. */
    assert(wm_match_wrestler_tune(7) == 0);
    assert(wm_match_wrestler_tune(8) == 3);
    /* Out of range, including the -1 a match with no winner gives. */
    assert(wm_match_wrestler_tune(-1) == 0);
    assert(wm_match_wrestler_tune(9) == 0);
}

/* ---- the sequence ------------------------------------------------ */

static int g_awards, g_winstreak, g_sounds[4], g_nsound;
static unsigned g_reset_mask;
static void cb_awards(void *u) { (void)u; ++g_awards; }
static void cb_winstreak(void *u) { (void)u; ++g_winstreak; }
static void cb_reset(void *u, unsigned m) { (void)u; g_reset_mask |= m; }
static void cb_sound(void *u, int s) {
    (void)u;
    if (g_nsound < 4) g_sounds[g_nsound++] = s;
}

static void test_sequence(void) {
    wm_match_end_t st;
    wm_match_streaks_t streaks;
    wm_arcade_match_score_t sc;
    wm_match_end_ctx_t ctx;
    int i;
    bool done = false;

    g_awards = g_winstreak = g_nsound = 0;
    g_reset_mask = 0;
    memset(&streaks, 0, sizeof streaks);
    memset(&sc, 0, sizeof sc);
    streaks.p1winstreak = 4;
    sc.p1rounds = 2;
    sc.match_winner = 1;

    memset(&ctx, 0, sizeof ctx);
    ctx.score = &sc;
    ctx.streaks = &streaks;
    ctx.pstatus = 1;
    ctx.match_cnt = 7;
    ctx.winner_wrestler_num = WM_ROSTER_TAKER;
    ctx.awards_done = true;
    ctx.run_awards = cb_awards;
    ctx.run_winstreak_award = cb_winstreak;
    ctx.reset_winstreak_rows = cb_reset;
    ctx.sound = cb_sound;

    wm_match_end_init(&st);
    /* Idle until started, however long it is ticked. */
    for (i = 0; i < 50; ++i) assert(!wm_match_end_tick(&st, &ctx));

    wm_match_end_start(&st);
    for (i = 0; i < 2000 && !done; ++i)
        done = wm_match_end_tick(&st, &ctx);
    assert(done);

    /* It really took the source's several hundred ticks rather than
       collapsing to one: SLEEPK 20, a second, 45 ticks of graphics,
       and three seconds. */
    assert(i > 20 + 53 + 45 + 53 * 3);

    assert(st.match_over == 2);
    assert(st.round_index == 2);
    assert(g_awards == 1);
    assert(g_winstreak == 1);
    /* Player 1 won and was on 4, so he goes to 5 and keeps his rows;
       player 2 was never in, so his cleared. */
    assert(streaks.p1winstreak == 5);
    assert(streaks.p1oldwinstreak == 4);
    assert(g_reset_mask == 2u);
    /* The two sounds it makes itself. */
    assert(g_nsound == 2);
    assert(g_sounds[0] == 0xc4);
    assert(g_sounds[1] == 0x4d);
    /* The winner's own theme. */
    assert(st.tune == 1);              /* the Undertaker's */
    /* A streak of 5 is not a multiple of 25, and match 7 is not a
       multiple of 100. */
    assert(!st.tip_due);

    /* Terminal: ticking on does nothing more. */
    for (i = 0; i < 50; ++i) assert(!wm_match_end_tick(&st, &ctx));
    assert(g_awards == 1);
}

/* The award bar holds the sequence until it says it is finished --
   `#wait_awards_dead`, a poll on @award_ok_to_die reaching 3. */
static void test_award_bar_holds(void) {
    wm_match_end_t st;
    wm_match_streaks_t streaks;
    wm_arcade_match_score_t sc;
    wm_match_end_ctx_t ctx;
    int i;

    memset(&streaks, 0, sizeof streaks);
    memset(&sc, 0, sizeof sc);
    sc.p1rounds = 2;
    sc.match_winner = 1;
    memset(&ctx, 0, sizeof ctx);
    ctx.score = &sc;
    ctx.streaks = &streaks;
    ctx.pstatus = 3;
    ctx.awards_done = false;           /* the bar is still running */

    wm_match_end_init(&st);
    wm_match_end_start(&st);
    for (i = 0; i < 3000; ++i) assert(!wm_match_end_tick(&st, &ctx));
    assert(st.match_over == 0);

    ctx.awards_done = true;
    for (i = 0; i < 2000 && st.match_over == 0; ++i)
        (void)wm_match_end_tick(&st, &ctx);
    assert(st.match_over == 2);
}

/* A match that is not actually over backs straight out -- DO_WAIT is
   reached only through the two-round test. */
static void test_refuses_unfinished(void) {
    wm_match_end_t st;
    wm_arcade_match_score_t sc;
    wm_match_end_ctx_t ctx;
    int i;

    memset(&sc, 0, sizeof sc);
    sc.p1rounds = 1;
    memset(&ctx, 0, sizeof ctx);
    ctx.score = &sc;

    wm_match_end_init(&st);
    wm_match_end_start(&st);
    for (i = 0; i < 200; ++i) assert(!wm_match_end_tick(&st, &ctx));
    assert(st.match_over == 0);
    assert(st.phase == (uint8_t)WM_MEND_IDLE);
}

/* ---- in a live match --------------------------------------------- */

static uint32_t hc(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t spf(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }
static wm_match_state M;

static void test_live_match_finishes(void) {
    WmRng r;
    uint32_t t = 1;
    wm_input_state in;
    int i;
    int32_t rounds = 0;

    wm_rng_init(&r, 0x12345678u, hc, spf, &t);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_match_start_selected(&M, &r, (uint8_t)WM_ROSTER_TAKER);
    memset(&in, 0, sizeof in);

    /* Hold side 1 dead and let the whole match play out. */
    for (i = 0; i < 4000 && M.match_end.match_over == 0; ++i) {
        unsigned k;
        for (k = 0; k < M.actor_count; ++k)
            if (M.actors[k].player_side == 1) {
                M.actors[k].player_mode = (uint16_t)WM_PMODE_DEAD;
                M.actors[k].life = 0;
            }
        wm_match_tick(&M, NULL, &in);
        if (M.score.p1rounds != rounds) rounds = M.score.p1rounds;
    }

    /* Two rounds, then the match. Before this the score reached 2 and
       nothing else ever happened. */
    assert(M.score.p1rounds == 2);
    assert(M.score.match_winner == 1);
    assert(M.match_end.match_over == 2);
    assert(M.match_end.round_index == 2);
    /* increment_wincount ran on the real path. */
    assert(M.streaks.p1winstreak == 1);
    assert(M.match_end.tune == wm_match_wrestler_tune(WM_ROSTER_TAKER));
}

int main(void) {
    test_increment_wincount();
    test_round_index();
    test_tip_rule();
    test_tunes();
    test_sequence();
    test_award_bar_holds();
    test_refuses_unfinished();
    test_live_match_finishes();
    return 0;
}
