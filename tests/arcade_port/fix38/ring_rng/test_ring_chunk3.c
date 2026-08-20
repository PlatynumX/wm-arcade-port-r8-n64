#include "wmania_ring_climb.h"
#include "wmania_ring_out.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    bool kill;
    unsigned calls;
    int last_delta;
    unsigned quirk_calls;
    size_t quirk_slot;
    int16_t quirk_value;
    bool quirk_valid;
} TestCtx;

static bool health_cb(
    void *user,
    uint8_t player_num,
    int16_t delta,
    int16_t a10_zero)
{
    TestCtx *c = (TestCtx *)user;
    (void)player_num;
    assert(a10_zero == 0);
    ++c->calls;
    c->last_delta = delta;
    return c->kill;
}

static bool quirk_cb(
    void *user,
    size_t next_process_slot,
    int16_t *value_out)
{
    TestCtx *c = (TestCtx *)user;
    ++c->quirk_calls;
    c->quirk_slot = next_process_slot;
    if (!c->quirk_valid) return false;
    *value_out = c->quirk_value;
    return true;
}

static WmRingClimbPlayer base_climber(void)
{
    WmRingClimbPlayer p;
    memset(&p, 0, sizeof(p));
    p.active = true;
    p.wrestler_num = 0;
    p.player_side = 0;
    p.player_mode = WM_RING_MODE_NORMAL;
    p.inring = 0;
    p.x_int = WM_RING_X_CENTER;
    p.z_int = WM_RING_Z_CENTER;
    p.coll_x1 = WM_RING_X_CENTER - 10;
    p.coll_x2 = WM_RING_X_CENTER + 10;
    p.facing_dir = WM_RING_MOVE_DOWN_LEFT;
    return p;
}

static void test_idiot_check(void)
{
    WmRingClimbPlayer p = base_climber();

    /* first observation starts a new continuous sequence */
    assert(!wm_ring_idiot_check(&p, 100u));
    assert(p.climb_start == 100u && p.climb_last == 100u);

    /* duplicate call in same tick must never advance */
    assert(!wm_ring_idiot_check(&p, 100u));
    assert(p.climb_last == 100u);

    for (uint32_t t = 101u; t < 121u; ++t) {
        assert(!wm_ring_idiot_check(&p, t));
        assert(!wm_ring_idiot_check(&p, t)); /* 2nd call same tick */
    }

    assert(wm_ring_idiot_check(&p, 121u));
}

static void test_any_opp_outside(void)
{
    WmRingClimbPlayer p[4];
    size_t next = 99u;

    for (unsigned i = 0; i < 4; ++i) p[i] = base_climber();

    p[0].player_side = 0;
    p[1].player_side = 0; /* teammate */
    p[1].inring = 1;
    p[2].player_side = 1;
    p[2].player_mode = WM_RING_MODE_DEAD; /* dead opponent skipped */
    p[2].inring = 1;
    p[3].player_side = 1;
    p[3].inring = 1;

    assert(wm_ring_any_opp_outside(&p[0], p, 4u, &next));
    assert(next == 4u); /* source a0 post-incremented past index 3 */

    p[3].inring = 0;
    assert(!wm_ring_any_opp_outside(&p[0], p, 4u, &next));
}

static void test_turnbuckle(void)
{
    WmRingClimbPlayer p[2];
    WmRingClimbResult r;

    p[0] = base_climber();
    p[1] = base_climber();

    p[0].x_int = WM_RING_X_CENTER - 20;
    p[0].z_int = WM_RING_TOP + 5;
    p[0].z_fp16 = ((int32_t)p[0].z_int) << 16;
    p[0].coll_x1 = 850;
    p[0].stick_val_cur = WM_RING_MOVE_UP_LEFT;
    p[0].facing_dir = WM_RING_MOVE_DOWN_RIGHT; /* Bret flips to opposite */
    p[0].wrestler_num = 0;

    p[1].active = false;

    r = wm_ring_climb_turnbuckle(&p[0], p, 2u, 845);
    assert(r.action == WM_RING_CLIMB_ACTION_START_ANIMATION);
    assert(strcmp(r.source_animation_label, "hrt_climb_up_anim") == 0);
    assert(p[0].player_mode == WM_RING_MODE_CLIMBTURNBUCKLE);
    assert(p[0].z_int == WM_RING_TOP);
    assert(p[0].new_facing_dir == WM_RING_MOVE_DOWN_RIGHT);

    /* same-side turnbuckle occupancy blocks */
    p[0] = base_climber();
    p[1] = base_climber();
    p[0].x_int = WM_RING_X_CENTER - 20;
    p[0].z_int = WM_RING_TOP;
    p[0].coll_x1 = 850;
    p[0].stick_val_cur = WM_RING_MOVE_UP_LEFT;
    p[1].x_int = WM_RING_X_CENTER - 5;
    p[1].player_mode = WM_RING_MODE_ONTURNBUCKLE;
    r = wm_ring_climb_turnbuckle(&p[0], p, 2u, 850);
    assert(r.action == WM_RING_CLIMB_ACTION_NONE);
}

