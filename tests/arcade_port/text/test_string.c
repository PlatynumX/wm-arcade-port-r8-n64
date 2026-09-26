/*
 * STRING.ASM's text engine, checked against the source's own arithmetic.
 *
 * The interesting cases here are the ones where the assembly does
 * something a rewrite would quietly "fix": the seven-digit ceiling on
 * dec_to_asc, the trailing spacing that comes back off a measurement, an
 * unmapped character costing nothing at all, and per-line justification.
 */
#include "wm/arcade/wm_arcade_string.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* --------------------------------------------------------------- */
/* a stand-in for the .LOD art the arcade measures glyphs out of.    */

typedef struct {
    int width;              /* every glyph the artwork cannot measure */
    int calls;
} fake_art;

static int fake_width(void *ctx, const char *glyph)
{
    fake_art *art = (fake_art *)ctx;
    (void)glyph;
    art->calls++;
    return art->width;
}

typedef struct {
    int n;
    char glyph[64][16];
    int32_t x[64];
    int32_t y[64];
    int32_t z[64];
} draw_log;

static void fake_draw(void *ctx, const char *glyph, int32_t x, int32_t y,
                      int32_t z, int16_t objid, uint32_t dma, const char *pal)
{
    draw_log *log = (draw_log *)ctx;
    (void)objid;
    (void)dma;
    (void)pal;
    if (log->n < 64) {
        strncpy(log->glyph[log->n], glyph, sizeof(log->glyph[0]) - 1u);
        log->glyph[log->n][sizeof(log->glyph[0]) - 1u] = '\0';
        log->x[log->n] = x;
        log->y[log->n] = y;
        log->z[log->n] = z;
        log->n++;
    }
}

static void wire(wm_string_state *st, fake_art *art, draw_log *log)
{
    wm_string_init(st);
    /* FNT9.IMG's names are all eight characters or fewer, so every glyph
     * font9_ascii names has a real extracted width -- which is what makes
     * the numbers below the arcade's own and not this test's. */
    st->font = &wm_font_font9;
    st->width_fn = fake_width;
    st->width_ctx = art;
    st->draw_fn = fake_draw;
    st->draw_ctx = log;
}

/* --------------------------------------------------------------- */

static void test_binbcd(void)
{
    /* ADJUST.ASM:1593.  The do-while means zero still produces zero. */
    assert(wm_bin_to_bcd(0u) == 0x00000000u);
    assert(wm_bin_to_bcd(9u) == 0x00000009u);
    assert(wm_bin_to_bcd(10u) == 0x00000010u);
    assert(wm_bin_to_bcd(1995u) == 0x00001995u);
    assert(wm_bin_to_bcd(99999999u) == 0x99999999u);
    /* "RETURN THE LARGEST NUMBER WE HAVE!" -- and note it is the BCD
     * constant 99999999h, not the BCD *of* anything. */
    assert(wm_bin_to_bcd(100000000u) == 0x99999999u);
    assert(wm_bin_to_bcd(0xFFFFFFFFu) == 0x99999999u);
}

static void test_dec_to_asc(void)
{
    wm_string_state st;
    wm_string_init(&st);

    wm_string_dec_to_asc(&st, 0u, 999999u);
    assert(strcmp(st.buffer2, "0") == 0);       /* last digit is uncondit'l */

    wm_string_dec_to_asc(&st, 1995u, 999999u);
    assert(strcmp(st.buffer2, "1995") == 0);    /* leading zeros suppressed */

    wm_string_dec_to_asc(&st, 1000000u, 9999999u);
    assert(strcmp(st.buffer2, "1000000") == 0); /* seven digits fit exactly */

    /* The clamp is `cmp a0,a1 / jrhi`: it fires unless max > value, so a
     * value at the ceiling is "clamped" to itself and one over is cut. */
    wm_string_dec_to_asc(&st, 500u, 100u);
    assert(strcmp(st.buffer2, "100") == 0);
    wm_string_dec_to_asc(&st, 100u, 100u);
    assert(strcmp(st.buffer2, "100") == 0);
    wm_string_dec_to_asc(&st, 99u, 100u);
    assert(strcmp(st.buffer2, "99") == 0);

    /* Only nibbles 6..0 are printed.  BINBCD can produce an eighth digit
     * and dec_to_asc drops it -- 99999999 prints as seven nines. */
    wm_string_dec_to_asc(&st, 99999999u, 99999999u);
    assert(strcmp(st.buffer2, "9999999") == 0);
}

