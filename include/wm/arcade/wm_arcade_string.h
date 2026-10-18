/*
 * STRING.ASM -- the text engine every message in the game goes through.
 *
 * Two buffers, one cursor, one font table, and a printer that walks a
 * NUL-terminated string a character at a time.  Translated from
 * original/wwf-wrestlemania/STRING.ASM, plus BINBCD (ADJUST.ASM:1593),
 * which is what turns a score into digits.
 *
 * What is here and what is not:
 *
 *   Here    the buffers, the binary->BCD->ASCII conversions, copy/concat,
 *           the pixel-width measurement, the three justifications, the
 *           newline handling, and the whole cursor walk.  All of it is
 *           arithmetic and all of it is exact.
 *
 *   Here    glyph widths, for 280 of the 375 symbols the font tables name.
 *           STRING.ASM does not hold them -- the printer reads the image
 *           header's first word at run time -- so they are extracted from
 *           the shipped .IMG containers into src/generated/font_metrics.c.
 *           That covers every letter and digit of every font.
 *
 *   Not     the remaining 95 widths, and drawing.  A WIMP image name field
 *           is eight characters, so 46 symbols are stored under a stem
 *           another symbol shares and cannot be told apart without
 *           FONTS.LOD's packing order (that file is zero bytes in this
 *           tree); the 49 `osgmd10_*` glyphs have no artwork here at all.
 *           Those fall through to a width seam rather than being invented.
 *           Drawing -- the arcade's BEGINOBJP -- is a seam outright.
 */
#ifndef WM_ARCADE_STRING_H
#define WM_ARCADE_STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * STRING.ASM: `MBUFF_SIZE equ 40` and `.bss message_buffer, 16*MBUFF_SIZE`
 * -- forty *words*.  Characters go in one per byte (`movb ... / addk 8`),
 * so the buffer really holds eighty of them, NUL included.
 */
#define WM_MESSAGE_BUFFER_WORDS 40
#define WM_MESSAGE_BUFFER_CHARS (WM_MESSAGE_BUFFER_WORDS * 2)

/* GAME.EQU:215 */
#define WM_TYPTEXT 0x0700
/* SYS.EQU:215 DMAWNZ, DISPLAY.EQU:115 M_SCRNREL */
#define WM_STRING_DMA_FLAGS (0x8002 | 0x2000)
/* The Z the two plain printers hardcode: `movi 20000,a3`. */
#define WM_STRING_DEFAULT_Z 20000

/* STRING.ASM's two in-band control codes.  Code 1 is a newline; code 0
 * ends the string.  Neither ever reaches the font table. */
#define WM_STRING_NEWLINE 1

/*
 * One `*_ascii` table: 128 ASCII slots, each an image symbol or NULL.
 * NULL is not a blank -- the printer skips the character outright, so it
 * costs neither its own width nor the inter-character spacing.
 */
typedef struct wm_font_table {
    const char *name;               /* the label in STRING.ASM */
    const char *const *slots;       /* [128], NULL where the source has 0 */
} wm_font_table;

extern const wm_font_table *const wm_font_tables[];
extern const int wm_font_table_count;

extern const wm_font_table wm_font_font9;
extern const wm_font_table wm_font_font9a;
extern const wm_font_table wm_font_font18;
extern const wm_font_table wm_font_osgemd;
extern const wm_font_table wm_font_osgmd8;
extern const wm_font_table wm_font_wgsf24;
extern const wm_font_table wm_font_wsf10;
extern const wm_font_table wm_font_wsf14;
extern const wm_font_table wm_font_ogmd10;
extern const wm_font_table wm_font_win;

/* One glyph's width in pixels, read from the .IMG artwork. */
typedef struct wm_glyph_metric {
    const char *name;
    int16_t width;
} wm_glyph_metric;

extern const wm_glyph_metric wm_glyph_metrics[];
extern const int wm_glyph_metric_count;

/* The extracted width of a glyph, or -1 when the artwork in this tree
 * cannot say (an eight-character name collision, or no image at all). */
int wm_glyph_width_from_art(const char *glyph);

/* Look a table up by its STRING.ASM label; NULL if there is no such font. */
const wm_font_table *wm_font_table_find(const char *name);

/* The glyph a font draws for one character, or NULL for none.
 *
 * The original masks the character with `andi 0ffh` and then indexes a
 * 128-entry table, so codes 128..255 read off the end of it.  That is a
 * genuine out-of-bounds read in the arcade code and is not reproduced:
 * anything above 127 has no glyph here. */
