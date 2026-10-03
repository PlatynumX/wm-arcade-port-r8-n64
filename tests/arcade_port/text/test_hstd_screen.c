/*
 * ATTRACT.ASM:1443 show_hstd -- the attract high-score tables.
 *
 * This file exists because of what was already here. wmania_hiscore_present.h
 * carried the row selection, both titles, the row positions, the highlight
 * flag and every one of the source's scroll timings, all correct, and
 * wm_hs_present_rows() was called by exactly one test and by no screen: the
 * routine's port status was not-started, so skip_untranslated_calls walked
 * straight past it. The first test below is the one that was missing.
 *
 * The rest guard the scroll, which is the part that can go subtly wrong: it
 * is a VELOCITY (two pixels a tick) with a row spawned every 35 ticks, not a
 * row-at-a-time jump, and it ends on whichever comes first of running out of
 * table and drawing a blank row.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/app.h"
#include "wm/attract.h"
#include "wm/arcade/wm_arcade_stringer.h"
#include "wm/arcade/wmania_hstd_screen.h"

/* ---- the join ------------------------------------------------------ */

static void test_show_hstd_is_reachable_at_all(void)
{
    static wm_app app;
    assert(wm_attract_call_is_translated(WM_ATTRACT_SHOW_HSTD));
    /* It is the first call of the attract loop, so a cold boot shows it. */
    wm_app_init(&app);
    assert(app.attract.call == WM_ATTRACT_SHOW_HSTD);
    assert(wm_source_attract_loop[0] == WM_ATTRACT_SHOW_HSTD);
}

/*
 * The assertion the whole change is about: real table entries reach the
 * screen through wm_hs_present_rows, which before this had no caller outside
 * one test.
 */
static void test_the_presentation_layer_now_has_a_consumer(void)
{
    static wm_app app;
    wm_input_state in;
    unsigned t, with_rows = 0;
    bool saw_mark = false;

    memset(&in, 0, sizeof in);
    wm_app_init(&app);

    for (t = 0; t < 4000u; ++t) {
        wm_app_tick(&app, &in);
        if (app.attract.call != WM_ATTRACT_SHOW_HSTD) continue;
        if (app.attract.hstd.row_count > 0u) ++with_rows;
        {
            WmHsDisplayRow row;
            size_t n = wm_hs_present_rows(
                &app.hiscore, wm_hstd_screen(&app.attract.hstd),
                app.attract.hstd.rows[0].rank, &row, 1u);
            if (n == 1u && memcmp(row.initials, "MARK", 4u) == 0)
                saw_mark = true;
        }
    }
    /* Rows are live for essentially the whole screen. */
    assert(with_rows > 1500u);
    /* And they are the factory table's own entries, not placeholders:
       inter_factory rank 1 is M,A,R,K,space. */
    assert(saw_mark);
}

/* ---- the two halves ------------------------------------------------ */

static void test_both_tables_run_in_the_sources_order(void)
{
    static WmHsSystem sys;
    WmHstdState st;
    unsigned t;
    bool saw_inter = false, saw_beaten = false;

    wm_hs_system_init(&sys, WM_HS_ADJUSTED_RESET_DEFAULT);
    (void)wm_hs_system_table_cmos_check(&sys);
    wm_hstd_begin(&st, &sys);

    assert(wm_hstd_screen(&st) == WM_HS_PRESENT_INTER);
    for (t = 0; t < 10000u; ++t) {
        if (wm_hstd_screen(&st) == WM_HS_PRESENT_INTER) {
            saw_inter = true;
            assert(!saw_beaten);     /* inter first, and never again after */
        } else {
            saw_beaten = true;
        }
        if (wm_hstd_tick(&st, &sys, false)) break;
    }
    assert(saw_inter && saw_beaten);

    /* The titles, as literals, out of #hscore_mes2 and #hscore_mes. */
    assert(strcmp(wm_hs_present_sequence[WM_HS_PRESENT_INTER].title,
                  "INTERCONTINENTAL CHAMPS") == 0);
    assert(strcmp(wm_hs_present_sequence[WM_HS_PRESENT_BEATEN].title,
                  "WORLD CHAMPIONS") == 0);
}