static void test_dec_to_pct(void)
{
    wm_string_state st;
    wm_string_init(&st);

    /* Three digits, no suppression at all. */
    wm_string_dec_to_pct(&st, 0u);
    assert(strcmp(st.buffer2, "000") == 0);
    wm_string_dec_to_pct(&st, 7u);
    assert(strcmp(st.buffer2, "007") == 0);
    wm_string_dec_to_pct(&st, 100u);
    assert(strcmp(st.buffer2, "100") == 0);
    wm_string_dec_to_pct(&st, 999u);
    assert(strcmp(st.buffer2, "999") == 0);
    /* Out of its documented 000-999 range it keeps the low three BCD
     * digits, because that is all the three nibbles it reads can hold. */
    wm_string_dec_to_pct(&st, 1234u);
    assert(strcmp(st.buffer2, "234") == 0);
}

static void test_copy_and_concat(void)
{
    wm_string_state st;
    wm_string_init(&st);

    strcpy(st.buffer2, "BRET");
    wm_string_copy(&st);
    assert(strcmp(st.buffer, "BRET") == 0);

    strcpy(st.buffer2, " HART");
    wm_string_concat(&st);
    assert(strcmp(st.buffer, "BRET HART") == 0);

    wm_string_copy_rom(&st, "YOKOZUNA");
    assert(strcmp(st.buffer, "YOKOZUNA") == 0);
    wm_string_concat_rom(&st, " WINS");
    assert(strcmp(st.buffer, "YOKOZUNA WINS") == 0);

    /* copy_string copies the terminator too, so a shorter source really
     * does shorten the destination rather than leaving a tail behind. */
    strcpy(st.buffer2, "OK");
    wm_string_copy(&st);
    assert(strcmp(st.buffer, "OK") == 0);

    wm_string_clear_buffers(&st);
    assert(st.buffer[0] == '\0' && st.buffer2[0] == '\0');
}

static void test_concat_stops_at_the_buffer(void)
{
    /* The arcade has no bound here at all; this port stops at 80 chars
     * (MBUFF_SIZE is 40 *words*) and stays terminated. */
    wm_string_state st;
    int i;
    wm_string_init(&st);
    for (i = 0; i < 30; ++i) {
        strcpy(st.buffer2, "ABCDE");
        wm_string_concat(&st);
    }
    assert(strlen(st.buffer) == WM_MESSAGE_BUFFER_CHARS - 1u);
    assert(st.buffer[WM_MESSAGE_BUFFER_CHARS - 1u] == '\0');
}

static void test_measurement(void)
{
    wm_string_state st;
    fake_art art;
    draw_log log;

    memset(&art, 0, sizeof(art));
    memset(&log, 0, sizeof(log));
    art.width = 10;
    wire(&st, &art, &log);
    st.spacing = 2;
    st.space_width = 6;

    /* FNT9_A is 11 pixels, FNT9_B and FNT9_C are 9 -- read out of
     * FNT9.IMG, not chosen here. */
    assert(wm_glyph_width_from_art("FNT9_A") == 11);
    assert(wm_glyph_width_from_art("FNT9_B") == 9);
    assert(wm_glyph_width_from_art("FNT9_C") == 9);
    assert(wm_string_len(&st, "ABC") == (11 + 2) + (9 + 2) + (9 + 2) - 2);

    /* A space costs mess_space_width and no spacing. */
    assert(wm_string_len(&st, "A B") == (11 + 2) + 6 + (9 + 2) - 2);

    /* The empty string measures -spacing.  That is the source's answer
     * (`sub a0,a2` runs unconditionally), not a rewrite of it. */
    assert(wm_string_len(&st, "") == -2);

    /* Measurement stops at the newline, so it is one line's width. */
    assert(wm_string_len(&st, "AB\001CDEF") == (11 + 2) + (9 + 2) - 2);

    /* A character with no glyph in this font costs nothing -- not even
     * the inter-character spacing.  '*' is 0 in every shipped table. */
    assert(wm_font_glyph(&wm_font_font9, '*') == NULL);
    assert(wm_string_len(&st, "A*B") == wm_string_len(&st, "AB"));

    /* IGNORE_CHAR_WIDTH keeps the spacing and drops the glyph width. */
    st.ignore_char_width = 1;
    assert(wm_string_len(&st, "ABC") == 3 * 2 - 2);
    st.ignore_char_width = 0;
}