const char *wm_font_glyph(const wm_font_table *font, int ch);

/* ADJUST.ASM:1593 BINBCD -- binary to packed BCD, eight digits.  Anything
 * over 99999999 comes back as the constant 0x99999999, which is what the
 * source returns ("the largest number we have"). */
uint32_t wm_bin_to_bcd(uint32_t value);

/*
 * ADJUST.ASM:1624 BCDBIN -- the other way: read each nibble as a
 * decimal digit and sum them with place values 1, 10, 100...
 *
 * It is not a validating conversion. A nibble of A-F is multiplied
 * by its place like any other, so 0x1A comes back as 10*1 + 1*10 =
 * 20 rather than being rejected, and BCDBIN(BINBCD(n)) == n only
 * holds for values BINBCD did not clamp.
 */
uint32_t wm_bcd_to_bin(uint32_t bcd);

/* STRING.ASM's justification methods.  WM_JUSTIFY_NONE is `setup_message`'s
 * `movi #rets` -- a justify hook that does nothing, leaving the working
 * cursor wherever it already was. */
typedef enum wm_justify {
    WM_JUSTIFY_LEFT = 0,
    WM_JUSTIFY_CENTER,
    WM_JUSTIFY_RIGHT,
    WM_JUSTIFY_NONE
} wm_justify;

/* The print methods a JAM_STR descriptor can name (MACROS.H:144).  The
 * shipped descriptors use only print_string_C2, print_string2,
 * print_string and 0; the rest are reachable by direct call. */
typedef enum wm_print_method {
    WM_PRINT_NONE = 0,          /* a descriptor with a 0 method */
    WM_PRINT_STRING,            /* print_string    -- buffer, left, z=20000 */
    WM_PRINT_STRING2,           /* print_string2   -- caller's text, left */
    WM_PRINT_STRING_C,          /* print_string_C  -- buffer, centred */
    WM_PRINT_STRING_C2,         /* print_string_C2 -- caller's text, centred */
    WM_PRINT_STRING_R,          /* print_string_R  -- buffer, right */
    WM_PRINT_STRING_R2,         /* print_string_R2 -- caller's text, right */
    WM_PRINT_STRING_Z2,         /* print_string_Z2 -- centred, mess_z */
    WM_PRINT_STRING_WITH_Z      /* print_string_with_z -- buffer, left, mess_z */
} wm_print_method;

/* Map a JAM_STR method symbol onto the enum.  Returns false, without
 * touching *out, for a name this port does not know -- including the two
 * descriptors whose method is a register computed at run time. */
bool wm_print_method_from_name(const char *name, wm_print_method *out);

/* The width seam.  Consulted only for the glyphs wm_glyph_width_from_art
 * cannot answer, so a port that measures nothing still gets every letter
 * and digit right. */
typedef int (*wm_glyph_width_fn)(void *ctx, const char *glyph);

/* The draw seam: the arcade's BEGINOBJP.  One call per glyph placed. */
typedef void (*wm_glyph_draw_fn)(void *ctx, const char *glyph,
                                 int32_t x, int32_t y, int32_t z,
                                 int16_t objid, uint32_t dma_flags,
                                 const char *palette);

typedef struct wm_string_state {
    /* message_buffer / message_buffer2 */
    char buffer[WM_MESSAGE_BUFFER_CHARS];
    char buffer2[WM_MESSAGE_BUFFER_CHARS];

    const wm_font_table *font;  /* message_ascii */
    const char *palette;        /* message_palette */

    int16_t space_width;        /* mess_space_width */
    int16_t spacing;            /* mess_spacing */
    int16_t line_spacing;       /* mess_line_spacing */
    int16_t cursx;              /* mess_cursx  -- the anchor */
    int16_t cursx2;             /* mess_cursx2 -- the working cursor */
    int16_t cursy;              /* mess_cursy */
    int16_t objid;              /* mess_objid */
    int32_t z;                  /* mess_z */

    /* UTIL/STRING's IGNORE_CHAR_WIDTH: when set, a glyph advances the
     * cursor by the inter-character spacing alone. */
    int16_t ignore_char_width;

    wm_justify justify;         /* mess_justify */

    wm_glyph_width_fn width_fn;
    void *width_ctx;
    wm_glyph_draw_fn draw_fn;
    void *draw_ctx;
} wm_string_state;

/* Zero the state and give it the defaults STRING.ASM's .bss starts at. */
void wm_string_init(wm_string_state *st);

