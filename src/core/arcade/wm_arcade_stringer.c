/*
 * UTIL.ASM's string engine: stringr1_1 (:1372) and STRNGLEN (:1495).
 * See wm/arcade/wm_arcade_stringer.h for what is and is not translated.
 */
#include "wm/arcade/wm_arcade_stringer.h"

#include "wm/arcade/wm_arcade_rd7font.h"

/*
 * Where the string ends.
 *
 * Both loops load the character with `movb`, which SIGN-EXTENDS, and then
 * test it: STRNGLEN's `jrgt stl10` continues only while it is positive, and
 * stringr1_1's `jrle strrx` finishes on anything at or below zero. So a byte
 * with the high bit set terminates the string -- it is not a character, and
 * it is not a space either. Only 0x01..0x7f are printable input.
 */
static bool is_end(unsigned char c) {
    return (int)(signed char)c <= 0;
}

/*
 * A character's slot, or -1 when it is a space.
 *
 * `subk 32,a1 / jrle strr1` -- at or below 32 is a space. Then `subk 1,a1`,
 * so the slot is char - 33.
 */
static int slot_of(unsigned char c, const wm_stringer_font *font) {
    int v = (int)(signed char)c;
    if (v - 32 <= 0) return -1;
    return v - font->first_char;
}

/* The glyph's width, or a negative value when it is not known. */
static int32_t glyph_width(int slot, const wm_stringer_font *font) {
    if (slot < 0) return WM_STRINGER_SPACE_WIDTH;
    if ((size_t)slot >= font->slots) return -1;
    return font->width[slot];
}

int32_t wm_stringer_length(const char *text, const wm_stringer_font *font,
                           int32_t spacing_x) {
    int32_t len = 0;
    const char *p;

    if (!text || !font || !font->width) return -1;

    /*
     * STRNGLEN's loop, in its own order: test the character first (`stl60`
     * is the loop entry, reached by `jruc stl60` before any add), then the
     * width, then the spacing. `addxy a10,a7` runs for a space too.
     */
    for (p = text; !is_end((unsigned char)*p); ++p) {
        int slot = slot_of((unsigned char)*p, font);
        int32_t w = glyph_width(slot, font);
        if (w < 0) return -1;
        len += w;
        len += spacing_x;
    }
    /* `zext a7` -- the result is the low 16 bits, unsigned. */
    return len & 0xFFFF;
}

bool wm_stringer_measurable(const char *text, const wm_stringer_font *font) {
    const char *p;
    if (!text || !font || !font->width) return false;
    for (p = text; !is_end((unsigned char)*p); ++p) {
        if (glyph_width(slot_of((unsigned char)*p, font), font) < 0) {
            return false;
        }
    }
    return true;
}

int32_t wm_stringer_place(const char *text, const wm_stringer_font *font,
                          int32_t spacing_x, wm_stringer_justify justify,
                          int32_t x, int32_t y,
                          wm_stringer_glyph *out, size_t capacity) {
    int32_t n = 0;
    const char *p;

    if (!text || !font || !font->width || !out) return -1;
    if (!wm_stringer_measurable(text, font)) return -1;

    /* The justification adjustment, `subxy a7,a9`. Left takes none: the
       source jumps straight to strr10 without touching a9. */
    if (justify != WM_STRINGER_LEFT) {
        int32_t len = wm_stringer_length(text, font, spacing_x);
        if (len < 0) return -1;
        /* `callr STRNGLEN` then, for centre only, `srl 1,a7`. */
        x -= (justify == WM_STRINGER_CENTRE) ? (len >> 1) : len;
    }

    for (p = text; !is_end((unsigned char)*p); ++p) {
        int slot = slot_of((unsigned char)*p, font);
        int32_t w = glyph_width(slot, font);

        if (w < 0) return -1;
        if ((size_t)n >= capacity) return -1;

        out[n].x = (int16_t)x;
        out[n].y = (int16_t)y;
        out[n].slot = (int16_t)slot;
        out[n].width = (int16_t)w;
        ++n;

        /*
         * A space adds five and falls into strr5; a glyph is emitted and
         * then `strrdun` adds its ISIZEX. Either way strr5 adds the spacing
         * afterwards, so the two paths differ only in which width is used.
         */
        x += w;
        x += spacing_x;
    }
    return n;
}

const wm_stringer_font *wm_stringer_rd7font(void) {
    static const wm_stringer_font f = {
        wm_rd7font_width, WM_RD7FONT_SLOTS, WM_RD7FONT_FIRST_CHAR
    };
    return &f;
}
