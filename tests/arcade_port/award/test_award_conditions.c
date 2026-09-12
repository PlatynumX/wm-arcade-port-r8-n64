/*
 * AWARD.ASM's award conditions -- the routines that decide whether an
 * award is earned. The accounting they feed is tested elsewhere; these
 * are the four checks and the row count that were not ported.
 *
 * Three of these pin behaviour that looks like a bug and is: the
 * unreachable quick-win branches, the round reset that stops one byte
 * short, and the winstreak arm that a streak of zero cannot clear.
 */
#include "wm/award.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_quick_win_and_its_dead_branches(void)
{
    wm_award_state st;

    /* score = (t & 0xF) * 10 + (t >> 16). */
    wm_award_init(&st);
    assert(wm_award_quick_win(&st, 0, 0, 0x00080000u) == -1);   /* 8 */
    assert(st.round[0][WM_AWARD_QUICK] == 0);

    wm_award_init(&st);
    assert(wm_award_quick_win(&st, 0, 0, 0x00000007u) == (int)WM_AWARD_QUICK);
    assert(st.round[0][WM_AWARD_QUICK] == wm_award_icon_value(WM_AWARD_QUICK));

    /* `jrgt` -- exactly 69 does not qualify. 0x0009_0006 is 6*10 + 9. */
    wm_award_init(&st);
    assert(wm_award_quick_win(&st, 0, 0, 0x00090006u) == -1);
    wm_award_init(&st);
    assert(wm_award_quick_win(&st, 0, 0, 0x000A0006u) == (int)WM_AWARD_QUICK);

    /* A score far past the super-quick threshold still only yields
     * QUICK: the `jruc` after the first comparison makes the finer
     * branches unreachable in the shipped code. */
    wm_award_init(&st);
    assert(wm_award_quick_win(&st, 0, 0, 0x00000009u) == (int)WM_AWARD_QUICK);
    assert(st.round[0][WM_AWARD_SUPER_QUICK] == 0);
    assert(st.round[0][WM_AWARD_VERY_QUICK] == 0);

    /* A drone scores nothing. */
    wm_award_init(&st);
    assert(wm_award_quick_win(&st, 2, 0, 0x00000009u) == (int)WM_AWARD_QUICK);
    assert(st.round[0][WM_AWARD_QUICK] == 0);
}

static void test_defeat_human(void)
{
    wm_award_state st;
    const uint8_t icons = wm_award_icon_value(WM_AWARD_DEFEAT_HUMAN);

    /* Each side asks whether the OTHER player is a started human. */
    wm_award_init(&st);
    wm_award_defeat_human(&st, 0, 0, 0x3u);
    assert(st.round[0][WM_AWARD_DEFEAT_HUMAN] == icons);

    wm_award_init(&st);
    wm_award_defeat_human(&st, 0, 0, 0x1u);     /* only player 1 in */
    assert(st.round[0][WM_AWARD_DEFEAT_HUMAN] == 0);

    wm_award_init(&st);
    wm_award_defeat_human(&st, 1, 1, 0x1u);     /* side 1 sees player 1 */
    assert(st.round[1][WM_AWARD_DEFEAT_HUMAN] == icons);

    wm_award_init(&st);
    wm_award_defeat_human(&st, 1, 1, 0x2u);
    assert(st.round[1][WM_AWARD_DEFEAT_HUMAN] == 0);
}

