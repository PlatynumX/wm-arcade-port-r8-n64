#ifndef WM_ARCADE_RD7FONT_H
#define WM_ARCADE_RD7FONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * RD7FONT (TEXT.ASM:52), the font the copyright and AAMA screens print with.
 *
 * 93 slots, and slot i is character i + 33. That is UTIL.ASM's own indexing:
 * stringr1_1 does `subk 32,a1`, treats <= 0 as a space, then `subk 1,a1` and
 * `sll 5,a1` for the long stride before adding the table base -- so the first
 * entry is character 33, and RD7FONT's first entry is FONT7excla, '!'.
 */
#define WM_RD7FONT_SLOTS 93
#define WM_RD7FONT_FIRST_CHAR 33

/*
 * Pixel width per slot, or -1 where it is NOT KNOWN.
 *
 * Width is the only number the engine reads out of a glyph: STRNGLEN's
 * `move *a14,a14  ;Get ISIZEX` and stringr1_1's `move *a1,a1` after the
 * header deref.
 *
 * Six slots are -1. WIMP truncates an image name to eight characters, so
 * FONT7parenl, FONT7parenr, FONT7paren2l and FONT7paren2r all land on
 * `FONT7par`, and FONT7percen and FONT7period both on `FONT7per`. TROGF7.IMG
 * holds four `FONT7par` images as two adjacent same-width pairs (3x10 and
 * 4x10), so one pair is the parens and the other the paren2 variants, and
 * nothing in the container says which. FONTS.LOD would give the packing
 * order; it is zero bytes in this tree, which is the same blocker
 * tools/wlstring.py records for 46 other symbols.
 *
 * -1 is not zero and must never be treated as zero: a parenthesis or a full
 * stop silently 3 or 11 pixels wide moves every centred line that contains
 * one. Callers fail closed instead -- see wm_arcade_stringer.h.
 */
extern const int16_t wm_rd7font_width[WM_RD7FONT_SLOTS];

#ifdef __cplusplus
}
#endif
#endif
