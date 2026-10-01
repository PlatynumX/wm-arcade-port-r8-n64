/*
 * The four table-driven attract screens: ATTRACT.ASM:1416 show_gen_tips,
 * :3390 DO_HINTS, :2067 show_bios and :2070/:2588 show_bios_tips /
 * show_wres_tips.
 *
 * These four do not number their lines. They reach them through a pointer
 * table -- #gen_tip_table, WHICH_HINT, #bio_data, wrestler_tips -- and the
 * table's order is the on-screen order. So this file guards the two things
 * that can go wrong with a table: that it is the SHIPPED table and not the
 * superseded dump's, and that the port draws what its count words say rather
 * than what is merely present.
 *
 * It also guards the join. All four were status not-started, so the app's
 * skip_untranslated_calls walked straight past them however correct their
 * data was -- the shape every commit in this port keeps finding.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/app.h"
#include "wm/attract.h"
#include "wm/arcade/wmania_attract_data.h"
#include "wm/arcade/wmania_attract_text.h"
#include "wm/arcade/wm_arcade_stringer.h"

/*
 * Three outcomes, kept apart on purpose. wm_text_build_draw_list returns -1
 * for a line it REFUSES to measure, which is not the same thing as a `blank`
 * spacer that has nothing to draw -- conflating the two would let a table
 * that had quietly emptied pass as a column of gaps.
 */
#define LINE_BLANK   (-2)
#define LINE_REFUSED (-1)

static int glyphs_for(const char *text)
{
    wm_text_draw_item items[64];
    int32_t n;
    if (!text || !*text) return LINE_BLANK;
    n = wm_text_build_draw_list(text, wm_stringer_rd7font(), 1,
                                WM_STRINGER_CENTRE, 200, 100, items, 64);
    return n > 0 ? (int)n : LINE_REFUSED;
}

static void test_all_four_screens_are_reachable_at_all(void)
{
    assert(wm_attract_call_is_translated(WM_ATTRACT_SHOW_GEN_TIPS));
    assert(wm_attract_call_is_translated(WM_ATTRACT_DO_HINTS));
    assert(wm_attract_call_is_translated(WM_ATTRACT_SHOW_BIOS));
    assert(wm_attract_call_is_translated(WM_ATTRACT_SHOW_BIOS_TIPS));
}

/*
 * NUM_HINTS is ten (ATTRACT.ASM:3386) and WHICH_HINT (:3625) holds ten
 * records in this order. The port held five -- ATTR.ASM:3482's NUM_HINTS --
 * and because the two files' orders diverge after index 2, that table also
 * named the WRONG hint at indices 3 and 4. Both the count and the order are
 * asserted against literals, not against each other: a constant-derived
 * expectation moves with the mutation it is supposed to catch.
 */
static void test_the_hint_table_is_the_shipped_files_ten(void)
{
    static const char *const order[] = {
        "IN-AIR PICK OFF", "COMBO MODE", "TURNBUCKLE LEAPS",
        "EXCESSIVE BLOCKING", "SECOND WIND", "REVERSALS",
        "HEAD HOLDS", "OUT OF RING", "HIGH RISK MANEUVERS",
        "CHOOSE WISELY"
    };
    unsigned i;

    assert(WM_ATTRACT_ACTIVE_HINTS == 10u);
    assert(sizeof order / sizeof order[0] == 10u);
    for (i = 0; i < 10u; ++i)
        assert(strcmp(wm_attract_hint_texts[i].title, order[i]) == 0);

    /* ATTR.ASM's fourth and fifth were HNT_7 and HNT_5; the shipped file's
       are HNT_9 and HNT_7. If the old table came back, these two fail. */
    assert(strcmp(wm_attract_hint_texts[3].title, "EXCESSIVE BLOCKING") == 0);
    assert(strcmp(wm_attract_hint_texts[4].title, "SECOND WIND") == 0);
}

/*
 * Two tables describe the same ten records: the hand-transcribed asset half
 * (wm_attract_hints, image symbols and the WHICH_22_NUM index) and the
 * generated text half. Nothing makes them agree except that both were read
 * off WHICH_HINT, so the agreement is asserted.
 */
