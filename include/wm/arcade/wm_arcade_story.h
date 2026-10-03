#ifndef WM_ARCADE_STORY_H
#define WM_ARCADE_STORY_H

#include <stdbool.h>
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


/* =================================================================
 * STORIES.ASM:95 `print_story` and :168 `show_wrestler_end_story`
 *
 * The 245 story lines were unpacked a while back and then sat there
 * with nothing to show them. These two routines are the part that
 * decides WHEN each line is on screen and for how long, and they are
 * almost entirely arithmetic on a cursor and two countdowns, so they
 * translate exactly.
 *
 * The one thing worth flagging before the constants: print_story's
 * page loop tests the string table's terminator at the TOP, and
 * `jrz #print_story_done` jumps past BOTH waits. So a page that ends
 * early -- the last page of any story whose line count is not a
 * multiple of twelve -- is displayed for TSEC/2 and no longer. That
 * reads like a bug until you look at the caller, which runs its own
 * fifteen-second button wait immediately after JSRP print_story. The
 * short page is held by that wait instead. A full final page gets
 * both, so the last page of a 24-line story is held far longer than
 * the last page of a 21-line one. That is what the source does.
 * ================================================================= */

/* `movi 90,a3` and `addi 12,a3` / `cmpi 230,a3 / jrlt`. */
#define WM_STORY_FIRST_Y 90
#define WM_STORY_LINE_STEP 12
#define WM_STORY_Y_LIMIT 230
/* 90,102,...,222 -- the twelfth line puts the cursor at 234. */
#define WM_STORY_LINES_PER_PAGE 12

/* DISPLAY.EQU:46 TSEC equ 53. */
#define WM_STORY_TSEC 53
/* `SLEEP TSEC*5` -- the minimum a full page is up, buttons ignored. */
#define WM_STORY_MIN_TICKS (WM_STORY_TSEC * 5)
/*
 * `movi TSEC*15,a9` -- and the comment above it still says "Allow up
 * to 1 minute per pg", left over from the TSEC*60 line commented out
 * directly above. Fifteen is what ships.
 */
#define WM_STORY_WAIT_TICKS (WM_STORY_TSEC * 15)
/* `SLEEPK TSEC/2` on the way out. */
#define WM_STORY_TRAIL_TICKS (WM_STORY_TSEC / 2)
/* `SLEEPK 30` while fade_down runs. */
#define WM_STORY_FADE_TICKS 30

typedef enum {
    /* Lay a page out. Costs no tick: the source has no SLEEP here. */
    WM_STORY_PHASE_PRINT = 0,
    WM_STORY_PHASE_MIN_SLEEP,
    WM_STORY_PHASE_WAIT,
    WM_STORY_PHASE_TRAIL,
    WM_STORY_PHASE_DONE
} wm_story_phase_t;

typedef struct {
    const char *const *lines;
    size_t line_count;

    /* `move *a1+,a0,L` -- the table cursor. */
    size_t next;
    /* The page currently on screen. */
    size_t page_first;
    size_t page_lines;
    int16_t page_y[WM_STORY_LINES_PER_PAGE];

    wm_story_phase_t phase;
    int32_t timer;
    unsigned pages_shown;

    /*
     * Set for the one tick on which `obj_del1c TYPWCCOUNT` is due --
     * the caller clears the printed lines and the next page is laid
     * out in the same frame, exactly as `jruc #do_more_lines` does.
     */
    bool erase_lines;
    /* The page ended on the terminator rather than on line twelve. */
    bool page_ended_short;
} wm_story_player_t;

void wm_story_play_begin(wm_story_player_t *p, const wm_story_t *story);

/*
 * One tick. `buttons` is what the source's two `get_but_val_cur`
 * calls come back with ORed together -- any button on either player,
 * non-zero meaning pressed. Returns true while the story is still
 * running, false once it has finished.
 */
bool wm_story_play_tick(wm_story_player_t *p, unsigned buttons);

/* ---- show_wrestler_end_story ------------------------------------ */

/*
 * `move @PSTATUS,a14 / srl 1,a14 / sll 4,a14 / addi which_player,a14`
 * -- which_player is one 32-bit cell holding two 16-bit wrestler
 * numbers (SELECT.ASM:89 and :2261 write it that way), so this is
 * which_player[PSTATUS >> 1]. A two-player PSTATUS of 3 therefore
 * reads player TWO's wrestler; the routine only ever runs when one
 * player is left, so that never comes up in a game, but it is what
 * the arithmetic says and it is not clamped.
 */
int wm_story_winning_wrestler(int32_t pstatus, const int16_t which_player[2]);

/*
 * `cmpi 7,a8 / jrz inval_wnum` -- slot 7 returns immediately without
 * fading, creating anything, or showing a story. It is the slot whose
 * stories table is Doink's hand-me-down and whose logo entry in
 * `w_logos` is a literal 0, so there was nothing to show anyway.
 */
#define WM_STORY_INVALID_WRESTLER 7

typedef enum {
    WM_STORY_END_FADE = 0,
    WM_STORY_END_STORY,
    WM_STORY_END_WAIT,
    WM_STORY_END_DONE
} wm_story_end_phase_t;

typedef struct {
    wm_story_end_phase_t phase;
    int32_t timer;
    int wrestler;
    wm_story_player_t player;

    /* Set for the one tick the mugshot, logo and belt are created. */
    bool build_scene;
    /* Set for the one tick `obj_del1c CLSMK3` + TYPWCCOUNT is due. */
    bool tear_down;
    /* Passed straight through from the story player. */
    bool erase_lines;
} wm_story_end_t;

/*
 * Returns false -- with phase already WM_STORY_END_DONE -- for the
 * invalid wrestler, so a caller that just loops on the tick does the
 * right nothing.
 */
bool wm_story_end_begin(wm_story_end_t *e, int32_t pstatus,
                        const int16_t which_player[2]);
bool wm_story_end_tick(wm_story_end_t *e, unsigned buttons);

/* ---- the scene it builds ---------------------------------------- */

/* GAME.EQU:211 CLSMK3, GAME.EQU:235 TYPWCCOUNT -- the two object
   handles this sequence deletes on the way out. */
#define WM_STORY_CLSMK3 0x6000
#define WM_STORY_TYPWCCOUNT 0x0150
/* SYS.EQU:215 DMAWNZ, DISPLAY.EQU:106 M_FLIPH. */
#define WM_STORY_DMAWNZ 0x8002
#define WM_STORY_M_FLIPH 0x0010

/* `movi [017ah,0],a0 / movi [0Afh,0],a1` -- every piece of the
   mugshot goes to the same spot, and every piece is flipped. */
#define WM_STORY_MUG_X 0x17a
#define WM_STORY_MUG_Y 0x0af
/* `movi [316,0],a0 / movi [198,0],a1` -- the WWF belt (SWWFBLT). */
#define WM_STORY_BELT_X 316
#define WM_STORY_BELT_Y 198

/*
 * The logo is CENTRED on (120,50): half its width and height are
 * negated and the centre added. `srl 1` before `sll 16` means the
 * halving is done in integer pixels and the odd bit is lost, which
 * this reproduces rather than halving in the fixed-point domain.
 */
void wm_story_logo_xy(int32_t isizex, int32_t isizey,
                      int32_t *x, int32_t *y);

/*
 * STORIES.ASM:157 `w_logos`, in WRESTLERNUM order. Slot 7 is a
 * literal 0 and is given here as NULL.
 */
extern const char *const wm_story_logos[WM_STORY_SLOTS];

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_STORY_H */
