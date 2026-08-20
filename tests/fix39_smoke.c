#include "wm_fix39_runtime.h"
#include "wm_arcade_roster.h"
#include "wmania_ring_geometry.h"
#include "wmania_attract_data.h"
#include "wmania_attract_live.h"
#include "wmania_attract_operator.h"
#include "wmania_attract_time.h"
#include "wmania_attract_visuals.h"
#include "wmania_rng.h"
#include "wmania_rope_command.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_rng_bridge(void)
{
    WmRng direct;
    uint32_t got;
    uint32_t expected;

    wm_fix39_runtime_init();
    wm_fix39_rng_set_entropy(0x0123u, 0x456789abu);
    got = wm_fix39_rndrng0(40u);

    wm_rng_init(&direct, 0u, 0, 0, 0);
    wm_rng_set_latched_inputs(&direct, 0x0123u, 0x456789abu);
    expected = wm_rng_rndrng0(&direct, 40u);
    assert(got == expected);
    assert(wm_fix39_rng_state() == direct.rand_state);
}

static size_t find_attract_screen(WmAttractScreen screen)
{
    size_t n = wm_fix39_status()->attract_step_count;
    for (size_t i = 0u; i < n; ++i) {
        const WmAttractStep *s = wm_fix39_attract_step(i);
        if (s && s->screen == screen) return i;
    }
    return (size_t)-1;
}

static void test_attract_live_cycle(void)
{
    static const uint8_t expected_hint_lines[WM_ATTRACT_ACTIVE_HINTS] = {
        4u, 6u, 6u, 5u, 6u, 5u, 6u, 4u, 3u, 4u
    };
    static const char *const expected_hint_numbers[WM_ATTRACT_ACTIVE_HINTS] = {
        "WGSF22_1", "WGSF22_2", "WGSF22_3", "WGSF22_4", "WGSF22_5",
        "WGSF22_6", "WGSF22_7", "WGSF22_8", "WGSF22_9", "WGSF22_0"
    };
    size_t n;
    size_t demos = 0u;

    assert(WM_ATTRACT_ACTIVE_HINTS == 10u);
    for (size_t i = 0u; i < WM_ATTRACT_ACTIVE_HINTS; ++i) {
        assert(wm_attract_hints[i].body_line_count == expected_hint_lines[i]);
        assert(strcmp(wm_attract_hints[i].number_image_symbol,
                      expected_hint_numbers[i]) == 0);
        assert(wm_attract_hints[i].number_image_index == i);
    }

    {
        WmAttractTextPlacement placements[12];
        size_t count = wm_attract_hint_placements(0u, placements, 12u);
        assert(count == 5u);
        assert(placements[0].x == WM_ATTRACT_HINT_TITLE_X);
        assert(placements[0].y == WM_ATTRACT_HINT_TITLE_Y);
        assert(strcmp(placements[0].source_label, "HNTT_2") == 0);
        assert(strcmp(placements[1].source_label, "HNT_2A") == 0);
        assert(placements[1].y == WM_ATTRACT_HINT_BODY_Y);
        assert(placements[4].y == WM_ATTRACT_HINT_BODY_Y + 3 * WM_ATTRACT_HINT_BODY_LINE_STEP);
        count = wm_attract_general_tip_placements(placements, 12u);
        assert(count == 12u);
        assert(strcmp(placements[0].source_label, WM_FIX39_ATTRACT_GENERAL_TIPS_TITLE_LABEL) == 0);
        assert(placements[1].y == WM_FIX39_ATTRACT_GENERAL_TIPS_FIRST_Y);
        assert(placements[11].y == WM_FIX39_ATTRACT_GENERAL_TIPS_FIRST_Y + 10 * WM_FIX39_ATTRACT_GENERAL_TIPS_LINE_STEP);

        count = wm_attract_time_date_placements(placements, 12u);
        assert(count == 6u);
        assert(strcmp(placements[0].source_label, "date_time_prompt") == 0);
        assert(placements[0].x == 200 && placements[0].y == 40);
        assert(strcmp(placements[5].source_label, "wrestlemania_message") == 0);
        assert(placements[5].y == 180);

        count = wm_attract_operator_placements(3u, placements, 12u);
        assert(count == 3u);
        assert(placements[0].x == 200 && placements[0].y == 50);
        assert(placements[1].y == 95);
        assert(placements[2].y == 140);
        assert(placements[0].source_label == 0);
    }

    {
        static const uint8_t raw[3][8] = {
            {'H','E','L','L','O',0,0,0},
            {0,'X','X','X',0,0,0,0},
            {'W','W','F',0,0,0,0,0}
        };
        WmAttractOperatorMessage msg = { &raw[0][0], 3u, 8u };
        char line[8];
        assert(wm_attract_operator_has_message(&msg));
        assert(wm_attract_operator_copy_line(&msg, 0u, line, sizeof(line)) == 5u);
        assert(strcmp(line, "HELLO") == 0);
        assert(wm_attract_operator_copy_line(&msg, 1u, line, sizeof(line)) == 0u);
        assert(strcmp(line, "") == 0);
        assert(wm_attract_operator_copy_line(&msg, 2u, line, 3u) == 2u);
        assert(strcmp(line, "WW") == 0);
    }

    wm_fix39_runtime_init();
    n = wm_fix39_attract_cycle_begin();
    assert(n == 13u);
    for (size_t i = 0; i < n; ++i) {
        const WmAttractStep *s = wm_fix39_attract_step(i);
        assert(s != 0);
        if (s->gameplay_demo_unimplemented) ++demos;
    }
    assert(demos == 2u);
    assert(wm_fix39_attract_step(find_attract_screen(
        WM_FIX39_ATTRACT_DESIGNER_HINT))->hint_index == 1u);
    assert(wm_fix39_status()->attract_cycles_built == 1u);

    /* BSS last_hint=0; DO_HINTS increments first, then wraps at NUM_HINTS=10. */
    for (uint8_t expected = 2u; expected <= 9u; ++expected) {
        (void)wm_fix39_attract_cycle_begin();
        assert(wm_fix39_attract_step(find_attract_screen(
            WM_FIX39_ATTRACT_DESIGNER_HINT))->hint_index == expected);
    }
    (void)wm_fix39_attract_cycle_begin();
    assert(wm_fix39_attract_step(find_attract_screen(
        WM_FIX39_ATTRACT_DESIGNER_HINT))->hint_index == 0u);
}

