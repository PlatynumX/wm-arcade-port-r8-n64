#include "wm_arcade_combat.h"
#include "wm_arcade_wrestle_core.h"
#include "wm_arcade_wrestle_input.h"
#include "wm_arcade_movement.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct Probe {
    int hit_count;
    int gidd_up_count;
    int live_teammates;
} Probe;

static void actor_init(wm_arcade_actor_t *a, int player_num, int side)
{
    memset(a, 0, sizeof(*a));
    a->active = 1;
    a->player_num = player_num;
    a->player_side = side;
    a->player_mode = WM_PMODE_NORMAL;
    a->attack_mode = WM_AMODE_PUNCH;
    a->facing_dir = WM_MOVE_RIGHT;
    a->new_facing_dir = WM_MOVE_RIGHT;
    a->in_ring = 1;
    a->life = 100;
}

static int live_teammates_cb(const wm_arcade_actor_t *victim, void *user)
{
    Probe *p = (Probe *)user;
    assert(victim != 0);
    return p->live_teammates;
}

static void wrestler_hit_cb(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim, void *user)
{
    Probe *p = (Probe *)user;
    assert(attacker != 0);
    assert(victim != 0);
    ++p->hit_count;
}

static void maybe_gidd_up_cb(wm_arcade_actor_t *victim, void *user)
{
    Probe *p = (Probe *)user;
    assert(victim != 0);
    ++p->gidd_up_count;
}

static wm_arcade_combat_callbacks_t combat_callbacks(Probe *p)
{
    wm_arcade_combat_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.victim_has_live_teammates = live_teammates_cb;
    cb.wrestler_hit = wrestler_hit_cb;
    cb.maybe_gidd_up = maybe_gidd_up_cb;
    cb.user = p;
    return cb;
}

static void test_hurt_and_attack_boxes(void)
{
    wm_arcade_actor_t a;
    wm_arcade_frame_box_t f;
    actor_init(&a, 0, 0);
    a.x_int = 100;
    a.y_int = 80;
    a.z_int = 200;
    f.iani3x = 10;
    f.iani3y = 40;
    f.iani3z = 30;
    f.iani3id = 60;

    wm_arcade_set_hurt_box(&a, &f);
    assert(a.hurt_box.y2 == 40);
    assert(a.hurt_box.y1 == -20);
    assert(a.hurt_box.z1 == 170);
    assert(a.hurt_box.z2 == 230);
    assert(a.hurt_box.x1 == 110);
    assert(a.hurt_box.x2 == 140);

    a.obj_control = WM_OBJ_FLIPH;
    wm_arcade_set_hurt_box(&a, &f);
    assert(a.hurt_box.x1 == 60);
    assert(a.hurt_box.x2 == 90);

    a.player_mode = WM_PMODE_RUNNING;
    wm_arcade_set_hurt_box(&a, &f);
    assert(a.hurt_box.z1 == 195);
    assert(a.hurt_box.z2 == 205);

    a.player_mode = WM_PMODE_NORMAL;
    a.obj_control = 0;
    a.attack_xoff = 7;
    a.attack_yoff = 5;
    a.attack_zoff = -2;
    a.attack_width = 11;
    a.attack_height = 10;
    a.attack_depth = 4;
    wm_arcade_set_attack_box(&a);
    assert(a.attack_box.x1 == 107);
    assert(a.attack_box.x2 == 118);
    assert(a.attack_box.y1 == 85);
    assert(a.attack_box.y2 == 95);
    assert(a.attack_box.z1 == 198);
    assert(a.attack_box.z2 == 202);

    a.obj_control = WM_OBJ_FLIPH;
    wm_arcade_set_attack_box(&a);
    assert(a.attack_box.x1 == 82);
    assert(a.attack_box.x2 == 93);
}

static void make_overlapping_hit(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim)
{
    attacker->attack_box.x1 = 0;
    attacker->attack_box.x2 = 10;
    attacker->attack_box.y1 = 0;
    attacker->attack_box.y2 = 10;
    attacker->attack_box.z1 = 0;
    attacker->attack_box.z2 = 10;
    victim->hurt_box = attacker->attack_box;
}

