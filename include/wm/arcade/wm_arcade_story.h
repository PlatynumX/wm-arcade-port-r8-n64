#ifndef WM_ARCADE_STORY_H
#define WM_ARCADE_STORY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * AWARD.ASM:3056 decompress_string, and the text it unpacks.
 *
 * Eleven instructions, and every piece of long text in the game goes
 * through them:
 *
 *     setf 6,0,0          ; field size = SIX BITS
 *   #loop
 *     move *a0+,a2        ; one 6-bit field, bit pointer += 6
 *     jrz  #done          ; a zero field ends the string
 *     addi 01fh,a2        ; every other value is the character
 *     movb a2,*a1         ;   minus 1Fh
 *     addk 8,a1
 *     jruc #loop
 *
 * So the alphabet is 0x20-0x5F -- space through underscore, which is
 * uppercase ASCII and punctuation and nothing else -- packed four
 * characters into every three bytes, least significant bits first.
 * That is the whole compressor, and it buys about a quarter.
 *
 * What it unpacks had never been read in this port: the two Mortal
 * Kombat 3 tips WrestleMania hides behind a win streak, and the
 * eight wrestlers' ending stories -- 245 lines across BAMST.H and
 * its seven siblings, none of it legible in the source.
 *
 * Every blob carries its own plaintext in a comment beside it, left
 * there by whatever tool Midway compressed them with, and
 * tools/wlstring.py checks each decode against that comment rather
 * than trusting this reading of the routine. 245 known answers.
 */

/*
 * One string. `out` takes at most `cap` bytes including the
 * terminator; the source writes into a fixed `message_buffer` and
 * does not bound itself, so the cap is this port's and is reported
 * rather than silently truncating into a lie.
 *
 * Returns the number of characters written, or (size_t)-1 if the
 * string did not fit.
 */
size_t wm_story_decompress(const uint8_t *packed, size_t packed_bytes,
                           char *out, size_t cap);

/* ---- what it unpacks -------------------------------------------- */

typedef struct {
    const char *const *lines;
    size_t line_count;
} wm_story_t;

typedef struct {
    /* The source's own table name, so a row traces back. */
    const char *name;
    const wm_story_t *stories;
    size_t story_count;
} wm_story_set_t;

/*
 * STORIES.ASM:73 #wrestler_stories_table, nine longs in WRESTLERNUM
 * order -- and slot 7, Adam Bomb, is given Doink's stories, the same
 * hand-me-down the animation dispatch tables make for him.
 */
#define WM_STORY_SLOTS 9
extern const wm_story_set_t wm_wrestler_stories[WM_STORY_SLOTS];

#define WM_MK3_TIP_LINES 3
#define WM_MK3_CODE_ICONS 6
extern const char *const wm_mk3_tip_lines[2][WM_MK3_TIP_LINES];
/* The icon sequence each tip tells you to enter, as the source's own
   image names -- `#mk_code1_1` to `_6` and `#mk_code2_1` to `_6`. */
extern const char *const wm_mk3_code_icons[2][WM_MK3_CODE_ICONS];

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_STORY_H */
