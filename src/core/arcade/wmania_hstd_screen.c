#include "wm/arcade/wmania_hstd_screen.h"

#include <string.h>

/*
 * ATTRACT.ASM:1443 show_hstd. See wm/arcade/wmania_hstd_screen.h for what
 * this screen is and why the port had every piece of it except a consumer.
 *
 * The two halves are the same code in the source -- SCROLL_TABLE_LOOP_*_I for
 * the intercontinental table and SCROLL_TABLE_LOOP_* for the world one, with
 * identical bodies -- so they are one machine here too, with `half` choosing
 * the table.
 */

WmHsPresentScreen wm_hstd_screen(const WmHstdState *state)
{
    return (state && state->half != 0u) ? WM_HS_PRESENT_BEATEN
                                        : WM_HS_PRESENT_INTER;
}

static uint8_t total_rows(const WmHstdState *state)
{
    return wm_hs_present_sequence[wm_hstd_screen(state)].total_rows;
}

/* One row out of wm_hs_present_rows, by rank. */
static bool row_at(const WmHsSystem *system, const WmHstdState *state,
                   uint16_t rank, WmHsDisplayRow *out)
{
    return wm_hs_present_rows(system, wm_hstd_screen(state), rank, out, 1u) == 1u;
}

static void add_row(WmHstdState *state, const WmHsSystem *system,
                    uint16_t rank, int16_t y)
{
    WmHsDisplayRow row;
    if (state->row_count >= WM_HSTD_MAX_LIVE_ROWS) return;
    if (!row_at(system, state, rank, &row)) return;

    state->rows[state->row_count].rank = rank;
    state->rows[state->row_count].y = y;
    state->rows[state->row_count].highlighted = row.highlighted;
    ++state->row_count;
    if (state->row_count > state->peak_row_count)
        state->peak_row_count = state->row_count;

    /*
     * draw_beaten_table_entry (HSTD.ASM:1563) reads the score and, when it is
     * zero, sets @not_blank. That is what ends the scroll early, and the
     * variable's name says the opposite of what it means.
     */
    if (row.score_bcd == 0u) state->blank_drawn = true;
}

/*
 * print_inter / print_beaten (HSTD.ASM:328 / :409): the title, the bar
 * objects, and the first three rows at y 42, 112 and 182 -- `movi [51-9,8]`
 * stepped by `addi [46h,0]`. starting_num is left one past the last of them.
 */
static void print_table(WmHstdState *state, const WmHsSystem *system)
{
    const WmHsPresentLayout *layout =
        &wm_hs_present_sequence[wm_hstd_screen(state)].layout;
    uint8_t rows_at_once =
        wm_hs_present_sequence[wm_hstd_screen(state)].rows_at_once;
    uint8_t i;

    state->row_count = 0u;
    state->scrolling = false;
    state->starting_num = 1;
    for (i = 0; i < rows_at_once; ++i) {
        add_row(state, system, (uint16_t)state->starting_num,
                (int16_t)(layout->first_initials_y +
                          (int16_t)i * layout->row_y_step));
        ++state->starting_num;
    }
}

/*
 * The DEC and clamp at ATTRACT.ASM:1461. print_* left starting_num one past
 * its last printed row and BACK_IN_AGAIN_I increments before drawing, so one
 * is taken back off here. The clamp -- `CMPI ENTRIES-6,A9 / JRLT / MOVI
 * ENTRIES-1,A9` -- only bites on a table small enough that printing three
 * rows already reached its end, which the shipped 31-entry tables never are.
 * It is translated because it is there.
 */
static void enter_scroll(WmHstdState *state)
{
    int entries = (int)total_rows(state) + 1;   /* ENTRIES, not ENTRIES-1 */

    --state->starting_num;
    if (state->starting_num >= entries - 6)
        state->starting_num = entries - 1;

    state->blank_drawn = false;                 /* SCROLL_TABLE_LOOP_2_I */
    state->burst_left = WM_HSTD_FIRST_BURST;
    state->step_ticks = (int)WM_HS_PRESENT_SCROLL_STEP_TICKS_A;
}

/* The per-tick effect of OYVEL, for every live row. */
static void advance_scroll(WmHstdState *state)
{
    uint8_t i;
    if (!state->scrolling) return;
    for (i = 0; i < state->row_count; ++i)
        state->rows[i].y = (int16_t)(state->rows[i].y -
                                     WM_HSTD_SCROLL_PIXELS_PER_TICK);
}

/* DELETE_ANY_OFF_TOP (HSTD.ASM:531). */
static void delete_off_top(WmHstdState *state)
{
    uint8_t i = 0u, kept = 0u;
    for (; i < state->row_count; ++i) {
        if (state->rows[i].y > WM_HSTD_DELETE_Y) {
            if (kept != i) state->rows[kept] = state->rows[i];
            ++kept;
        }
    }
    state->row_count = kept;
}

