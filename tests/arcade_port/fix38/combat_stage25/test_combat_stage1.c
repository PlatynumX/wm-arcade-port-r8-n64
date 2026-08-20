#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wm_arcade_combat.h"
#include "wm_arcade_damage.h"

static int hit_count;
static int gidd_count;

static int no_live_teammates(const wm_arcade_actor_t *v, void *u)
{
    (void)v; (void)u; return 0;
}
static void count_hit(wm_arcade_actor_t *a, wm_arcade_actor_t *v, void *u)
{
    (void)a; (void)v; (void)u; ++hit_count;
}
static void count_gidd(wm_arcade_actor_t *v, void *u)
{
    (void)v; (void)u; ++gidd_count;
}

static wm_arcade_combat_callbacks_t callbacks(void)
{
    wm_arcade_combat_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.victim_has_live_teammates = no_live_teammates;
    cb.wrestler_hit = count_hit;
    cb.maybe_gidd_up = count_gidd;
    return cb;
}

static wm_arcade_actor_t base_actor(void)
{
    wm_arcade_actor_t a;
    memset(&a, 0, sizeof(a));
    a.active = 1;
    a.in_ring = 0;
    a.player_mode = WM_PMODE_NORMAL;
    return a;
}

static void test_hurt_boxes(void)
{
    wm_arcade_actor_t a = base_actor();
    wm_arcade_frame_box_t f = { 10, 20, 40, 60 };
    a.x_int = 100; a.y_int = 200; a.z_int = 300;
    wm_arcade_set_hurt_box(&a, &f);
    assert(a.hurt_box.x1 == 110 && a.hurt_box.x2 == 150);
    assert(a.hurt_box.y2 == 180 && a.hurt_box.y1 == 120);
    assert(a.hurt_box.z1 == 270 && a.hurt_box.z2 == 330);

    a.obj_control = WM_OBJ_FLIPH;
    a.player_mode = WM_PMODE_RUNNING;
    wm_arcade_set_hurt_box(&a, &f);
    assert(a.hurt_box.x2 == 90 && a.hurt_box.x1 == 50);
    assert(a.hurt_box.z1 == 295 && a.hurt_box.z2 == 305);
}

static void test_attack_box_and_hit(void)
{
    wm_arcade_actor_t a = base_actor();
    wm_arcade_actor_t v = base_actor();
    wm_arcade_combat_callbacks_t cb = callbacks();

    a.x_int = 100; a.y_int = 200; a.z_int = 300; a.z_fixed = 300 << 16;
    a.attack_xoff = 10; a.attack_yoff = -20; a.attack_zoff = -5;
    a.attack_width = 40; a.attack_height = 50; a.attack_depth = 10;
    a.anim_mode = WM_MODE_CHECKHIT | WM_MODE_WAITHITOPP;
    a.ani_count = 9; a.ani_count2 = 8;

    wm_arcade_set_attack_box(&a);
    assert(a.attack_box.x1 == 110 && a.attack_box.x2 == 150);
    assert(a.attack_box.y1 == 180 && a.attack_box.y2 == 230);
    assert(a.attack_box.z1 == 295 && a.attack_box.z2 == 305);

    /* Edge contact counts because source rejects only < and >. */
    v.hurt_box.x1 = 150; v.hurt_box.x2 = 180;
    v.hurt_box.y1 = 180; v.hurt_box.y2 = 230;
    v.hurt_box.z1 = 295; v.hurt_box.z2 = 305;
    v.x_int = 160; v.z_fixed = 310 << 16;

    hit_count = 0;
    assert(wm_arcade_try_attack_hit(&a, &v, &cb) == WM_HIT_ACCEPTED);
    assert(hit_count == 1);
    assert((a.anim_mode & WM_MODE_STATUS) != 0);
    assert((a.anim_mode & WM_MODE_WAITHITOPP) == 0);
    assert(a.ani_count == 0 && a.ani_count2 == 0);
    assert(a.hit_side == (WM_MOVE_LEFT | WM_MOVE_UP));
}

static void test_rejections(void)
{
    wm_arcade_actor_t a = base_actor();
    wm_arcade_actor_t v = base_actor();
    wm_arcade_actor_t other = base_actor();
    wm_arcade_combat_callbacks_t cb = callbacks();

    a.attack_box = (wm_arcade_box3_t){0,10,0,10,0,10};
    v.hurt_box = (wm_arcade_box3_t){0,10,0,10,0,10};

    a.status_flags = WM_STATUS_SMART_ATTACK;
    a.smart_target = &other;
    assert(wm_arcade_try_attack_hit(&a, &v, &cb) == WM_HIT_REJECTED);

    a.status_flags = 0;
    a.attack_mode = WM_AMODE_PUSH;
    v.player_mode = WM_PMODE_INAIR;
    assert(wm_arcade_try_attack_hit(&a, &v, &cb) == WM_HIT_REJECTED);

    a.attack_mode = WM_AMODE_PUNCH;
    v.player_mode = WM_PMODE_NORMAL;
    v.status_flags = WM_STATUS_PUSH;
    assert(wm_arcade_try_attack_hit(&a, &v, &cb) == WM_HIT_REJECTED);

    a.attack_mode = WM_AMODE_FLYKICK;
    assert(wm_arcade_try_attack_hit(&a, &v, &cb) == WM_HIT_ACCEPTED);
}

