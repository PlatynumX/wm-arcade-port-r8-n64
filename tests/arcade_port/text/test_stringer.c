/*
 * UTIL.ASM's string engine, checked against the source's own arithmetic.
 *
 * stringr1_1 (:1372) and STRNGLEN (:1495) are the printer the attract screens
 * go through -- show_copyright's nine `JSRP STRCNRMO_2` calls and
 * aama_message's. STRING.ASM's engine, which this port had already
 * translated, is a different one and could not draw them.
 */
#include "wm/arcade/wm_arcade_stringer.h"
#include "wm/arcade/wm_arcade_rd7font.h"
#include "wm/arcade/wmania_attract_text.h"
#include "wm/arcade/wmania_attract_data.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* A synthetic font so the arithmetic is checkable by hand: every glyph is
   10 wide, so a length is (glyphs*10 + spaces*5 + chars*spacing). */
static int16_t s_ten[WM_RD7FONT_SLOTS];
static wm_stringer_font s_fixed;

static const wm_stringer_font *fixed(void) {
    size_t i;
    for (i = 0; i < WM_RD7FONT_SLOTS; ++i) s_ten[i] = 10;
    s_fixed.width = s_ten;
    s_fixed.slots = WM_RD7FONT_SLOTS;
    s_fixed.first_char = WM_RD7FONT_FIRST_CHAR;
    return &s_fixed;
}

static void test_strnglen_arithmetic(void) {
    const wm_stringer_font *f = fixed();

    /* "AB": two glyphs at 10, spacing 1 added after EACH -- including the
       last, which is where the source leaves it (`addxy a10,a7` runs before
       the loop test, not after a lookahead). */
    assert(wm_stringer_length("AB", f, 1) == 10 + 1 + 10 + 1);
    assert(wm_stringer_length("A", f, 0) == 10);
    assert(wm_stringer_length("", f, 3) == 0);

    /* A space is five, hard-coded, and still takes the spacing after it.
       `subk 32,a14 / jrgt stl20` fails for 32, so `addk 5,a7` then stl40. */
    assert(wm_stringer_length(" ", f, 0) == WM_STRINGER_SPACE_WIDTH);
    assert(wm_stringer_length("A B", f, 0) == 10 + 5 + 10);
    assert(wm_stringer_length("A B", f, 2) == (10 + 2) + (5 + 2) + (10 + 2));

    /* Every character at or below 32 takes the space path, not just ' '. */
    assert(wm_stringer_length("\t", f, 0) == WM_STRINGER_SPACE_WIDTH);
    puts("stringer: STRNGLEN arithmetic PASS");
}

/*
 * `movb` sign-extends and the loop tests `jrgt` / `jrle`, so a byte with the
 * high bit set ENDS the string. It is not a character and not a space.
 */
static void test_a_high_bit_byte_ends_the_string(void) {
    const wm_stringer_font *f = fixed();
    assert(wm_stringer_length("AB\x80" "CD", f, 0) == 20);
    assert(wm_stringer_length("\x80", f, 0) == 0);
    puts("stringer: high-bit byte terminates PASS");
}

static void test_justification(void) {
    const wm_stringer_font *f = fixed();
    wm_stringer_glyph g[64];
    int32_t n, len;

    /* Left takes no adjustment: the source jumps to strr10 without touching
       a9, so the first glyph starts exactly at the x passed in. */
    n = wm_stringer_place("AB", f, 1, WM_STRINGER_LEFT, 200, 110, g, 64);
    assert(n == 2);
    assert(g[0].x == 200);
    assert(g[0].y == 110);
    assert(g[1].x == 200 + 10 + 1);

    len = wm_stringer_length("AB", f, 1);
    assert(len == 22);

    /* Centre is `STRNGLEN` then `srl 1` -- an arithmetic halving, so 22/2. */
    n = wm_stringer_place("AB", f, 1, WM_STRINGER_CENTRE, 200, 110, g, 64);
    assert(n == 2);
    assert(g[0].x == 200 - (len / 2));

    /* Right subtracts the whole length. */
    n = wm_stringer_place("AB", f, 1, WM_STRINGER_RIGHT, 200, 110, g, 64);
    assert(g[0].x == 200 - len);

    /* An odd length halves toward zero, the way `srl` does on a positive. */
    len = wm_stringer_length("A", f, 1);   /* 11 */
    assert(len == 11);
    n = wm_stringer_place("A", f, 1, WM_STRINGER_CENTRE, 100, 0, g, 64);
    assert(g[0].x == 100 - 5);
    puts("stringer: justification PASS");
}