static void test_the_asset_and_text_hint_tables_describe_one_table(void)
{
    unsigned i;
    for (i = 0; i < WM_ATTRACT_ACTIVE_HINTS; ++i) {
        const char *by_label = wm_attract_text(wm_attract_hints[i].title_label);
        assert(by_label);
        assert(strcmp(by_label, wm_attract_hint_texts[i].title) == 0);
        /* DO_HINTS reaches WHICH_22_NUM with the same hint number it used
           for WHICH_HINT (ATTRACT.ASM:3471), so this is the index itself. */
        assert(wm_attract_hints[i].number_image_index == (uint8_t)i);
        assert(wm_attract_hints[i].tip_image_symbol);
        assert(wm_attract_hints[i].mug_image_symbol);
    }
    /* The source's own reuse, not a transcription slip: hint 9 borrows hint
       4's tip-name object and hint 3's mugshot. */
    assert(strcmp(wm_attract_hints[9].tip_image_symbol,
                  wm_attract_hints[4].tip_image_symbol) == 0);
    assert(strcmp(wm_attract_hints[9].mug_image_symbol,
                  wm_attract_hints[3].mug_image_symbol) == 0);
}

/*
 * #HNT_7 declares 6 in its count word and lists eight pointers, and
 * NEXT_HINT (ATTRACT.ASM:3545) uses the count as its DSJS counter. So two
 * lines of "SECOND WIND" are in the ROM and have never been on a screen.
 * Exactly one hint is like this; the other nine must not be.
 */
static void test_one_hint_stores_lines_the_arcade_never_draws(void)
{
    unsigned i, mismatched = 0, which = 99u;

    for (i = 0; i < WM_ATTRACT_ACTIVE_HINTS; ++i) {
        const wm_attract_hint_text *h = &wm_attract_hint_texts[i];
        assert(h->line_count >= 1u);
        assert(h->line_count <= h->line_total);
        assert(h->line_total <= WM_ATTRACT_HINT_MAX_LINES);
        if (h->line_count != h->line_total) { ++mismatched; which = i; }
    }
    assert(mismatched == 1u);
    assert(which == 4u);
    assert(wm_attract_hint_texts[4].line_count == 6u);
    assert(wm_attract_hint_texts[4].line_total == 8u);

    /* The two dead lines, including the doubled IF that nobody saw because
       nobody could see them. */
    assert(strcmp(wm_attract_hint_texts[4].lines[6],
                  "NOTE THAT THIS DOES NOT WORK IF") == 0);
    assert(strcmp(wm_attract_hint_texts[4].lines[7],
                  "IF YOU DIE OUTSIDE THE RING.") == 0);
}

static void test_a_hint_has_text_up_to_its_total_and_null_past_it(void)
{
    unsigned i, n;
    for (i = 0; i < WM_ATTRACT_ACTIVE_HINTS; ++i) {
        const wm_attract_hint_text *h = &wm_attract_hint_texts[i];
        for (n = 0; n < h->line_total; ++n) {
            assert(h->lines[n]);
            assert(*h->lines[n]);   /* no hint body line is a spacer */
        }
        for (n = h->line_total; n < WM_ATTRACT_HINT_MAX_LINES; ++n)
            assert(h->lines[n] == NULL);
    }
}

/*
 * #gen_tip_table is four tips of two lines with a `blank` between each pair.
 * A blank is "" and not absent: #sgt_loop (ATTRACT.ASM:1344) still steps a3
 * by 15 for it, so slots 2, 5 and 8 are gaps in the column. A port that
 * skipped them would close the column up by 45 pixels.
 */
static void test_the_general_tips_are_four_pairs_with_three_gaps(void)
{
    unsigned i, slots = 0, blanks = 0;

    for (i = 0; wm_attract_general_tip_labels[i]; ++i) {
        const char *text = wm_attract_text(wm_attract_general_tip_labels[i]);
        assert(text);                      /* every label must resolve */
        ++slots;
        if (!*text) {
            ++blanks;
            assert(i == 2u || i == 5u || i == 8u);
        }
    }
    assert(slots == WM_ATTRACT_GEN_TIP_SLOTS);
    assert(slots == 11u);
    assert(blanks == 3u);

    assert(strcmp(wm_attract_text("gen_tip1"),
                  "PUT YOUR OPPONENT INTO A HEAD HOLD") == 0);
    assert(strcmp(wm_attract_text("gen_tip4a"),
                  "USE POWERPUNCH & AWAY TO FLING HIM.") == 0);
    assert(strcmp(wm_attract_text(WM_ATTRACT_GENERAL_TIPS_TITLE_LABEL),
                  "TONS O' TIPS") == 0);
}

