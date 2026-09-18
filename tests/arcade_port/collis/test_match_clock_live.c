/*
 * The clock in a real match. WRESTLE.ASM:1631 creates it with the
 * match, WRESTLE.ASM:2102 ends the round on it, and WRESTLE.ASM:2115
 * rolls it over instead when nobody is playing.
 */
#include <assert.h>
#include <string.h>

#include "wm/match.h"
#include "wm_arcade_match_clock.h"
#include "wm_arcade_roster.h"

static uint32_t hc(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t spf(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static wm_match_state M;

static void start(bool human) {
    static WmRng r;
    static uint32_t t;
    t = 1;
    wm_rng_init(&r, 0x2468ACE0u, hc, spf, &t);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    if (human) wm_match_start_selected(&M, &r, (uint8_t)WM_ROSTER_TAKER);
    else       wm_match_start_attract(&M, &r);
}

/* A match starts on a full clock at the factory-default rate, and
   does not begin counting for two seconds. */
static void test_match_starts_the_clock(void) {
    wm_input_state in;
    int i;
    memset(&in, 0, sizeof in);
    start(true);
    assert(wm_match_clock_value(&M.clock) == 99);
    assert(M.clock.rate == wm_match_clock_rate(3, false, false, false));
    assert(M.clock.start_delay == WM_MATCH_CLOCK_START_DELAY);
    for (i = 0; i < WM_MATCH_CLOCK_START_DELAY; ++i) {
        wm_match_tick(&M, NULL, &in);
        assert(wm_match_clock_value(&M.clock) == 99);
    }
    wm_match_tick(&M, NULL, &in);
    assert(wm_match_clock_value(&M.clock) == 98);
}

/* And the clock runs while the match does. */
static void test_clock_runs(void) {
    wm_input_state in;
    int i;
    memset(&in, 0, sizeof in);
    start(true);
    for (i = 0; i < 200; ++i) wm_match_tick(&M, NULL, &in);
    assert(wm_match_clock_value(&M.clock) < 99);
    assert(wm_match_clock_value(&M.clock) > 90);
}

/*
 * Run it out. Before this the round could only end by knockout or
 * pin, so a match where neither happened simply never finished.
 */
static void test_the_round_ends_on_time(void) {
    wm_input_state in;
    int i;
    int32_t before;

    memset(&in, 0, sizeof in);
    start(true);
    before = M.score.p1rounds + M.score.p2rounds;

    /* Give side 0 the edge on life, so #tmout's average-life rule has
       an answer that is not the tie-break. */
    M.actors[0].player_side = 0;
    M.actors[1].player_side = 1;

    for (i = 0; i < 20000; ++i) {
        M.actors[0].life = 120;
        M.actors[1].life = 40;
        wm_match_tick(&M, NULL, &in);
        if (M.score.p1rounds + M.score.p2rounds != before) break;
    }
    assert(M.score.p1rounds + M.score.p2rounds == before + 1);
    /* The healthier side took it. */
    assert(M.score.p1rounds == 1);
    /* And it took roughly the 4282 ticks the clock is worth, rather
       than ending early on something else. */
    assert(i > 4000 && i < 4600);
}

/*
 * WRESTLE.ASM:2115 `#wraparound`. With nobody playing, the clock
 * rolls back to 99 and the demo keeps going -- an attract match is
 * never ended by it.
 */
static void test_attract_wraps_instead(void) {
    wm_input_state in;
    int i;
    int wraps = 0;
    int32_t last;

    memset(&in, 0, sizeof in);
    start(false);
    assert(!M.has_human);
    last = wm_match_clock_value(&M.clock);

    for (i = 0; i < 12000; ++i) {
        int32_t now;
        wm_match_tick(&M, NULL, &in);
        now = wm_match_clock_value(&M.clock);
        if (now > last) ++wraps;
        last = now;
        /* Whatever else the demo does, the clock never decides it. */
        if (M.score.p1rounds + M.score.p2rounds != 0) break;
    }
    assert(wraps >= 2);
    assert(M.score.p1rounds + M.score.p2rounds == 0);
}

/* The between-round reset puts a full clock back. */
static void test_round_two_gets_a_full_clock(void) {
    wm_input_state in;
    int i;
    int32_t rounds;

    memset(&in, 0, sizeof in);
    start(true);
    rounds = M.score.p1rounds + M.score.p2rounds;

    /*
     * Fight for a while first, with both men alive, so the clock has
     * actually moved off 99 before the knockout. It matters: the
     * clock does NOT run while one side is wiped out, so a round
     * that begins with a corpse in it never ticks at all.
     */
    for (i = 0; i < 400; ++i) wm_match_tick(&M, NULL, &in);
    assert(wm_match_clock_value(&M.clock) < 99);

    /* Knock side 1 out and wait for the reset to land. */
    for (i = 0; i < 2000; ++i) {
        unsigned k;
        for (k = 0; k < M.actor_count; ++k)
            if (M.actors[k].player_side == 1) {
                M.actors[k].player_mode = (uint16_t)WM_PMODE_DEAD;
                M.actors[k].life = 0;
            }
        wm_match_tick(&M, NULL, &in);
        if (M.score.p1rounds + M.score.p2rounds != rounds) break;
    }
    assert(M.score.p1rounds + M.score.p2rounds == rounds + 1);
    assert(wm_match_clock_value(&M.clock) < 99);

    /*
     * The reset is owed on the deciding tick and run on a later one.
     * Watch for the clock jumping UP rather than for a literal 99:
     * the reset zeroes the fraction, and a zero fraction always
     * borrows on its very next tick, so 99 lasts less than a whole
     * tick before becoming 98.
     */
    {
        int32_t low = wm_match_clock_value(&M.clock);
        for (i = 0; i < 8; ++i) {
            unsigned k;
            for (k = 0; k < M.actor_count; ++k) {
                M.actors[k].player_mode = (uint16_t)WM_PMODE_NORMAL;
                M.actors[k].life = 163;
            }
            wm_match_tick(&M, NULL, &in);
            if (wm_match_clock_value(&M.clock) > low) break;
        }
        assert(wm_match_clock_value(&M.clock) >= 98);
    }
    /* The rate survives the reset: it belongs to the match. */
    assert(M.clock.rate == wm_match_clock_rate(3, false, false, false));
    /* And so does the start delay, already spent. */
    assert(M.clock.start_delay == 0);
}

int main(void) {
    test_match_starts_the_clock();
    test_clock_runs();
    test_the_round_ends_on_time();
    test_attract_wraps_instead();
    test_round_two_gets_a_full_clock();
    return 0;
}