static void tick_live_exact(WmAttractLive *live, uint32_t ticks, bool done_last)
{
    for (uint32_t i = 0u; i < ticks; ++i) {
        bool done = wm_attract_live_tick(live, false);
        if (i + 1u < ticks || !done_last) assert(!done);
        else assert(done);
    }
}

static void test_attract_source_timing(void)
{
    WmAttractLive live;
    WmAttractStep step = {0};

    step.screen = WM_FIX39_ATTRACT_DESIGNER_HINT;
    step.hint_index = 4u;
    assert(wm_attract_live_begin(&live, &step));
    assert(live.phase == WM_ATTRACT_LIVE_WAIT_EXTERNAL);
    assert(wm_attract_live_signal_external_complete(&live));
    tick_live_exact(&live, WM_ATTRACT_HINT_PREWAIT_TICKS, false);
    assert(live.phase == WM_ATTRACT_LIVE_WAIT_BUTTON);
    tick_live_exact(&live, WM_ATTRACT_HINT_WAIT_TSEC * WM_FIX39_ATTRACT_TSEC,
                    true);

    step.screen = WM_FIX39_ATTRACT_GENERAL_TIPS;
    assert(wm_attract_live_begin(&live, &step));
    assert(wm_attract_live_signal_external_complete(&live));
    tick_live_exact(&live,
        WM_FIX39_ATTRACT_GENERAL_TIPS_PREWAIT_TSEC * WM_FIX39_ATTRACT_TSEC,
        false);
    assert(live.phase == WM_ATTRACT_LIVE_WAIT_BUTTON);
    assert(wm_attract_live_tick(&live, true));

    step.screen = WM_FIX39_ATTRACT_COPYRIGHT;
    assert(wm_attract_live_begin(&live, &step));
    tick_live_exact(&live, WM_FIX39_ATTRACT_COPYRIGHT_INITIAL_SLEEP_TICKS,
                    false);
    assert(live.phase == WM_ATTRACT_LIVE_COPYRIGHT_PAGE1_SETTLE);
    tick_live_exact(&live, 2u + WM_FIX39_ATTRACT_COPYRIGHT_FADE_SETTLE_TICKS,
                    false);
    assert(live.phase == WM_ATTRACT_LIVE_COPYRIGHT_PAGE1_WAIT);
    tick_live_exact(&live,
        WM_FIX39_ATTRACT_COPYRIGHT_PAGE_WAIT_TSEC * WM_FIX39_ATTRACT_TSEC,
        false);
    assert(live.page == 2u);
    tick_live_exact(&live, WM_FIX39_ATTRACT_COPYRIGHT_FADE_SETTLE_TICKS,
                    false);
    assert(live.phase == WM_ATTRACT_LIVE_COPYRIGHT_PAGE2_WAIT);
    tick_live_exact(&live,
        WM_FIX39_ATTRACT_COPYRIGHT_PAGE_WAIT_TSEC * WM_FIX39_ATTRACT_TSEC,
        true);

    step.screen = WM_FIX39_ATTRACT_AAMA;
    assert(wm_attract_live_begin(&live, &step));
    tick_live_exact(&live,
        WM_FIX39_ATTRACT_AAMA_INITIAL_SLEEP_TICKS +
        WM_FIX39_ATTRACT_AAMA_GRADIENT_PREWAIT_TICKS +
        WM_FIX39_ATTRACT_AAMA_GRADIENT_POSTWAIT_TICKS, false);
    assert(live.phase == WM_ATTRACT_LIVE_AAMA_SETTLE);
    tick_live_exact(&live,
        WM_FIX39_ATTRACT_AAMA_POST_FADE_SLEEP_TICKS +
        WM_FIX39_ATTRACT_AAMA_FADE_SETTLE_TICKS, false);
    assert(live.phase == WM_ATTRACT_LIVE_AAMA_WAIT);
    tick_live_exact(&live,
        WM_FIX39_ATTRACT_AAMA_WAIT_TSEC * WM_FIX39_ATTRACT_TSEC, true);

    /* show_time_date: READ_DIP false exits immediately. */
    step.screen = WM_FIX39_ATTRACT_TIME_DATE;
    assert(wm_attract_live_begin(&live, &step));
    assert(live.phase == WM_ATTRACT_LIVE_TIME_DATE_QUERY);
    assert(live.waiting_external);
    assert(wm_attract_live_signal_external_result(&live, false));
    assert(live.done);

    /* READ_DIP true -> _GetTime -> 30 ticks -> GENERIC_DISPLAY/text ->
       minimum 25 ticks -> up to 5*TSEC button poll. */
    assert(wm_attract_live_begin(&live, &step));
    assert(wm_attract_live_signal_external_result(&live, true));
    assert(live.phase == WM_ATTRACT_LIVE_TIME_DATE_PREWAIT);
    tick_live_exact(&live, WM_FIX39_ATTRACT_TIME_DATE_MIN_PRE_TICKS, false);
    assert(live.phase == WM_ATTRACT_LIVE_TIME_DATE_PRESENT);
    assert(live.waiting_external);
    assert(wm_attract_live_signal_external_complete(&live));
    tick_live_exact(&live, WM_FIX39_ATTRACT_TIME_DATE_MIN_DISPLAY_TICKS, false);
    assert(live.phase == WM_ATTRACT_LIVE_TIME_DATE_WAIT);
    assert(live.button_enabled);
    assert(wm_attract_live_tick(&live, true));

    /* show_operatormsg: no nonzero first byte means immediate return. */
    step.screen = WM_FIX39_ATTRACT_OPERATOR_MESSAGE;
    assert(wm_attract_live_begin(&live, &step));
    assert(live.phase == WM_ATTRACT_LIVE_OPERATOR_QUERY);
    assert(wm_attract_live_signal_external_result(&live, false));
    assert(live.done);

    /* Message present -> blocking dan_test (2+1+32) -> print rows -> 2 sec
       -> wait_on_butn 6*TSEC -> scrn_scaleout/WIPEOUT. */
    assert(wm_attract_live_begin(&live, &step));
    assert(wm_attract_live_signal_external_result(&live, true));
    assert(live.phase == WM_ATTRACT_LIVE_OPERATOR_DAN_SETUP);
    tick_live_exact(&live, WM_FIX39_ATTRACT_OPERATOR_DAN_SETUP_TICKS, false);
    assert(live.phase == WM_ATTRACT_LIVE_OPERATOR_PRESENT);
    assert(live.waiting_external);
    assert(wm_attract_live_signal_external_complete(&live));
    tick_live_exact(&live, WM_ATTRACT_OPERATOR_PREWAIT_TICKS, false);
    assert(live.phase == WM_ATTRACT_LIVE_OPERATOR_WAIT);
    assert(live.button_enabled);
    assert(!wm_attract_live_tick(&live, true));
    assert(live.phase == WM_ATTRACT_LIVE_OPERATOR_CLEANUP);
    assert(live.waiting_external);
    assert(wm_attract_live_signal_external_complete(&live));
    assert(live.done);
}

