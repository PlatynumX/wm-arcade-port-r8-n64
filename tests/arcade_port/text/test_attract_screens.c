/*
 * The two attract screens that put TEXT on the display:
 * ATTRACT.ASM:1095 show_copyright and :274 aama_message.
 *
 * Every layer under these existed before and none was joined to the
 * next. The .string literals were extracted, RD7FONT's widths and ink
 * masks were recovered from the artwork, UTIL.ASM's centring was
 * translated and the DMACNZ stencil blit was written -- but
 * wm_rd7text_draw had no caller, wm_text_build_draw_list was called only
 * by tests, and both routines' port status was not-started, so the app's
 * own skip_untranslated_calls walked straight past them.
 *
 * This file guards the join, not the pieces: that the live attract flow
 * REACHES each screen, that it holds for exactly as long as the source
 * says, and that the lines which cannot be measured are absent rather
 * than drawn wrong.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/app.h"
#include "wm/attract.h"
#include "wm/arcade/wmania_attract_data.h"
#include "wm/arcade/wmania_attract_text.h"
#include "wm/arcade/wm_arcade_stringer.h"

/* Both must be translated, or the app skips them and nothing below can
   possibly happen. This is the assertion the five previous commits'
   worth of work was missing. */
static void test_both_screens_are_reachable_at_all(void)
{
    assert(wm_attract_call_is_translated(WM_ATTRACT_SHOW_COPYRIGHT));
    assert(wm_attract_call_is_translated(WM_ATTRACT_AAMA_MESSAGE));
}

/* How long each call lasts, and the page state through it. */
typedef struct {
    unsigned copyright_ticks, page0_ticks, page1_ticks;
    unsigned aama_ticks, aama_placed_ticks;
} screen_run;

static void play_until_both_seen(screen_run *r)
{
    static wm_app app;
    wm_input_state in;
    unsigned t;

    memset(r, 0, sizeof *r);
    memset(&in, 0, sizeof in);
    wm_app_init(&app);

    /* The copyright/AAMA pair runs once every eight attract loops
       (advance_call's `amode_loops & 7`), which is ~23500 source ticks
       in. The guard is generous rather than tuned. */
    for (t = 0; t < 200000u; ++t) {
        wm_app_tick(&app, &in);
        if (app.attract.call == WM_ATTRACT_SHOW_COPYRIGHT) {
            ++r->copyright_ticks;
            if (app.attract.copyright_page == 0) ++r->page0_ticks;
            else if (app.attract.copyright_page == 1) ++r->page1_ticks;
        } else if (app.attract.call == WM_ATTRACT_AAMA_MESSAGE) {
            ++r->aama_ticks;
            if (app.attract.aama_placed) ++r->aama_placed_ticks;
        } else if (r->copyright_ticks && r->aama_ticks) {
            return;     /* both seen and both finished */
        }
    }
    assert(!"the attract flow never reached the copyright/AAMA pair");
}

/*
 * The source's own sleep boundaries, end to end.
 *
 *   show_copyright: SLEEPK 2, place; SLEEPK 2 + unblank; SLEEPK 20;
 *                   wait_on_butn 3*TSEC; obj_del1c; page two;
 *                   SLEEPK 20; wait_on_butn 3*TSEC
 *   aama_message:   SLEEPK 2, place; SLEEPK 2 + unblank; SLEEPK 20;
 *                   wait_on_butn 4*TSEC
 *
 * With no button pressed both run to their full length, and the two
 * pages split at the obj_del1c in the middle.
 */
static void test_each_screen_holds_for_its_source_time(void)
{
    screen_run r;
    play_until_both_seen(&r);

    assert(r.copyright_ticks == WM_COPYRIGHT_TOTAL_TICKS);
    assert(r.aama_ticks == WM_AAMA_TOTAL_TICKS);

    /*
     * AND THE LITERAL SOURCE NUMBERS, which is the only form of this
     * check that can fail. WM_*_TOTAL_TICKS are DERIVED from the wait
     * constants, so comparing a run against them only proves the port
     * agrees with itself: mutation testing showed that changing
     * aama_message's `wait_on_butn 4*TSEC` to 3*TSEC moved the
     * measurement and the expectation together and was not caught.
     *
     * DISPLAY.EQU:46 is `TSEC equ 53`, so:
     *   show_copyright  2 + 2 + 20 + 3*53 = 183 to the end of page one,
     *                   + 20 + 3*53 = 362 in total
     *   aama_message    2 + 2 + 20 + 4*53 = 236
     */
    assert(WM_SOURCE_TICKS_PER_SEC == 53u);
    assert(WM_ATTRACT_COPYRIGHT_PAGE_WAIT_TSEC == 3);
    assert(WM_ATTRACT_AAMA_WAIT_TSEC == 4);
    assert(WM_COPYRIGHT_PAGE1_TOTAL_TICKS == 183u);
    assert(WM_COPYRIGHT_TOTAL_TICKS == 362u);
    assert(WM_AAMA_TOTAL_TICKS == 236u);
    assert(r.copyright_ticks == 362u);
    assert(r.aama_ticks == 236u);

    /* Both pages are shown, and the split is where page one's wait ends. */
    assert(r.page0_ticks > 0 && r.page1_ticks > 0);
    /* Page one is on screen from the tick after the SLEEPK 2 places it
       until the tick its wait expires, which is PAGE1_TOTAL minus those
       two placing ticks. */
    assert(r.page0_ticks == WM_COPYRIGHT_PAGE1_TOTAL_TICKS -
                            WM_COPYRIGHT_PLACE_TICKS);
    /* The two ticks before the text is placed belong to neither page. */
    assert(r.page0_ticks + r.page1_ticks ==
           WM_COPYRIGHT_TOTAL_TICKS - WM_COPYRIGHT_PLACE_TICKS);

    /* AAMA has one page, placed after its own SLEEPK 2. */
    assert(r.aama_placed_ticks ==
           WM_AAMA_TOTAL_TICKS - WM_AAMA_PLACE_TICKS);
}

