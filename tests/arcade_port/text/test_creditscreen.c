/*
 * ATTRACT.ASM:793 creditscreen, through AUDIT.ASM:998 CRD_SCRN2.
 *
 * THIS SCREEN IS DIFFERENT IN KIND from every other attract screen, and the
 * tests below are shaped by that. The others display data the port has; this
 * one displays the cabinet's COIN STATE, of which the port has none -- no
 * credits, no coin inputs, no CMOS adjustments, no pricing tables, no DIP
 * bank, no tamper audit. credits_string EVALUATES its text, and every branch
 * is picked by a setting that does not exist here.
 *
 * So the screen shows exactly one state, and these tests pin down which one
 * and WHY: every factory value the five lines were derived from is asserted
 * as a literal, and so is each step of the arithmetic that places them. The
 * point is that a reader can check the derivation instead of trusting it,
 * because there is no coin subsystem to check it against.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/app.h"
#include "wm/attract.h"
#include "wm/arcade/wm_arcade_stringer.h"
#include "wm/arcade/wmania_attract_data.h"

static void test_creditscreen_is_reachable_at_all(void)
{
    assert(wm_attract_call_is_translated(WM_ATTRACT_CREDITSCREEN));
}

/*
 * The factory values the five lines were evaluated at, as literals out of
 * AUDIT.ASM:2905's FACTORY_TABLE. If any of these is wrong then the lines
 * below are the wrong lines, and nothing else in the port would notice --
 * there is no coin subsystem for them to disagree with.
 */
static void test_the_factory_values_are_the_sources(void)
{
    assert(WM_ATTRACT_CREDIT_ADJ_CSTRT == 2);    /* ADJCSTRT, index 12 */
    assert(WM_ATTRACT_CREDIT_ADJ_CCONT == 2);    /* ADJCCONT, index 13 */
    assert(WM_ATTRACT_CREDIT_ADJ_MAXC == 50);    /* ADJMAXC,  index 18 */
    assert(WM_ATTRACT_CREDIT_ADJ_FREPL == 0);    /* ADJFREPL, index 19 */
    assert(WM_ATTRACT_CREDIT_ADJ_PRICE == 1);    /* ADJPRICE, index 1  */
    assert(WM_ATTRACT_CREDIT_ADJ_1ST6 == 1);     /* ADJ1ST6,  index 22 */
    /* And the port's own state: a cabinet with nothing in it. */
    assert(WM_ATTRACT_CREDIT_CREDITS == 0);
}

/*
 * Each branch credits_string takes at those values, asserted as the condition
 * rather than as the outcome -- so a future change to a factory value makes
 * the branch assertion fail, not just the string.
 */
static void test_each_branch_follows_from_a_factory_value(void)
{
    /* ADJFREPL == 0 -> #not_freeply, so no "FREE  PLAY". */
    assert(WM_ATTRACT_CREDIT_ADJ_FREPL == 0);
    /* ADJ1ST6 non-zero -> TAMPEREDP non-zero -> the pricing line prints.
       Note the polarity: non-zero means NOT tampered, despite what
       TAMPEREDP's own header comment at ADJUST.ASM:946 says. */
    assert(WM_ATTRACT_CREDIT_ADJ_1ST6 != 0);
    /* `dec a0` on 2 is non-zero -> the PLURAL suffix, both times. */
    assert(WM_ATTRACT_CREDIT_ADJ_CSTRT - 1 != 0);
    assert(WM_ATTRACT_CREDIT_ADJ_CCONT - 1 != 0);
    /* 0 < 50 -> no " (MAXIMUM)" concatenated onto the credit count. */
    assert(WM_ATTRACT_CREDIT_CREDITS < WM_ATTRACT_CREDIT_ADJ_MAXC);
    /* 0 / 2 == 0 -> #not_ready -> "INSERT COINS", and none of the four
       "READY FOR ..." lines nor "PRESS  START". */
    assert(WM_ATTRACT_CREDIT_CREDITS / WM_ATTRACT_CREDIT_ADJ_CSTRT == 0);
}

/* The five lines themselves, as literals. */
static void test_the_five_lines_are_the_evaluated_text(void)
{
    assert(WM_ATTRACT_CREDIT_LINES == 5u);
    assert(strcmp(wm_attract_credit_lines[0].text, "CREDITS : 0") == 0);
    assert(strcmp(wm_attract_credit_lines[1].text, "1 CREDIT / 1 COIN") == 0);
    assert(strcmp(wm_attract_credit_lines[2].text, "2 CREDITS TO START") == 0);
    assert(strcmp(wm_attract_credit_lines[3].text,
                  "2 CREDITS TO CONTINUE") == 0);
    assert(strcmp(wm_attract_credit_lines[4].text, "INSERT COINS") == 0);

    /* The credit count and both adjustment numbers must agree with the
       factory values they were formatted from -- the one place the derived
       strings and the derived-from constants can drift apart. */
    assert(wm_attract_credit_lines[0].text[strlen("CREDITS : ")] ==
           (char)('0' + WM_ATTRACT_CREDIT_CREDITS));
    assert(wm_attract_credit_lines[2].text[0] ==
           (char)('0' + WM_ATTRACT_CREDIT_ADJ_CSTRT));
    assert(wm_attract_credit_lines[3].text[0] ==
           (char)('0' + WM_ATTRACT_CREDIT_ADJ_CCONT));
    /* Plural, both of them, because the count is 2 and not 1. */
    assert(strstr(wm_attract_credit_lines[2].text, "CREDITS TO START"));
    assert(strstr(wm_attract_credit_lines[3].text, "CREDITS TO CONTINUE"));
}