static void test_top_bottom_climbs(void)
{
    WmRingClimbPlayer players[2];
    WmRingClimbResult r;

    players[0] = base_climber();
    players[1] = base_climber();
    players[1].player_side = 1;
    players[1].inring = 1;

    players[0].x_int = WM_RING_X_CENTER;
    players[0].z_fp16 = ((int32_t)WM_RING_BOT) << 16;
    players[0].but_val_cur = 1u;
    r = wm_ring_ck_climb_out_bot(&players[0], players, 2u, 10u);
    assert(r.action == WM_RING_CLIMB_ACTION_START_ANIMATION);
    assert(strcmp(r.source_animation_label, "hrt_climbthru_bot_anim") == 0);
    assert(players[0].climbing_thru == 2);

    players[0] = base_climber();
    players[0].z_fp16 = ((int32_t)(WM_MAT_TOP - 5)) << 16;
    players[0].move_dir = 1u << WM_RING_MOVE_DOWN_BIT;
    players[0].but_val_cur = 1u;
    r = wm_ring_ck_climb_in_top(&players[0], 20u);
    assert(r.action == WM_RING_CLIMB_ACTION_START_ANIMATION);
    assert(strcmp(r.source_animation_label, "hrt_climbin_top_anim") == 0);

    players[0] = base_climber();
    players[0].z_fp16 = ((int32_t)(WM_MAT_BOT + 5)) << 16;
    players[0].move_dir = 1u << WM_RING_MOVE_UP_BIT;
    players[0].but_val_cur = 1u;
    r = wm_ring_ck_climb_in_bot(&players[0], 30u);
    assert(r.action == WM_RING_CLIMB_ACTION_START_ANIMATION);
    assert(strcmp(r.source_animation_label, "hrt_climbin_bot_anim") == 0);
}

static void test_zombie_roll_top(void)
{
    WmRingClimbPlayer p[2];
    WmRingClimbResult r;

    p[0] = base_climber();
    p[1] = base_climber();
    p[0].status_flags = WM_RING_STATUS_ZOMBIE;
    p[0].player_mode = WM_RING_MODE_DEAD; /* bypassed by zombie path */
    p[0].wrestler_num = 8;

    r = wm_ring_ck_climb_out_top(&p[0], p, 2u, 10u);
    assert(r.action == WM_RING_CLIMB_ACTION_START_ANIMATION);
    assert(strcmp(r.source_animation_label, "lex_rollthru_top_anim") == 0);
    assert(p[0].climbing_thru == 2);

    p[0].animbase_label = "lex_rollthru_top_anim";
    r = wm_ring_ck_climb_out_top(&p[0], p, 2u, 11u);
    assert(r.action == WM_RING_CLIMB_ACTION_NONE);
}

static void test_side_out_source_quirk(void)
{
    WmRingClimbPlayer p[2];
    WmRingClimbResult r;
    TestCtx ctx = {0};

    p[0] = base_climber();
    p[1] = base_climber();

    p[0].x_int = WM_RING_X_CENTER - 30;
    p[0].z_int = WM_RING_Z_CENTER;
    p[0].coll_x1 = 900;
    p[0].stick_val_cur = 1u << WM_RING_MOVE_LEFT_BIT;
    p[0].but_val_cur = 1u;
    p[0].facing_dir = WM_RING_MOVE_DOWN_LEFT;

    p[1].player_side = 1;
    p[1].inring = 1;

    /* no adapter = explicitly unresolved source quirk */
    r = wm_ring_ck_climb_out_side(
        &p[0], p, 2u, 100u, 905, 0, 0);
    assert(r.action == WM_RING_CLIMB_ACTION_SOURCE_QUIRK_INPUT_REQUIRED);

    ctx.quirk_valid = true;
    ctx.quirk_value = 1;
    r = wm_ring_ck_climb_out_side(
        &p[0], p, 2u, 101u, 905, quirk_cb, &ctx);
    assert(r.action == WM_RING_CLIMB_ACTION_NONE);
    assert(ctx.quirk_slot == 2u);

    ctx.quirk_value = 0;
    r = wm_ring_ck_climb_out_side(
        &p[0], p, 2u, 102u, 905, quirk_cb, &ctx);
    assert(r.action == WM_RING_CLIMB_ACTION_START_ANIMATION);
    assert(strcmp(r.source_animation_label, "hrt_climbthru_side_anim") == 0);
    assert(p[0].climbing_thru == 1);
    assert(p[0].player_mode == WM_RING_MODE_NORMAL);
}

