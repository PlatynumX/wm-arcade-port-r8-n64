/*
 * LIFEBAR.ASM's life_data table.
 *
 * The interesting part is that the three resets are not the same
 * routine with different scopes -- they disagree about the displayed
 * value and about whether the combo bar is cleared, and each
 * disagreement is visible on screen.
 */
#include "wm/arcade/wm_arcade_life_data.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_constants(void)
{
    assert(WM_LIFE_MAX == 163);
    assert(WM_TURBO_MAX == 0x5400);
    assert(WM_LIFEBAR_DELTAVEE == 2);
}

static void test_match_start_fills_the_bar_from_empty(void)
{
    wm_life_data ld;
    int i;
    int ticks = 0;

    memset(&ld, 0xAA, sizeof(ld));
    wm_life_init_match(&ld);

    for (i = 0; i < WM_LIFE_NUM_WRES; ++i) {
        assert(ld.row[i].life == WM_LIFE_MAX);
        assert(ld.row[i].turbo == WM_TURBO_MAX);
        /* `clr a4 / move a4,*a1(PLT_CLIFE)` -- the bar starts EMPTY. */
        assert(ld.row[i].clife == 0);
        assert(ld.row[i].combo_size == 0);
    }

    /* So the first thing inc_life does at a match start is run it up,
     * two pixels a tick. 163 is odd, so the last step is the clamp. */
    while (wm_life_inc_life(&ld, 0)) {
        ++ticks;
        assert(ticks < 200);
    }
    assert(ld.row[0].clife == WM_LIFE_MAX);
    assert(ticks == 81);        /* 81 full steps of 2, then 162 -> 163 */
}

static void test_a_wrestler_swap_snaps_the_bar(void)
{
    /* init_wres_life_data is for change_wrestler, replacing someone who
     * just died -- there is nothing to animate, so CLIFE goes straight
     * to full. That is the one difference from init_life_data. */
    wm_life_data ld;

    wm_life_init_match(&ld);
    ld.row[2].life = 10;
    ld.row[2].clife = 10;
    ld.row[2].combo_size = 7;

    wm_life_init_wrestler(&ld, 2);
    assert(ld.row[2].life == WM_LIFE_MAX);
    assert(ld.row[2].clife == WM_LIFE_MAX);
    assert(ld.row[2].combo_size == 0);
    assert(ld.row[2].turbo == WM_TURBO_MAX);
    assert(!wm_life_inc_life(&ld, 2));      /* nothing to animate */

    /* One wrestler only -- nobody else is touched. */
    assert(ld.row[1].clife == 0);

    wm_life_init_wrestler(&ld, -1);
    wm_life_init_wrestler(&ld, WM_LIFE_NUM_WRES);
}

static void test_a_new_round_leaves_the_combo_bar_alone(void)
{
    /* init_rnd_life_data writes LIFE and CLIFE and stops. The other two
     * resets clear combo_size; this one does not. */
    wm_life_data ld;
    int i;

    wm_life_init_match(&ld);
    for (i = 0; i < WM_LIFE_NUM_WRES; ++i) {
        ld.row[i].life = 40;
        ld.row[i].clife = 40;
        ld.row[i].combo_size = 9;
        ld.row[i].turbo = 1;
    }

    wm_life_init_round(&ld);
    for (i = 0; i < WM_LIFE_NUM_WRES; ++i) {
        assert(ld.row[i].life == WM_LIFE_MAX);
        assert(ld.row[i].clife == WM_LIFE_MAX);
        assert(ld.row[i].combo_size == 9);  /* untouched */
        assert(ld.row[i].turbo == 1);       /* untouched */
    }
}

static void test_the_displayed_value_chases_without_overshooting(void)
{
    wm_life_data ld;

    wm_life_init_match(&ld);

    /* Draining: CLIFE walks down to meet a reduced LIFE. */
    ld.row[0].clife = 100;
    ld.row[0].life = 95;
    assert(wm_life_inc_life(&ld, 0) && ld.row[0].clife == 98);
    assert(wm_life_inc_life(&ld, 0) && ld.row[0].clife == 96);
    /* One pixel left and a two-pixel step: it clamps rather than
     * stepping past and oscillating. */
    assert(!wm_life_inc_life(&ld, 0));
    assert(ld.row[0].clife == 95);
    assert(!wm_life_inc_life(&ld, 0));

    /* Filling clamps the same way. */
    ld.row[0].clife = 0;
    ld.row[0].life = 3;
    assert(wm_life_inc_life(&ld, 0) && ld.row[0].clife == 2);
    assert(!wm_life_inc_life(&ld, 0));
    assert(ld.row[0].clife == 3);

    /* Already level: nothing happens and nothing is reported. */
    assert(!wm_life_inc_life(&ld, 0));
    assert(!wm_life_inc_life(&ld, 99));
}

static void test_clear_lifebar_and_get_health(void)
{
    wm_life_data ld;
    int ticks = 0;

    wm_life_init_round(&ld);
    assert(wm_life_get_health(&ld, 1) == WM_LIFE_MAX);

    /* clear_lifebar zeroes the real value only -- the bar then drains
     * to meet it rather than vanishing. */
    wm_life_clear(&ld, 1);
    assert(wm_life_get_health(&ld, 1) == 0);
    assert(ld.row[1].clife == WM_LIFE_MAX);

    while (wm_life_inc_life(&ld, 1)) {
        ++ticks;
        assert(ticks < 200);
    }
    assert(ld.row[1].clife == 0);
    assert(ticks == 81);

    assert(wm_life_get_health(NULL, 0) == 0);
    assert(wm_life_get_health(&ld, WM_LIFE_NUM_WRES) == 0);
}

static void test_turbo_is_maintained_and_never_read(void)
{
    /* Two of the three resets write PLT_TURBO. Nothing in the whole
     * source reads it, and there is no turbo bar on screen. */
    assert(!wm_life_turbo_is_ever_read());
}

int main(void)
{
    test_constants();
    test_match_start_fills_the_bar_from_empty();
    test_a_wrestler_swap_snaps_the_bar();
    test_a_new_round_leaves_the_combo_bar_alone();
    test_the_displayed_value_chases_without_overshooting();
    test_clear_lifebar_and_get_health();
    test_turbo_is_maintained_and_never_read();
    printf("LIFEBAR.ASM life_data: all checks passed\n");
    return 0;
}
