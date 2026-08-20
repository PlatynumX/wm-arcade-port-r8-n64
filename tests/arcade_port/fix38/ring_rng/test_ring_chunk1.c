#include "wmania_ring_geometry.h"
#include "wmania_rope_command.h"
#include "wmania_ring_player_fields.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_geometry(void)
{
    unsigned i;

    assert(WM_RING_Y_SCALE_MULTIPLIER == 0x3566);
    assert(WM_RING_X_CENTER == 1074);
    assert(WM_RING_Z_CENTER == 1184);

    assert(WM_RING_TOP == 1023);
    assert(WM_RING_BOT == 1345);
    assert(WM_RING_DEPTH == 322);

    assert(WM_RING_TOP_LEFT == 856);
    assert(WM_RING_BOT_LEFT == 805);
    assert(WM_RING_LEFT_WIDTH == 51);
    assert(WM_RING_TOP_RIGHT == 1297);
    assert(WM_RING_BOT_RIGHT == 1348);
    assert(WM_RING_RIGHT_WIDTH == 51);

    assert(WM_MAT_TOP == 967);
    assert(WM_MAT_BOT == 1521);
    assert(WM_MAT_DEPTH == 554);
    assert(WM_MAT2_TOP == 962);
    assert(WM_MAT2_BOT == 1526);
    assert(WM_MAT2_DEPTH == 564);
    assert(WM_MAT_Y == 62);

    assert(WM_ARENA_TOP == 592);
    assert(WM_ARENA_BOT == 1896);
    assert(WM_ARENA_DEPTH == 1304);

    for (i = 0; i < WM_RING_BOUNDARY_COUNT; ++i) {
        assert(wm_ring_boundary_seed_consistent(
            &wm_ring_boundary_seeds[i]));
    }

    assert(wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE)->top_x
           == WM_RING_TOP_RIGHT);
    assert(wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_FENCE)->bottom_x
           == WM_ARENA_BOT_LEFT);
    assert(wm_ring_boundary_seed((WmRingBoundaryId)99) == 0);
}

static void test_lanes(void)
{
    const int32_t t1 = (1087 << 16);
    const int32_t t2 = (1151 << 16);
    const int32_t t3 = (1215 << 16);
    const int32_t t4 = (1279 << 16);

    assert(WM_ROPE_LANE_WIDTH == 64);

    assert(wm_rope_side_lane_from_z_fp16(t1 - 1) == 0u);
    assert(wm_rope_side_lane_from_z_fp16(t1) == 1u);
    assert(wm_rope_side_lane_from_z_fp16(t2 - 1) == 1u);
    assert(wm_rope_side_lane_from_z_fp16(t2) == 2u);
    assert(wm_rope_side_lane_from_z_fp16(t3) == 3u);
    assert(wm_rope_side_lane_from_z_fp16(t4) == 4u);
}

static void test_command_routing(void)
{
    WmRopeCommand c;

    assert(wm_rope_resolve_command(
        WM_ROPE_FRONT, WM_ROPE_BOUNCE_UD, 0u, 0, &c));
    assert(strcmp(c.source_script_table, "front_bounceud1_t") == 0);
    assert(c.priority == WM_ROPE_SHAKE_PRIORITY);
    assert(c.lane == 0xffu);

    assert(wm_rope_resolve_command(
        WM_ROPE_BACK, WM_ROPE_BOUNCE_UD, 3u, 0, &c));
    assert(strcmp(c.source_script_table, "back_bounceud4_t") == 0);

    /* front/back command_table contains zero for every other action */
    assert(!wm_rope_resolve_command(
        WM_ROPE_FRONT, WM_ROPE_BOUNCE_IO, 0u, 0, &c));

    assert(wm_rope_resolve_command(
        WM_ROPE_LEFT, WM_ROPE_BOUNCE_IO, 0u, 0, &c));
    assert(strcmp(c.source_script_table, "side_bounceio_t") == 0);

    assert(wm_rope_resolve_command(
        WM_ROPE_LEFT, WM_ROPE_SIDE_SPRING, 1u,
        (1151 << 16), &c));
    assert(c.lane == 2u);
    assert(strcmp(c.source_script_table, "sspr32_t") == 0);
    assert(c.priority == WM_ROPE_SIDE_SPRING_PRIORITY);

    /* sixth sspring entry is NULL in every source lane */
    assert(!wm_rope_resolve_command(
        WM_ROPE_RIGHT, WM_ROPE_SIDE_SPRING, 5u,
        (1100 << 16), &c));

    assert(wm_rope_resolve_command(
        WM_ROPE_RIGHT, WM_ROPE_DOWN_SPRING, 5u,
        (1300 << 16), &c));
    assert(c.lane == 4u);
    assert(strcmp(c.source_script_table, "dspr56_t") == 0);
    assert(c.priority == WM_ROPE_DOWN_SPRING_PRIORITY);

    assert(wm_rope_resolve_command(
        WM_ROPE_LEFT, WM_ROPE_SIDE_SPRING_RELEASE, 99u, 0, &c));
    assert(strcmp(c.source_script_table, "sspr_trans_t") == 0);

    assert(wm_rope_resolve_command(
        WM_ROPE_LEFT, WM_ROPE_DOWN_SPRING_RELEASE, 99u, 0, &c));
    assert(strcmp(c.source_script_table, "dspr_trans_t") == 0);
}

static void test_priority_and_z(void)
{
    assert(wm_rope_priority_accepts(5u, 5u));
    assert(wm_rope_priority_accepts(5u, 9u));
    assert(!wm_rope_priority_accepts(10u, 9u));

    assert(wm_rope_second_half_z(0x1234u, WM_ROPE_Z_HIGH)
           == 0x15a9u);
    assert(wm_rope_second_half_z(0x1234u, WM_ROPE_Z_NORM)
           == 0x1234u);
}

static void test_player_map(void)
{
    WmRingPlayerFields p = {0};

    p.inring = 0;
    p.climbing_thru = 1;
    p.player_mode = WM_RING_MODE_BOUNCING;

    assert(p.inring == 0);
    assert(p.climbing_thru == 1);
    assert(p.player_mode == 5);
    assert(WM_RING_MODE_ONTURNBUCKLE == 6);
    assert(WM_RING_MODE_CLIMBTURNBUCKLE == 11);
}

int main(void)
{
    test_geometry();
    test_lanes();
    test_command_routing();
    test_priority_and_z();
    test_player_map();

    puts("wmania_ring_chunk1 tests: PASS");
    return 0;
}
