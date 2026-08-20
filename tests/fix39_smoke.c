#include "wm_fix39_runtime.h"
#include "wm_arcade_roster.h"
#include "wmania_ring_geometry.h"
#include "wmania_rng.h"
#include "wmania_rope_command.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

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

static void test_attract_shadow(void)
{
    size_t n;
    size_t demos = 0u;
    wm_fix39_runtime_init();
    n = wm_fix39_attract_cycle_begin();
    assert(n == 13u);
    for (size_t i = 0; i < n; ++i) {
        const WmAttractStep *s = wm_fix39_attract_step(i);
        assert(s != 0);
        if (s->gameplay_demo_unimplemented) ++demos;
    }
    assert(demos == 2u);
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
}

static void test_rope_processes(void)
{
    wm_fix39_runtime_init();
    assert(wm_fix39_rope_process_alive(WM_ROPE_FRONT));
    assert(wm_fix39_rope_process_alive(WM_ROPE_BACK));
    assert(wm_fix39_rope_process_alive(WM_ROPE_LEFT));
    assert(wm_fix39_rope_process_alive(WM_ROPE_RIGHT));
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
}

int main(void)
{
    test_rng_bridge();
    test_attract_shadow();
    test_match_seed_and_mapping();
    test_rope_processes();
    test_hiscore_counter_binding();
    puts("Fix39 integration smoke: PASS");
    return 0;
}