/*
 * End to end, against the source's own sleeps. The literals matter: every
 * total here is derived from those constants, so asserting only the total
 * would pass with each of them wrong.
 */
static void test_the_screen_holds_for_its_source_time(void)
{
    static WmHsSystem sys;
    WmHstdState st;
    unsigned t, half0 = 0;
    uint8_t half = 0u;

    assert(WM_SOURCE_TICKS_PER_SEC == 53u);
    assert(WM_ATTRACT_BLOW_0_TO_1_TICKS == 89);
    assert(WM_HS_PRESENT_SCROLL_STEP_TICKS_A == 36u);
    assert(WM_HS_PRESENT_SCROLL_STEP_TICKS_B == 34u);
    assert(WM_HS_PRESENT_SCROLL_GROUP_HOLD_TICKS == 85u);
    assert(WM_HS_PRESENT_SCROLL_LAST_OFF_TICKS == 0x15u);
    assert(WM_HS_PRESENT_TRANSITION_TSEC_NUM == 1u);
    assert(WM_HS_PRESENT_TRANSITION_TSEC_DEN == 2u);
    assert(WM_HS_PRESENT_FINAL_HOLD_TSEC == 5u);

    wm_hs_system_init(&sys, WM_HS_ADJUSTED_RESET_DEFAULT);
    (void)wm_hs_system_table_cmos_check(&sys);
    wm_hstd_begin(&st, &sys);
    for (t = 1; t <= 10000u; ++t) {
        bool done = wm_hstd_tick(&st, &sys, false);
        if (st.half != half) { half0 = t; half = st.half; }
        if (done) break;
    }
    /* Measured on the factory tables, where the blank row ends each scroll. */
    assert(half0 == 1051u);
    assert(t == 1891u);
}

/* ---- the scroll ---------------------------------------------------- */

/* MOVE_ALL_OBJS_UP sets OYVEL to [-2,0]; every live row moves by exactly
   that, every tick the velocity is on, and by nothing when it is off. */
static void test_the_scroll_is_a_velocity_of_two_pixels_a_tick(void)
{
    static WmHsSystem sys;
    WmHstdState st;
    unsigned t, moved = 0, held = 0;

    assert(WM_HSTD_SCROLL_PIXELS_PER_TICK == 2);

    wm_hs_system_init(&sys, WM_HS_ADJUSTED_RESET_DEFAULT);
    (void)wm_hs_system_table_cmos_check(&sys);
    wm_hstd_begin(&st, &sys);

    for (t = 0; t < 10000u; ++t) {
        WmHstdRow before[WM_HSTD_MAX_LIVE_ROWS];
        uint8_t n = st.row_count, i;
        bool scrolling = st.scrolling;
        memcpy(before, st.rows, sizeof before);

        if (wm_hstd_tick(&st, &sys, false)) break;

        for (i = 0; i < n && i < st.row_count; ++i) {
            if (before[i].rank != st.rows[i].rank) continue;
            if (scrolling) {
                assert(st.rows[i].y - before[i].y == -2);
                ++moved;
            } else {
                assert(st.rows[i].y == before[i].y);
                ++held;
            }
        }
    }
    /* Both states are genuinely exercised. */
    assert(moved > 2000u);
    assert(held > 500u);
}

/*
 * One row every 35 ticks at 2px a tick is a 70-pixel step, which is the
 * layout's row_y_step. The gap between the last PRINTED row and the first
 * SCROLLED-IN one is 112 instead, because the first spawn lands at 294 before
 * any velocity has been set. Both are the source's.
 */
