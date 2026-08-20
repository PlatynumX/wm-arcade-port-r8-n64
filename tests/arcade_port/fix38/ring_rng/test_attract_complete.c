#include "wmania_attract_adapter.h"
#include "wmania_attract_core.h"
#include "wmania_attract_data.h"
#include "wmania_attract_operator.h"
#include "wmania_attract_secret.h"
#include "wmania_attract_time.h"
#include "wmania_attract_visuals.h"
#include "wmania_hiscore_system.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>


static void test_cycle(void)
{
    WmAttractState s;
    WmAttractStep steps[20];
    size_t n;

    wm_attract_init(&s);

    n = wm_attract_build_cycle(&s, steps, 20u);
    assert(n == 13u);
    assert(steps[0].screen == WM_ATTRACT_HISCORES);
    assert(steps[1].screen == WM_ATTRACT_DCS_LOGO);
    assert(steps[2].screen == WM_ATTRACT_SPORTS_LOGO);
    assert(steps[3].screen == WM_ATTRACT_GAMEPLAY_DEMO_1);
    assert(steps[3].gameplay_demo_unimplemented);
    assert(steps[4].screen == WM_ATTRACT_CREDITS_1);
    assert(steps[5].screen == WM_ATTRACT_TITLE);
    assert(steps[6].screen == WM_ATTRACT_GAMEPLAY_DEMO_2);
    assert(steps[7].screen == WM_ATTRACT_CREDITS_2);
    assert(steps[8].screen == WM_ATTRACT_DESIGNER_HINT);
    assert(steps[9].screen == WM_ATTRACT_GENERAL_TIPS);
    assert(steps[10].screen == WM_ATTRACT_BIO);
    assert(steps[11].screen == WM_ATTRACT_BIO_TIPS);
    assert(steps[12].screen == WM_ATTRACT_OPERATOR_MESSAGE);
    assert(steps[8].hint_index == 1u);     /* BSS last_hint starts at 0 */
    assert(steps[10].wrestler_index == 1u);/* BSS next_bio starts at 0 */
    assert(steps[11].wrestler_index == steps[10].wrestler_index);
    assert(steps[10].source_amode_loops == 0u);
    assert(s.amode_loops == 1u);

    n = wm_attract_build_cycle(&s, steps, 20u);
    assert(n == 15u);
    assert(steps[13].screen == WM_ATTRACT_EVEN_LOOP_CREDITS);
    assert(steps[14].screen == WM_ATTRACT_TIME_DATE);
    assert(steps[10].source_amode_loops == 1u);
    assert(s.amode_loops == 2u);

    /* Cycles 3..7 are normal/even; cycle 8 adds legal + AAMA and resets. */
    for (int cycle = 3; cycle <= 7; ++cycle) {
        n = wm_attract_build_cycle(&s, steps, 20u);
        (void)n;
    }
    n = wm_attract_build_cycle(&s, steps, 20u);
    assert(n == 17u);
    assert(steps[15].screen == WM_ATTRACT_COPYRIGHT);
    assert(steps[16].screen == WM_ATTRACT_AAMA);
    assert(s.amode_loops == 0u);
    assert(s.soundsup == 0u);
}

static void test_sound_policy(void)
{
    WmAttractState s;
    wm_attract_init(&s);

    assert(!wm_attract_demo_should_suppress_sound(&s, false));
    assert(wm_attract_demo_should_suppress_sound(&s, true));
    assert(wm_attract_bio_music_allowed(&s, false));

    s.amode_loops = 2u;
    assert(wm_attract_demo_should_suppress_sound(&s, false));
    assert(!wm_attract_bio_music_allowed(&s, false));

    s.soundsup = 2u;
    {
        uint16_t old = wm_attract_wait_button_begin(&s);
        assert(old == 2u);
        assert(s.soundsup == 0u);
        wm_attract_wait_button_end(&s, old);
        assert(s.soundsup == 2u);
    }
}