static void test_the_width_seam_covers_only_what_the_art_cannot(void)
{
    /* SGMD8.IMG stores osgmd8_A, osgmd8_AND and osgmd8_APO all three
     * under the eight-character name `osgmd8_A`, and the .LOD that would
     * order them is empty here -- so that glyph has no extracted width
     * and the seam is what answers for it.  osgmd8_B has no such twin. */
    wm_string_state st;
    fake_art art;
    draw_log log;

    memset(&art, 0, sizeof(art));
    memset(&log, 0, sizeof(log));
    art.width = 10;
    wire(&st, &art, &log);
    st.font = &wm_font_osgmd8;
    st.spacing = 0;

    assert(wm_glyph_width_from_art("osgmd8_A") == -1);
    assert(wm_glyph_width_from_art("osgmd8_B") == 8);
    assert(wm_glyph_width_from_art("no_such_glyph") == -1);

    assert(wm_string_len(&st, "B") == 8);
    assert(art.calls == 0);         /* the art answered; no seam call */
    assert(wm_string_len(&st, "A") == 10);
    assert(art.calls == 1);         /* only the unmeasurable one asks */

    /* With no seam wired, an unmeasurable glyph is zero wide and a
     * measured one is unaffected. */
    st.width_fn = NULL;
    assert(wm_string_len(&st, "A") == 0);
    assert(wm_string_len(&st, "B") == 8);
}

static void test_justification(void)
{
    wm_string_state st;
    fake_art art;
    draw_log log;
    int width;

    memset(&art, 0, sizeof(art));
    memset(&log, 0, sizeof(log));
    art.width = 10;
    wire(&st, &art, &log);
    st.spacing = 2;
    st.cursx = 200;

    width = wm_string_len(&st, "ABC");          /* 13 + 11 + 11 - 2 */
    assert(width == 33);

    st.justify = WM_JUSTIFY_LEFT;
    wm_string_justify(&st, "ABC");
    assert(st.cursx2 == 200);

    st.justify = WM_JUSTIFY_CENTER;
    wm_string_justify(&st, "ABC");
    assert(st.cursx2 == 200 - (33 >> 1));       /* the halving is a shift */

    st.justify = WM_JUSTIFY_RIGHT;
    wm_string_justify(&st, "ABC");
    assert(st.cursx2 == 200 - 33);

    /* setup_message's justify hook is a bare `rets`: it must not move the
     * working cursor, whatever the anchor says. */
    st.cursx2 = 123;
    st.justify = WM_JUSTIFY_NONE;
    wm_string_justify(&st, "ABC");
    assert(st.cursx2 == 123);
}

static void test_printing(void)
{
    wm_string_state st;
    fake_art art;
    draw_log log;

    memset(&art, 0, sizeof(art));
    memset(&log, 0, sizeof(log));
    art.width = 10;
    wire(&st, &art, &log);
    st.spacing = 2;
    st.space_width = 6;
    st.cursx = 100;
    st.cursy = 50;
    st.objid = WM_TYPTEXT;

    wm_string_print(&st, "AB", WM_JUSTIFY_LEFT, WM_STRING_DEFAULT_Z);
    assert(log.n == 2);
    assert(strcmp(log.glyph[0], "FNT9_A") == 0);
    assert(strcmp(log.glyph[1], "FNT9_B") == 0);
    /* BEGINOBJP takes x and y as `sll 16` -- pixels in the high half. */
    assert(log.x[0] == (100 << 16));
    assert(log.x[1] == ((100 + 11 + 2) << 16));
    assert(log.y[0] == (50 << 16));
    assert(log.z[0] == WM_STRING_DEFAULT_Z);
    assert(st.cursx2 == 100 + (11 + 2) + (9 + 2));

    /* A glyph-less character advances nothing, so the next one lands
     * exactly where it would have without it. */
    memset(&log, 0, sizeof(log));
    st.cursy = 50;
    wm_string_print(&st, "A*B", WM_JUSTIFY_LEFT, WM_STRING_DEFAULT_Z);
    assert(log.n == 2);
    assert(log.x[1] == ((100 + 11 + 2) << 16));
}

