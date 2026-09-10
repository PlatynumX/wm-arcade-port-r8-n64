/*
 * STRING.ASM's text engine, plus ADJUST.ASM's BINBCD.
 * See include/wm/arcade/wm_arcade_string.h for what is translated and what
 * is left behind a seam.
 */
#include "wm/arcade/wm_arcade_string.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* font tables                                                         */

const wm_font_table *wm_font_table_find(const char *name)
{
    int i;
    if (!name) {
        return NULL;
    }
    for (i = 0; i < wm_font_table_count; ++i) {
        if (strcmp(wm_font_tables[i]->name, name) == 0) {
            return wm_font_tables[i];
        }
    }
    return NULL;
}

int wm_glyph_width_from_art(const char *glyph)
{
    int lo = 0;
    int hi = wm_glyph_metric_count - 1;

    if (!glyph) {
        return -1;
    }
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int cmp = strcmp(glyph, wm_glyph_metrics[mid].name);
        if (cmp == 0) {
            return wm_glyph_metrics[mid].width;
        }
        if (cmp < 0) {
            hi = mid - 1;
        } else {
            lo = mid + 1;
        }
    }
    return -1;
}

const char *wm_font_glyph(const wm_font_table *font, int ch)
{
    /* The arcade does `andi 0ffh,a0` and then indexes a 128-entry table,
     * so 128..255 read past its end.  Refuse rather than reproduce it. */
    if (!font || ch < 0 || ch > 127) {
        return NULL;
    }
    return font->slots[ch];
}

/* ------------------------------------------------------------------ */
/* ADJUST.ASM:1593 BINBCD                                              */

uint32_t wm_bin_to_bcd(uint32_t value)
{
    uint32_t result = 0;
    unsigned shift = 0;

    /* CMPI 99999999,A0 / JRLS BBIR / MOVI 99999999H,A0 */
    if (value > 99999999u) {
        return 0x99999999u;
    }
    /* The loop is do-while, so zero still runs once and yields zero. */
    do {
        uint32_t digit = value % 10u;
        value /= 10u;
        result += digit << shift;
        shift += 4u;
    } while (value != 0u);

    return result;
}

/* ------------------------------------------------------------------ */
/* state                                                               */

void wm_string_init(wm_string_state *st)
{
    if (!st) {
        return;
    }
    memset(st, 0, sizeof(*st));
    st->justify = WM_JUSTIFY_NONE;
    st->z = WM_STRING_DEFAULT_Z;
}

void wm_string_clear_buffer(wm_string_state *st)
{
    if (st) {
        memset(st->buffer, 0, sizeof(st->buffer));
    }
}

void wm_string_clear_buffer2(wm_string_state *st)
{
    if (st) {
        memset(st->buffer2, 0, sizeof(st->buffer2));
    }
}

void wm_string_clear_buffers(wm_string_state *st)
{
    wm_string_clear_buffer(st);
    wm_string_clear_buffer2(st);
}

/* ------------------------------------------------------------------ */
/* number formatting                                                   */

void wm_bcd_to_asc(char *dst, size_t dst_len, uint32_t bcd)
{
    /* dec_to_asc_new_entry: nibbles 6 down to 0 -- seven digits, not the
     * eight BINBCD can hold.  a3 is the "something has been printed yet"
     * flag that suppresses leading zeros; the last digit ignores it. */
    int leading = 0;
    size_t out = 0;
    int nib;

    if (!dst || dst_len == 0) {
        return;
    }
    for (nib = 6; nib >= 1; --nib) {
        unsigned digit = (bcd >> (unsigned)(nib * 4)) & 0xFu;
        if (digit == 0u && !leading) {
            continue;
        }
        leading = 1;
        if (out + 1u < dst_len) {
            dst[out++] = (char)('0' + digit);
        }
    }
    if (out + 1u < dst_len) {
        dst[out++] = (char)('0' + (bcd & 0xFu));
    }
    dst[out] = '\0';
}

void wm_string_dec_to_asc(wm_string_state *st, uint32_t value, uint32_t max)
{
    if (!st) {
        return;
    }
    /* cmp a0,a1 / jrhi #not_max -- take the branch only when max > value,
     * so max == value clamps as well (to the same number). */
    if (!(max > value)) {
        value = max;
    }
    wm_bcd_to_asc(st->buffer2, sizeof(st->buffer2), wm_bin_to_bcd(value));
}