static void test_attack_hit_accepts_and_rejects_source_gates(void)
{
    wm_arcade_actor_t a, b, c;
    Probe p;
    wm_arcade_combat_callbacks_t cb;
    memset(&p, 0, sizeof(p));
    cb = combat_callbacks(&p);
    actor_init(&a, 0, 0);
    actor_init(&b, 1, 1);
    actor_init(&c, 2, 1);
    a.x_int = 40;
    a.z_fixed = 10 << 16;
    b.x_int = 20;
    b.z_fixed = 20 << 16;
    make_overlapping_hit(&a, &b);
    a.anim_mode = WM_ARCADE_MODE_WAITHITOPP;
    a.ani_count = 99;
    a.ani_count2 = 88;

    assert(wm_arcade_try_attack_hit(&a, &b, &cb) == WM_HIT_ACCEPTED);
    assert(p.hit_count == 1);
    assert((a.anim_mode & WM_ARCADE_MODE_WAITHITOPP) == 0);
    assert(a.ani_count == 0);
    assert(a.ani_count2 == 0);
    assert((a.anim_mode & WM_ARCADE_MODE_STATUS) != 0);
    assert(a.hit_side == b.hit_side);
    assert(a.hit_blocker == 0);

    memset(&p, 0, sizeof(p));
    actor_init(&a, 0, 0);
    actor_init(&b, 1, 1);
    actor_init(&c, 2, 1);
    make_overlapping_hit(&a, &b);
    a.status_flags = WM_STATUS_SMART_ATTACK;
    a.smart_target = &c;
    assert(wm_arcade_try_attack_hit(&a, &b, &cb) == WM_HIT_REJECTED);
    assert(p.hit_count == 0);

    actor_init(&a, 0, 0);
    actor_init(&b, 1, 1);
    make_overlapping_hit(&a, &b);
    a.combo_count = 1;
    a.who_i_hit = &c;
    assert(wm_arcade_try_attack_hit(&a, &b, &cb) == WM_HIT_REJECTED);

    actor_init(&a, 0, 0);
    actor_init(&b, 1, 1);
    make_overlapping_hit(&a, &b);
    b.player_mode = WM_PMODE_BLOCK;
    assert(wm_arcade_try_attack_hit(&a, &b, &cb) == WM_HIT_ACCEPTED);
    assert(a.hit_blocker == 1);
}

static void test_collision_loop_exits_after_first_source_hit(void)
{
    wm_arcade_actor_t a, b, c;
    wm_arcade_actor_t *actors[3];
    Probe p;
    wm_arcade_combat_callbacks_t cb;
    memset(&p, 0, sizeof(p));
    cb = combat_callbacks(&p);
    actor_init(&a, 0, 0);
    actor_init(&b, 1, 1);
    actor_init(&c, 2, 1);
    a.anim_mode = WM_ARCADE_MODE_CHECKHIT;
    a.attack_xoff = 0;
    a.attack_yoff = 0;
    a.attack_zoff = 0;
    a.attack_width = 100;
    a.attack_height = 100;
    a.attack_depth = 100;
    a.x_int = a.y_int = a.z_int = 0;
    b.hurt_box.x1 = b.hurt_box.y1 = b.hurt_box.z1 = 0;
    b.hurt_box.x2 = b.hurt_box.y2 = b.hurt_box.z2 = 10;
    c.hurt_box = b.hurt_box;
    actors[0] = &a;
    actors[1] = &b;
    actors[2] = &c;
    assert(wm_arcade_check_wrestler_collisions(actors, 3u, 0u, &cb) == 1);
    assert(p.hit_count == 1);
}

static void test_overlap_collision_off_and_getup(void)
{
    wm_arcade_actor_t a, b;
    Probe p;
    wm_arcade_combat_callbacks_t cb;
    memset(&p, 0, sizeof(p));
    cb = combat_callbacks(&p);
    actor_init(&a, 0, 0);
    actor_init(&b, 1, 1);
    a.hurt_box.x1 = a.hurt_box.y1 = a.hurt_box.z1 = 0;
    a.hurt_box.x2 = a.hurt_box.y2 = a.hurt_box.z2 = 10;
    b.hurt_box.x1 = b.hurt_box.y1 = b.hurt_box.z1 = 5;
    b.hurt_box.x2 = b.hurt_box.y2 = b.hurt_box.z2 = 20;
    assert(wm_arcade_resolve_overlap(&a, &b) == 1);

    a.anim_mode = WM_ARCADE_MODE_CHECKHIT | WM_ARCADE_MODE_STATUS;
    wm_arcade_wrestler_collisions_off(&a);
    assert((a.anim_mode & WM_ARCADE_MODE_CHECKHIT) == 0);
    assert((a.anim_mode & WM_ARCADE_MODE_STATUS) != 0);

    actor_init(&a, 0, 0);
    actor_init(&b, 1, 1);
    a.attack_mode = WM_AMODE_FLYKICK;
    wm_arcade_set_getup_time(&a, &b, &cb);
    assert(b.getup_time == WM_STAY_TIME);
    assert(p.gidd_up_count == 1);

    memset(&p, 0, sizeof(p));
    actor_init(&a, 0, 0);
    actor_init(&b, 1, 1);
    b.delay_meter = 1;
    a.attack_mode = WM_AMODE_FLYKICK;
    wm_arcade_set_getup_time(&a, &b, &cb);
    assert(b.getup_time == WM_STAY_TIME);
    assert(p.gidd_up_count == 0);
}

