/*
 * The cabinet's switch word.
 *
 * The bit offsets are the point: `but_offs2 .word 20h-3` means a
 * player's high two buttons are a sixteen-bit field starting at bit
 * 0x1D and masked to bits 3 and 4 -- latch bits 0x20 and 0x21, on the
 * far side of the halfway line. Getting that wrong gives a port where
 * block and super punch silently never register.
 */
#include "wm/arcade/wm_arcade_switches.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define BIT(n) (((uint64_t)1) << (n))

static void test_offsets(void)
{
    assert(wm_sw_joy_offs[0] == 0x00 && wm_sw_joy_offs[1] == 0x08);
    assert(wm_sw_joy_offs[2] == 0x20 && wm_sw_joy_offs[3] == 0x28);
    assert(wm_sw_but_offs[0] == 0x04 && wm_sw_but_offs[1] == 0x0C);
    assert(wm_sw_but_offs2[0] == 0x1D && wm_sw_but_offs2[1] == 0x21);
    assert(wm_sw_start_offs[0] == 0x12 && wm_sw_start_offs[1] == 0x15);
}

static void test_latch_transitions(void)
{
    wm_switch_latch sw;

    wm_switch_latch_init(&sw);
    assert(sw.cur == 0 && sw.down == 0 && sw.up == 0);

    /* down = (old ^ new) & new; up = (new ^ old) & old. */
    wm_switch_latch_update(&sw, BIT(4) | BIT(5));
    assert(sw.cur == (BIT(4) | BIT(5)));
    assert(sw.down == (BIT(4) | BIT(5)));
    assert(sw.up == 0);

    /* Held: no new transition either way. */
    wm_switch_latch_update(&sw, BIT(4) | BIT(5));
    assert(sw.down == 0 && sw.up == 0);
    assert(sw.old == (BIT(4) | BIT(5)));

    /* One released, one newly pressed, in the same frame. */
    wm_switch_latch_update(&sw, BIT(5) | BIT(6));
    assert(sw.down == BIT(6));
    assert(sw.up == BIT(4));

    /* The top half works the same -- it is one 64-bit word. */
    wm_switch_latch_update(&sw, BIT(0x21));
    assert(sw.down == BIT(0x21));
    assert(sw.up == (BIT(5) | BIT(6)));
}

static void test_stick_slicing(void)
{
    wm_switch_latch sw;
    wm_switch_latch_init(&sw);

    /* Player 0's stick is latch bits 0..3, player 1's is 8..11. */
    wm_switch_latch_update(&sw, 0x5ull | (0xAull << 8));
    assert(wm_get_stick_val_cur(&sw, 0) == 0x5);
    assert(wm_get_stick_val_cur(&sw, 1) == 0xA);

    /* Only four bits: bit 4 is player 0's first button, not his stick. */
    wm_switch_latch_update(&sw, 0x1Full);
    assert(wm_get_stick_val_cur(&sw, 0) == 0xF);

    /* Players 2 and 3 exist in the table and share bits with the
     * humans' high buttons -- that is the four-player wiring. */
    wm_switch_latch_update(&sw, 0xFull << 0x20);
    assert(wm_get_stick_val_cur(&sw, 2) == 0xF);
    assert(wm_get_stick_val_cur(&sw, 3) == 0x0);

    /* down and up read the same fields off the transition words. */
    wm_switch_latch_init(&sw);
    wm_switch_latch_update(&sw, 0x3ull);
    assert(wm_get_stick_val_down(&sw, 0) == 0x3);
    assert(wm_get_stick_val_up(&sw, 0) == 0x0);
    wm_switch_latch_update(&sw, 0x1ull);
    assert(wm_get_stick_val_down(&sw, 0) == 0x0);
    assert(wm_get_stick_val_up(&sw, 0) == 0x2);
}