static void test_the_row_spacing_is_the_sources(void)
{
    static WmHsSystem sys;
    WmHstdState st;
    unsigned t;
    int gap_3_to_4 = 0, gap_4_to_5 = 0, gap_5_to_6 = 0;

    assert(wm_hs_present_sequence[WM_HS_PRESENT_INTER].layout.row_y_step == 70);
    assert(WM_HSTD_SPAWN_Y == 0x126);
    assert(WM_HSTD_FIRST_SPAWN_GAP == 112);

    wm_hs_system_init(&sys, WM_HS_ADJUSTED_RESET_DEFAULT);
    (void)wm_hs_system_table_cmos_check(&sys);
    wm_hstd_begin(&st, &sys);

    /* The three printed rows, before anything scrolls. */
    assert(st.row_count == 3u);
    assert(st.rows[0].rank == 1u && st.rows[0].y == 42);
    assert(st.rows[1].rank == 2u && st.rows[1].y == 112);
    assert(st.rows[2].rank == 3u && st.rows[2].y == 182);

    for (t = 0; t < 10000u; ++t) {
        uint8_t i, j;
        if (wm_hstd_tick(&st, &sys, false)) break;
        for (i = 0; i < st.row_count; ++i)
            for (j = 0; j < st.row_count; ++j) {
                if (st.rows[j].rank != st.rows[i].rank + 1u) continue;
                if (st.rows[i].rank == 3u) gap_3_to_4 = st.rows[j].y - st.rows[i].y;
                if (st.rows[i].rank == 4u) gap_4_to_5 = st.rows[j].y - st.rows[i].y;
                if (st.rows[i].rank == 5u) gap_5_to_6 = st.rows[j].y - st.rows[i].y;
            }
    }
    assert(gap_3_to_4 == 112);   /* the one-off wider gap */
    assert(gap_4_to_5 == 70);
    assert(gap_5_to_6 == 70);
}

/* Three rows per burst, however A11 is spelled. */
static void test_each_burst_spawns_three_rows(void)
{
    static WmHsSystem sys;
    WmHstdState st;
    unsigned t, bursts = 0;
    int spawns = 0, prev_start;
    bool in_scroll = false;

    assert(WM_HSTD_FIRST_BURST == 4);
    assert(WM_HSTD_LATER_BURST == 3);

    wm_hs_system_init(&sys, WM_HS_ADJUSTED_RESET_DEFAULT);
    (void)wm_hs_system_table_cmos_check(&sys);
    wm_hstd_begin(&st, &sys);

    for (t = 0; t < 10000u; ++t) {
        prev_start = st.starting_num;
        if (wm_hstd_tick(&st, &sys, false)) break;
        /* Count spawns only once the scroll proper has begun, so
           enter_scroll's own decrement is not mistaken for one. */
        if (st.phase == WM_HSTD_PHASE_WAIT_BUTS && st.scrolling) in_scroll = true;
        if (in_scroll && st.starting_num == prev_start + 1) ++spawns;
        if (st.phase == WM_HSTD_PHASE_DELAY_SCROLL && spawns) {
            assert(spawns == 3);
            ++bursts;
            spawns = 0;
        }
        if (st.phase == WM_HSTD_PHASE_PLACE) { spawns = 0; in_scroll = false; }
    }
    /* Three complete bursts on the inter table, two on the world one. */
    assert(bursts == 5u);
}

/* ---- how the scroll ends ------------------------------------------- */

/*
 * @not_blank, whose name says the opposite of what it means: set when a row
 * with a ZERO score is drawn. On a factory-fresh machine it fires, because
 * only ten of the inter table's thirty ranks carry a score, so the scroll
 * stops well short of rank 30. That is the arcade's own behaviour on a new
 * cabinet, not a port limitation.
 */
static void test_a_factory_table_stops_the_scroll_early(void)
{
    static WmHsSystem sys;
    WmHstdState st;
    unsigned t;
    int max_inter = 0, max_beaten = 0;

    wm_hs_system_init(&sys, WM_HS_ADJUSTED_RESET_DEFAULT);
    (void)wm_hs_system_table_cmos_check(&sys);
    wm_hstd_begin(&st, &sys);

    for (t = 0; t < 10000u; ++t) {
        if (st.half == 0u) {
            if (st.starting_num > max_inter) max_inter = st.starting_num;
        } else if (st.starting_num > max_beaten) {
            max_beaten = st.starting_num;
        }
        if (wm_hstd_tick(&st, &sys, false)) break;
    }
    assert(st.blank_drawn);
    assert(max_inter == 12);
    assert(max_beaten == 9);
    /* Emphatically not the whole table. */
    assert(max_inter < (int)wm_hs_present_sequence[WM_HS_PRESENT_INTER].total_rows);
}

