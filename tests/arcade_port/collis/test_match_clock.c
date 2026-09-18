/*
 * The round clock: WRESTLE2.ASM:4098 match_timer and LIFEBAR.ASM:5149
 * set_winner's #tmout branch.
 */
#include <assert.h>
#include <string.h>

#include "wm_arcade_match_clock.h"
#include "wm_arcade_round.h"

/* WRESTLE2.ASM:4339 #timer_table, `.asg 1500,BASETM`. */
static void test_timer_table(void) {
    assert(wm_match_clock_timer_table[0] == 1050);
    assert(wm_match_clock_timer_table[1] == 1275);
    assert(wm_match_clock_timer_table[2] == 1500);
    assert(wm_match_clock_timer_table[3] == 1725);
    assert(wm_match_clock_timer_table[4] == 1950);
    /* A bigger number is a FASTER clock -- it is the amount taken off
       the fraction each tick -- so row 5 is "fastest" as the source
       labels it, not slowest. */
    assert(wm_match_clock_timer_table[4] > wm_match_clock_timer_table[0]);
}

static void test_rate(void) {
    int32_t base = wm_match_clock_rate(3, false, false, false);
    assert(base == 1500);

    /* `BADCHK a0,1,5,3` replaces an out-of-range setting with the
       DEFAULT, it does not clamp to the nearest end. */
    assert(wm_match_clock_rate(0, false, false, false) == base);
    assert(wm_match_clock_rate(6, false, false, false) == base);
    assert(wm_match_clock_rate(99, false, false, false) == base);
    assert(wm_match_clock_rate(1, false, false, false) == 1050);
    assert(wm_match_clock_rate(5, false, false, false) == 1950);

    /* `movi >AAAA,a14 / mpyu a14,a1 / srl 16,a1` -- two thirds, with
       the source's own truncation. */
    assert(wm_match_clock_rate(3, false, true, false)
           == (int32_t)(((uint32_t)1500 * 0xAAAAu) >> 16));
    assert(wm_match_clock_rate(3, false, true, false) == 999);

    /* `sra 1,a1` on a final match. */
    assert(wm_match_clock_rate(3, false, false, true) == 750);

    /*
     * The royal rumble takes BOTH -- the two-thirds and then the
     * halving -- which is how the source's own comment, "slow the
     * clock to 1/3 speed if this is the royal rumble", comes out
     * right. Reading either branch alone gives 2/3 or 1/2.
     */
    assert(wm_match_clock_rate(3, true, false, false) == 999 / 2);
}

/* #dec_timer: the fraction is a 16-bit borrow chain, not a counter of
   seconds. */
static void test_dec_borrows(void) {
    wm_match_clock_t c;
    bool warn;
    int ticks;

    wm_match_clock_start(&c, 1500);
    assert(c.tens == 9 && c.ones == 9 && c.frac == 0);
    assert(wm_match_clock_value(&c) == 99);
    assert(!wm_match_clock_expired(&c));

    /*
     * The fraction starts at ZERO, so the very first tick borrows
     * immediately and the clock drops to 98 at once. That is the
     * source's behaviour, not an off-by-one: `sub a10,a0` on a zero
     * fraction always borrows.
     */
    assert(wm_match_clock_dec(&c, &warn));
    assert(wm_match_clock_value(&c) == 98);
    assert(c.frac == (uint16_t)(0u - 1500u));

    /* And then it takes a run of ticks before the next one: the
       fraction sits at 64036 and 1500 comes off it each tick, so 42
       ticks change nothing and the 43rd borrows. */
    for (ticks = 0; ticks < 42; ++ticks)
        assert(!wm_match_clock_dec(&c, &warn));
    assert(wm_match_clock_dec(&c, &warn));
    assert(wm_match_clock_value(&c) == 97);
}

/*
 * The whole clock, start to finish, which is the number worth having
 * written down: 4282 ticks at the factory-default rate. DISPLAY.EQU:
 * 46 makes TSEC 53, so a round is about 81 seconds.
 *
 * The source's own comment on that table row says 53.6 seconds, and
 * this does not match it. The `.asg 1500,BASETM ;2100 ;16` line shows
 * BASETM was lowered from 2100 and the comments beside it were not
 * updated; 2100 would give 3060 ticks, 58 seconds -- still not 53.6.
 * The arithmetic is transcribed as the shipped code performs it.
 */
