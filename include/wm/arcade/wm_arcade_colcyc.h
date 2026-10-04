#ifndef WM_ARCADE_COLCYC_H
#define WM_ARCADE_COLCYC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * UTIL.ASM:1174 CYCLE_TABLE, and the two HSTD.ASM routines that
 * start it.
 *
 * "Cycle a pal with a fixed ROM color table": it writes a sliding
 * window of a word table into a palette, N colours at a time, every
 * few ticks, forever. The high-score pages run two of them --
 * hscore_colcyc on BLUE with COLTAB2, hscore_colcyc2 on RUBYPAL with
 * COLTAB3 -- and both are six instructions that load the same four
 * arguments and CREATE the process, so the cycler is the routine and
 * the two are its arguments.
 *
 * The loop restarts when the word it lands on is NEGATIVE or equal
 * to the table's FIRST word:
 *
 *     ADDK  >10,A9
 *     MOVE  *A9,A0
 *     JRN   RESTUFF               ; bit 15 set -> restart
 *     MOVE  *A13(PDATA+16),A1     ; word zero, saved on entry
 *     CMP   A0,A1
 *     jrne  #loop                 ; equal -> falls into RESTUFF
 *
 * so the cycle length is the distance to the next repeat of word
 * zero, and everything past that exists only so the window can be
 * read from the last few positions without running off the end.
 * Both tables are 78 words with a period of 63 and a 15-word tail,
 * for a 7-colour window. tools/wlcolcyc.py works the period out
 * rather than assuming it, and refuses a table where word zero never
 * repeats -- because CYCLE_TABLE would then never restart.
 *
 * One more thing the loop does every pass, before writing: it
 * re-reads the palette record it found on entry and DIEs if it has
 * changed. A cycler outlives the page it was started for, and that
 * check is what stops it writing into whatever got the palette slot
 * next.
 */

typedef struct {
    /* The HSTD.ASM routine that starts this one. */
    const char *routine;
    /* The source's own table label. */
    const char *name;
    /* The palette it cycles, by the source's name for it. */
    const char *palette;
    const uint16_t *words;
    size_t word_count;
    /* Where word zero next appears -- the cycle length. */
    size_t period;
} wm_colcyc_table_t;

#define WM_COLCYC_TABLES 2
extern const wm_colcyc_table_t wm_colcyc_tables[WM_COLCYC_TABLES];

/* `movi [10,7],a8` -- colour 10, seven of them. */
#define WM_COLCYC_START_COLOR 10
#define WM_COLCYC_COLORS 7
/* `movk 3,a11` -- the rate, in ticks. */
#define WM_COLCYC_TICKS 3
/* `CYC0 SLEEP 60` -- how long it waits before looking for the
   palette again. The first look happens immediately. */
#define WM_COLCYC_RETRY_TICKS 60

typedef enum {
    /* pal_find came back empty; sleeping before trying again. */
    WM_COLCYC_WAIT_PAL = 0,
    WM_COLCYC_RUN,
    /* KILL_US: the palette record changed under it. */
    WM_COLCYC_DEAD
} wm_colcyc_phase_t;

typedef struct {
    const wm_colcyc_table_t *table;
    wm_colcyc_phase_t phase;
    int32_t timer;
    /* The window's position in the table. */
    size_t pos;
    uint16_t start_color;
    uint16_t count;
    /* PDATA+16: word zero, the value that ends a pass. */
    uint16_t stop_word;
    /* How many times the window has wrapped. */
    unsigned passes;

    /* Raised for the one tick `pal_set` is due; `src` is the window
       and `count` colours are written at `start_color`. */
    bool write;
    const uint16_t *src;
} wm_colcyc_t;

void wm_colcyc_begin(wm_colcyc_t *c, const wm_colcyc_table_t *table,
                     uint16_t start_color, uint16_t count);

/*
 * One tick. `palette_found` is what pal_find would answer; it is
 * only consulted while waiting. `palette_is_still_ours` is the
 * KILL_US guard, checked at the top of every pass. Returns true
 * while the cycler is alive.
 */
bool wm_colcyc_tick(wm_colcyc_t *c, bool palette_found,
                    bool palette_is_still_ours);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_COLCYC_H */