static void test_newline_rejustifies_each_line(void)
{
    /* This is the detail worth having a test for: on a newline the printer
     * calls the justification hook again over the *rest* of the string, so
     * a two-line centred message centres each line on its own width. */
    wm_string_state st;
    fake_art art;
    draw_log log;

    memset(&art, 0, sizeof(art));
    memset(&log, 0, sizeof(log));
    art.width = 10;
    wire(&st, &art, &log);
    st.spacing = 2;
    st.line_spacing = 20;
    st.cursx = 200;
    st.cursy = 50;

    /* "ABCD" is 11+9+9+9 + 4*2 - 2 = 44 wide; "EF" is 7+7 + 2*2 - 2 = 16. */
    assert(wm_string_len(&st, "ABCD") == 44);
    assert(wm_string_len(&st, "EF") == 16);
    wm_string_print(&st, "ABCD\001EF", WM_JUSTIFY_CENTER, WM_STRING_DEFAULT_Z);
    assert(log.n == 6);
    assert(log.x[0] == ((200 - (44 >> 1)) << 16));
    assert(log.y[0] == (50 << 16));
    /* Second line: its own centring, one line_spacing down. */
    assert(log.x[4] == ((200 - (16 >> 1)) << 16));
    assert(log.y[4] == (70 << 16));
    assert(st.cursy == 70);
}

static void test_font_tables(void)
{
    int i;

    assert(wm_font_table_count == 10);
    for (i = 0; i < wm_font_table_count; ++i) {
        assert(wm_font_tables[i]->name != NULL);
        assert(wm_font_tables[i]->slots != NULL);
        /* Codes 0..15 are control space in every shipped table except
         * the three high-score editing glyphs at $10..$12. */
        assert(wm_font_tables[i]->slots[0] == NULL);
        /* Space is never looked up -- the printer handles it first. */
        assert(wm_font_tables[i]->slots[' '] == NULL);
        /* Lower case aliases upper case in all ten tables. */
        assert(wm_font_tables[i]->slots['a'] == wm_font_tables[i]->slots['A']);
    }

    assert(wm_font_table_find("font9_ascii") == &wm_font_font9);
    assert(wm_font_table_find("win_ascii") == &wm_font_win);
    assert(wm_font_table_find("sgmd8_ascii") == NULL);  /* commented out */

    /* font9A is font9 with the alternate digits and nothing else. */
    for (i = 0; i < 128; ++i) {
        const char *a = wm_font_font9.slots[i];
        const char *b = wm_font_font9a.slots[i];
        if (i >= '0' && i <= '9') {
            assert(a && b && strcmp(a, b) != 0);
            assert(strcmp(b, a) != 0 && b[strlen(b) - 1u] == 'A');
        } else {
            assert(a == b || (a && b && strcmp(a, b) == 0));
        }
    }
    /* win_ascii is ogmd10 with WFONT digits, same rule. */
    for (i = 0; i < 128; ++i) {
        const char *a = wm_font_ogmd10.slots[i];
        const char *b = wm_font_win.slots[i];
        if (i >= '0' && i <= '9') {
            assert(strncmp(b, "WFONT_", 6) == 0);
        } else {
            assert(a == b || (a && b && strcmp(a, b) == 0));
        }
    }

    /* The high-score editor's three cursor glyphs live at codes $10-$12. */
    assert(strcmp(wm_font_font9.slots[0x10], "FNT9_SPC") == 0);
    assert(strcmp(wm_font_font9.slots[0x11], "FNT9_DEL") == 0);
    assert(strcmp(wm_font_font9.slots[0x12], "FNT9_END") == 0);

    /* Codes above 127 have no glyph here; the arcade would read past the
     * end of the table instead. */
    assert(wm_font_glyph(&wm_font_font9, 200) == NULL);
    assert(wm_font_glyph(&wm_font_font9, -1) == NULL);
    assert(wm_font_glyph(NULL, 'A') == NULL);
}

static void test_message_descriptor(void)
{
    /* The real descriptor from src/generated/sports_motto.c:
     *   JAM_STR osgmd8_ascii,6,0,200,225,SGMD8WHT,print_string_C2 */
    wm_string_state st;
    fake_art art;
    draw_log log;
    wm_message_desc desc;
    wm_print_method method;

    memset(&art, 0, sizeof(art));
    memset(&log, 0, sizeof(log));
    art.width = 10;
    wire(&st, &art, &log);

    assert(wm_print_method_from_name("print_string_C2", &method));
    assert(method == WM_PRINT_STRING_C2);
    assert(wm_print_method_from_name("0", &method));
    assert(method == WM_PRINT_NONE);
    /* Two shipped descriptors name a register instead of a routine. */
    assert(!wm_print_method_from_name("a10", &method));

    desc.font = "osgmd8_ascii";
    desc.space_width = 6;
    desc.spacing = 0;
    desc.cursx = 200;
    desc.cursy = 225;
    desc.palette = "SGMD8WHT";
    desc.method = WM_PRINT_STRING_C2;
    desc.text = "BB";

    wm_string_print_message(&st, &desc);
    assert(st.objid == WM_TYPTEXT);
    assert(st.font == &wm_font_osgmd8);
    assert(st.space_width == 6);
    assert(st.cursx == 200 && st.cursy == 225);
    assert(strcmp(st.palette, "SGMD8WHT") == 0);
    /* osgmd8_B is 8 wide and spacing is 0, so "BB" is 16, centred on 200. */
    assert(log.n == 2);
    assert(log.x[0] == ((200 - 8) << 16));
    assert(log.y[0] == (225 << 16));

    /* setup_message loads the same six fields but leaves justification
     * doing nothing and never looks at the method or the text. */
    memset(&log, 0, sizeof(log));
    wm_string_init(&st);
    st.cursx2 = 77;
    wm_string_setup_message(&st, &desc);
    assert(st.justify == WM_JUSTIFY_NONE);
    assert(st.objid == WM_TYPTEXT);
    assert(st.cursx2 == 77);
    assert(log.n == 0);
}