static void test_attract_ownership_gate(void)
{
    size_t dcs;
    size_t hint;
    size_t operator_msg;
    size_t time_date;
    size_t hstd;
    wm_fix39_runtime_init();
    (void)wm_fix39_attract_cycle_begin();
    dcs = find_attract_screen(WM_FIX39_ATTRACT_DCS_LOGO);
    hint = find_attract_screen(WM_FIX39_ATTRACT_DESIGNER_HINT);
    operator_msg = find_attract_screen(WM_FIX39_ATTRACT_OPERATOR_MESSAGE);
    time_date = (size_t)-1;
    hstd = find_attract_screen(WM_FIX39_ATTRACT_HISCORES);

    assert(wm_fix39_attract_step_owner(dcs) == WM_ATTRACT_OWNER_EXISTING_FRONTEND);
    assert(wm_fix39_attract_step_runnable(dcs));
    assert(wm_fix39_attract_step_owner(hint) == WM_ATTRACT_OWNER_FIX39_LIVE);
    assert(!wm_fix39_attract_step_runnable(hint));
    assert(wm_fix39_attract_step_owner(operator_msg) == WM_ATTRACT_OWNER_FIX39_LIVE);
    assert(!wm_fix39_attract_step_runnable(operator_msg));
    assert(wm_attract_live_owner(WM_FIX39_ATTRACT_TIME_DATE) == WM_ATTRACT_OWNER_FIX39_LIVE);
    assert(wm_fix39_attract_step_owner(hstd) == WM_ATTRACT_OWNER_PENDING_DEPENDENCY);
    assert(!wm_fix39_attract_step_runnable(hstd));

    wm_fix39_attract_note_pending_skip(hstd);
    assert(wm_fix39_status()->attract_pending_skips == 1u);

    /* TIME_DATE is the even-loop tail, so it appears on the next cycle. */
    (void)wm_fix39_attract_cycle_begin();
    time_date = find_attract_screen(WM_FIX39_ATTRACT_TIME_DATE);
    assert(time_date != (size_t)-1);
    assert(!wm_fix39_attract_step_runnable(time_date));

    wm_fix39_attract_set_platform_capabilities(
        WM_FIX39_ATTRACT_CAP_DESIGNER_HINT |
        WM_FIX39_ATTRACT_CAP_OPERATOR_MESSAGE |
        WM_FIX39_ATTRACT_CAP_TIME_DATE);
    assert(wm_fix39_attract_step_runnable(hint));
    assert(wm_fix39_attract_step_runnable(operator_msg));
    assert(wm_fix39_attract_step_runnable(time_date));
    assert(wm_fix39_attract_screen_begin(hint));
    assert(wm_fix39_attract_live_state()->waiting_external);
    assert(wm_fix39_status()->attract_external_waits == 1u);
    assert(wm_fix39_attract_screen_signal_external_complete());
    assert(!wm_fix39_attract_screen_tick(false));
    assert(wm_fix39_status()->attract_live_ticks == 1u);
}