static void test_every_wrestler_has_three_tips_and_two_gaps(void)
{
    unsigned w, n;

    assert(WM_ATTRACT_WRES_TIP_LINES == 8u);
    for (w = 0; w < WM_ATTRACT_WRESTLERS; ++w) {
        unsigned blanks = 0;
        for (n = 0; n < WM_ATTRACT_WRES_TIP_LINES; ++n) {
            const char *text = wm_attract_wres_tip_lines[w][n];
            assert(text);
            if (!*text) { ++blanks; assert(n == 2u || n == 5u); }
        }
        assert(blanks == 2u);
    }
    /* First and last wrestler, from bret_tips and lex_tips. */
    assert(strcmp(wm_attract_wres_tip_lines[0][0],
                  "CHARGE POWERPUNCH 2 SEC.") == 0);
    assert(strcmp(wm_attract_wres_tip_lines[0][1], "TO DO A DDT.") == 0);
    assert(strcmp(wm_attract_wres_tip_lines[7][7],
                  "DOES A HAMMER BLOW.") == 0);
    assert(strcmp(wm_attract_text(WM_ATTRACT_WRES_TIPS_TITLE_LABEL),
                  "SPECIAL MOVES") == 0);
}

/*
 * #bio_data's own numbers, as literals. The from-strings and quotes are
 * reached through the labels the asset table already carried and which,
 * before this, resolved to nothing at all.
 */
static void test_the_bio_records_are_the_source_records(void)
{
    static const struct { uint16_t lbs; uint8_t ft, in, half; const char *from; }
    want[] = {
        { 234u, 6u,  1u, 79u, "CALGARY" },
        { 262u, 6u,  7u, 77u, "MIAMI, FLORIDA" },
        { 322u, 6u, 11u, 78u, "DEATH VALLEY" },
        { 568u, 6u,  4u, 78u, "TOKYO, JAPAN" },
        { 235u, 6u,  1u, 78u, "SAN ANTONIO, TX" },
        { 400u, 6u,  4u, 78u, "ASBURY PARK, NJ" },
        { 243u, 6u,  0u, 78u, "THE CIRCUS" },
        { 270u, 6u,  4u, 81u, "ATLANTA, GA" }
    };
    unsigned w;

    assert(sizeof want / sizeof want[0] == WM_ATTRACT_WRESTLERS);
    for (w = 0; w < WM_ATTRACT_WRESTLERS; ++w) {
        const WmAttractBio *b = &wm_attract_bios[w];
        const char *from = wm_attract_text(b->from_label);
        assert(b->weight_lbs == want[w].lbs);
        assert(b->height_ft == want[w].ft);
        assert(b->height_in == want[w].in);
        assert(b->halfwidth == want[w].half);
        assert(from && strcmp(from, want[w].from) == 0);
        assert(wm_attract_text(b->quote_label));
    }

    /* " FT. " keeps its trailing space; " IN." and " LBS." do not have one. */
    assert(strcmp(wm_attract_text("feet"), " FT. ") == 0);
    assert(strcmp(wm_attract_text("inches"), " IN.") == 0);
    assert(strcmp(wm_attract_text("pounds"), " LBS.") == 0);
}

/*
 * halfwidth is in the record and the source applies it to nothing: :2177
 * reads it into a11, and the only code that subtracted it from #bio_center
 * (:2184, :2192) is commented out, while the live path at :2204 writes
 * #bio_center straight into mess_cursx.
 *
 * So the from-string's x must be #bio_center itself. If a future change
 * "fixed" the unused field by applying it, this x would become per-wrestler
 * and could not be a constant at all -- and Bret's line would land at 21
 * instead of 100.
 */