/* clear_buffers / its two halves (STRING.ASM:95). */
void wm_string_clear_buffers(wm_string_state *st);
void wm_string_clear_buffer(wm_string_state *st);
void wm_string_clear_buffer2(wm_string_state *st);

/*
 * dec_to_asc (STRING.ASM:129): clamp `value` to `max`, convert to BCD, and
 * write it into buffer2 as decimal ASCII with leading zeros suppressed.
 *
 * Two details are the source's, not a simplification.  The clamp is
 * `cmp a0,a1 / jrhi`, so it fires when max <= value -- equal clamps too,
 * harmlessly.  And only seven nibbles are printed (6 down to 0): the
 * ten-millions digit BINBCD can produce is never emitted, so a value of
 * 99999999 prints as "9999999".
 */
void wm_string_dec_to_asc(wm_string_state *st, uint32_t value, uint32_t max);

/* dec_to_asc_new_entry (STRING.ASM:141) -- the same seven-digit formatter,
 * entered with a BCD value and a destination of the caller's choosing.
 * HSTD.ASM and LIFEBAR.ASM both jump in here.  Writes at most 8 bytes. */
void wm_bcd_to_asc(char *dst, size_t dst_len, uint32_t bcd);

/* dec_to_pct (STRING.ASM:242): three digits, no suppression, 000-999. */
void wm_string_dec_to_pct(wm_string_state *st, uint32_t value);

/* copy_string / concat_string (STRING.ASM:278, :298) -- buffer2 -> buffer. */
void wm_string_copy(wm_string_state *st);
void wm_string_concat(wm_string_state *st);

/* copy_rom_string / concat_rom_string (STRING.ASM:325, :348). */
void wm_string_copy_rom(wm_string_state *st, const char *text);
void wm_string_concat_rom(wm_string_state *st, const char *text);

/*
 * get_string_len2 (STRING.ASM:378): the width of one line in pixels.
 *
 * Stops at NUL *and* at a newline, so it measures a single line.  Ends by
 * subtracting one inter-character spacing, which means an empty line
 * measures -spacing -- the source's own answer, kept.
 */
int wm_string_len(const wm_string_state *st, const char *text);

/* The width of `st->buffer`, which is what get_string_len itself does. */
int wm_string_buffer_len(const wm_string_state *st);

/* Apply one justification to the working cursor, from the text at `text`.
 * This is #left_justify / #center_justify / #right_justify. */
void wm_string_justify(wm_string_state *st, const char *text);

/*
 * print_string2b / print_string_with_zb (STRING.ASM:504, :585) -- walk the
 * string, place a glyph per drawable character, and leave the cursor after
 * the last one.  `z` is WM_STRING_DEFAULT_Z for the plain printers and
 * st->z for the _with_z pair.
 *
 * Sets st->justify first, then applies it, exactly as the entry points do.
 */
void wm_string_print(wm_string_state *st, const char *text,
                     wm_justify justify, int32_t z);

/* Run one of the named print methods.  `text` is ignored (the buffer is
 * used instead) by the methods whose arcade entry point starts with
 * `movi message_buffer,a4`. */
void wm_string_print_method(wm_string_state *st, wm_print_method method,
                            const char *text);

/*
 * A JAM_STR descriptor (MACROS.H:144): font, space width, char spacing,
 * cursor X, cursor Y, palette, print method -- then the text.
 */
typedef struct wm_message_desc {
    const char *font;           /* a wm_font_table label */
    int16_t space_width;
    int16_t spacing;
    int16_t cursx;
    int16_t cursy;
    const char *palette;
    wm_print_method method;
    const char *text;
} wm_message_desc;

/* setup_message (STRING.ASM:657): load the six layout fields, set the
 * object id to TYPTEXT, and leave justification doing nothing.  The
 * descriptor's method and text are deliberately not read. */
void wm_string_setup_message(wm_string_state *st, const wm_message_desc *desc);

/* print_message (STRING.ASM:686): setup_message's six fields plus the
 * method, then print the descriptor's own text through it. */
void wm_string_print_message(wm_string_state *st, const wm_message_desc *desc);