static void test_spaces_are_placed_too(void) {
    const wm_stringer_font *f = fixed();
    wm_stringer_glyph g[64];
    int32_t n = wm_stringer_place("A B", f, 0, WM_STRINGER_LEFT, 0, 0, g, 64);

    assert(n == 3);
    assert(g[0].slot >= 0);
    assert(g[1].slot == -1);                          /* the space */
    assert(g[1].width == WM_STRINGER_SPACE_WIDTH);
    assert(g[2].slot >= 0);
    assert(g[0].x == 0 && g[1].x == 10 && g[2].x == 15);
    puts("stringer: spaces occupy a placement PASS");
}

/* Slot 0 is '!' -- RD7FONT's first entry is FONT7excla, and stringr1_1's
   index is char - 33. Getting this off by one shifts the whole alphabet. */
static void test_the_slot_base(void) {
    const wm_stringer_font *f = fixed();
    wm_stringer_glyph g[8];

    assert(wm_stringer_place("!", f, 0, WM_STRINGER_LEFT, 0, 0, g, 8) == 1);
    assert(g[0].slot == 0);
    assert(wm_stringer_place("A", f, 0, WM_STRINGER_LEFT, 0, 0, g, 8) == 1);
    assert(g[0].slot == 'A' - 33);
    puts("stringer: slot base PASS");
}

/*
 * The refusal that matters. Six RD7FONT slots have no measurable width --
 * FONT7parenl/parenr and FONT7paren2l/paren2r share the truncated name
 * `FONT7par`, FONT7percen and FONT7period share `FONT7per`, and FONTS.LOD,
 * which would give the packing order, is zero bytes in this tree. The engine
 * must refuse to measure or place a string containing one rather than
 * treating -1 as zero and quietly shifting a centred line.
 */
static void test_unknown_widths_fail_closed(void) {
    const wm_stringer_font *rd7 = wm_stringer_rd7font();
    wm_stringer_glyph g[128];
    int unknown = 0;
    size_t i;

    for (i = 0; i < WM_RD7FONT_SLOTS; ++i) {
        if (wm_rd7font_width[i] < 0) ++unknown;
    }
    /* Four slots: FONT7parenl, FONT7parenr, FONT7paren2l, FONT7paren2r.
       None is reused elsewhere in the table, unlike FONT7excla and
       FONT7dash which each fill two slots. */
    assert(unknown == 4);
    assert(wm_rd7font_width['(' - WM_RD7FONT_FIRST_CHAR] < 0);
    assert(wm_rd7font_width[')' - WM_RD7FONT_FIRST_CHAR] < 0);
    assert(wm_rd7font_width['{' - WM_RD7FONT_FIRST_CHAR] < 0);
    assert(wm_rd7font_width['}' - WM_RD7FONT_FIRST_CHAR] < 0);

    /*
     * '.' and '%' WERE in that set and are not any more: they were settled
     * by reading the artwork rather than the truncated name. A 3x2 solid
     * block is a full stop and an 11x8 pair of rings joined by a diagonal
     * is a percent sign. These two widths are the whole reason 22 of the 25
     * attract lines are measurable instead of 14.
     */
    assert(wm_rd7font_width['.' - WM_RD7FONT_FIRST_CHAR] == 3);
    assert(wm_rd7font_width['%' - WM_RD7FONT_FIRST_CHAR] == 11);
    /* And a letter IS measured, so -1 is not simply everywhere. */
    assert(wm_rd7font_width['A' - WM_RD7FONT_FIRST_CHAR] > 0);

    assert(!wm_stringer_measurable("(C) 1995", rd7));  /* the paren */
    assert(wm_stringer_length("(C) 1995", rd7, 1) == -1);
    assert(wm_stringer_place("(C) 1995", rd7, 1, WM_STRINGER_CENTRE,
                             200, 110, g, 128) == -1);

    /* A line with none of them measures fine, so the refusal is specific
       rather than the engine simply not working with RD7FONT. */
    assert(wm_stringer_measurable("ALL RIGHTS RESERVED", rd7));
    assert(wm_stringer_length("ALL RIGHTS RESERVED", rd7, 1) > 0);
    /* And a full stop no longer blocks a line, which is the point of the
       artwork identification. */
    assert(wm_stringer_measurable("ALL RIGHTS RESERVED.", rd7));
    assert(wm_stringer_measurable("FROM ACCLAIM ENTERTAINMENT INC.", rd7));
    puts("stringer: unknown widths fail closed PASS");
}