static void test_match_seed_and_mapping(void)
{
    const wm_arcade_actor_t *p1;
    const wm_arcade_actor_t *p2;

    wm_fix39_runtime_init();
    /* Current frontend enum: Bret=0, Bam=1. */
    wm_fix39_match_begin(0u, 1u);
    assert(wm_fix39_match_started());
    p1 = wm_fix39_actor(0u);
    p2 = wm_fix39_actor(1u);
    assert(p1 != 0 && p2 != 0);
    assert(p1->wrestler_num == WM_ROSTER_BRET);
    assert(p2->wrestler_num == WM_ROSTER_BAM);
    assert(p1->x_int == WM_RING_X_CENTER - 85);
    assert(p1->z_int == 1127 + 93);
    assert(p1->facing_dir == 9);
    assert(p2->x_int == WM_RING_X_CENTER + 85);
    assert(p2->z_int == 1103 + 93);
    assert(p2->facing_dir == 6);
    assert(p1->ground_y == WM_MAT_Y && p2->ground_y == WM_MAT_Y);
    assert(p1->smart_target == p2 && p2->smart_target == p1);

    wm_fix39_match_tick(90, 90, false, true, false, false, false, true);
    p1 = wm_fix39_actor(0u);
    assert(p1->stick_val_cur == WM_MOVE_UP_RIGHT);
    assert((p1->but_val_cur & WM_BTN_PUNCH) != 0u);
    assert((p1->but_val_cur & WM_BTN_BLOCK) != 0u);
    assert(wm_fix39_status()->pcnt == 1u);
    assert(wm_fix39_status()->round_tickcount == 1u);
    assert(wm_fix39_status()->wrestler_dispatch_ticks == 2u);
    assert(wm_fix39_status()->wrestler_dispatch_ticks_by_player[0] == 1u);
    assert(wm_fix39_status()->wrestler_dispatch_ticks_by_player[1] == 1u);
    assert(wm_fix39_actor_trace(0u) != 0);
    /* Bret's direct ani_init emitted the source standing/torso tokens. */
    assert(wm_fix39_actor_trace(0u)->animation_events >= 2u);
}