/*
 * The other exit -- `cmpi ENTRIES-1,a9 / JRGE JUST_WAIT_I` -- which factory
 * data never reaches because the blank check fires first. Filling both tables
 * is the only way to exercise it.
 */
static void test_a_full_table_runs_to_the_last_rank(void)
{
    static WmHsSystem sys;
    WmHstdState st;
    unsigned t, tab, i;
    int max_inter = 0, max_beaten = 0;

    wm_hs_system_init(&sys, WM_HS_ADJUSTED_RESET_DEFAULT);
    (void)wm_hs_system_table_cmos_check(&sys);
    for (tab = 0; tab < 2u; ++tab) {
        WmHsTable *table = wm_hs_system_table(
            &sys, tab ? WM_HS_TABLE_BEATEN : WM_HS_TABLE_INTER);
        for (i = 1u; i <= 30u; ++i)
            wm_hs_entry_set_score_bcd(&table->entries[i], 0x1000u + i);
    }

    wm_hstd_begin(&st, &sys);
    /* Ticks counted from 1, as test_the_screen_holds_for_its_source_time
       does, so the two totals are directly comparable. */
    for (t = 1; t <= 20000u; ++t) {
        if (st.half == 0u) {
            if (st.starting_num > max_inter) max_inter = st.starting_num;
        } else if (st.starting_num > max_beaten) {
            max_beaten = st.starting_num;
        }
        if (wm_hstd_tick(&st, &sys, false)) break;
    }
    assert(!st.blank_drawn);
    assert(max_inter == 30);
    assert(max_beaten == 30);
    assert(max_inter == (int)wm_hs_present_sequence[WM_HS_PRESENT_INTER].total_rows);
    /* Longer than the factory run, necessarily: the blank row is what cuts
       that one short, and here there is no blank row to find. */
    assert(t == 4422u);
    assert(t > 1891u);
}

/* DELETE_ANY_OFF_TOP keeps the live set bounded, and the bound is real. */
static void test_the_live_row_set_stays_bounded(void)
{
    static WmHsSystem sys;
    WmHstdState st;
    unsigned t, tab, i;

    assert(WM_HSTD_DELETE_Y == -30);

    wm_hs_system_init(&sys, WM_HS_ADJUSTED_RESET_DEFAULT);
    (void)wm_hs_system_table_cmos_check(&sys);
    for (tab = 0; tab < 2u; ++tab) {
        WmHsTable *table = wm_hs_system_table(
            &sys, tab ? WM_HS_TABLE_BEATEN : WM_HS_TABLE_INTER);
        for (i = 1u; i <= 30u; ++i)
            wm_hs_entry_set_score_bcd(&table->entries[i], 0x1000u + i);
    }

    wm_hstd_begin(&st, &sys);
    for (t = 0; t < 20000u; ++t) {
        assert(st.row_count <= WM_HSTD_MAX_LIVE_ROWS);
        if (wm_hstd_tick(&st, &sys, false)) break;
    }
    /* Five, on the longest run there is -- so the 8 is headroom and not a
       guess, and a change that leaked rows would show up here. */
    assert(st.peak_row_count == 5u);
}

/* ---- the button ---------------------------------------------------- */

