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
 * FOUR slots are -1, and they are the only four. WIMP truncates an image name
 * to eight characters, so FONT7parenl, FONT7parenr, FONT7paren2l and
 * FONT7paren2r all land on `FONT7par`, and FONT7percen and FONT7period both on
 * `FONT7per`. FONTS.LOD would give the packing order that separates them and
 * is zero bytes in this tree -- the same blocker tools/wlstring.py records for
 * 46 other symbols.
 *
 * Two of the six were settled by reading the ARTWORK instead of the name. The
 * slot position already says which character a slot is; what the truncated
 * name could not say is which image is that character. TROGF7.IMG holds an
 * 11x8 figure of two rings joined by a diagonal and a 3x2 solid block: one is
 * a percent sign and the other a full stop, and nobody would order them the
 * other way. Both pixel maps are printed in src/generated/rd7font.c so the
 * identification can be checked rather than taken on trust.
 *
 * The four `FONT7par` slots are NOT settled, deliberately. They are two
 * mirrored pairs, 3x10 and 4x10; one pair is () and the other {}. At this size
 * a left parenthesis and a left brace have the same outline -- stroke at the
 * outer columns top and bottom, inner column across the middle -- and the spur
 * that distinguishes a brace in a real typeface is not present in four pixels
 * to be read. Container order was checked as a second source and carries no
 * information: across the 78 glyphs already certain, container and table order
 * disagree 1517 times.
 *
 * -1 is not zero and must never be treated as zero: a parenthesis silently 3
 * or 4 pixels wide moves every centred line that contains one. Callers fail
 * closed instead -- see wm_arcade_stringer.h.
 */
extern const int16_t wm_rd7font_width[WM_RD7FONT_SLOTS];

/*
 * The glyph's ink mask: `ink[y * w + x]` is non-zero where the glyph draws.
 *
 * A mask is the whole artwork this path needs. SYS.EQU:217 is
 * `DMACNZ .equ 8008h  ;WRITE CONSTANT ON NON-ZERO DATA`, and STRCNRMO_2 sets
 * those flags, so a glyph is stencilled in the single colour its caller
 * passes -- show_copyright's `movi [>1111,0000],a6  ;pal 0, color 17`. The
 * glyph's own palette never reaches the screen on this path.
 *
 * `ink == NULL` means the backing image could not be established, not that
 * the glyph is blank. The four `FONT7par` slots are NULL: their facing IS
 * known (a left form anchors its ends right of its middle, a right form the
 * reverse), but which of the two mirrored pairs is () and which is {} is not,
 * and assigning them on "braces are wider" would be a typographic guess
 * rather than a reading of the data.
 */
typedef struct wm_rd7font_glyph {
    int16_t width;
    int16_t height;
    const uint8_t *ink;
} wm_rd7font_glyph;

extern const wm_rd7font_glyph wm_rd7font_glyph_table[WM_RD7FONT_SLOTS];

#ifdef __cplusplus
}
#endif
#endif