static void test_print_methods_pick_their_source(void)
{
    /* Half the entry points start with `movi message_buffer,a4` and ignore
     * whatever the caller was pointing at; the `2` variants do not. */
    wm_string_state st;
    fake_art art;
    draw_log log;

    memset(&art, 0, sizeof(art));
    memset(&log, 0, sizeof(log));
    art.width = 10;
    wire(&st, &art, &log);
    strcpy(st.buffer, "A");

    wm_string_print_method(&st, WM_PRINT_STRING, "BB");
    assert(log.n == 1 && strcmp(log.glyph[0], "FNT9_A") == 0);

    memset(&log, 0, sizeof(log));
    wm_string_print_method(&st, WM_PRINT_STRING2, "BB");
    assert(log.n == 2 && strcmp(log.glyph[0], "FNT9_B") == 0);

    /* The two _with_z printers use mess_z; the rest hardcode 20000. */
    memset(&log, 0, sizeof(log));
    st.z = 4242;
    wm_string_print_method(&st, WM_PRINT_STRING_WITH_Z, NULL);
    assert(log.n == 1 && log.z[0] == 4242);

    memset(&log, 0, sizeof(log));
    wm_string_print_method(&st, WM_PRINT_STRING_C, NULL);
    assert(log.n == 1 && log.z[0] == WM_STRING_DEFAULT_Z);

    /* A descriptor with no method prints nothing. */
    memset(&log, 0, sizeof(log));
    wm_string_print_method(&st, WM_PRINT_NONE, "BB");
    assert(log.n == 0);
}

static void test_score_through_the_whole_pipeline(void)
{
    /* What LIFEBAR/HSTD actually do: BINBCD the score, format it into
     * buffer2, copy it across, and print it. */
    wm_string_state st;
    fake_art art;
    draw_log log;

    memset(&art, 0, sizeof(art));
    memset(&log, 0, sizeof(log));
    art.width = 8;
    wire(&st, &art, &log);
    st.spacing = 1;
    st.cursx = 160;

    wm_string_dec_to_asc(&st, 12345u, 9999999u);
    wm_string_copy(&st);
    assert(strcmp(st.buffer, "12345") == 0);
    /* FNT9_1 is 5 wide and 2..5 are 9 -- a 1 really is narrower, so the
     * score is not laid out on a fixed pitch. */
    assert(wm_glyph_width_from_art("FNT9_1") == 5);
    assert(wm_glyph_width_from_art("FNT9_4") == 11);
    assert(wm_string_buffer_len(&st) == (5 + 9 + 9 + 11 + 9) + 5 * 1 - 1);

    wm_string_print_method(&st, WM_PRINT_STRING_C, NULL);
    assert(log.n == 5);
    assert(strcmp(log.glyph[0], "FNT9_1") == 0);
    assert(strcmp(log.glyph[4], "FNT9_5") == 0);
    assert(log.x[0] == ((160 - (47 >> 1)) << 16));
    assert(log.x[1] == ((160 - (47 >> 1) + 5 + 1) << 16));
}

int main(void)
{
    test_binbcd();
    test_dec_to_asc();
    test_dec_to_pct();
    test_copy_and_concat();
    test_concat_stops_at_the_buffer();
    test_measurement();
    test_the_width_seam_covers_only_what_the_art_cannot();
    test_justification();
    test_printing();
    test_newline_rejustifies_each_line();
    test_font_tables();
    test_message_descriptor();
    test_print_methods_pick_their_source();
    test_score_through_the_whole_pipeline();
    printf("STRING.ASM text engine: all checks passed\n");
    return 0;
}