static void test_full_round_length(void) {
    wm_match_clock_t c;
    bool warn;
    int ticks = 0;

    wm_match_clock_start(&c, wm_match_clock_rate(3, false, false, false));
    while (!wm_match_clock_expired(&c)) {
        (void)wm_match_clock_dec(&c, &warn);
        ++ticks;
        assert(ticks < 100000);
    }
    assert(ticks == 4282);

    /* A final match halves the rate, so it takes about twice as long
       -- not exactly, because the fraction's truncation does not
       scale cleanly. */
    wm_match_clock_start(&c, wm_match_clock_rate(3, false, false, true));
    ticks = 0;
    while (!wm_match_clock_expired(&c)) {
        (void)wm_match_clock_dec(&c, &warn);
        ++ticks;
        assert(ticks < 100000);
    }
    assert(ticks > 4282 * 19 / 10 && ticks < 4282 * 21 / 10);
}

/* The digit borrow: 90 -> 89, not 90 -> 8something. */
static void test_digit_borrow(void) {
    wm_match_clock_t c;
    bool warn;

    wm_match_clock_start(&c, 1500);
    c.tens = 9; c.ones = 0; c.frac = 0;
    assert(wm_match_clock_dec(&c, &warn));
    assert(c.tens == 8 && c.ones == 9);

    /* And the floor: the source never decrements an expired clock
       because the loop is gated on it, so this only has to not go
       negative. */
    c.tens = 0; c.ones = 0; c.frac = 0;
    (void)wm_match_clock_dec(&c, &warn);
    assert(c.tens == 0 && c.ones == 0);
    assert(wm_match_clock_expired(&c));
}

/*
 * The warning sound. The digits are packed as BCD and compared with
 * 0x10, so it starts at TEN seconds -- the comment above it in the
 * source says fifteen and the code says ten.
 */
static void test_warning_starts_at_ten(void) {
    wm_match_clock_t c;
    bool warn;

    wm_match_clock_start(&c, 1500);
    c.tens = 1; c.ones = 1; c.frac = 0;
    assert(wm_match_clock_dec(&c, &warn));
    assert(wm_match_clock_value(&c) == 10);
    assert(warn);

    /* Eleven does not. */
    c.tens = 1; c.ones = 2; c.frac = 0;
    assert(wm_match_clock_dec(&c, &warn));
    assert(wm_match_clock_value(&c) == 11);
    assert(!warn);

    /* Nor does a tick where no digit moved, however low the clock. */
    c.tens = 0; c.ones = 5; c.frac = 0xffff;
    c.rate = 1;
    assert(!wm_match_clock_dec(&c, &warn));
    assert(!warn);
}

/* AWARD.ASM:996's own reading, which looks like it swaps the halves
   and does not. */
static void test_award_score(void) {
    wm_match_clock_t c;
    wm_match_clock_start(&c, 1500);
    assert(wm_match_clock_award_score(&c) == 99);
    c.tens = 7; c.ones = 0;
    assert(wm_match_clock_award_score(&c) == 70);
    /* `cmpi 69,a1 / jrgt #quick` -- 70 is quick, 69 is not. */
    c.tens = 6; c.ones = 9;
    assert(wm_match_clock_award_score(&c) == 69);
}

/* The gate around #dec_timer, and the one piece of it that is easy to
   miss: the clock stops while either side is wiped out. */
static void test_tick_gate(void) {
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *ptrs[2];
    wm_match_clock_t c;
    bool warn;
    int i;

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    a.active = 1; a.player_side = 0;
    b.active = 1; b.player_side = 1;
    ptrs[0] = &a; ptrs[1] = &b;

    wm_match_clock_start(&c, 1500);
    /*
     * `callr #create_timer / SLEEP TSEC*2`: two seconds of nothing
     * before the loop is entered at all, so a match does not start
     * losing time on the bell.
     */
    for (i = 0; i < WM_MATCH_CLOCK_START_DELAY; ++i) {
        assert(!wm_match_clock_tick(&c, false, ptrs, 2, &warn));
        assert(wm_match_clock_value(&c) == 99);
    }
    assert(wm_match_clock_tick(&c, false, ptrs, 2, &warn));
    assert(wm_match_clock_value(&c) == 98);

    /* @HALT. */
    c.frac = 0;
    assert(!wm_match_clock_tick(&c, true, ptrs, 2, &warn));
    assert(wm_match_clock_value(&c) == 98);

    /*
     * `get_live_bits / cmpi 3,a3 / jrne #1tmded` -- one side dead and
     * the clock stops dead with it, so the five-second pin window
     * after a knockout does not eat into the round.
     */
    b.player_mode = WM_PMODE_DEAD;
    assert(!wm_match_clock_tick(&c, false, ptrs, 2, &warn));
    assert(wm_match_clock_value(&c) == 98);
    b.player_mode = WM_PMODE_NORMAL;
    assert(wm_match_clock_tick(&c, false, ptrs, 2, &warn));
    assert(wm_match_clock_value(&c) == 97);

    /* An expired clock does not keep ticking. */
    c.tens = 0; c.ones = 0; c.frac = 0;
    assert(!wm_match_clock_tick(&c, false, ptrs, 2, &warn));
}