static void test_the_bio_halfwidth_is_carried_and_not_applied(void)
{
    assert(WM_ATTRACT_BIO_TEXT_CENTER_X == 100);
    assert(WM_ATTRACT_BIO_TEXT_Y == 106);
    assert(WM_ATTRACT_BIO_FROM_X == WM_ATTRACT_BIO_TEXT_CENTER_X);
    assert(WM_ATTRACT_BIO_FROM_X != 100 - 79);
    /* The offsets the source writes explicitly, as literals. */
    assert(WM_ATTRACT_BIO_HEIGHT_X == 108);
    assert(WM_ATTRACT_BIO_HEIGHT_Y == 131);
    assert(WM_ATTRACT_BIO_WEIGHT_X == 116);
    assert(WM_ATTRACT_BIO_WEIGHT_Y == 156);
    assert(WM_ATTRACT_BIO_QUOTE_X == 115);
    assert(WM_ATTRACT_BIO_QUOTE_Y == 206);
    assert(WM_ATTRACT_BIO_QUOTE_LINE_STEP == 14);
}

/*
 * What actually reaches a screen. Everything draws except the eight bio
 * quotes, and those refuse for the one established reason: they are wrapped
 * in the source's brace-quotes and the four FONT7par widths could not be
 * read out of the artwork, so the engine refuses the whole line rather than
 * drawing it short.
 *
 * The counts are literal. "Everything that has no brace draws" on its own
 * would still pass if a table silently emptied.
 */
static void test_only_the_brace_quotes_refuse(void)
{
    unsigned i, n, w, draws = 0, refusals = 0, blanks = 0;

    /* gen tips: title + 11 slots */
    assert(glyphs_for(wm_attract_text(WM_ATTRACT_GENERAL_TIPS_TITLE_LABEL)) > 0);
    ++draws;
    for (i = 0; wm_attract_general_tip_labels[i]; ++i) {
        int g = glyphs_for(wm_attract_text(wm_attract_general_tip_labels[i]));
        if (g == LINE_BLANK) ++blanks;
        else if (g == LINE_REFUSED) ++refusals;
        else ++draws;
    }

    /* hints: 10 titles + every line the arcade draws */
    for (i = 0; i < WM_ATTRACT_ACTIVE_HINTS; ++i) {
        const wm_attract_hint_text *h = &wm_attract_hint_texts[i];
        if (glyphs_for(h->title) > 0) ++draws; else ++refusals;
        for (n = 0; n < h->line_count; ++n) {
            if (glyphs_for(h->lines[n]) > 0) ++draws; else ++refusals;
        }
    }

    /* wres tips: title + 8 x 8 slots */
    assert(glyphs_for(wm_attract_text(WM_ATTRACT_WRES_TIPS_TITLE_LABEL)) > 0);
    ++draws;
    for (w = 0; w < WM_ATTRACT_WRESTLERS; ++w)
        for (n = 0; n < WM_ATTRACT_WRES_TIP_LINES; ++n) {
            int g = glyphs_for(wm_attract_wres_tip_lines[w][n]);
            if (g == LINE_BLANK) ++blanks;
            else if (g == LINE_REFUSED) ++refusals;
            else ++draws;
        }

    /* bios: from, the two composed runs, and the quote */
    for (w = 0; w < WM_ATTRACT_WRESTLERS; ++w) {
        const WmAttractBio *b = &wm_attract_bios[w];
        char run[32];
        const char *quote = wm_attract_text(b->quote_label);

        assert(quote);   /* the label must resolve before it can refuse */
        if (glyphs_for(wm_attract_text(b->from_label)) > 0) ++draws;
        else ++refusals;

        snprintf(run, sizeof run, "%u%s%u%s", (unsigned)b->height_ft,
                 wm_attract_text("feet"), (unsigned)b->height_in,
                 wm_attract_text("inches"));
        if (glyphs_for(run) > 0) ++draws; else ++refusals;
        snprintf(run, sizeof run, "%u%s", (unsigned)b->weight_lbs,
                 wm_attract_text("pounds"));
        if (glyphs_for(run) > 0) ++draws; else ++refusals;

        /* Every quote carries braces, and every one of them refuses. */
        assert(strpbrk(quote, "(){}"));
        assert(glyphs_for(quote) == LINE_REFUSED);
        ++refusals;
    }

    assert(blanks == 3u + 16u);          /* 3 general gaps + 2 per wrestler */
    assert(refusals == 8u);              /* the eight quotes, and nothing else */
    assert(draws == 1u + 8u             /* gen tips title + lines */
                  + 10u + 49u           /* hint titles + drawn body lines */
                  + 1u + 48u            /* wres tips title + lines */
                  + 8u + 16u);          /* bio from + height/weight runs */
    assert(draws == 141u);
}

