/*
 * LIFEBAR.ASM:3302 BONUS_MESS, which the seam ledger called display.
 *
 * It is not. Every path past its first test writes DAM_MULT, scores
 * HIGH_RISK_AWD and plays the guitar, so every secret move in the game
 * is meant to hit harder and to score -- and with the seam empty it did
 * neither. These tests pin the three things easiest to get backwards:
 * which path sets the multiplier, what the first-time-only flag gates,
 * and how much of the flag start_match actually clears.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_bonus_mess.h"
#include "wm/arcade/wm_arcade_react.h"

/*
 * `move a10,a10 / jrz #already / jrnn #reg`.
 *
 * NEGATIVE is ANIM.ASM:2230's taunt-style high risk. Its caller has
 * already put 4 in DAM_MULT and #tag's own write is commented out at
 * :3320, so this path must leave the caller's value alone -- which the
 * translation reports as 0.
 */
static void test_the_taunt_path_leaves_the_multiplier_alone(void) {
    wm_bonus_mess_state s;
    wm_bonus_mess_result r;

    wm_bonus_mess_init(&s);
    r = wm_bonus_mess(&s, -1, 0);
    assert(r.ran);
    assert(r.dam_mult == 0);          /* the caller's 4 stands */
    assert(r.high_risk_award);
    assert(r.guitar);
    /* #tag pushes a14 still clear and never loads #message_tbl, so its
       closing `jaz SUCIDE` dies before DO_THIS_MESS: no words. */
    assert(!r.show_text);
}

/* The wrestler files' path sets it itself, to 2 -- which LIFEBAR.ASM:1451
   makes x1.5, not the "2x damage" its own header claims. */
static void test_the_secret_move_path_sets_two(void) {
    wm_bonus_mess_state s;
    wm_bonus_mess_result r;

    wm_bonus_mess_init(&s);
    r = wm_bonus_mess(&s, 5, 0);
    assert(r.ran);
    assert(r.dam_mult == 2);
    assert(r.high_risk_award);
    assert(r.guitar);
    assert(r.show_text);              /* first time */
}

/*
 * `#reg` re-tests *a8(RISK) bit 15 and diverts to #tag, so a high-risk
 * secret move takes the taunt path whatever number it passed -- and
 * therefore does NOT set the multiplier.
 */
static void test_risk_bit_fifteen_diverts_a_positive_number(void) {
    wm_bonus_mess_state s;
    wm_bonus_mess_result r;

    wm_bonus_mess_init(&s);
    r = wm_bonus_mess(&s, 5, (uint16_t)WM_ARCADE_RISK_HIGH_BIT);
    assert(r.ran);
    assert(r.dam_mult == 0);
    assert(!r.show_text);
}

/*
 * THE FLAG GATES THE TEXT AND NOTHING ELSE. `jrnz #already` lands ABOVE
 * the DAM_MULT write and the award, so a repeat still hits harder and
 * still scores. Reading it as an early return would silently halve the
 * damage of every secret move after its first use, which is the single
 * most consequential way to get this routine wrong.
 */
static void test_a_repeat_still_hits_harder_and_still_scores(void) {
    wm_bonus_mess_state s;
    wm_bonus_mess_result a, b;

    wm_bonus_mess_init(&s);
    a = wm_bonus_mess(&s, 7, 0);
    b = wm_bonus_mess(&s, 7, 0);

    assert(a.show_text);              /* first time only */
    assert(!b.show_text);
    /* Everything else is identical. */
    assert(a.dam_mult == 2 && b.dam_mult == 2);
    assert(a.high_risk_award && b.high_risk_award);
    assert(a.guitar && b.guitar);
}

/* A zero message number takes `jrz #already`, which is the same label --
   so it too hits harder and scores, and only loses the words. */
static void test_zero_is_not_a_no_op(void) {
    wm_bonus_mess_state s;
    wm_bonus_mess_result r;

    wm_bonus_mess_init(&s);
    r = wm_bonus_mess(&s, 0, 0);
    assert(r.ran);
    assert(r.dam_mult == 2);
    assert(r.high_risk_award);
    assert(!r.show_text);
}

/*
 * LIFEBAR.ASM:108 declares `message_flag,32*2` and WRESTLE.ASM:1621
 * clears it with `move a0,@message_flag,L` -- one LONG, 32 bits. So a
 * move numbered 32 or above keeps its already-shown bit for the rest of
 * the credit. That is what the instruction does.
 */
static void test_start_match_clears_only_the_low_half(void) {
    wm_bonus_mess_state s;
    wm_bonus_mess_result r;

    assert(WM_BONUS_MESS_FLAG_BITS == 64);
    assert(WM_BONUS_MESS_FLAG_CLEARED_BITS == 32);

    wm_bonus_mess_init(&s);

    /* One move in each half, both shown once. */
    assert(wm_bonus_mess(&s, 3, 0).show_text);
    assert(!wm_bonus_mess(&s, 3, 0).show_text);
    assert(wm_bonus_mess(&s, 40, 0).show_text);
    assert(!wm_bonus_mess(&s, 40, 0).show_text);

    wm_bonus_mess_reset_match(&s);

    /* The low one comes back. */
    r = wm_bonus_mess(&s, 3, 0);
    assert(r.show_text);
    /* The high one does NOT. */
    r = wm_bonus_mess(&s, 40, 0);
    assert(!r.show_text);
}

int main(void) {
    test_the_taunt_path_leaves_the_multiplier_alone();
    test_the_secret_move_path_sets_two();
    test_risk_bit_fifteen_diverts_a_positive_number();
    test_a_repeat_still_hits_harder_and_still_scores();
    test_zero_is_not_a_no_op();
    test_start_match_clears_only_the_low_half();
    printf("bonus_mess tests passed\n");
    return 0;
}