/* BACK_IN_AGAIN_I: one more row in, and the velocity on. */
static void back_in_again(WmHstdState *state, const WmHsSystem *system)
{
    ++state->starting_num;
    add_row(state, system, (uint16_t)state->starting_num, WM_HSTD_SPAWN_Y);
    state->scrolling = true;                    /* MOVE_ALL_OBJS_UP */
    state->step_ticks = (int)WM_HS_PRESENT_SCROLL_STEP_TICKS_B;
    state->phase = WM_HSTD_PHASE_WAIT_BUTS;
}

static void to_phase(WmHstdState *state, WmHstdPhase phase)
{
    state->phase = phase;
    state->phase_ticks = 0u;
}

/* JUST_WAIT_I: STOP_ALL_OBJS, then wait_on_butn 5*TSEC. */
static void to_just_wait(WmHstdState *state)
{
    state->scrolling = false;
    to_phase(state, WM_HSTD_PHASE_JUST_WAIT);
}

void wm_hstd_begin(WmHstdState *state, const WmHsSystem *system)
{
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->half = 0u;
    print_table(state, system);
    to_phase(state, WM_HSTD_PHASE_PLACE);
}

bool wm_hstd_tick(WmHstdState *state, const WmHsSystem *system, bool button)
{
    if (!state) return true;

    switch (state->phase) {
    case WM_HSTD_PHASE_PLACE:
        /* SLEEPK 1 between the printing and the wipe. */
        if (++state->phase_ticks >= 1u) to_phase(state, WM_HSTD_PHASE_WIPE);
        return false;

    case WM_HSTD_PHASE_WIPE:
        /* JSRP BLOW_0_TO_1, then RESET_FROM_PIXEL_WIPE, which costs nothing. */
        if (++state->phase_ticks >= (unsigned)WM_ATTRACT_BLOW_0_TO_1_TICKS)
            to_phase(state, WM_HSTD_PHASE_SETTLE);
        return false;

    case WM_HSTD_PHASE_SETTLE:
        /* SLEEP TSEC/2. */
        if (++state->phase_ticks >=
            (unsigned)(WM_SOURCE_TICKS_PER_SEC *
                       WM_HS_PRESENT_TRANSITION_TSEC_NUM /
                       WM_HS_PRESENT_TRANSITION_TSEC_DEN)) {
            enter_scroll(state);
            to_phase(state, WM_HSTD_PHASE_WAIT_BUTS);
        }
        return false;

    case WM_HSTD_PHASE_WAIT_BUTS:
        /* WAIT_ON_THOSE_BUTS_I: `SLEEP 1`, sample, `DSJS A10`. */
        advance_scroll(state);
        if (button) { to_just_wait(state); return false; }
        if (--state->step_ticks <= 0) {
            delete_off_top(state);
            to_phase(state, WM_HSTD_PHASE_AFTER_DELETE);
        }
        return false;

    case WM_HSTD_PHASE_AFTER_DELETE:
        /* The lone `sleep 1` after DELETE_ANY_OFF_TOP. */
        advance_scroll(state);
        /* `cmpi ENTRIES-1,a9 / JRGE JUST_WAIT_I` -- the table has run out. */
        if (state->starting_num >= (int)total_rows(state)) {
            to_just_wait(state);
            return false;
        }
        if (--state->burst_left <= 0) {
            state->burst_left = WM_HSTD_LATER_BURST;
            to_phase(state, WM_HSTD_PHASE_DELAY_SCROLL);
        } else {
            back_in_again(state, system);
        }
        return false;

    case WM_HSTD_PHASE_DELAY_SCROLL:
        /* `SLEEP 15H ;scroll last three off`, still moving. */
        advance_scroll(state);
        if (++state->phase_ticks >=
            (unsigned)WM_HS_PRESENT_SCROLL_LAST_OFF_TICKS) {
            state->scrolling = false;           /* STOP_ALL_OBJS */
            state->hold_ticks = (int)WM_HS_PRESENT_SCROLL_GROUP_HOLD_TICKS;
            to_phase(state, WM_HSTD_PHASE_MORE_WAITING);
        }
        return false;

    case WM_HSTD_PHASE_MORE_WAITING:
        /* MORE_WAITING_I: 85 ticks with the rows held still. */
        if (button) { to_just_wait(state); return false; }
        if (--state->hold_ticks <= 0) {
            /* `move @not_blank,a0 / jrnz JUST_WAIT_I`. */
            if (state->blank_drawn) to_just_wait(state);
            else back_in_again(state, system);
        }
        return false;

    case WM_HSTD_PHASE_JUST_WAIT:
        /* wait_on_butn 5*TSEC, then either the second table or the end. */
        ++state->phase_ticks;
        if (button ||
            state->phase_ticks >= (unsigned)(WM_HS_PRESENT_FINAL_HOLD_TSEC *
                                             WM_SOURCE_TICKS_PER_SEC)) {
            if (state->half == 0u) {
                state->half = 1u;
                print_table(state, system);
                to_phase(state, WM_HSTD_PHASE_PLACE);
                return false;
            }
            to_phase(state, WM_HSTD_PHASE_DONE);
            return true;
        }
        return false;

    case WM_HSTD_PHASE_DONE:
    default:
        return true;
    }
}
