/*
 * PROGRESS.ASM:1501 is_final_match, :1516 is_8_on_1 and :692
 * NUM_OF_OPPS -- three routines the rest of the port has been
 * reasoning about without having.
 */
#include "wm/pregame.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_is_final_match(void)
{
    wm_pregame_state s;
    int i;

    memset(&s, 0, sizeof(s));

    /*
     * `cmpi LADDER+((FINAL_BATTLE-1)*20h),a14` with FINAL_BATTLE 7 and
     * a 20h-bit (one long) entry, so the last battle is index 6 -- not
     * the 13 the equ's own out-of-date comment ("14th battle is last
     * one") would suggest.
     */
    assert(WM_PREGAME_FINAL_LADDER_INDEX == 6);

    for (i = 0; i < 8; ++i) {
        s.current_ladder_index = i;
        assert(wm_pregame_is_final_match(&s) == (i == 6));
    }
    assert(!wm_pregame_is_final_match(NULL));
}

static void test_is_8_on_1(void)
{
    wm_pregame_state s;
    int i;

    memset(&s, 0, sizeof(s));

    /* "no 8-on-1 in intercontinental belt table" -- the first line of
       the routine, and it beats the ladder position outright. */
    s.belt_type = WM_PREGAME_BELT_INTERCONTINENTAL;
    for (i = 0; i < 8; ++i) {
        s.current_ladder_index = i;
        assert(!wm_pregame_is_8_on_1(&s));
    }

    /* On the championship ladder it is the final battle and only that. */
    s.belt_type = WM_PREGAME_BELT_WWF;
    for (i = 0; i < 8; ++i) {
        s.current_ladder_index = i;
        assert(wm_pregame_is_8_on_1(&s) == (i == 6));
    }

    /*
     * So is_8_on_1 and is_final_match agree only on the WWF ladder.
     * Several headers in this port say is_8_on_1 always reports "no" --
     * that is true of the match code, which never sets up a final
     * battle, and is not a property of the routine.
     */
    s.belt_type = WM_PREGAME_BELT_INTERCONTINENTAL;
    s.current_ladder_index = WM_PREGAME_FINAL_LADDER_INDEX;
    assert(wm_pregame_is_final_match(&s));
    assert(!wm_pregame_is_8_on_1(&s));

    assert(!wm_pregame_is_8_on_1(NULL));
}

static void test_num_of_opps(void)
{
    /* `SRL 24,A3` -- the count byte at the top of a packed entry,
       whose other three bytes are the opponents themselves. */
    assert(wm_pregame_num_of_opps(0x03000000u) == 3);
    assert(wm_pregame_num_of_opps(0x01080604u) == 1);
    assert(wm_pregame_num_of_opps(0x00ffffffu) == 0);
    assert(wm_pregame_num_of_opps(0xff000000u) == 255);
}

int main(void)
{
    test_is_final_match();
    test_is_8_on_1();
    test_num_of_opps();
    printf("is_final_match, is_8_on_1, NUM_OF_OPPS: all checks passed\n");
    return 0;
}