static void test_clock(void)
{
    WmAttractClock c = { 5, 8, 20, 26, 0, 7 };
    WmAttractClockText t;

    assert(wm_attract_format_clock(&c, &t));
    assert(strcmp(t.weekday, "THURSDAY") == 0);
    assert(strcmp(t.date, "AUGUST 20, 1926") == 0);
    assert(strcmp(t.time, "12:07") == 0);

    c.day_of_week = 99;
    c.month = 99;
    c.day = 99;
    c.year_2digit = 99;
    c.hour_24 = 99;
    c.minute = 99;
    assert(wm_attract_format_clock(&c, &t));
    assert(strcmp(t.weekday, "SUNDAY") == 0);
    assert(strcmp(t.date, "JANUARY 1, 1900") == 0);
    assert(strcmp(t.time, "12:00") == 0);
}

static void test_operator(void)
{
    WmAttractOperatorMessage m;
    WmAttractBall balls[WM_ATTRACT_OPERATOR_BALL_COUNT];
    WmRng rng;
    uint8_t message_bytes[4u * 12u];

    memset(message_bytes, 0, sizeof(message_bytes));
    m.bytes = message_bytes;
    m.line_count = 4u;
    m.line_size = 12u;
    assert(!wm_attract_operator_has_message(&m));
    message_bytes[2u * 12u] = 'H';
    assert(wm_attract_operator_has_message(&m));

    wm_rng_init(&rng, 0x12345678u, 0, 0, 0);
    wm_rng_set_latched_inputs(&rng, 0x15u, 0x00100000u);
    wm_attract_operator_init_balls(balls, &rng);
    for (unsigned i = 0; i < WM_ATTRACT_OPERATOR_BALL_COUNT; ++i) {
        assert(balls[i].active);
        assert(balls[i].x_fp >= 0);
        assert(balls[i].y_fp >= 0);
        assert(balls[i].x_fp <= (WM_ATTRACT_OPERATOR_BALL_X_LIMIT << 16));
        assert(balls[i].y_fp <= (WM_ATTRACT_OPERATOR_BALL_Y_LIMIT << 16));
        assert(balls[i].vx_fp >= -0x40000 && balls[i].vx_fp <= 0x40000);
        assert(balls[i].vy_fp >= -0x30000 && balls[i].vy_fp <= 0x30000);
    }

    for (unsigned i = 0; i < 1000u; ++i) {
        wm_attract_operator_tick_balls(balls);
    }
}

static void test_secret(void)
{
    WmAttractSecretState s;
    static const WmAttractSecretButton seq[6] = {
        WM_ATTRACT_SECRET_KICK,
        WM_ATTRACT_SECRET_BLOCK,
        WM_ATTRACT_SECRET_POWER_PUNCH,
        WM_ATTRACT_SECRET_PUNCH,
        WM_ATTRACT_SECRET_BLOCK,
        WM_ATTRACT_SECRET_POWER_KICK
    };

    wm_attract_secret_begin(&s, 60u);
    for (unsigned i = 0; i < 6u; ++i) {
        assert(wm_attract_secret_tick(&s, true, seq[i]) == (i == 5u));
    }
    assert(s.succeeded);

    wm_attract_secret_begin(&s, 60u);
    for (unsigned i = 0; i < 21u; ++i) {
        (void)wm_attract_secret_tick(
            &s, true, WM_ATTRACT_SECRET_OTHER);
    }
    assert(!s.active);
    assert(!s.succeeded);
}

static void test_visuals_and_data(void)
{
    WmAttractGradientRow grad[63];
    WmAttractTextPlacement p[10];

    assert(wm_attract_aama_gradient(grad, 63u) == 63u);
    assert(grad[0].palette_index == 31u);
    assert(grad[30].palette_index == 1u);
    assert(grad[31].palette_index == 0u);

    assert(wm_attract_copyright_page1_placements(p, 10u) == 9u);
    assert(p[0].y == 110);
    assert(p[8].y == 206);

    assert(wm_attract_aama_placements(p, 10u) == 6u);
    assert(p[0].y == 94);
    assert(p[0].x == 200);

    assert(strcmp(wm_attract_hints[0].title_label, "HNTT_2") == 0);
    assert(strcmp(wm_attract_hints[4].title_label, "HNTT_5") == 0);

    assert(wm_attract_bios[0].weight_lbs == 234u);
    assert(wm_attract_bios[2].height_in == 11u);
    assert(wm_attract_bios[7].tune_id == 3u);
}

int main(void)
{
    test_cycle();
    test_sound_policy();
    test_clock();
    test_operator();
    test_secret();
    test_visuals_and_data();
    puts("wmania_attract_nongameplay_complete tests: PASS");
    return 0;
}
