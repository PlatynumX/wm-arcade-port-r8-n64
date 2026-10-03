#ifndef WMANIA_HSTD_SCREEN_H
#define WMANIA_HSTD_SCREEN_H

#include "wm/arcade/wmania_attract_data.h"   /* WM_ATTRACT_BLOW_0_TO_1_TICKS */
#include "wm/arcade/wmania_hiscore_present.h"
#include "wm/arcade/wmania_hiscore_system.h"
#include "wm/source_clock.h"                  /* WM_SOURCE_TICKS_PER_SEC */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ATTRACT.ASM:1443 show_hstd -- the attract high-score tables.
 *
 * WHY THIS EXISTS AT ALL. Everything this screen needs was already here and
 * joined to nothing. wmania_hiscore_present.h carries the row selection, the
 * two titles, the row positions, the highlight flag and every one of the
 * source's scroll timings (WM_HS_PRESENT_SCROLL_STEP_TICKS_A/_B,
 * _SCROLL_GROUP_HOLD_TICKS, _SCROLL_LAST_OFF_TICKS, _FINAL_HOLD_TSEC), all
 * correct -- and wm_hs_present_rows() was called by exactly one test and by
 * no screen, because show_hstd's port status was not-started and the app's
 * skip_untranslated_calls walked straight past it. This module is the
 * consumer that was missing.
 *
 * WHAT THE ROUTINE IS. Two tables in one call, run by the same machine:
 * INTERCONTINENTAL CHAMPS first, then WORLD CHAMPIONS. Each half prints a
 * title and the first three rows, runs BLOW_0_TO_1, waits half a second, and
 * then SCROLLS the rest of the table up past them in bursts, ending on a
 * five-second skippable hold.
 *
 * THE SCROLL IS A VELOCITY, NOT A JUMP. MOVE_ALL_OBJS_UP (HSTD.ASM:512) sets
 * OYVEL to [-2,0] on every object whose palette is not RUBYPAL and whose
 * OZPOS is at or below 1798h; STOP_ALL_OBJS (:501) clears it; and
 * DELETE_ANY_OFF_TOP (:531) deletes anything whose OYVAL has reached -30. So
 * the rows glide two pixels a tick and a new one is spawned below the screen
 * every 35 ticks, which is exactly the 70-pixel row step -- the port models
 * the same way round rather than moving rows a row at a time.
 *
 * This module owns no renderer. It says which rows are live and where they
 * are; drawing them is the platform's job, as it is for every other screen.
 *
 * A ROW IS INITIALS PLUS ICONS, NOT INITIALS PLUS A SCORE. For these two
 * tables the stored "score" is a bitmask of defeated wrestlers:
 * draw_beaten_table_entry walks its bits from `next_icon` (HSTD.ASM:1574) and draws one
 * which_crouton icon per set bit, 48 pixels apart, then OSGEMD_DOT for each
 * of the eight that is clear. WmHsDisplayRow already carries that as
 * defeated_count and defeated_wrestler_bits. The icons are objects and this
 * port has no pipeline for them, so a row comes out as initials alone.
 */

/*
 * The source's own scroll mechanics, which wmania_hiscore_present.h did not
 * carry because nothing was scrolling.
 */
/* MOVE_ALL_OBJS_UP's `MOVI [-2,0],A1` into OYVEL (HSTD.ASM:516). */
#define WM_HSTD_SCROLL_PIXELS_PER_TICK 2
/* DELETE_ANY_OFF_TOP's `MOVI [-30,0],A3` compared against OYVAL (:533). */
#define WM_HSTD_DELETE_Y (-30)
/*
 * draw_each_inter_table_entry (:1503) spawns the new row at `movi [126H,8]`
 * with a score position of `movi [13BH,10]` that nothing uses (see below).
 * 294 is below a 256-high screen, so the row glides up into view.
 */
#define WM_HSTD_SPAWN_Y 0x126

/*
 * THE INITIALS DO NOT PRINT AT THE X THE CALLER PASSES. a9 is packed [Y,X] --
 * HSTD.ASM:1649 does `move a9,a0 / sll 16,a0 / srl 16,a0 / addi 50,a0 /
 * move a0,@mess_cursx` and :1654 `move a9,a0 / srl 16,a0 / move a0,@mess_cursy`
 * -- so cursy is the high half as the port's layout already said, but cursx is
 * the low half PLUS 50. The printed rows' x 8 therefore lands at 58.
 */
#define WM_HSTD_INITIALS_X_BIAS 50