static void test_getup(void)
{
    wm_arcade_actor_t a = base_actor();
    wm_arcade_actor_t v = base_actor();
    wm_arcade_combat_callbacks_t cb = callbacks();

    a.attack_mode = WM_AMODE_FLYKICK;
    gidd_count = 0;
    wm_arcade_set_getup_time(&a, &v, &cb);
    assert(v.getup_time == WM_STAY_TIME);
    assert(gidd_count == 1);

    v.getup_time = 99;
    a.attack_mode = WM_AMODE_PUNCH;
    wm_arcade_set_getup_time(&a, &v, &cb);
    assert(v.getup_time == 99);
}


static wm_arcade_actor_t *last_hit_attacker;

static void record_attacker(wm_arcade_actor_t *a, wm_arcade_actor_t *v, void *u)
{
    (void)v; (void)u; last_hit_attacker = a;
}

static void test_overlap_resolution(void)
{
    wm_arcade_actor_t a = base_actor();
    wm_arcade_actor_t b = base_actor();

    a.hurt_box = (wm_arcade_box3_t){0,10,0,10,0,10};
    b.hurt_box = (wm_arcade_box3_t){8,18,0,10,2,12};
    a.x_int = 100;
    a.z_int = 200;
    a.move_dir = 0;

    /* X penetration is 2, Z penetration is 8: source resolves X. */
    assert(wm_arcade_resolve_overlap(&a, &b) == 1);
    assert(a.x_int == 98);
    assert(a.z_int == 200);

    a = base_actor();
    b = base_actor();
    a.hurt_box = (wm_arcade_box3_t){0,10,0,10,0,10};
    b.hurt_box = (wm_arcade_box3_t){2,12,0,10,8,18};
    a.x_int = 100;
    a.z_int = 200;

    /* Z penetration is 2, X penetration is 8: source resolves Z. */
    assert(wm_arcade_resolve_overlap(&a, &b) == 1);
    assert(a.x_int == 100);
    assert(a.z_int == 198);
}

static void test_alternating_attacker_priority(void)
{
    wm_arcade_actor_t a = base_actor();
    wm_arcade_actor_t b = base_actor();
    wm_arcade_actor_t v = base_actor();
    wm_arcade_actor_t *actors[3] = { &a, &b, &v };
    wm_arcade_combat_callbacks_t cb = callbacks();

    cb.wrestler_hit = record_attacker;

    a.anim_mode = WM_MODE_CHECKHIT;
    b.anim_mode = WM_MODE_CHECKHIT;
    a.attack_width = b.attack_width = 10;
    a.attack_height = b.attack_height = 10;
    a.attack_depth = b.attack_depth = 10;
    a.attack_xoff = b.attack_xoff = 0;
    a.attack_yoff = b.attack_yoff = 0;
    a.attack_zoff = b.attack_zoff = 0;
    a.x_int = b.x_int = 0;
    a.y_int = b.y_int = 0;
    a.z_int = b.z_int = 0;
    v.hurt_box = (wm_arcade_box3_t){0,10,0,10,0,10};

    last_hit_attacker = NULL;
    assert(wm_arcade_check_wrestler_collisions(actors, 3, 0, &cb) == 1);
    assert(last_hit_attacker == &a);

    a.anim_mode = WM_MODE_CHECKHIT;
    b.anim_mode = WM_MODE_CHECKHIT;
    v.anim_mode = 0;
    last_hit_attacker = NULL;
    assert(wm_arcade_check_wrestler_collisions(actors, 3, 1, &cb) == 1);
    assert(last_hit_attacker == &b);
}

static void test_damage_constants(void)
{
    assert(WM_D_PUNCH == 8);
    assert(WM_D_FLYKICK == 28);
    assert(WM_D_BSLAM == 27);
    assert(WM_D_PILEDRIVER == 33);
    assert(WM_RD_SPDKIK == 3);
}

int main(void)
{
    test_hurt_boxes();
    test_attack_box_and_hit();
    test_rejections();
    test_getup();
    test_overlap_resolution();
    test_alternating_attacker_priority();
    test_damage_constants();
    puts("combat stage1 tests: PASS");
    return 0;
}