/*
 * What this unblocks, and what it does not. Every copyright and AAMA line
 * that avoids the six unmeasured glyphs now has a computable centred layout.
 * Report the split rather than asserting a number, so the count moves with
 * the data instead of pinning today's.
 */
static void test_the_real_attract_lines(void) {
    const wm_stringer_font *rd7 = wm_stringer_rd7font();
    size_t i, ok = 0, blocked = 0;

    for (i = 0; i < wm_attract_text_count; ++i) {
        const wm_attract_text_entry *e = &wm_attract_text_entries()[i];
        if (wm_stringer_measurable(e->text, rd7)) {
            wm_stringer_glyph g[128];
            int32_t n = wm_stringer_place(
                e->text, rd7, 1, WM_STRINGER_CENTRE,
                WM_ATTRACT_COPYRIGHT_X, WM_ATTRACT_COPYRIGHT_FIRST_Y,
                g, 128);
            assert(n == (int32_t)strlen(e->text));
            /* A centred line straddles its centre. */
            assert(g[0].x <= WM_ATTRACT_COPYRIGHT_X);
            ++ok;
        } else {
            ++blocked;
        }
    }
    assert(ok + blocked == wm_attract_text_count);
    assert(ok > 0 && blocked > 0);
    printf("stringer: %zu of %zu attract lines have a computable layout; "
           "%zu still blocked on the four unresolved paren slots\n",
           ok, wm_attract_text_count, blocked);
}


/* ---- the draw list ------------------------------------------------ */

/*
 * The two generated tables come out of one resolution pass, so a slot with a
 * width must have a mask and the two must agree. If they ever disagree, one
 * of the files was edited by hand -- both say "Do not edit" -- and the
 * stringer would place a glyph at a position its artwork does not fill.
 */
static void test_widths_and_masks_agree(void) {
    size_t i, with_ink = 0;

    for (i = 0; i < WM_RD7FONT_SLOTS; ++i) {
        const wm_rd7font_glyph *g = &wm_rd7font_glyph_table[i];
        if (wm_rd7font_width[i] < 0) {
            assert(g->ink == NULL);          /* unresolved both ways */
            continue;
        }
        assert(g->ink != NULL);
        assert(g->width == wm_rd7font_width[i]);
        assert(g->height > 0);
        ++with_ink;
    }
    /* 93 slots, 4 unresolved. */
    assert(with_ink == WM_RD7FONT_SLOTS - 4);
    puts("stringer: widths and masks agree PASS");
}

/* Every mask must have at least one ink pixel -- an all-zero mask would
   draw nothing while the pen advanced, which is the failure a NULL is
   supposed to prevent. */
static void test_no_mask_is_blank(void) {
    size_t i, j;
    for (i = 0; i < WM_RD7FONT_SLOTS; ++i) {
        const wm_rd7font_glyph *g = &wm_rd7font_glyph_table[i];
        int ink = 0;
        if (!g->ink) continue;
        for (j = 0; j < (size_t)(g->width * g->height); ++j) {
            if (g->ink[j]) { ink = 1; break; }
        }
        assert(ink && "a resolved glyph with no ink pixels");
    }
    puts("stringer: no resolved mask is blank PASS");
}