void wm_string_dec_to_pct(wm_string_state *st, uint32_t value)
{
    uint32_t bcd;
    if (!st) {
        return;
    }
    bcd = wm_bin_to_bcd(value);
    /* Three nibbles, unconditionally -- no leading-zero suppression here. */
    st->buffer2[0] = (char)('0' + ((bcd >> 8) & 0xFu));
    st->buffer2[1] = (char)('0' + ((bcd >> 4) & 0xFu));
    st->buffer2[2] = (char)('0' + (bcd & 0xFu));
    st->buffer2[3] = '\0';
}

/* ------------------------------------------------------------------ */
/* copy and concatenate                                                */

static void copy_into(char *dst, size_t dst_len, size_t at, const char *src)
{
    size_t i = 0;
    if (!src) {
        if (at < dst_len) {
            dst[at] = '\0';
        }
        return;
    }
    /* The arcade copies until it has written the NUL, with no bound at
     * all; this stops at the buffer instead of running past it. */
    while (at + i + 1u < dst_len && src[i] != '\0') {
        dst[at + i] = src[i];
        ++i;
    }
    dst[at + i] = '\0';
}

void wm_string_copy(wm_string_state *st)
{
    if (st) {
        copy_into(st->buffer, sizeof(st->buffer), 0, st->buffer2);
    }
}

void wm_string_concat(wm_string_state *st)
{
    if (st) {
        copy_into(st->buffer, sizeof(st->buffer),
                  strlen(st->buffer), st->buffer2);
    }
}

void wm_string_copy_rom(wm_string_state *st, const char *text)
{
    if (st) {
        copy_into(st->buffer, sizeof(st->buffer), 0, text);
    }
}

void wm_string_concat_rom(wm_string_state *st, const char *text)
{
    if (st) {
        copy_into(st->buffer, sizeof(st->buffer), strlen(st->buffer), text);
    }
}

/* ------------------------------------------------------------------ */
/* measurement                                                         */

static int glyph_width(const wm_string_state *st, const char *glyph)
{
    int width;

    if (!glyph) {
        return 0;
    }
    /* The artwork answers first; the seam covers only what it cannot. */
    width = wm_glyph_width_from_art(glyph);
    if (width >= 0) {
        return width;
    }
    if (!st->width_fn) {
        return 0;
    }
    return st->width_fn(st->width_ctx, glyph);
}

int wm_string_len(const wm_string_state *st, const char *text)
{
    int width = 0;
    const unsigned char *p;

    if (!st || !text) {
        return 0;
    }
    for (p = (const unsigned char *)text; ; ++p) {
        int ch = (int)*p;
        const char *glyph;

        if (ch == 0 || ch == WM_STRING_NEWLINE) {
            break;              /* one line only */
        }
        if (ch == ' ') {
            width += st->space_width;
            continue;
        }
        glyph = wm_font_glyph(st->font, ch);
        if (!glyph) {
            /* `jrz #next_char` -- no width and no spacing either. */
            continue;
        }
        if (!st->ignore_char_width) {
            width += glyph_width(st, glyph);
        }
        width += st->spacing;
    }
    /* `move @mess_spacing,a0 / sub a0,a2` -- the trailing gap comes back
     * off, so an empty line really does measure -spacing. */
    return width - st->spacing;
}

int wm_string_buffer_len(const wm_string_state *st)
{
    return st ? wm_string_len(st, st->buffer) : 0;
}

void wm_string_justify(wm_string_state *st, const char *text)
{
    if (!st) {
        return;
    }
    switch (st->justify) {
    case WM_JUSTIFY_LEFT:
        st->cursx2 = st->cursx;
        break;
    case WM_JUSTIFY_CENTER:
        /* srl 1,a2 -- the halving is a shift, so it truncates. */
        st->cursx2 = (int16_t)(st->cursx - (wm_string_len(st, text) >> 1));
        break;
    case WM_JUSTIFY_RIGHT:
        st->cursx2 = (int16_t)(st->cursx - wm_string_len(st, text));
        break;
    case WM_JUSTIFY_NONE:
    default:
        /* setup_message's `movi #rets` -- leave the cursor alone. */
        break;
    }
}

/* ------------------------------------------------------------------ */
/* the printer                                                         */