/*
 * HSTD.ASM:826 strip_white -- trim spaces off both ends of the
 * high-score work buffer and copy what is left into message_buffer.
 * `length` is the source's a3, the buffer's used length; the buffer is
 * `.bss work_buffer,8*10`, so ten characters.
 *
 * Two things about it worth having written down, neither of which the
 * port smooths over:
 *
 *   The leading-space scan has no bound. Its only exit is finding a
 *   character that is not a space: `cmpi 20h,a2 / jrnz #first_found`
 *   leaves on a non-space, and the `move a2,a2 / jrz` that follows can
 *   only be reached when a2 IS 20h, so it never fires. An all-spaces
 *   buffer runs the scan off the end and into whatever follows. This
 *   port stops at `length` instead of reading out of bounds, and the
 *   result for an all-spaces buffer is therefore the source's second
 *   case rather than whatever the arcade's BSS happened to hold.
 *
 *   When the trimmed range comes out empty or inverted (`cmp a0,a1 /
 *   jrgt`), it does not produce an empty string: it resets both ends
 *   and copies the whole untrimmed buffer.
 *
 * It does not write a terminator. `out` is filled for `out_max`
 * characters at most and the count is returned.
 */
size_t wm_string_strip_white(const char *work_buffer, size_t length,
                             char *out, size_t out_max);

/*
 * HSTD.ASM:1105 val_to_dec_tenths_asc -- a pin time, rendered. The
 * value is in hundredths and comes out as tenths: the integer part is
 * `value / 100` through dec_to_asc with a 1000 cap, then a ".", then
 * one decimal digit, `(value / 10) % 10`.
 *
 * That last digit is written two different ways. Nonzero, it goes
 * through dec_to_asc with a cap of 10; zero, the routine concatenates
 * the ROM string "0" directly -- because dec_to_asc suppresses leading
 * zeros and would contribute nothing at all.
 *
 * The result is left in the string state's buffer.
 */
void wm_string_val_to_dec_tenths(wm_string_state *st, uint32_t value);

/*
 * SELECT.ASM:533 work_out_match_time -- the pin-speed clock, and the
 * one place in the game that disagrees with itself about how fast
 * time runs.
 *
 * It answers two questions at once. The first is the match clock as
 * a number, described by the source's own comment above the caller:
 * "Return 100 - time elapsed in BCD, 0xIIFF format, where II is the
 * integer part and FF is the fractional. Then convert this back to
 * hex from BCD." The integer part is shuffled out of @match_time
 * (`srl 8` for the high bits, `sll 24 / srl 12` to put the low byte
 * back at bits 12-19) and the fractional part is the 16-bit word at
 * match_time+20h scaled by 100 and shifted down 16 -- a fixed-point
 * fraction turned into hundredths.
 *
 * The second is how long the round took, and this is the
 * interesting one:
 *
 *     move @round_end_time,a1
 *     move @round_start_time,a14
 *     sub  a14,a1
 *     movi (100<<8)/55,a14
 *     mpyu a14,a1
 *     srl  8,a1
 *
 * -- ticks to hundredths of a second at FIFTY-FIVE ticks per second.
 * DISPLAY.EQU:46 says TSEC equ 53, and everything else in the game
 * counts in 53s. So every pin time this routine produces is about
 * 3.6% short of the time the player actually took, and the high
 * score table has been recording it that way since 1995. The
 * constant is also truncated at assembly time -- (100<<8)/55 is
 * 465, not 465.45 -- which takes another 0.1% off.
 *
 * That is not corrected here. A pin time that does not match the
 * arcade's is a different game.
 */
#define WM_MATCH_TIME_TICK_DIVISOR 55
/* (100 << 8) / 55, as the assembler computes it. */
#define WM_MATCH_TIME_SCALE 465

/* `round_end_time - round_start_time` in hundredths, as above. A
   negative span is not something the source guards against; this
   keeps the arithmetic signed so it does not become enormous. */
int32_t wm_match_time_ticks_to_hundredths(int32_t ticks);

/*
 * The first answer: @match_time (32 bits) and the fractional word at
 * match_time+20h, folded together and run back through BCDBIN.
 */
uint32_t wm_match_time_value(uint32_t match_time, uint16_t fraction);

/*
 * SELECT.ASM:496 calc_match_time_2 -- what gets stored.
 *
 * The round's hundredths are ADDED to whatever the player's
 * MATCH_TIMERS slot already holds, so the figure accumulates across
 * the rounds of a match. Then two rejections: 50000 or more (`cmpi
 * 50000,a0 / jrge`) and negative (`move a0,a0 / jrn`) both store -1
 * instead, which is how "no pin speed to record" is spelled.
 *
 * Returns the value to store. `total` is the accumulated sum.
 */
int32_t wm_match_time_store(int32_t total);
#define WM_MATCH_TIME_LIMIT 50000
#define WM_MATCH_TIME_NONE (-1)

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_STRING_H */
