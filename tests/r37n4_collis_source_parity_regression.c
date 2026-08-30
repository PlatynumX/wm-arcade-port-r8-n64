#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_fix39_runtime.h"

static wm_arcade_frame_box_t source_box(int16_t x, int16_t y, int16_t z, int16_t id)
{
    wm_arcade_frame_box_t box;
    memset(&box, 0, sizeof(box));
    box.iani3x = x;
    box.iani3y = y;
    box.iani3z = z;
    box.iani3id = id;
    return box;
}

static void clear_all_boxes(void)
{
    size_t i;
    for (i = 0u; i < 4u; ++i)
        wm_fix39_match_clear_frame_box(i);
}

static void test_two_actor_readiness(void)
{
    wm_arcade_frame_box_t box = source_box(-18, 42, 18, 84);

    wm_fix39_runtime_init();
    wm_fix39_match_begin(0u, 1u);
    assert(wm_fix39_match_started());
    assert(wm_fix39_active_actor_count() == 2u);

    clear_all_boxes();
    assert(!wm_fix39_status()->collision_boxes_ready);
    assert(wm_fix39_match_set_frame_box(0u, &box));
    assert(!wm_fix39_status()->collision_boxes_ready);
    assert(wm_fix39_match_set_frame_box(1u, &box));
    assert(wm_fix39_status()->collision_boxes_ready);
}

static void test_four_actor_readiness(void)
{
    WmAttractDemoPlan plan;
    wm_arcade_frame_box_t box = source_box(-21, 48, 20, 96);
    size_t i;

    memset(&plan, 0, sizeof(plan));
    plan.player_wrestler = 0u;
    plan.num_opps = 3u;
    plan.opponent_wrestlers[0] = 1u;
    plan.opponent_wrestlers[1] = 2u;
    plan.opponent_wrestlers[2] = 3u;

    wm_fix39_runtime_init();
    assert(wm_fix39_attract_match_begin(&plan));
    assert(wm_fix39_match_started());
    assert(wm_fix39_active_actor_count() == 4u);

    clear_all_boxes();
    assert(!wm_fix39_status()->collision_boxes_ready);

    for (i = 0u; i < 3u; ++i) {
        assert(wm_fix39_match_set_frame_box(i, &box));
        assert(!wm_fix39_status()->collision_boxes_ready);
    }

    assert(wm_fix39_match_set_frame_box(3u, &box));
    assert(wm_fix39_status()->collision_boxes_ready);

    wm_fix39_match_clear_frame_box(2u);
    assert(!wm_fix39_status()->collision_boxes_ready);
}

int main(void)
{
    test_two_actor_readiness();
    test_four_actor_readiness();
    puts("R37N4 COLLIS all-active source-parity regression: PASS");
    return 0;
}