void wm_string_print(wm_string_state *st, const char *text,
                     wm_justify justify, int32_t z)
{
    const unsigned char *p;

    if (!st || !text) {
        return;
    }
    st->justify = justify;
    wm_string_justify(st, text);

    for (p = (const unsigned char *)text; ; ++p) {
        int ch = (int)*p;
        const char *glyph;

        if (ch == 0) {
            break;
        }
        if (ch == WM_STRING_NEWLINE) {
            st->cursy = (int16_t)(st->cursy + st->line_spacing);
            /* The justify hook re-runs over the *rest* of the string, so
             * every line is justified on its own width. */
            wm_string_justify(st, (const char *)(p + 1));
            continue;
        }
        if (ch == ' ') {
            st->cursx2 = (int16_t)(st->cursx2 + st->space_width);
            continue;
        }
        glyph = wm_font_glyph(st->font, ch);
        if (!glyph) {
            continue;           /* no glyph, no advance at all */
        }
        if (st->draw_fn) {
            /* BEGINOBJP: x and y are `sll 16` -- whole pixels in the high
             * half of a 16.16 word. */
            st->draw_fn(st->draw_ctx, glyph,
                        (int32_t)st->cursx2 << 16, (int32_t)st->cursy << 16,
                        z, st->objid, WM_STRING_DMA_FLAGS, st->palette);
        }
        if (!st->ignore_char_width) {
            st->cursx2 = (int16_t)(st->cursx2 + glyph_width(st, glyph));
        }
        st->cursx2 = (int16_t)(st->cursx2 + st->spacing);
    }
}

bool wm_print_method_from_name(const char *name, wm_print_method *out)
{
    static const struct { const char *name; wm_print_method method; } rows[] = {
        { "print_string",        WM_PRINT_STRING },
        { "print_string2",       WM_PRINT_STRING2 },
        { "print_string_C",      WM_PRINT_STRING_C },
        { "print_string_C2",     WM_PRINT_STRING_C2 },
        { "print_string_R",      WM_PRINT_STRING_R },
        { "print_string_R2",     WM_PRINT_STRING_R2 },
        { "print_string_Z2",     WM_PRINT_STRING_Z2 },
        { "print_string_with_z", WM_PRINT_STRING_WITH_Z },
    };
    size_t i;

    if (!name || !out) {
        return false;
    }
    if (strcmp(name, "0") == 0) {
        *out = WM_PRINT_NONE;
        return true;
    }
    for (i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        if (strcmp(rows[i].name, name) == 0) {
            *out = rows[i].method;
            return true;
        }
    }
    return false;               /* e.g. the two descriptors naming `a10` */
}

void wm_string_print_method(wm_string_state *st, wm_print_method method,
                            const char *text)
{
    if (!st) {
        return;
    }
    switch (method) {
    case WM_PRINT_STRING:
        wm_string_print(st, st->buffer, WM_JUSTIFY_LEFT, WM_STRING_DEFAULT_Z);
        break;
    case WM_PRINT_STRING2:
        wm_string_print(st, text, WM_JUSTIFY_LEFT, WM_STRING_DEFAULT_Z);
        break;
    case WM_PRINT_STRING_C:
        wm_string_print(st, st->buffer, WM_JUSTIFY_CENTER, WM_STRING_DEFAULT_Z);
        break;
    case WM_PRINT_STRING_C2:
        wm_string_print(st, text, WM_JUSTIFY_CENTER, WM_STRING_DEFAULT_Z);
        break;
    case WM_PRINT_STRING_R:
        wm_string_print(st, st->buffer, WM_JUSTIFY_RIGHT, WM_STRING_DEFAULT_Z);
        break;
    case WM_PRINT_STRING_R2:
        wm_string_print(st, text, WM_JUSTIFY_RIGHT, WM_STRING_DEFAULT_Z);
        break;
    case WM_PRINT_STRING_Z2:
        /* print_string_Z2 takes the buffer and prints it centred at mess_z. */
        wm_string_print(st, st->buffer, WM_JUSTIFY_CENTER, st->z);
        break;
    case WM_PRINT_STRING_WITH_Z:
        wm_string_print(st, st->buffer, WM_JUSTIFY_LEFT, st->z);
        break;
    case WM_PRINT_NONE:
    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/* JAM_STR descriptors                                                 */

static void load_desc(wm_string_state *st, const wm_message_desc *desc)
{
    st->objid = WM_TYPTEXT;
    st->font = wm_font_table_find(desc->font);
    st->space_width = desc->space_width;
    st->spacing = desc->spacing;
    st->cursx = desc->cursx;
    st->cursy = desc->cursy;
    st->palette = desc->palette;
}

void wm_string_setup_message(wm_string_state *st, const wm_message_desc *desc)
{
    if (!st || !desc) {
        return;
    }
    load_desc(st, desc);
    /* `movi #rets,a0 / move a0,@mess_justify` */
    st->justify = WM_JUSTIFY_NONE;
}

void wm_string_print_message(wm_string_state *st, const wm_message_desc *desc)
{
    if (!st || !desc) {
        return;
    }
    load_desc(st, desc);
    /* `move a2,a4` -- the text sits immediately after the descriptor. */
    wm_string_print_method(st, desc->method, desc->text);
}
