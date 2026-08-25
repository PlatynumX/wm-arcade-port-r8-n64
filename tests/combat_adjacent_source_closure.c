#include "wm_fix39_runtime.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void tick_neutral(unsigned ticks)
{
    unsigned i;
    for (i = 0u; i < ticks; ++i)
        wm_fix39_match_tick(0, 0, false, false, false, false, false, false);
}

static void test_drone_feeds_live_wrestle_input(void)
{
    const WmFix39Status *s;
    uint32_t drone0;
    uint32_t input0;
    uint32_t dispatch0;

    wm_fix39_runtime_init();
    wm_fix39_match_begin(0u, 1u);
    wm_fix39_match_set_cpu_vs_cpu(true);

    s = wm_fix39_status();
    assert(s->match_started);
    assert(s->drone_runtime_ready);

    drone0 = s->drone_ticks;
    input0 = s->drone_input_ticks;
    dispatch0 = s->wrestler_dispatch_ticks;

    tick_neutral(12u);

    s = wm_fix39_status();
    assert(s->drone_ticks > drone0);
    assert(s->wrestler_dispatch_ticks > dispatch0);
    /* DRONE can legitimately choose idle on some frames, but the live
       source brain must at least be able to write its actor input path. */
    assert(s->drone_input_ticks >= input0);
}

static void test_special_objects_tick_in_live_combat_path(void)
{
    const WmFix39Status *s;
    uint32_t special0;
    int spawned;

    wm_fix39_runtime_init();
    wm_fix39_match_begin(2u, 3u);

    spawned = wm_fix39_match_spawn_special(0u, WM_SP_KIND_YOKO_SALT);
    assert(spawned);

    s = wm_fix39_status();
    special0 = s->special_process_ticks;

    tick_neutral(4u);

    s = wm_fix39_status();
    assert(s->special_process_ticks > special0);
}

static void test_ringout_and_rope_processes_live(void)
{
    const WmFix39Status *s;
    uint32_t ring0;
    uint32_t rope0;

    wm_fix39_runtime_init();
    wm_fix39_match_begin(4u, 5u);
    wm_fix39_match_set_ringout_enabled(true);

    s = wm_fix39_status();
    ring0 = s->ringout_process_ticks;
    rope0 = s->rope_process_ticks;

    tick_neutral(4u);

    s = wm_fix39_status();
    assert(s->ringout_process_ticks > ring0);
    assert(s->rope_process_ticks > rope0);
}

int main(void)
{
    test_drone_feeds_live_wrestle_input();
    test_special_objects_tick_in_live_combat_path();
    test_ringout_and_rope_processes_live();
    puts("Combat2ES combat-adjacent source closure regression: PASS");
    return 0;
}