static void test_buttons_span_the_two_halves(void)
{
    wm_switch_latch sw;
    wm_switch_latch_init(&sw);

    /* Player 0: low three buttons at bits 4..6. */
    wm_switch_latch_update(&sw, BIT(4) | BIT(6));
    assert(wm_get_but_val_cur(&sw, 0) == 0x5);

    /* Player 0: high two at bits 0x20 and 0x21, arriving as bits 3
     * and 4 of the result. This is the read that crosses the halfway
     * line, and the one a 32-bit model would lose. */
    wm_switch_latch_update(&sw, BIT(0x20));
    assert(wm_get_but_val_cur(&sw, 0) == 0x08);
    wm_switch_latch_update(&sw, BIT(0x21));
    assert(wm_get_but_val_cur(&sw, 0) == 0x10);
    wm_switch_latch_update(&sw, BIT(4) | BIT(0x20) | BIT(0x21));
    assert(wm_get_but_val_cur(&sw, 0) == 0x19);

    /* Player 1: low at 0x0C..0x0E, high at 0x24 and 0x25. */
    wm_switch_latch_update(&sw, BIT(0x0C) | BIT(0x25));
    assert(wm_get_but_val_cur(&sw, 1) == 0x11);
    assert(wm_get_but_val_cur(&sw, 0) == 0x00);

    /* Nothing outside the two masks leaks in. */
    wm_switch_latch_update(&sw, ~(uint64_t)0);
    assert(wm_get_but_val_cur(&sw, 0) == 0x1F);
    assert(wm_get_stick_val_cur(&sw, 0) == 0x0F);
}

static void test_start_buttons(void)
{
    wm_switch_latch sw;
    wm_switch_latch_init(&sw);

    wm_switch_latch_update(&sw, BIT(0x12));
    assert(wm_get_start_cur(&sw, 0) == 1);
    assert(wm_get_start_cur(&sw, 1) == 0);
    assert(wm_get_start_down(&sw, 0) == 1);

    wm_switch_latch_update(&sw, BIT(0x12) | BIT(0x15));
    assert(wm_get_start_cur(&sw, 1) == 1);
    assert(wm_get_start_down(&sw, 0) == 0);     /* held, not new */
    assert(wm_get_start_down(&sw, 1) == 1);

    /* One bit only -- 0x13 is not part of player 1's start. */
    wm_switch_latch_update(&sw, BIT(0x13));
    assert(wm_get_start_cur(&sw, 0) == 0);
}

static void test_aggregators_respect_pstatus(void)
{
    wm_switch_latch sw;
    wm_switch_latch_init(&sw);
    /* Player 0 holds stick 0x1, player 1 holds 0x2. */
    wm_switch_latch_update(&sw, 0x1ull | (0x2ull << 8));

    assert(wm_get_all_sticks_cur(&sw, 0x0u) == 0x0);
    assert(wm_get_all_sticks_cur(&sw, 0x1u) == 0x1);
    assert(wm_get_all_sticks_cur(&sw, 0x2u) == 0x2);
    assert(wm_get_all_sticks_cur(&sw, 0x3u) == 0x3);

    /* The `2` forms ignore PSTATUS entirely -- that is how the credit
     * screen reads a pad before anyone has paid. */
    assert(wm_get_all_sticks_cur2(&sw) == 0x3);

    wm_switch_latch_init(&sw);
    wm_switch_latch_update(&sw, BIT(4) | BIT(0x0C));
    assert(wm_get_all_buttons_cur(&sw, 0x1u) == 0x1);
    assert(wm_get_all_buttons_cur(&sw, 0x2u) == 0x1);
    assert(wm_get_all_buttons_cur2(&sw) == 0x1);
    assert(wm_get_all_buttons_down2(&sw) == 0x1);

    wm_switch_latch_init(&sw);
    wm_switch_latch_update(&sw, BIT(0x12) | BIT(0x15));
    assert(wm_get_all_starts_cur(&sw, 0x3u) == 0x1);
    assert(wm_get_all_starts_down(&sw, 0x0u) == 0x0);
    assert(wm_get_all_starts_down(&sw, 0x2u) == 0x1);
}