static void test_draw_list_positions_and_spaces(void) {
    const wm_stringer_font *rd7 = wm_stringer_rd7font();
    wm_text_draw_item items[128];
    wm_stringer_glyph placed[128];
    int32_t n, m, i, j;

    /* A space produces no item but still advances the pen, so the item
       count is the non-space character count and the x of the character
       after a space reflects the five pixels plus spacing. */
    n = wm_text_build_draw_list("AB CD", rd7, 1, WM_STRINGER_LEFT,
                                0, 0, items, 128);
    assert(n == 4);

    m = wm_stringer_place("AB CD", rd7, 1, WM_STRINGER_LEFT, 0, 0, placed, 128);
    assert(m == 5);
    /* The items must line up with the non-space placements, in order. */
    for (i = 0, j = 0; i < m; ++i) {
        if (placed[i].slot < 0) continue;
        assert(items[j].x == placed[i].x);
        assert(items[j].y == placed[i].y);
        assert(items[j].width == placed[i].width);
        ++j;
    }
    assert(j == n);
    puts("stringer: draw list positions PASS");
}

/*
 * All or nothing. A line containing an unresolved glyph must return -1
 * rather than a list with that character quietly dropped -- a copyright
 * notice missing its parentheses is worse than one not drawn.
 */
static void test_draw_list_refuses_a_partial_line(void) {
    const wm_stringer_font *rd7 = wm_stringer_rd7font();
    wm_text_draw_item items[128];

    assert(wm_text_build_draw_list("(C) 1995", rd7, 1, WM_STRINGER_CENTRE,
                                   200, 110, items, 128) == -1);
    /* Not because the call is broken: the same line without the parens works. */
    assert(wm_text_build_draw_list("C 1995", rd7, 1, WM_STRINGER_CENTRE,
                                   200, 110, items, 128) > 0);
    /* A capacity too small is also a refusal, not a truncation. */
    assert(wm_text_build_draw_list("ALL RIGHTS RESERVED", rd7, 1,
                                   WM_STRINGER_LEFT, 0, 0, items, 2) == -1);
    /* A font with no masks loaded can measure but not draw. */
    {
        wm_stringer_font no_art = *rd7;
        no_art.glyphs = NULL;
        assert(wm_stringer_length("ABC", &no_art, 1) > 0);
        assert(wm_text_build_draw_list("ABC", &no_art, 1, WM_STRINGER_LEFT,
                                       0, 0, items, 128) == -1);
    }
    puts("stringer: draw list refuses partial lines PASS");
}

/* The real copyright page, end to end: every line the widths allow must
   produce a complete draw list at the source's own geometry. */
static void test_the_copyright_page_draws(void) {
    const wm_stringer_font *rd7 = wm_stringer_rd7font();
    wm_text_draw_item items[256];
    size_t i, drew = 0, refused = 0;

    for (i = 0; i < WM_ATTRACT_COPYRIGHT_PAGE1_LINES; ++i) {
        const char *t = wm_attract_text(wm_attract_copyright_page1_labels[i]);
        int32_t n = wm_text_build_draw_list(
            t, rd7, 1, WM_STRINGER_CENTRE, WM_ATTRACT_COPYRIGHT_X,
            (int32_t)(WM_ATTRACT_COPYRIGHT_FIRST_Y +
                      i * WM_ATTRACT_COPYRIGHT_LINE_STEP),
            items, 256);
        if (n < 0) { ++refused; continue; }
        ++drew;
        /* The line sits on its own row and straddles the centre. */
        assert(items[0].y == (int16_t)(WM_ATTRACT_COPYRIGHT_FIRST_Y +
                                      i * WM_ATTRACT_COPYRIGHT_LINE_STEP));
        assert(items[0].x <= WM_ATTRACT_COPYRIGHT_X);
        assert(items[n - 1].x >= items[0].x);
    }
    printf("stringer: copyright page 1 -- %zu of %u lines draw, %zu refused\n",
           drew, (unsigned)WM_ATTRACT_COPYRIGHT_PAGE1_LINES, refused);
    assert(drew > 0);
}

int main(void) {
    test_strnglen_arithmetic();
    test_a_high_bit_byte_ends_the_string();
    test_justification();
    test_spaces_are_placed_too();
    test_the_slot_base();
    test_unknown_widths_fail_closed();
    test_the_real_attract_lines();
    test_widths_and_masks_agree();
    test_no_mask_is_blank();
    test_draw_list_positions_and_spaces();
    test_draw_list_refuses_a_partial_line();
    test_the_copyright_page_draws();
    puts("stringer: all checks passed");
    return 0;
}