static void test_side_in_running_and_rotate(void)
{
    WmRingClimbPlayer p = base_climber();
    WmRingClimbResult r;

    p.x_int = WM_RING_X_CENTER + 30; /* right of center -> move left */
    p.z_int = WM_RING_Z_CENTER;
    p.coll_x1 = 1000;
    p.move_dir = 1u << WM_RING_MOVE_LEFT_BIT;
    p.player_mode = WM_RING_MODE_RUNNING;
    p.getup_time = 0;
    p.but_val_cur = 0; /* running path does not require button */
    p.facing_dir = WM_RING_MOVE_DOWN_RIGHT; /* wrong target */

    r = wm_ring_ck_climb_in_side(&p, 200u, 1005);
    assert(r.action == WM_RING_CLIMB_ACTION_ROTATE_THEN_CONTINUE);
    assert(r.continuation == WM_RING_CLIMB_CONT_IN_SIDE);
    assert(r.target_facing == WM_RING_MOVE_DOWN_LEFT);
    assert(p.player_mode == WM_RING_MODE_WAITANIM);
    assert(p.climbing_thru == 1);

    r = wm_ring_climb_continue(&p, r.continuation);
    assert(r.action == WM_RING_CLIMB_ACTION_START_ANIMATION);
    assert(strcmp(r.source_animation_label, "hrt_climbin_side_anim") == 0);
    assert(p.player_mode == WM_RING_MODE_NORMAL);
}

static WmRingOutPlayer base_out_player(void)
{
    WmRingOutPlayer p;
    memset(&p, 0, sizeof(p));
    p.active = true;
    p.player_mode = WM_RING_MODE_NORMAL;
    p.inring = 0;
    p.ring_time = 1;
    p.ptime = 1;
    return p;
}

static void test_ring_time_transitions(void)
{
    WmRingOutPlayer p[2];
    WmRingOutEvents e;

    p[0] = base_out_player();
    p[1] = base_out_player();
    p[0].closest_num = 1;
    p[1].player_side = 1;

    /* inside stays positive and increments */
    p[0].inring = 0;
    p[0].ring_time = 5;
    e = wm_ring_are_we_in_ring_tick(
        &p[0], p, 2u, 1u, 60u, false, false, 0, 0);
    assert(p[0].ring_time == 6);
    assert(!e.spawn_kill_when_hit_ground);

    /* crossing outside forces -1 and ring_out_on spawns ground-hit helper */
    p[0].ring_time = 6;
    p[0].inring = 1;
    e = wm_ring_are_we_in_ring_tick(
        &p[0], p, 2u, 2u, 60u, false, true, 0, 0);
    assert(p[0].ring_time == -1);
    assert(e.spawn_kill_when_hit_ground);

    /* crossing back inside forces +1 */
    p[0].ring_time = -50;
    p[0].inring = 0;
    e = wm_ring_are_we_in_ring_tick(
        &p[0], p, 2u, 3u, 60u, false, false, 0, 0);
    assert(p[0].ring_time == 1);
}

static void test_dufus(void)
{
    WmRingOutPlayer p[2];

    p[0] = base_out_player();
    p[1] = base_out_player();
    p[0].player_side = 0;
    p[1].player_side = 1;

    p[0].ring_time = -241; /* below -4*60 */
    p[1].ring_time = 241;  /* above +4*60 */

    assert(wm_ring_do_ringout_dufus(&p[0], p, 2u, 60u, false));
    assert(!wm_ring_do_ringout_dufus(&p[0], p, 2u, 60u, true));

    p[1].ring_time = 240;
    assert(!wm_ring_do_ringout_dufus(&p[0], p, 2u, 60u, false));
}