/*
 * The y positions, re-derived here rather than copied. LN1+4 for the credit
 * count; then `140 - YSPACE0*a10/2` with YSPACE0 17 and a10 3, each of three
 * lines adding 17 before printing; then LN5+10 plus YSPACE 25.
 */
static void test_the_y_positions_are_the_arithmetic(void)
{
    const int LN1 = 70, LN2_BASE = 140, LN5 = 190;
    const int YSPACE0 = 17, YSPACE = 25;
    /* a10: credits_string's `movk 2,a10` floor, plus one per CS_LIST string
       (Q_Q holds exactly one, C11), plus one for the ready case, which 0
       credits does not take. */
    const int a10 = 2 + 1 + 0;
    int y;

    assert(a10 == 3);
    assert(wm_attract_credit_lines[0].y == LN1 + 4);
    assert(wm_attract_credit_lines[0].y == 74);

    y = LN2_BASE - (YSPACE0 * a10) / 2;
    assert(y == 115);
    y += YSPACE0; assert(wm_attract_credit_lines[1].y == y);   /* 132 */
    y += YSPACE0; assert(wm_attract_credit_lines[2].y == y);   /* 149 */
    y += YSPACE0; assert(wm_attract_credit_lines[3].y == y);   /* 166 */

    /* LN5_setup resets to LN5+10 and nothing prints there in the not-ready
       case; the one line that does print is YSPACE lower. */
    assert(wm_attract_credit_lines[4].y == LN5 + 10 + YSPACE);
    assert(wm_attract_credit_lines[4].y == 225);

    /* Every line is centred on the same x. */
    assert(WM_ATTRACT_CREDIT_X == 200);
}

/* All five reach the screen; none refuses. */
static void test_all_five_lines_draw(void)
{
    unsigned i, drew = 0;
    for (i = 0; i < WM_ATTRACT_CREDIT_LINES; ++i) {
        wm_text_draw_item items[64];
        int32_t n = wm_text_build_draw_list(
            wm_attract_credit_lines[i].text, wm_stringer_rd7font(), 1,
            WM_STRINGER_CENTRE, WM_ATTRACT_CREDIT_X,
            wm_attract_credit_lines[i].y, items, 64);
        assert(n > 0);
        /* No brace or parenthesis anywhere, so nothing can refuse the way
           the bio quotes and the copyright's "(C)" lines do. */
        assert(!strpbrk(wm_attract_credit_lines[i].text, "(){}"));
        ++drew;
    }
    assert(drew == 5u);
}

/*
 * Timing. Text up at tick 2 (crd_updatetxt runs between the first and second
 * SLEEPK 2), display_unblank at 6, SLEEP 1*TSEC, then a 4*TSEC button loop.
 * The literals are asserted as well as the totals: the totals are derived
 * from them, so asserting only the totals would pass with each input wrong.
 */
static void test_the_screen_holds_for_its_source_time(void)
{
    static wm_app app;
    wm_input_state in;
    unsigned t, ticks = 0;
    bool seen = false;

    assert(WM_SOURCE_TICKS_PER_SEC == 53u);
    assert(WM_ATTRACT_CREDIT_PLACE_TICKS == 2);
    assert(WM_ATTRACT_CREDIT_UNBLANK_TICKS == 6);
    assert(WM_ATTRACT_CREDIT_SETTLE_TSEC == 1);
    assert(WM_ATTRACT_CREDIT_WAIT_TSEC == 4);
    assert(WM_CREDIT_SETTLE_TICKS == 59);     /* 6 + 53 */
    assert(WM_CREDIT_TOTAL_TICKS == 271);     /* 59 + 212 */

    memset(&in, 0, sizeof in);
    wm_app_init(&app);
    for (t = 1; t <= 60000u; ++t) {
        wm_app_tick(&app, &in);
        if (app.attract.call == WM_ATTRACT_CREDITSCREEN) {
            ++ticks;
            seen = true;
        } else if (seen) {
            break;
        }
    }
    assert(seen);
    assert(ticks == WM_CREDIT_TOTAL_TICKS);
    assert(ticks == 271u);
}

/*
 * A button cuts it short, but not before the unblank and the one-second
 * hold: the source does not sample buttons until the 4*TSEC loop.
 */
static void test_a_button_cuts_the_screen_short_but_not_early(void)
{
    static wm_app app;
    wm_input_state held;
    unsigned t, ticks = 0;
    bool seen = false;

    memset(&held, 0, sizeof held);
    held.run = true;
    wm_app_init(&app);
    for (t = 1; t <= 60000u; ++t) {
        wm_app_tick(&app, &held);
        if (app.attract.call == WM_ATTRACT_CREDITSCREEN) {
            ++ticks;
            seen = true;
        } else if (seen) {
            break;
        }
    }
    assert(seen);
    /* Live from the first tick of the wait, which is one past the settle. */
    assert(ticks == WM_CREDIT_SETTLE_TICKS + 1u);
    assert(ticks == 60u);
    assert(ticks < WM_CREDIT_TOTAL_TICKS);
}

int main(void)
{
    test_creditscreen_is_reachable_at_all();
    test_the_factory_values_are_the_sources();
    test_each_branch_follows_from_a_factory_value();
    test_the_five_lines_are_the_evaluated_text();
    test_the_y_positions_are_the_arithmetic();
    test_all_five_lines_draw();
    test_the_screen_holds_for_its_source_time();
    test_a_button_cuts_the_screen_short_but_not_early();
    printf("creditscreen OK\n");
    return 0;
}