static void test_stick_flip(void)
{
    int i;

    /* The table swaps left and right and leaves up and down alone. */
    assert(wm_stick_flip(0x0) == 0x0);
    assert(wm_stick_flip(0x1) == 0x1);          /* up */
    assert(wm_stick_flip(0x2) == 0x2);          /* down */
    assert(wm_stick_flip(0x4) == 0x8);          /* left -> right */
    assert(wm_stick_flip(0x8) == 0x4);          /* right -> left */
    assert(wm_stick_flip(0x9) == 0x5);          /* up-right -> up-left */
    assert(wm_stick_flip(0x6) == 0xA);          /* down-left -> down-right */
    assert(wm_stick_flip(0xF) == 0xF);

    /* It is its own inverse, which is what makes it safe to apply
     * whenever the wrestler turns round. */
    for (i = 0; i < 16; ++i) {
        assert(wm_stick_flip(wm_stick_flip((uint16_t)i)) == (uint16_t)i);
    }
}

static void test_read_switches_human(void)
{
    wm_switch_latch sw;
    wm_switch_readings r;

    wm_switch_latch_init(&sw);
    wm_switch_latch_update(&sw, 0x8ull | BIT(4) | BIT(0x21));

    wm_read_switches_one(&r, &sw, 0, false, NULL, 0, true);
    assert(r.stick_cur == 0x8);
    assert(r.but_cur == 0x11);
    assert(r.stick_down == 0x8 && r.but_down == 0x11);
    assert(r.stick_up == 0);
    /* Facing right, so the relative value is the absolute one. */
    assert(r.stick_rel_cur == 0x8);
    /* Something changed this frame, so rel_new carries it. */
    assert(r.stick_rel_new == 0x8);

    /* Facing left mirrors it. */
    wm_read_switches_one(&r, &sw, 0, false, NULL, 0, false);
    assert(r.stick_rel_cur == 0x4);
    assert(r.stick_rel_new == 0x4);

    /* Held with no transition: rel_new goes to zero even though the
     * stick is still pushed. `or a1,a0 / jrz #no_stick`. */
    wm_switch_latch_update(&sw, 0x8ull | BIT(4) | BIT(0x21));
    wm_read_switches_one(&r, &sw, 0, false, NULL, 0, true);
    assert(r.stick_cur == 0x8);
    assert(r.stick_down == 0 && r.stick_up == 0);
    assert(r.stick_rel_cur == 0x8);
    assert(r.stick_rel_new == 0x0);
}

static void test_read_switches_drone_and_immobilized(void)
{
    wm_switch_latch sw;
    wm_switch_readings r;
    wm_drone_switches drone;

    wm_switch_latch_init(&sw);
    wm_switch_latch_update(&sw, 0xFFFFull);     /* the pad is mashed */

    memset(&drone, 0, sizeof(drone));
    drone.joy = 0x4;
    drone.joy_down = 0x4;
    drone.but = 0x2;

    /* A drone ignores the latch completely. */
    wm_read_switches_one(&r, &sw, 0, true, &drone, 0, true);
    assert(r.stick_cur == 0x4 && r.but_cur == 0x2);
    assert(r.stick_rel_cur == 0x4);
    assert(r.stick_rel_new == 0x4);
    /* ...and it still gets the relative derivation: `#cont` is below
     * the drone branch, not inside the human one. */
    wm_read_switches_one(&r, &sw, 0, true, &drone, 0, false);
    assert(r.stick_rel_cur == 0x8);

    /* Immobilized zeroes everything, both relative values included,
     * for a drone as much as a human. */
    wm_read_switches_one(&r, &sw, 0, false, NULL, 1, true);
    assert(r.but_cur == 0 && r.stick_cur == 0);
    assert(r.stick_rel_cur == 0 && r.stick_rel_new == 0);
    wm_read_switches_one(&r, &sw, 0, true, &drone, 5, false);
    assert(r.stick_cur == 0 && r.stick_rel_cur == 0);

    /* `jrp` is strictly positive: a zero timer does not immobilize,
     * and neither does a negative one. */
    wm_read_switches_one(&r, &sw, 0, false, NULL, 0, true);
    assert(r.stick_cur == 0xF);
    wm_read_switches_one(&r, &sw, 0, false, NULL, -1, true);
    assert(r.stick_cur == 0xF);
}

int main(void)
{
    test_offsets();
    test_latch_transitions();
    test_stick_slicing();
    test_buttons_span_the_two_halves();
    test_start_buttons();
    test_aggregators_respect_pstatus();
    test_stick_flip();
    test_read_switches_human();
    test_read_switches_drone_and_immobilized();
    printf("cabinet switch word: all checks passed\n");
    return 0;
}