static void test_ringout_damage_and_death(void)
{
    WmRingOutPlayer p[3];
    WmRingOutEvents e;
    TestCtx ctx = {0};

    p[0] = base_out_player();
    p[1] = base_out_player();
    p[2] = base_out_player();

    p[0].player_num = 0;
    p[0].player_side = 0;
    p[0].closest_num = 1;
    p[0].inring = 1;
    p[0].ring_time = -421; /* tick makes -422, beyond -7*60 */

    p[1].player_side = 1;
    p[1].ring_time = 421;  /* safely > +7*60 */
    p[1].ptime = 1;

    /* teammate inactive for first death case */
    p[2].active = false;
    p[2].player_side = 0;

    ctx.kill = true;
    e = wm_ring_are_we_in_ring_tick(
        &p[0], p, 3u, 8u, 60u, false, false, health_cb, &ctx);

    assert(e.health_adjusted && e.health_delta == -1);
    assert(ctx.calls == 1u && ctx.last_delta == -1);
    assert(e.death_by_ringout);
    assert(p[0].player_mode == WM_RING_MODE_DEAD);
    assert(e.create_disqual);
    assert(e.announce_round_winner);

    /* living teammate suppresses disqual */
    p[0] = base_out_player();
    p[0].player_num = 0;
    p[0].player_side = 0;
    p[0].closest_num = 1;
    p[0].inring = 1;
    p[0].ring_time = -421;

    p[1] = base_out_player();
    p[1].player_side = 1;
    p[1].ring_time = 421;
    p[1].ptime = 1;

    p[2] = base_out_player();
    p[2].player_side = 0;
    p[2].player_mode = WM_RING_MODE_NORMAL;

    ctx.calls = 0;
    e = wm_ring_are_we_in_ring_tick(
        &p[0], p, 3u, 16u, 60u, false, false, health_cb, &ctx);
    assert(e.death_by_ringout);
    assert(!e.create_disqual);

    /* dead zombie teammate also suppresses */
    p[0].player_mode = WM_RING_MODE_NORMAL;
    p[0].ring_time = -421;
    p[2].player_mode = WM_RING_MODE_DEAD;
    p[2].status_flags = WM_RING_STATUS_ZOMBIE;
    e = wm_ring_are_we_in_ring_tick(
        &p[0], p, 3u, 24u, 60u, false, false, health_cb, &ctx);
    assert(e.death_by_ringout);
    assert(!e.create_disqual);
}

static void test_sleeping_drone_hack(void)
{
    WmRingOutPlayer p[2];
    WmRingOutEvents e;
    TestCtx ctx = {0};

    p[0] = base_out_player();
    p[1] = base_out_player();

    p[0].player_side = 0;
    p[0].closest_num = 1;
    p[0].inring = 1;
    p[0].ring_time = -421;

    p[1].player_side = 1;
    p[1].ring_time = 0;
    p[1].ptime = 1;

    /* no sleep adjustment: opponent <= 420 suppresses damage */
    e = wm_ring_are_we_in_ring_tick(
        &p[0], p, 2u, 8u, 60u, false, false, health_cb, &ctx);
    assert(!e.health_adjusted);

    p[0].ring_time = -421;
    p[0].player_mode = WM_RING_MODE_NORMAL;
    p[1].ptime = 0x7000; /* adds 0x0fff, making adjusted safely >420 */

    e = wm_ring_are_we_in_ring_tick(
        &p[0], p, 2u, 16u, 60u, false, false, health_cb, &ctx);
    assert(e.health_adjusted);
}

static void test_dead_path_and_ground_hit(void)
{
    WmRingOutPlayer p[2];
    WmRingOutEvents e;
    TestCtx ctx = {0};

    p[0] = base_out_player();
    p[1] = base_out_player();
    p[0].player_mode = WM_RING_MODE_DEAD;
    p[0].ring_time = -50;
    e = wm_ring_are_we_in_ring_tick(
        &p[0], p, 2u, 0u, 60u, false, false, 0, 0);
    assert(p[0].ring_time == 1);
    assert(!e.health_adjusted);

    p[0].ring_time = 4;
    e = wm_ring_are_we_in_ring_tick(
        &p[0], p, 2u, 0u, 60u, false, false, 0, 0);
    assert(p[0].ring_time == 5);
    assert(!e.health_adjusted);

    p[0].ground_y = 62;
    p[0].object_y_int = 61;
    assert(!wm_ring_kill_when_hit_ground_ready(&p[0]));
    p[0].object_y_int = 62;
    assert(wm_ring_kill_when_hit_ground_ready(&p[0]));
    assert(wm_ring_kill_when_hit_ground_apply(&p[0], health_cb, &ctx));
    assert(ctx.last_delta == -150);
}

int main(void)
{
    test_idiot_check();
    test_any_opp_outside();
    test_turnbuckle();
    test_top_bottom_climbs();
    test_zombie_roll_top();
    test_side_out_source_quirk();
    test_side_in_running_and_rotate();
    test_ring_time_transitions();
    test_dufus();
    test_ringout_damage_and_death();
    test_sleeping_drone_hack();
    test_dead_path_and_ground_hit();

    puts("wmania_ring_chunk3 tests: PASS");
    return 0;
}