static void test_a_button_cuts_the_screen_short(void)
{
    static WmHsSystem sys;
    WmHstdState st;
    unsigned t;

    wm_hs_system_init(&sys, WM_HS_ADJUSTED_RESET_DEFAULT);
    (void)wm_hs_system_table_cmos_check(&sys);
    wm_hstd_begin(&st, &sys);

    /* Held from the first tick. The wipe and the half-second settle are not
       skippable -- the source does not sample buttons inside them -- so this
       cannot end before they are past. */
    for (t = 1; t <= 10000u; ++t)
        if (wm_hstd_tick(&st, &sys, true)) break;

    assert(t > (unsigned)WM_ATTRACT_BLOW_0_TO_1_TICKS);
    assert(t < 1891u / 2u);
}

/* ---- what a row actually shows ------------------------------------- */

/*
 * A row is initials plus icons, and this port draws the initials only. The
 * score POSITION every caller passes is dead in draw_beaten_table_entry, and
 * the initials print 50 pixels right of the x it is handed.
 */
static void test_a_row_is_initials_at_x_plus_fifty(void)
{
    const WmHsPresentLayout *inter =
        &wm_hs_present_sequence[WM_HS_PRESENT_INTER].layout;

    assert(WM_HSTD_INITIALS_X_BIAS == 50);
    assert(inter->first_initials_x == 8);
    assert(inter->first_initials_y == 42);
    /* So the text lands at 58, not 8. */
    assert(inter->first_initials_x + WM_HSTD_INITIALS_X_BIAS == 58);

    /* The score position is still carried, because the source carries it --
       and this marker says nothing may draw at it on this path. Deleting the
       marker is the deliberate act of claiming otherwise. */
    assert(WM_HSTD_SCORE_POSITION_IS_DEAD == 1);
    assert(inter->first_score_y == 63);
    assert(inter->first_score_x == 10);
}

/*
 * A blank entry draws nothing, and that is NOT the same as a line the engine
 * refuses: wm_text_build_draw_list returns -1 when it cannot measure a line
 * and 0 when the line is all spaces, which is what a zero-score row's
 * initials are. Keeping them apart is what stops "the table went empty" from
 * reading as "the font is missing a glyph".
 */
static void test_a_blank_row_draws_nothing_and_is_not_a_refusal(void)
{
    static WmHsSystem sys;
    wm_text_draw_item items[64];
    unsigned rank, drawn = 0, blank = 0, refused = 0;

    wm_hs_system_init(&sys, WM_HS_ADJUSTED_RESET_DEFAULT);
    (void)wm_hs_system_table_cmos_check(&sys);

    for (rank = 1u; rank <= 30u; ++rank) {
        WmHsDisplayRow row;
        char initials[WM_HS_NUM_INITIALS + 1u];
        int32_t n;
        if (wm_hs_present_rows(&sys, WM_HS_PRESENT_INTER, (uint16_t)rank,
                               &row, 1u) != 1u)
            continue;
        memcpy(initials, row.initials, sizeof initials);
        initials[WM_HS_NUM_INITIALS] = '\0';
        n = wm_text_build_draw_list(initials, wm_stringer_rd7font(), 1,
                                    WM_STRINGER_LEFT, 58, 42, items, 64);
        if (n > 0) ++drawn;
        else if (n == 0) ++blank;
        else ++refused;
    }
    /* The factory inter table: nine named ranks, the rest blank, and nothing
       the font cannot measure. */
    assert(drawn == 9u);
    assert(blank == 21u);
    assert(refused == 0u);
    assert(drawn + blank + refused == 30u);
}

int main(void)
{
    test_show_hstd_is_reachable_at_all();
    test_the_presentation_layer_now_has_a_consumer();
    test_both_tables_run_in_the_sources_order();
    test_the_screen_holds_for_its_source_time();
    test_the_scroll_is_a_velocity_of_two_pixels_a_tick();
    test_the_row_spacing_is_the_sources();
    test_each_burst_spawns_three_rows();
    test_a_factory_table_stops_the_scroll_early();
    test_a_full_table_runs_to_the_last_rank();
    test_the_live_row_set_stays_bounded();
    test_a_button_cuts_the_screen_short();
    test_a_row_is_initials_at_x_plus_fifty();
    test_a_blank_row_draws_nothing_and_is_not_a_refusal();
    printf("show_hstd OK\n");
    return 0;
}