static int32_t line_items(const char *label, int32_t x, int32_t y)
{
    wm_text_draw_item items[64];
    const char *text = wm_attract_text(label);
    assert(text != NULL);
    return wm_text_build_draw_list(text, wm_stringer_rd7font(), 1,
                                   WM_STRINGER_CENTRE, x, y, items, 64);
}

/*
 * EXACTLY three of the nineteen copyright lines refuse to draw, and
 * they are the three containing parentheses.
 *
 * wm_text_build_draw_list returns -1 for a line it cannot measure
 * rather than drawing it short, because the four FONT7par widths could
 * not be established from the artwork. A notice with characters
 * silently missing is worse than one not drawn.
 *
 * Asserting the COUNT and WHICH matters both ways: if the widths are
 * ever recovered these numbers must change deliberately, and if a
 * measurable line starts refusing, that is a regression in the font
 * table rather than a known gap.
 */
static void test_only_the_parenthesis_lines_refuse(void)
{
    unsigned i, refused = 0, drawn = 0;

    for (i = 0; i < WM_ATTRACT_COPYRIGHT_PAGE1_LINES; ++i) {
        const char *label = wm_attract_copyright_page1_labels[i];
        int32_t n = line_items(label, WM_ATTRACT_COPYRIGHT_X,
                               WM_ATTRACT_COPYRIGHT_FIRST_Y +
                                   (int32_t)i * WM_ATTRACT_COPYRIGHT_LINE_STEP);
        const char *text = wm_attract_text(label);
        if (n < 0) {
            ++refused;
            /* A refusal must be explained by a parenthesis or a brace. */
            assert(strpbrk(text, "(){}") != NULL);
        } else {
            ++drawn;
            assert(n > 0);
            assert(strpbrk(text, "(){}") == NULL);
        }
    }
    assert(drawn == 7u && refused == 2u);

    refused = drawn = 0;
    for (i = 0; i < WM_ATTRACT_COPYRIGHT_PAGE2_LINES; ++i) {
        const char *label = wm_attract_copyright_page2_labels[i];
        int32_t n = line_items(label, WM_ATTRACT_COPYRIGHT_X,
                               WM_ATTRACT_COPYRIGHT_FIRST_Y +
                                   (int32_t)i * WM_ATTRACT_COPYRIGHT_LINE_STEP);
        if (n < 0) ++refused; else ++drawn;
    }
    assert(drawn == 9u && refused == 1u);

    /* AAMA is all plain capitals, so every one of its lines draws. */
    for (i = 0; i < WM_ATTRACT_AAMA_LINES; ++i) {
        const WmAttractAamaLine *ln = &wm_attract_aama_lines[i];
        assert(line_items(ln->label, ln->x, ln->y) > 0);
    }
}

/*
 * aama_message's layout is not a column, which is the whole reason it
 * needed a table of its own. ATTRACT.ASM:308-343.
 */
static void test_the_aama_layout_is_not_a_column(void)
{
    const WmAttractAamaLine *l = wm_attract_aama_lines;

    assert(l[0].y == 94);                    /* [54+60-20,200] */
    assert(l[1].y == 114 && l[2].y == 114);  /* ln2 and ln2b share a y */
    assert(l[3].y == 125 && l[4].y == 136 && l[5].y == 147);

    /* ...and ln2/ln2b sit side by side on it, neither at the x the
       other four share. */
    assert(l[1].x == 177 && l[2].x == 260);
    assert(l[0].x == 200 && l[3].x == 200 && l[4].x == 200 && l[5].x == 200);
    assert(l[1].x != l[0].x && l[2].x != l[0].x);

    /* ln2b is the one line on either screen with a different source
       colour. The port draws both the same -- see the note in
       wmania_attract_data.c -- but the data must keep the distinction,
       or recovering the palette later would have nothing to act on. */
    assert(l[2].colour != l[1].colour);
    assert(l[2].colour == 0x0606u);
    assert(l[0].colour == 0x1111u && l[1].colour == 0x1111u);
    assert(l[3].colour == 0x1111u && l[4].colour == 0x1111u &&
           l[5].colour == 0x1111u);
}

int main(void)
{
    test_both_screens_are_reachable_at_all();
    test_each_screen_holds_for_its_source_time();
    test_only_the_parenthesis_lines_refuse();
    test_the_aama_layout_is_not_a_column();
    printf("attract screens: copyright and AAMA reach the display\n");
    return 0;
}
