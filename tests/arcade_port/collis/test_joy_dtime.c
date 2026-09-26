/*
 * WRESTLE.ASM's two input-timing routines.
 *
 * update_joy_dtime (:4023) keeps nine hold counters per wrestler, and
 * only three of the nine were translated. calc_closest2 (:4108) is the
 * gate in front of calc_closest, and had been dismissed as a CPU
 * saving -- it is not, because it skips the distance update the AI
 * reads.
 */
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_closest.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_every_counter_counts(void)
{
    wm_arcade_actor_t a;
    int i;

    memset(&a, 0, sizeof(a));

    /* Hold everything for three ticks. */
    a.stick_val_cur = 0x0f;
    a.but_val_cur = 0x1f;
    for (i = 0; i < 3; ++i) wm_arcade_update_joy_dtime(&a);
    for (i = WM_DTIME_UP; i <= WM_DTIME_POWERK; ++i)
        assert(wm_arcade_get_dtime(&a, (wm_arcade_dtime_t)i) == 3);

    /* Release everything: back to zero the same tick, not a decay. */
    a.stick_val_cur = 0;
    a.but_val_cur = 0;
    wm_arcade_update_joy_dtime(&a);
    for (i = WM_DTIME_UP; i <= WM_DTIME_POWERK; ++i)
        assert(wm_arcade_get_dtime(&a, (wm_arcade_dtime_t)i) == 0);
}

static void test_the_bit_order_is_the_arrays_order(void)
{
    /*
     * #update_but walks BUT_VAL_CUR from bit 0 upward across the five
     * arrays in their BSSX order, which the source flags with its own
     * "!KEEP THIS ORDER!". So bit 1 is BLOCK and bit 2 is the power
     * punch -- not the order the buttons are usually named in, and
     * getting it wrong would credit a block to the power punch.
     */
    wm_arcade_actor_t a;

    memset(&a, 0, sizeof(a));
    a.but_val_cur = WM_BTN_BLOCK;
    wm_arcade_update_joy_dtime(&a);
    assert(wm_arcade_get_dtime(&a, WM_DTIME_BLOCK) == 1);
    assert(wm_arcade_get_dtime(&a, WM_DTIME_PUNCH) == 0);
    assert(wm_arcade_get_dtime(&a, WM_DTIME_POWERP) == 0);

    memset(&a, 0, sizeof(a));
    a.but_val_cur = WM_BTN_SPUNCH;
    wm_arcade_update_joy_dtime(&a);
    assert(wm_arcade_get_dtime(&a, WM_DTIME_POWERP) == 1);
    assert(wm_arcade_get_dtime(&a, WM_DTIME_BLOCK) == 0);

    memset(&a, 0, sizeof(a));
    a.but_val_cur = WM_BTN_SKICK;
    a.stick_val_cur = WM_MOVE_LEFT;
    wm_arcade_update_joy_dtime(&a);
    assert(wm_arcade_get_dtime(&a, WM_DTIME_POWERK) == 1);
    assert(wm_arcade_get_dtime(&a, WM_DTIME_KICK) == 0);
    assert(wm_arcade_get_dtime(&a, WM_DTIME_LEFT) == 1);
    assert(wm_arcade_get_dtime(&a, WM_DTIME_RIGHT) == 0);

    /* One held input does not disturb another's count. */
    a.but_val_cur = 0;
    wm_arcade_update_joy_dtime(&a);
    assert(wm_arcade_get_dtime(&a, WM_DTIME_POWERK) == 0);
    assert(wm_arcade_get_dtime(&a, WM_DTIME_LEFT) == 2);

    wm_arcade_init_joy_dtime(&a);
    assert(wm_arcade_get_dtime(&a, WM_DTIME_LEFT) == 0);
    wm_arcade_update_joy_dtime(NULL);
    wm_arcade_init_joy_dtime(NULL);
    assert(wm_arcade_get_dtime(NULL, WM_DTIME_LEFT) == 0);
}

static void test_calc_closest2_throttle(void)
{
    wm_arcade_actor_t me, opp;
    uint32_t t;
    int recomputed = 0;

    memset(&me, 0, sizeof(me));
    memset(&opp, 0, sizeof(opp));
    me.player_num = 0;
    opp.player_num = 1;
    opp.player_mode = (uint16_t)WM_PMODE_NORMAL;
    me.x_int = 0; opp.x_int = 100;

    /* Exactly one tick in four, and it is the one where the low two
       bits of PCNT match the low two bits of PLYRNUM. */
    for (t = 0; t < 8; ++t) {
        bool did = wm_arcade_calc_closest2(&me, &opp, t);
        assert(did == ((t & 3u) == 0u));
        recomputed += did ? 1 : 0;
    }
    assert(recomputed == 2);

    /* Staggered: player 1 refreshes on a different tick from player 0. */
    me.player_num = 1;
    for (t = 0; t < 8; ++t)
        assert(wm_arcade_calc_closest2(&me, &opp, t) == ((t & 3u) == 1u));

    /* "Always recalculate if our current closest is dead" beats it. */
    opp.player_mode = (uint16_t)WM_PMODE_DEAD;
    for (t = 0; t < 8; ++t)
        assert(wm_arcade_calc_closest2(&me, &opp, t));

    assert(!wm_arcade_calc_closest2(NULL, &opp, 0));
    assert(!wm_arcade_calc_closest2(&me, NULL, 0));
}

static void test_the_throttle_really_holds_stale_distances(void)
{
    /*
     * The point of translating this at all: the skip does not just
     * avoid picking a new target, it avoids recomputing CLOSEST_DIST,
     * and wm_arcade_drone.c and wm_arcade_doink.c both gate on that
     * number. A drone is acting on data up to three ticks old.
     */
    wm_arcade_actor_t me, opp;
    int32_t at_refresh;

    memset(&me, 0, sizeof(me));
    memset(&opp, 0, sizeof(opp));
    me.player_num = 0;
    opp.player_mode = (uint16_t)WM_PMODE_NORMAL;

    opp.x_int = 100;
    assert(wm_arcade_calc_closest2(&me, &opp, 0));
    at_refresh = me.closest_xdist;
    assert(at_refresh == 100);

    /* He runs away over the next three ticks and the number does not
       move. */
    opp.x_int = 220;
    assert(!wm_arcade_calc_closest2(&me, &opp, 1));
    assert(me.closest_xdist == at_refresh);
    assert(!wm_arcade_calc_closest2(&me, &opp, 2));
    assert(!wm_arcade_calc_closest2(&me, &opp, 3));
    assert(me.closest_xdist == at_refresh);

    /* And then catches up all at once. */
    assert(wm_arcade_calc_closest2(&me, &opp, 4));
    assert(me.closest_xdist == 220);
}

int main(void)
{
    test_every_counter_counts();
    test_the_bit_order_is_the_arrays_order();
    test_calc_closest2_throttle();
    test_the_throttle_really_holds_stale_distances();
    printf("update_joy_dtime and calc_closest2: all checks passed\n");
    return 0;
}