/* ---- the live flow ------------------------------------------------- */

typedef struct {
    unsigned gen_tips_ticks, hints_ticks, bios_ticks, tips_ticks;
    int hint_seen, bio_seen, tips_bio_seen;
    bool tips_flag_during_bios, tips_flag_during_tips;
} run;

static void play_one_cycle(run *r)
{
    static wm_app app;
    wm_input_state in;
    unsigned t;
    bool after = false;

    memset(r, 0, sizeof *r);
    r->hint_seen = r->bio_seen = r->tips_bio_seen = -1;
    memset(&in, 0, sizeof in);
    wm_app_init(&app);

    for (t = 0; t < 200000u; ++t) {
        wm_app_tick(&app, &in);
        switch (app.attract.call) {
        case WM_ATTRACT_SHOW_GEN_TIPS:
            ++r->gen_tips_ticks;
            break;
        case WM_ATTRACT_DO_HINTS:
            ++r->hints_ticks;
            r->hint_seen = app.attract.hint_index;
            break;
        case WM_ATTRACT_SHOW_BIOS:
            ++r->bios_ticks;
            r->bio_seen = app.attract.bio_index;
            r->tips_flag_during_bios = app.attract.bios_tips;
            break;
        case WM_ATTRACT_SHOW_BIOS_TIPS:
            ++r->tips_ticks;
            r->tips_bio_seen = app.attract.bio_index;
            r->tips_flag_during_tips = app.attract.bios_tips;
            after = true;
            break;
        default:
            if (after) return;
            break;
        }
    }
    assert(!"the attract flow never reached all four screens");
}

/*
 * Each screen's length, end to end, against the source's own sleeps.
 *
 *   show_gen_tips: SLEEPK 1; BLOW_0_TO_1; SLEEP TSEC; wait_on_butn 10*TSEC
 *   DO_HINTS:      CLOSE_SCREEN_LINE; OPEN_SCREEN_LINE; SLEEP 80;
 *                  wait_on_butn 15*TSEC
 *   show_bios:     BLOW_0_TO_1; SLEEP 2*TSEC; wait_on_butn 6*TSEC
 *
 * The literals matter. WM_*_TOTAL_TICKS is derived from TSEC and from the
 * wipe durations, so asserting only `measured == TOTAL` would pass with
 * every one of those constants wrong -- the expectation moves with the
 * mutation. So the inputs are asserted too.
 */
static void test_each_screen_holds_for_its_source_time(void)
{
    run r;

    assert(WM_SOURCE_TICKS_PER_SEC == 53u);
    assert(WM_ATTRACT_BLOW_0_TO_1_TICKS == 89);
    assert(WM_ATTRACT_SCREEN_LINE_TICKS == 41);
    assert(WM_ATTRACT_GENERAL_TIPS_PREWAIT_TSEC == 1);
    assert(WM_ATTRACT_GENERAL_TIPS_WAIT_TSEC == 10);
    assert(WM_ATTRACT_HINT_PREWAIT_TICKS == 80);
    assert(WM_ATTRACT_HINT_WAIT_TSEC == 15);
    assert(WM_ATTRACT_BIO_NORMAL_PREWAIT_TSEC == 2);
    assert(WM_ATTRACT_BIO_NORMAL_WAIT_TSEC == 6);

    assert(WM_GEN_TIPS_TOTAL_TICKS == 673u);    /* 1 + 89 + 53 + 530 */
    assert(WM_HINTS_TOTAL_TICKS == 957u);       /* 41 + 41 + 80 + 795 */
    assert(WM_BIOS_TOTAL_TICKS == 513u);        /* 89 + 106 + 318 */

    play_one_cycle(&r);
    assert(r.gen_tips_ticks == WM_GEN_TIPS_TOTAL_TICKS);
    assert(r.hints_ticks == WM_HINTS_TOTAL_TICKS);
    assert(r.bios_ticks == WM_BIOS_TOTAL_TICKS);
    /* show_bios_tips is the same routine, so the same sleeps. */
    assert(r.tips_ticks == WM_BIOS_TOTAL_TICKS);
}