/*
 * WRESTLE.ASM:2657's between-round reset is the three stores and
 * nothing else: the rate and the two-second delay belong to the one
 * timer process the MATCH created, which by round two is long past
 * its own sleep.
 */
static void test_between_round_reset(void) {
    wm_match_clock_t c;
    wm_match_clock_start(&c, 1275);
    c.start_delay = 0;
    c.tens = 1; c.ones = 2; c.frac = 0x8000;
    wm_match_clock_reset(&c);
    assert(wm_match_clock_value(&c) == 99);
    assert(c.frac == 0);
    assert(c.rate == 1275);
    assert(c.start_delay == 0);
}

/* WRESTLE.ASM:2115 #wraparound: the attract demo is never ended by
   the clock. */
static void test_wrap(void) {
    wm_match_clock_t c;
    wm_match_clock_start(&c, 1500);
    c.tens = 0; c.ones = 0; c.frac = 0x1234;
    wm_match_clock_wrap(&c);
    assert(c.tens == 9 && c.ones == 9);
    /* `move a14,@match_time,L` is the two DIGITS; the fraction is
       left exactly where it was. */
    assert(c.frac == 0x1234);
}

/* LIFEBAR.ASM:5149 #tmout. */
static void test_timeout_winner(void) {
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *ptrs[2];

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    a.active = 1; a.player_side = 0;
    b.active = 1; b.player_side = 1;
    ptrs[0] = &a; ptrs[1] = &b;

    /* "Award victory to the team with the highest average life points
       remaining." */
    a.life = 100; b.life = 40;
    assert(wm_match_timeout_winner(ptrs, 2) == 0);
    a.life = 40; b.life = 100;
    assert(wm_match_timeout_winner(ptrs, 2) == 1);

    /* "In case of a tie, winner is the last team to land a hit." And
       LAST_HIT_TIME is stamped on the ATTACKER, so this is the side
       that hit last, not the side that was hit last. */
    a.life = 80; b.life = 80;
    a.last_hit_time = 500;
    b.last_hit_time = 900;
    assert(wm_match_timeout_winner(ptrs, 2) == 1);
    b.last_hit_time = 100;
    assert(wm_match_timeout_winner(ptrs, 2) == 0);

    /* "If there have been no hits, we'll wanna drop out and go
       straight to game over." */
    a.last_hit_time = 0;
    b.last_hit_time = 0;
    assert(wm_match_timeout_winner(ptrs, 2) == WM_MATCH_TIMEOUT_NO_WINNER);

    /*
     * A dead wrestler is still counted, with the zero life he has --
     * #loop2 skips inactive slots and nothing else -- so his side's
     * average goes down rather than his being left out of it.
     */
    a.life = 0; a.player_mode = WM_PMODE_DEAD;
    b.life = 1; b.player_mode = WM_PMODE_NORMAL;
    a.last_hit_time = b.last_hit_time = 0;
    assert(wm_match_timeout_winner(ptrs, 2) == 1);
}

/* Two wrestlers a side, so the AVERAGE really is an average. */
static void test_timeout_averages(void) {
    wm_arcade_actor_t w[4];
    wm_arcade_actor_t *ptrs[4];
    size_t i;

    memset(w, 0, sizeof w);
    for (i = 0; i < 4; ++i) {
        w[i].active = 1;
        w[i].player_side = (int32_t)(i & 1);
        ptrs[i] = &w[i];
    }
    /*
     * Side 1 has the single healthiest wrestler in the ring and
     * still loses, because the rule is the AVERAGE: side 0 is 70 and
     * 80 (75), side 1 is 120 and 0 (60).
     */
    w[0].life = 70; w[2].life = 80;
    w[1].life = 120; w[3].life = 0;
    assert(w[1].life > w[0].life && w[1].life > w[2].life);
    assert(wm_match_timeout_winner(ptrs, 4) == 0);

    /* Dropping side 1's dead man out of the count would flip it, so
       this is the assertion that the source's "count everyone" loop
       is really what is translated. */
    w[3].active = 0;
    assert(wm_match_timeout_winner(ptrs, 4) == 1);
}

int main(void) {
    test_timer_table();
    test_rate();
    test_dec_borrows();
    test_full_round_length();
    test_digit_borrow();
    test_warning_starts_at_ten();
    test_award_score();
    test_tick_gate();
    test_between_round_reset();
    test_wrap();
    test_timeout_winner();
    test_timeout_averages();
    return 0;
}