/*
 * AND THE SCORE POSITION IS DEAD ON THIS PATH. Every caller computes one --
 * [63,10] for the printed rows, [13BH,10] for the spawned ones -- and
 * draw_beaten_table_entry restores it with `pull a9,a10` at HSTD.ASM:1624 and
 * then does not mention a10 again in its remaining 111 lines. What a row
 * actually shows is the initials plus a row of defeated-wrestler icons, drawn
 * off a9; there is no score text at all.
 *
 * This is scoped to draw_beaten_table_entry, which is the only drawer
 * show_hstd's two tables use. draw_tag_table_entry (:885),
 * draw_pinspeed_table_entry (:1149) and draw_winstreak_table_entry (:1324) all
 * do use a10, so WmHsPresentLayout.first_score_y/_x are live for those three
 * screens and dead for these two. Nothing here draws at them.
 */
#define WM_HSTD_SCORE_POSITION_IS_DEAD 1
/*
 * A11. SCROLL_TABLE_LOOP_2_I enters with 4 and DELAY_SCROLL_I restarts with
 * 3, but the first burst spawns three rows either way: the 4 is consumed by
 * the DEC that happens before the first spawn, whereas DELAY_SCROLL_I falls
 * through to BACK_IN_AGAIN_I and spawns without a DEC first.
 */
#define WM_HSTD_FIRST_BURST 4
#define WM_HSTD_LATER_BURST 3
/*
 * The gap between the last PRINTED row and the first SCROLLED-IN one is 112
 * pixels, not the 70 every other gap is. print_inter leaves its three rows at
 * y 42, 112 and 182 and the scroll's first spawn lands at 294 before any
 * velocity has been set, so that one gap is wider. It is the source's and is
 * not corrected here.
 */
#define WM_HSTD_FIRST_SPAWN_GAP \
    (WM_HSTD_SPAWN_Y - (42 + 2 * 70))

/* How many rows can be on screen at once: 294 + 30 of travel over a 70-pixel
   step, plus the one being spawned. Asserted, not assumed. */
#define WM_HSTD_MAX_LIVE_ROWS 8u

typedef struct {
    uint16_t rank;        /* 1-based table row, as wm_hs_present_rows wants */
    int16_t y;            /* arcade y; the x comes from the layout plus
                             WM_HSTD_INITIALS_X_BIAS */
    bool highlighted;     /* the GOLD/BLUE pick; see wm_hstd_tick */
} WmHstdRow;

/*
 * One state per source label, so the tick counts come out of the control
 * flow rather than out of arithmetic done by hand.
 */
typedef enum {
    WM_HSTD_PHASE_PLACE = 0,     /* print_inter/print_beaten, then SLEEPK 1 */
    WM_HSTD_PHASE_WIPE,          /* JSRP BLOW_0_TO_1 */
    WM_HSTD_PHASE_SETTLE,        /* SLEEP TSEC/2 */
    WM_HSTD_PHASE_WAIT_BUTS,     /* WAIT_ON_THOSE_BUTS_I, A10 ticks */
    WM_HSTD_PHASE_AFTER_DELETE,  /* the `sleep 1` after DELETE_ANY_OFF_TOP */
    WM_HSTD_PHASE_DELAY_SCROLL,  /* DELAY_SCROLL_I's SLEEP 15H */
    WM_HSTD_PHASE_MORE_WAITING,  /* MORE_WAITING_I, A8 ticks, objects stopped */
    WM_HSTD_PHASE_JUST_WAIT,     /* JUST_WAIT_I's wait_on_butn 5*TSEC */
    WM_HSTD_PHASE_DONE
} WmHstdPhase;

typedef struct {
    WmHstdPhase phase;
    uint8_t half;            /* 0 = the INTER table, 1 = the BEATEN table */
    unsigned phase_ticks;

    /* The source's registers and .bss, kept under their own names. */
    int step_ticks;          /* A10 */
    int burst_left;          /* A11 */
    int hold_ticks;          /* A8 */
    int starting_num;        /* @starting_num */
    /*
     * @not_blank, and the name is the source's and reads backwards:
     * draw_beaten_table_entry (HSTD.ASM:1563) sets it when the score it just
     * read was ZERO, so a set flag means a BLANK row has been drawn. It is
     * what stops the scroll early, and on a factory-fresh table it fires,
     * because only ten of the thirty rows carry a score.
     */
    bool blank_drawn;
    bool scrolling;          /* whether OYVEL is currently set */

    WmHstdRow rows[WM_HSTD_MAX_LIVE_ROWS];
    uint8_t row_count;
    /* Highest live row count this call reached, so a test can show the
       WM_HSTD_MAX_LIVE_ROWS bound is real and not just generous. */
    uint8_t peak_row_count;
} WmHstdState;

/* Which of the two tables the state is showing. */
WmHsPresentScreen wm_hstd_screen(const WmHstdState *state);

/* Enter show_hstd at its first half. */
void wm_hstd_begin(WmHstdState *state, const WmHsSystem *system);

/*
 * One source tick. Returns true when the whole call is over -- both halves
 * done, or a button pressed during the second half's final hold.
 */
bool wm_hstd_tick(WmHstdState *state, const WmHsSystem *system, bool button);

#ifdef __cplusplus
}
#endif
#endif