static void test_wrestle_core_pin_countdown_and_reset(void)
{
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *actors[2];
    wm_arcade_auto_pin_env_t env;
    actor_init(&a, 0, 0);
    actor_init(&b, 1, 1);
    actors[0] = &a;
    actors[1] = &b;
    memset(&env, 0, sizeof(env));

    a.closest_num = 1;
    a.closest_dist = 0x40;
    a.closest_zdist = 0x20;
    b.player_mode = WM_PMODE_DEAD;
    b.status_flags = WM_STATUS_PINABLE | WM_STATUS_KOD;
    b.x_vel = 1;
    b.y_vel = 2;
    b.z_vel = 3;
    assert(wm_arcade_can_pin(&a, actors, 2u) == 1);
    assert((b.status_flags & WM_STATUS_PINNED) != 0u);
    assert((b.status_flags & WM_STATUS_KOD) == 0u);
    assert(b.who_pinned_me == &a);
    assert(b.x_vel == 0 && b.y_vel == 0 && b.z_vel == 0);
    assert(b.ptime == 1);

    actor_init(&a, 0, 0);
    actor_init(&b, 1, 1);
    b.player_mode = WM_PMODE_DEAD;
    for (unsigned i = 0; i < (WM_ARCADE_TSEC * 3u) - 1u; ++i)
        assert(wm_arcade_auto_pin_check(&a, &b, &env) == 0);
    assert(wm_arcade_auto_pin_check(&a, &b, &env) == 1);
    assert(a.player_type == WM_PTYPE_DRONE);

    actor_init(&a, 0, 0);
    a.getup_time = 10;
    a.but_val_down = WM_BTN_PUNCH;
    wm_arcade_wrestler_countdown_tail(&a, false);
    assert(a.getup_time == 6);
    assert((a.status_flags & WM_STATUS_PRESS_LAST) != 0u);

    a.status_flags = WM_STATUS_TEMP_PAL | WM_STATUS_KOD | WM_STATUS_DID_BUCKOFF;
    a.immobilize_time = 99;
    a.dizzy_count = 9;
    a.getup_time = 77;
    a.special_move_addr = 1234u;
    wm_arcade_reset_wrestle2_state(&a);
    assert(a.immobilize_time == 30);
    assert(a.dizzy_count == 0);
    assert(a.getup_time == 0);
    assert(a.special_move_addr == 0u);
    assert((a.status_flags & WM_STATUS_KOD) == 0u);
    assert((a.status_flags & WM_STATUS_TEMP_PAL) != 0u);
    assert((a.status_flags & WM_STATUS_DID_BUCKOFF) != 0u);
}

static void test_input_history_dtime_and_movement_helpers(void)
{
    wm_arcade_actor_t a;
    actor_init(&a, 0, 0);
    wm_arcade_wrestle_input_init(&a);
    a.facing_dir = WM_MOVE_RIGHT;
    a.stick_val_cur = WM_MOVE_RIGHT;
    a.stick_val_down = WM_MOVE_RIGHT;
    a.but_val_cur = WM_BTN_PUNCH | WM_BTN_SKICK;
    a.but_val_down = WM_BTN_PUNCH;
    wm_arcade_update_joystat(&a, 100u, false);
    assert(a.wrest_joystat[0] != 0u);
    wm_arcade_update_joy_dtime(&a);
    assert(wm_arcade_get_button_dtime(&a, WM_BTN_PUNCH) == 1u);
    assert(wm_arcade_get_button_dtime(&a, WM_BTN_SKICK) == 1u);
    a.but_val_cur = WM_BTN_PUNCH;
    wm_arcade_update_joy_dtime(&a);
    assert(wm_arcade_get_button_dtime(&a, WM_BTN_PUNCH) == 2u);
    assert(wm_arcade_get_button_dtime(&a, WM_BTN_SKICK) == 0u);

    assert(wm_arcade_xflip_joy(WM_MOVE_LEFT) == WM_MOVE_RIGHT);
    assert(wm_arcade_xflip_joy(WM_MOVE_RIGHT) == WM_MOVE_LEFT);
    assert(wm_arcade_stick_relative(WM_MOVE_LEFT, true) == WM_MOVE_LEFT);
    assert(wm_arcade_stick_relative(WM_MOVE_LEFT, false) == WM_MOVE_RIGHT);
    assert(wm_arcade_stick_relative_new(WM_MOVE_RIGHT, 0u, 0u, true) == 0u);
    assert(wm_arcade_stick_relative_new(WM_MOVE_RIGHT, WM_MOVE_RIGHT, 0u, true) == WM_MOVE_RIGHT);
}

int main(void)
{
    test_hurt_and_attack_boxes();
    test_attack_hit_accepts_and_rejects_source_gates();
    test_collision_loop_exits_after_first_source_hit();
    test_overlap_collision_off_and_getup();
    test_wrestle_core_pin_countdown_and_reset();
    test_input_history_dtime_and_movement_helpers();
    puts("Combat non-SMOVE core regression: PASS");
    return 0;
}
