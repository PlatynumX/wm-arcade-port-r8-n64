#ifndef WM_ARCADE_STRINGER_H
#define WM_ARCADE_STRINGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * UTIL.ASM's string engine -- stringr1_1 (:1372) and STRNGLEN (:1495).
 *
 * This is a SECOND text engine, not STRING.ASM's. STRING.ASM's print_string2b
 * walks a message buffer with in-band control codes; this one walks a plain
 * C string and emits one object or one DMA blit per character. The attract
 * screens use this one: show_copyright's nine `JSRP STRCNRMO_2` calls and
 * aama_message's go through it, so nothing the port had translated of
 * STRING.ASM could draw them.
 *
 * WHAT IS TRANSLATED HERE is the part that decides WHERE each glyph goes:
 * the justification, the character walk, the space rule, the per-character
 * spacing and the glyph indexing. That is the whole geometry of a line.
 *
 * WHAT IS NOT is the emission. stringr1_1 branches on bit 16 of its flag
 * word: set means GETOBJ/INSOBJ, one object per character with OIMG, OSAG,
 * OFLAGS, OPAL, OSCALE, OCTRL, OID and OZPOS filled; clear means a direct
 * QDMAN blit. Both are the object/DMA renderer this port does not have,
 * the same seam as screen_flash and pal_getf. So these functions answer
 * "what should be drawn and where", and a renderer decides how.
 */

/*
 * The justification, out of the flag word's low half. stringr1_1 reads it as
 * `movx a14,a7 / subk 1,a7`, then branches on the sign: negative is left,
 * zero is centre, positive is right. So the low half is 0, 1 or 2 --
 * STRLNRMO_1 passes >10000 (left) and STRCNRMO_2 passes >10001 (centre).
 */
typedef enum {
    WM_STRINGER_LEFT = 0,    /* flag low half 0: `jrn strr10`, no adjustment */
    WM_STRINGER_CENTRE = 1,  /* flag low half 1: start X -= STRNGLEN/2 */
    WM_STRINGER_RIGHT = 2    /* flag low half 2: start X -= STRNGLEN */
} wm_stringer_justify;

/* A space is not looked up. `subk 32,a1 / jrle strr1` sends every character
   at or below 32 to `addk 5,a9`, a hard-coded five pixels -- the source's own
   comment says "Hard code a space". STRNGLEN does the same with `addk 5,a7`. */
#define WM_STRINGER_SPACE_WIDTH 5

typedef struct {
    /* Glyph widths by slot, slot 0 being `first_char`. A negative width
       means the width is unknown, and the engine refuses rather than
       treating it as zero. */
    const int16_t *width;
    size_t slots;
    int first_char;
} wm_stringer_font;

/*
 * STRNGLEN (UTIL.ASM:1495): the length of a string in pixels.
 *
 * Per character: a space adds WM_STRINGER_SPACE_WIDTH, any other adds its
 * glyph width, and EVERY character -- space or not -- then adds spacing_x.
 * That trailing add happens for the last character too, so a one-character
 * string measures width + spacing, exactly as the source leaves it.
 *
 * Returns the length, or -1 if any character's width is unknown or has no
 * slot. -1 is the honest answer: a caller must not centre a line it cannot
 * measure.
 */
int32_t wm_stringer_length(const char *text, const wm_stringer_font *font,
                           int32_t spacing_x);

/* True when every character of `text` can be measured with `font`. */
bool wm_stringer_measurable(const char *text, const wm_stringer_font *font);

typedef struct {
    int16_t x;
    int16_t y;
    int16_t slot;        /* index into the font table; -1 for a space */
    int16_t width;       /* the glyph's width; WM_STRINGER_SPACE_WIDTH for a space */
} wm_stringer_glyph;

/*
 * stringr1_1's placement walk.
 *
 * `x`/`y` are the position the caller passes in a9 -- for a centred line that
 * is the CENTRE, which is why show_copyright passes x=200 for a 400-wide
 * screen. Justification adjusts the start with `subxy a7,a9` before the walk.
 *
 * Fills `out` with one entry per character, spaces included, in source order,
 * and returns the count. Returns -1 without touching `out` when the string
 * cannot be measured and the justification needs a length (centre or right),
 * or when any glyph on the way is unknown.
 *
 * Note spacing is applied AFTER each glyph (`strr5: addxy a10,a9`), and a
 * space adds five pixels and then the spacing as well.
 */
int32_t wm_stringer_place(const char *text, const wm_stringer_font *font,
                          int32_t spacing_x, wm_stringer_justify justify,
                          int32_t x, int32_t y,
                          wm_stringer_glyph *out, size_t capacity);

/* RD7FONT as a wm_stringer_font. */
const wm_stringer_font *wm_stringer_rd7font(void);

#ifdef __cplusplus
}
#endif
#endif