static void test_comeback(void)
{
    wm_award_state st;
    bool live[WM_AWARD_NUM_WRES];
    unsigned side_of[WM_AWARD_NUM_WRES];
    int health[WM_AWARD_NUM_WRES];
    unsigned i;

    /* (163*80)/100 truncates to 130. */
    assert(WM_AWARD_COMEBACK_HEALTH == 130);

    for (i = 0; i < WM_AWARD_NUM_WRES; ++i) {
        live[i] = false;
        side_of[i] = 1u;
        health[i] = WM_AWARD_LIFE_MAX;
    }

    wm_award_init(&st);
    live[0] = true;
    wm_award_arm_comeback(&st, 0, live, side_of, health);
    assert(st.comeback_armed[0]);

    /* One enemy under 80% and the arm is refused. */
    wm_award_init(&st);
    health[0] = 129;
    wm_award_arm_comeback(&st, 0, live, side_of, health);
    assert(!st.comeback_armed[0]);
    health[0] = 130;
    wm_award_arm_comeback(&st, 0, live, side_of, health);
    assert(st.comeback_armed[0]);

    /* A weak TEAMMATE is skipped, not counted -- the source compares
     * PLYR_SIDE against the arming player's index. */
    wm_award_init(&st);
    live[1] = true;
    side_of[1] = 0u;
    health[1] = 1;
    wm_award_arm_comeback(&st, 0, live, side_of, health);
    assert(st.comeback_armed[0]);
    /* ...but the same wrestler IS an enemy from the other side. */
    wm_award_init(&st);
    wm_award_arm_comeback(&st, 1, live, side_of, health);
    assert(!st.comeback_armed[1]);

    /* An inactive slot is ignored however weak it is. */
    wm_award_init(&st);
    live[1] = false;
    wm_award_arm_comeback(&st, 1, live, side_of, health);
    assert(st.comeback_armed[1]);

    /* Drones never arm. */
    wm_award_init(&st);
    wm_award_arm_comeback(&st, 2, live, side_of, health);
    assert(!st.comeback_armed[0] && !st.comeback_armed[1]);

    /* The check turns an armed flag into a round award. */
    wm_award_init(&st);
    st.comeback_armed[1] = true;
    wm_award_check_comeback(&st, 1, 1);
    assert(st.round[1][WM_AWARD_COMEBACK] ==
           wm_award_icon_value(WM_AWARD_COMEBACK));
    wm_award_check_comeback(&st, 0, 0);
    assert(st.round[0][WM_AWARD_COMEBACK] == 0);
}

static void test_the_round_reset_stops_one_byte_short(void)
{
    /* `movi (NUM_AWARDS*2)-1` clears sixty-three bytes across the two
     * adjacent arrays, so player 2's last round slot survives. Award
     * 31 is unused, which is why nobody noticed. */
    wm_award_state st;

    wm_award_init(&st);
    st.round[0][WM_AWARD_COUNT - 1u] = 9u;
    st.round[1][WM_AWARD_COUNT - 1u] = 9u;
    st.round[1][WM_AWARD_COUNT - 2u] = 9u;
    st.comeback_armed[0] = true;
    st.comeback_armed[1] = true;

    wm_award_reset_round(&st);
    assert(st.round[0][WM_AWARD_COUNT - 1u] == 0u);
    assert(st.round[1][WM_AWARD_COUNT - 2u] == 0u);
    assert(st.round[1][WM_AWARD_COUNT - 1u] == 9u);
    assert(wm_award_icon_value(WM_AWARD_COUNT - 1u) == 0u);
    /* rst_awards also zeroes both pcomeback longs. */
    assert(!st.comeback_armed[0] && !st.comeback_armed[1]);
}

static void test_num_rows(void)
{
    wm_award_state st;
    unsigned i;

    wm_award_init(&st);
    assert(wm_award_num_rows(&st, 0) == 0u);

    st.match[0][WM_AWARD_PERFECT] = 2u;
    st.match[0][WM_AWARD_QUICK] = 1u;
    st.match[0][WM_AWARD_FIVE_WINS] = 3u;
    assert(wm_award_num_rows(&st, 0) == 3u);
    assert(wm_award_num_rows(&st, 1) == 0u);

    /* Capped at six however many are set. */
    for (i = 0; i < WM_AWARD_COUNT; ++i) {
        st.match[0][i] = 1u;
    }
    assert(wm_award_num_rows(&st, 0) == WM_AWARD_MAX_ROWS);

    /* A row draws the big icon once its award is worth five or more. */
    assert(!wm_award_row_uses_big_icon(4u));
    assert(wm_award_row_uses_big_icon(5u));
    assert(wm_award_row_uses_big_icon(200u));
}

static void test_winstreak_arming_is_not_symmetric(void)
{
    /* arm_winstreak_award returns before touching anything when the
     * streak is zero, so a reset to zero leaves the flag armed. */
    wm_award_state st;

    wm_award_init(&st);
    wm_award_arm_winstreak(&st, 0, 5u);
    assert(st.winstreak_armed[0]);
    wm_award_arm_winstreak(&st, 0, 0u);
    assert(st.winstreak_armed[0]);
    /* Any other non-multiple explicitly disarms. */
    wm_award_arm_winstreak(&st, 0, 6u);
    assert(!st.winstreak_armed[0]);
}

int main(void)
{
    test_quick_win_and_its_dead_branches();
    test_defeat_human();
    test_comeback();
    test_the_round_reset_stops_one_byte_short();
    test_num_rows();
    test_winstreak_arming_is_not_symmetric();
    printf("AWARD.ASM award conditions: all checks passed\n");
    return 0;
}
