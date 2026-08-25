
#include "wm_fix39_runtime.h"
#include "wm_arcade_source_attack_frames.h"
#include "wm_arcade_combat_defs.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_source_attack_frame_bridge_sets_actor_attack_window(void)
{
    const wm_arcade_actor_t *a;
    wm_fix39_runtime_init();
    wm_fix39_match_begin(0u, 4u); /* Bret vs Razor frontend order. */

    wm_fix39_match_bind_source_frame_attack(0u, 0u, "H4PL3X+FR4");
    a = wm_fix39_actor(0u);
    assert(a != 0);
    assert((a->anim_mode & WM_ARCADE_MODE_CHECKHIT) != 0u);
    assert(a->attack_mode == WM_AMODE_PUNCH);
    assert(a->attack_xoff == 30);
    assert(a->attack_yoff == 51);
    assert(a->attack_width == 80);
    assert(a->attack_height == 45);

    wm_fix39_match_bind_source_frame_attack(0u, 0u, "NOT_A_SOURCE_FRAME");
    a = wm_fix39_actor(0u);
    assert(a != 0);
    assert((a->anim_mode & WM_ARCADE_MODE_CHECKHIT) == 0u);
    assert((a->anim_mode & WM_ARCADE_MODE_WAITHITOPP) == 0u);
}

static void test_attack_frame_tables_are_multi_wrestler_source_data(void)
{
    bool uses_z = false;
    wm_arcade_attack_on_z_args_t zargs;
    wm_arcade_attack_on_args_t args;
    memset(&zargs, 0, sizeof(zargs));
    memset(&args, 0, sizeof(args));

    assert(wm_arcade_character_attack_for_source_frame(0u, "H4PL3X+FR4", &uses_z, &zargs, &args));
    assert(uses_z);
    assert(zargs.attack_mode == WM_AMODE_PUNCH);

    memset(&zargs, 0, sizeof(zargs));
    memset(&args, 0, sizeof(args));
    assert(wm_arcade_character_attack_for_source_frame(1u, "R2PU3A+FR4", &uses_z, &zargs, &args));
    assert(!uses_z);
    assert(args.attack_mode == WM_AMODE_PUNCH);
}

static void test_whole_runtime_tick_reaches_live_combat_spine(void)
{
    const WmFix39Status *before;
    const WmFix39Status *after;
    uint32_t before_dispatch;
    uint32_t before_collision;
    uint16_t before_tick;
    wm_arcade_frame_box_t wide;
    memset(&wide, 0, sizeof(wide));
    wide.iani3x = -120;
    wide.iani3y = -120;
    wide.iani3z = 360;
    wide.iani3id = 360;

    wm_fix39_runtime_init();
    wm_fix39_match_begin(0u, 4u);
    assert(wm_fix39_match_set_frame_box(0u, &wide));
    assert(wm_fix39_match_set_frame_box(1u, &wide));
    wm_fix39_match_bind_source_frame_attack(0u, 0u, "H4PL3X+FR4");

    before = wm_fix39_status();
    assert(before != 0);
    before_dispatch = before->wrestler_dispatch_ticks;
    before_collision = before->combat_collision_ticks;
    before_tick = before->round_tickcount;

    wm_fix39_match_tick(0, 0, false, false, false, false, false, false);
    after = wm_fix39_status();
    assert(after != 0);

    assert(after->wrestler_dispatch_ticks > before_dispatch);
    assert(after->combat_collision_ticks >= before_collision);
    assert(after->round_tickcount == (uint16_t)(before_tick + 1u));
}

int main(void)
{
    test_source_attack_frame_bridge_sets_actor_attack_window();
    test_attack_frame_tables_are_multi_wrestler_source_data();
    test_whole_runtime_tick_reaches_live_combat_spine();
    puts("Combat2ES whole combat source cutover regression: PASS");
    return 0;
}
