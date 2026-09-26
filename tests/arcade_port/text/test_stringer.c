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
    /* Six slots: FONT7percen, FONT7parenl, FONT7parenr, FONT7period,
       FONT7paren2l, FONT7paren2r. None of the six is reused elsewhere in
       the table, unlike FONT7excla and FONT7dash which each fill two
       slots -- so six names means six slots. */
    assert(unknown == 6);

    /* '(' ')' and '.' are among them, and the copyright text uses all
       three, which is why this blocks real lines rather than only exotic
       ones. */
    assert(wm_rd7font_width['(' - WM_RD7FONT_FIRST_CHAR] < 0);
    assert(wm_rd7font_width[')' - WM_RD7FONT_FIRST_CHAR] < 0);
    assert(wm_rd7font_width['.' - WM_RD7FONT_FIRST_CHAR] < 0);
    assert(wm_rd7font_width['%' - WM_RD7FONT_FIRST_CHAR] < 0);
    /* And a letter IS measured, so -1 is not simply everywhere. */
    assert(wm_rd7font_width['A' - WM_RD7FONT_FIRST_CHAR] > 0);

    assert(!wm_stringer_measurable("(C) 1995", rd7));
    assert(wm_stringer_length("(C) 1995", rd7, 1) == -1);
    assert(wm_stringer_place("(C) 1995", rd7, 1, WM_STRINGER_CENTRE,
                             200, 110, g, 128) == -1);

    /* A line with none of them measures fine, so the refusal is specific
       rather than the engine simply not working with RD7FONT. */
    assert(wm_stringer_measurable("ALL RIGHTS RESERVED", rd7));
    assert(wm_stringer_length("ALL RIGHTS RESERVED", rd7, 1) > 0);
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
           "%zu blocked on the six unmeasured glyphs\n",
           ok, wm_attract_text_count, blocked);
}

int main(void) {
    test_strnglen_arithmetic();
    test_a_high_bit_byte_ends_the_string();
    test_justification();
    test_spaces_are_placed_too();
    test_the_slot_base();
    test_unknown_widths_fail_closed();
    test_the_real_attract_lines();
    puts("stringer: all checks passed");
    return 0;
}