/*
 * The first bio the cabinet ever shows is wrestler 1, not 0: next_bio starts
 * at 0 and show_bios increments before using (ATTRACT.ASM:2073). And the
 * tips page shows the SAME wrestler, because attract_mode backs next_bio up
 * one between the two calls (:237-:241) so the increment lands back on it.
 */
static void test_the_bios_pair_shows_one_wrestler_starting_at_one(void)
{
    run r;
    play_one_cycle(&r);

    assert(r.bio_seen == 1);
    assert(r.tips_bio_seen == r.bio_seen);
    /* @bios_type: 0 for the bio page, 1 for SPECIAL MOVES. */
    assert(!r.tips_flag_during_bios);
    assert(r.tips_flag_during_tips);

    /* And the hint picked is a real record, not the uninitialised -1. */
    assert(r.hint_seen >= 0);
    assert(r.hint_seen < (int)WM_ATTRACT_ACTIVE_HINTS);
}

/*
 * last_hint and next_bio survive the call, so over several attract cycles
 * the hint steps and wraps at ten and the bio steps and wraps at eight. A
 * port that reset either per call would show the same hint and the same
 * wrestler for ever.
 *
 * BOTH SEQUENCES START AT 1, NOT 0, and that is the source's. Each routine
 * increments before using (ATTRACT.ASM:3446, :2075) and neither counter is
 * written anywhere else in the file -- `.bss last_hint,16` (:3388) and
 * `.bss next_bio,16` (:2065) are all there is. DO_HINTS' `jrn #reset_chint`
 * exists precisely because .bss is not guaranteed zero on a cold TMS34010;
 * from the port's zeroed state the first hint shown is index 1 and the first
 * wrestler is 1.
 */
static void test_the_hint_and_bio_choices_step_across_cycles(void)
{
    static wm_app app;
    wm_input_state in;
    unsigned t;
    int hints[12], bios[12];
    unsigned nh = 0, nb = 0;
    wm_attract_call last = WM_ATTRACT_CALL_COUNT;
    unsigned i;

    memset(&in, 0, sizeof in);
    wm_app_init(&app);

    for (t = 0; t < 400000u && (nh < 12u || nb < 12u); ++t) {
        wm_app_tick(&app, &in);
        if (app.attract.call != last) {
            last = app.attract.call;
            if (last == WM_ATTRACT_DO_HINTS && nh < 12u)
                hints[nh++] = app.attract.hint_index;
            else if (last == WM_ATTRACT_SHOW_BIOS && nb < 12u)
                bios[nb++] = app.attract.bio_index;
        }
    }
    assert(nh == 12u);
    assert(nb == 12u);

    for (i = 0; i < 12u; ++i) {
        /* 1,2,...,9,0,1,2 */
        assert(hints[i] == (int)((i + 1u) % WM_ATTRACT_ACTIVE_HINTS));
        /* 1,2,3,4,5,6,7,0,1,2,3,4 -- `(next_bio & 15) + 1`, 0 at 8. */
        {
            int want = (int)(i % 8u) + 1;
            if (want == 8) want = 0;
            assert(bios[i] == want);
        }
    }
    /* Not a constant sequence: the whole point of the persistence. */
    assert(hints[0] == 1);
    assert(hints[9] == 0);   /* the wrap */
    assert(hints[0] != hints[1]);
    assert(bios[0] == 1);
    assert(bios[7] == 0);   /* the wrap */
    assert(bios[0] != bios[1]);
}

int main(void)
{
    test_all_four_screens_are_reachable_at_all();
    test_the_hint_table_is_the_shipped_files_ten();
    test_the_asset_and_text_hint_tables_describe_one_table();
    test_one_hint_stores_lines_the_arcade_never_draws();
    test_a_hint_has_text_up_to_its_total_and_null_past_it();
    test_the_general_tips_are_four_pairs_with_three_gaps();
    test_every_wrestler_has_three_tips_and_two_gaps();
    test_the_bio_records_are_the_source_records();
    test_the_bio_halfwidth_is_carried_and_not_applied();
    test_only_the_brace_quotes_refuse();
    test_each_screen_holds_for_its_source_time();
    test_the_bios_pair_shows_one_wrestler_starting_at_one();
    test_the_hint_and_bio_choices_step_across_cycles();
    printf("attract table screens OK\n");
    return 0;
}