static void test_all_eight_live_dispatchers(void)
{
    for (unsigned frontend = 0u; frontend < 8u; ++frontend) {
        wm_fix39_runtime_init();
        wm_fix39_match_begin(frontend, 0u);
        assert(wm_fix39_match_started());
        wm_fix39_match_tick(0, 0, false, false, false, false, false, false);
        assert(wm_fix39_status()->wrestler_dispatch_ticks_by_player[0] == 1u);
        assert(wm_fix39_actor_trace(0u) != 0);
    }
}

static void test_rope_processes(void)
{
    wm_fix39_runtime_init();
    assert(wm_fix39_rope_process_alive(WM_FIX39_ROPE_FRONT));
    assert(wm_fix39_rope_process_alive(WM_FIX39_ROPE_BACK));
    assert(wm_fix39_rope_process_alive(WM_FIX39_ROPE_LEFT));
    assert(wm_fix39_rope_process_alive(WM_FIX39_ROPE_RIGHT));
    /* Front/back BOUNCE_UD is a complete rope_command -> direct ROPES.ASM
       program path in the supplied ring bundle. */
    assert(wm_fix39_rope_apply_command(WM_FIX39_ROPE_FRONT,
                                       WM_FIX39_ROPE_BOUNCE_UD, 0u, 0));
    wm_fix39_match_begin(0u, 1u);
    wm_fix39_match_tick(0, 0, false, false, false, false, false, false);
    assert(wm_fix39_status()->rope_process_ticks == 4u);
}

static void test_hiscore_counter_binding(void)
{
    uint32_t remaining = 0xffffffffu;
    wm_fix39_runtime_init();
    assert(wm_fix39_status()->hiscore_tables_valid);
    assert(!wm_fix39_hiscore_player_start_or_continue(&remaining));
    wm_fix39_hiscore_bind_reset_value(1000u);
    assert(wm_fix39_hiscore_player_start_or_continue(&remaining));
    assert(remaining == 999u);
    assert(strcmp(wm_fix39_hiscore_recent_initials(true), "MIKE ") == 0);
    assert(strcmp(wm_fix39_hiscore_recent_initials(false), "MARK ") == 0);
}

int main(void)
{
    test_rng_bridge();
    test_attract_live_cycle();
    test_attract_source_timing();
    test_attract_ownership_gate();
    test_match_seed_and_mapping();
    test_all_eight_live_dispatchers();
    test_rope_processes();
    test_hiscore_counter_binding();
    puts("Fix39 integration smoke: PASS");
    return 0;
}
