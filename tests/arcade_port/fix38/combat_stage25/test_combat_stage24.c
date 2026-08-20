#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_special.h"
#include "wm_arcade_damage.h"

#define FX16(n) ((int32_t)((n) * 65536))

struct ctx {
    int adjust_calls;
    int last_delta;
    int sound_calls;
    wm_arcade_react1_sound_t last_sound;
    int anim_calls;
    wm_arcade_react1_anim_group_t last_anim;
    int breakout_calls;
    int ditch_calls;
};

static void adjust_cb(wm_arcade_actor_t *victim, int16_t delta,
                      wm_arcade_actor_t *source, void *user)
{
    struct ctx *c = (struct ctx *)user;
    (void)source;
    c->adjust_calls++;
    c->last_delta = delta;
    victim->life += delta;
    if (victim->life < 0) victim->life = 0;
}
static void sound_cb(wm_arcade_actor_t *v, wm_arcade_react1_sound_t s, void *u)
{ (void)v; ((struct ctx *)u)->sound_calls++; ((struct ctx *)u)->last_sound = s; }
static void anim_cb(wm_arcade_actor_t *v, wm_arcade_react1_anim_group_t a, void *u)
{ (void)v; ((struct ctx *)u)->anim_calls++; ((struct ctx *)u)->last_anim = a; }
static void breakout_cb(wm_arcade_actor_t *p, wm_arcade_partner_breakout_t k, void *u)
{ (void)p; (void)k; ((struct ctx *)u)->breakout_calls++; }
static void ditch_cb(wm_arcade_actor_t *v, void *u)
{ (void)v; ((struct ctx *)u)->ditch_calls++; }

static void actor_init(wm_arcade_actor_t *a, int x, int y, int z, int side)
{
    memset(a, 0, sizeof(*a));
    a->active = 1;
    a->player_mode = WM_PMODE_NORMAL;
    a->life = 100;
    a->x_int = x; a->y_int = y; a->z_int = z;
    a->x_fixed = FX16(x); a->y_fixed = FX16(y); a->z_fixed = FX16(z);
    a->ground_y = 0;
    a->player_side = side;
    a->hurt_box.x1 = x - 10; a->hurt_box.x2 = x + 10;
    a->hurt_box.y1 = y - 10; a->hurt_box.y2 = y + 10;
    a->hurt_box.z1 = z - 10; a->hurt_box.z2 = z + 10;
}

static wm_arcade_special_callbacks_t callbacks(struct ctx *c,
                                                 wm_arcade_react_callbacks_t *r,
                                                 wm_arcade_react1_callbacks_t *r1,
                                                 wm_arcade_react1_context_t *r1ctx)
{
    wm_arcade_special_callbacks_t sc;
    memset(r, 0, sizeof(*r));
    memset(r1, 0, sizeof(*r1));
    r->adjust_health = adjust_cb;
    r->partner_breakout = breakout_cb;
    r->ditch_getup_meter = ditch_cb;
    r->user = c;
    r1->play_sound = sound_cb;
    r1->change_anim = anim_cb;
    r1->user = c;
    wm_arcade_react1_context_init(r1ctx, r1);
    sc.react = r;
    sc.react1 = r1ctx;
    return sc;
}

static void test_lists_and_spawn_state(void)
{
    wm_arcade_special_lists_t l;
    wm_arcade_special_obj_t pie, fire, spirit, reaper, salt;
    wm_arcade_actor_t p1, p2;
    actor_init(&p1, 100, 20, 30, 0);
    actor_init(&p2, 200, 20, 30, 1);
    p2.obj_control = WM_OBJ_FLIPH;
    wm_arcade_special_lists_init(&l);
    wm_arcade_special_obj_init(&pie);
    wm_arcade_special_obj_init(&fire);
    wm_arcade_special_obj_init(&spirit);
    wm_arcade_special_obj_init(&reaper);
    wm_arcade_special_obj_init(&salt);

    wm_arcade_spawn_doink_pie(&l, &pie, &p1);
    assert(l.p1 == &pie && pie.in_list);
    assert(pie.id == 0); /* cold slot is zero; constructor itself never writes SP_ID */
    assert(pie.x_fixed == FX16(186) && pie.x_vel == FX16(6));
    assert(pie.anim == WM_SP_ANIM_PIE);
    assert(pie.xoff == -10 && pie.width == 20 && pie.zoff == -10);

    wm_arcade_spawn_bam_fireball(&l, &fire, &p1);
    assert(l.p1 == &fire && fire.next == &pie && fire.id == 0);

    /* Recycled process PDATA is not cleared by GETPRC.  These constructors
       therefore preserve stale SP_ID exactly as the source does. */
    wm_arcade_special_delete(&l, &pie);
    pie.id = WM_SP_ID_SALT;
    wm_arcade_spawn_doink_pie(&l, &pie, &p1);
    assert(pie.id == WM_SP_ID_SALT);
    wm_arcade_special_delete(&l, &fire);
    fire.id = WM_SP_ID_REAPER;
    wm_arcade_spawn_bam_fireball(&l, &fire, &p1);
    assert(fire.id == WM_SP_ID_REAPER);

    wm_arcade_spawn_taker_spirit(&l, &spirit, &p1);
    assert(spirit.id == WM_SP_ID_SPIRIT && spirit.x_fixed == FX16(132));
    assert(spirit.xoff == 0 && spirit.width == 10 && spirit.zoff == -10);

    wm_arcade_spawn_taker_reaper(&l, &reaper, &p2);
    assert(l.p2 == &reaper && reaper.id == WM_SP_ID_REAPER);
    assert(reaper.x_fixed == FX16(198) && reaper.x_vel == -FX16(4));
    assert(reaper.zoff == -1000);
    assert(reaper.source_phase_ticks == 5);
    for (int i = 0; i < 4; i++) wm_arcade_special_tick_source_state(&reaper);
    assert(reaper.anim == WM_SP_ANIM_REAPER_GROW && reaper.x_vel == -FX16(4));
    wm_arcade_special_tick_source_state(&reaper);
    assert(reaper.anim == WM_SP_ANIM_REAPER && reaper.x_vel == -FX16(7));
    assert(reaper.zoff == -1000 && reaper.source_phase_ticks == 6);
    for (int i = 0; i < 5; i++) wm_arcade_special_tick_source_state(&reaper);
    assert(reaper.zoff == -1000);
    wm_arcade_special_tick_source_state(&reaper);
    assert(reaper.zoff == -10 && reaper.source_phase_ticks == 0);

    wm_arcade_spawn_yoko_salt(&l, &salt, &p2);
    assert(salt.id == WM_SP_ID_SALT && salt.gravity == 0x4800);
    assert(salt.y_vel == 0x30000 && salt.zoff == -1000);
    assert(salt.source_phase_ticks == 1);
    wm_arcade_special_tick_source_state(&salt);
    assert(salt.zoff == -40 && salt.depth == 80 && salt.x_vel == -FX16(7));
    assert(salt.source_phase_ticks == 20);
    for (int i = 0; i < 19; i++) wm_arcade_special_tick_source_state(&salt);
    assert(salt.zoff == -40);
    wm_arcade_special_tick_source_state(&salt);
    assert(salt.zoff == -1000 && salt.gravity == 0);
    assert(salt.x_vel == 0 && salt.y_vel == 0 && salt.z_vel == 0);

    wm_arcade_special_delete(&l, &fire);
    assert(!fire.in_list && l.p1 != &fire);
}

static void test_set_boxes_flip_and_velocity(void)
{
    wm_arcade_special_obj_t o;
    wm_arcade_special_obj_init(&o);
    o.x_fixed = FX16(100); o.y_fixed = FX16(50); o.z_fixed = FX16(20);
    o.xoff = 5; o.width = 10; o.yoff = -2; o.height = 4; o.zoff = -3; o.depth = 6;
    wm_arcade_special_set_boxes(&o);
    assert(o.collision_box.x1 == 105 && o.collision_box.x2 == 115);
    assert(o.collision_box.y1 == 48 && o.collision_box.y2 == 52);
    assert(o.collision_box.z1 == 17 && o.collision_box.z2 == 23);
    o.obj_control = WM_OBJ_FLIPH;
    wm_arcade_special_set_boxes(&o);
    assert(o.collision_box.x2 == 95 && o.collision_box.x1 == 85);

    o.x_fixed = 0; o.y_fixed = FX16(10); o.z_fixed = 0;
    o.x_vel = FX16(2); o.y_vel = FX16(3); o.z_vel = FX16(1); o.gravity = FX16(1);
    wm_arcade_special_velocity_add(&o);
    assert(o.x_fixed == FX16(2));
    assert(o.y_vel == FX16(2) && o.y_fixed == FX16(12));
    assert(o.z_fixed == FX16(1));
}

static void test_spirit_and_reaper_hit(void)
{
    wm_arcade_special_lists_t l;
    wm_arcade_special_obj_t spirit, reaper;
    wm_arcade_actor_t taker, victim;
    wm_arcade_combat_runtime_t rt;
    wm_arcade_react_callbacks_t r;
    wm_arcade_react1_callbacks_t r1;
    wm_arcade_react1_context_t r1ctx;
    struct ctx c = {0};
    wm_arcade_special_callbacks_t sc = callbacks(&c, &r, &r1, &r1ctx);

    actor_init(&taker, 0, 0, 20, 0);
    actor_init(&victim, 120, 0, 20, 1);
    victim.facing_dir = WM_MOVE_RIGHT;
    wm_arcade_combat_runtime_init(&rt); rt.pcnt = 777;
    wm_arcade_special_lists_init(&l);
    wm_arcade_special_obj_init(&spirit);
    wm_arcade_special_obj_init(&reaper);
    wm_arcade_spawn_taker_spirit(&l, &spirit, &taker);
    spirit.x_fixed = FX16(110); wm_arcade_special_set_boxes(&spirit);
    assert(wm_arcade_wrestler_hit_special(&l, &spirit, &victim, &rt, &sc));
    assert(taker.last_hit_time == 777);
    assert(victim.immobilize_time == 60);
    assert(c.adjust_calls == 0);
    assert(!spirit.in_list && spirit.anim == WM_SP_ANIM_SPIRIT_SPLAT);
    assert(victim.usr_var1 == 1 && victim.delay_meter == 600);
    assert(victim.x_vel == FX16(4));
    assert(victim.y_vel == FX16(3) && victim.z_vel == 0);
    assert(c.last_anim == WM_R1_ANIM_SPECIAL_BODY_HIT2);

    memset(&c, 0, sizeof(c));
    sc = callbacks(&c, &r, &r1, &r1ctx);
    actor_init(&victim, 20, 0, 20, 1); victim.facing_dir = 0;
    wm_arcade_spawn_taker_reaper(&l, &reaper, &taker);
    wm_arcade_special_reaper_finish_grow(&reaper);
    wm_arcade_special_reaper_enable_collision(&reaper);
    assert(wm_arcade_wrestler_hit_special(&l, &reaper, &victim, &rt, &sc));
    assert(c.adjust_calls == 1 && c.last_delta == -3 && victim.life == 97);
    assert(reaper.anim == WM_SP_ANIM_REAPER_SPLAT && !reaper.in_list);
    assert(victim.x_vel == FX16(3)); /* source starts -3, negates when not facing right */
}

static void test_salt_block_and_nonblock(void)
{
    wm_arcade_special_lists_t l;
    wm_arcade_special_obj_t salt;
    wm_arcade_actor_t yoko, victim;
    wm_arcade_combat_runtime_t rt;
    wm_arcade_react_callbacks_t r;
    wm_arcade_react1_callbacks_t r1;
    wm_arcade_react1_context_t r1ctx;
    struct ctx c = {0};
    wm_arcade_special_callbacks_t sc = callbacks(&c, &r, &r1, &r1ctx);

    actor_init(&yoko, 0, 0, 50, 0);
    actor_init(&victim, 100, 0, 50, 1);
    wm_arcade_combat_runtime_init(&rt); rt.pcnt = 900;
    wm_arcade_special_lists_init(&l);
    wm_arcade_special_obj_init(&salt);
    wm_arcade_spawn_yoko_salt(&l, &salt, &yoko);
    wm_arcade_special_salt_become_live(&salt);
    salt.x_fixed = FX16(90); salt.z_fixed = FX16(50); wm_arcade_special_set_boxes(&salt);
    victim.player_mode = WM_PMODE_BLOCK;
    {
        int32_t collz1_at_call = salt.collision_box.z1;
        assert(wm_arcade_wrestler_hit_special(&l, &salt, &victim, &rt, &sc));
        assert(c.adjust_calls == 0);
        /* Bug-for-bug REACT1: a4 is SP_COLLZ1 at the COLLIS call site. */
        assert(yoko.usr_var2 == collz1_at_call);
    }
    assert(salt.in_list); /* salt path changes anim but does not unlink */
    assert(salt.anim == WM_SP_ANIM_SALT_SPLAT && salt.zoff == -1000);
    assert(victim.x_vel == FX16(3));
    assert(c.last_sound == WM_R1_SND_BLOCK && c.last_anim == WM_R1_ANIM_HITBLOCK);

    memset(&c, 0, sizeof(c));
    sc = callbacks(&c, &r, &r1, &r1ctx);
    actor_init(&victim, 80, 0, 50, 1); victim.facing_dir = WM_MOVE_RIGHT;
    wm_arcade_spawn_yoko_salt(&l, &salt, &yoko);
    wm_arcade_special_salt_become_live(&salt);
    salt.x_vel = FX16(6); salt.x_fixed = FX16(70); wm_arcade_special_set_boxes(&salt);
    assert(wm_arcade_wrestler_hit_special(&l, &salt, &victim, &rt, &sc));
    assert(c.adjust_calls == 1 && c.last_delta == -WM_D_SALT && victim.life == 85);
    assert(victim.delay_meter == 480 && victim.usr_var1 == 0);
    assert(victim.x_vel == -FX16(1) && victim.y_vel == FX16(3));
    assert(victim.z_vel == 0x7000 && salt.z_vel == 0x7000);
    assert(salt.x_vel == FX16(3));
    assert(c.last_anim == WM_R1_ANIM_SPECIAL_HEAD_HIT2_SAND);
}

static void test_object_collision_order_and_default_id_quirk(void)
{
    wm_arcade_special_lists_t l;
    wm_arcade_special_obj_t p1, p2, pie;
    wm_arcade_actor_t a1, a2, victim;
    wm_arcade_actor_t *actors[1];
    wm_arcade_combat_runtime_t rt;
    wm_arcade_react_callbacks_t r;
    wm_arcade_react1_callbacks_t r1;
    wm_arcade_react1_context_t r1ctx;
    struct ctx c = {0};
    wm_arcade_special_callbacks_t sc = callbacks(&c, &r, &r1, &r1ctx);
    wm_arcade_special_collision_result_t cr;

    actor_init(&a1, 0, 0, 0, 0); actor_init(&a2, 0, 0, 0, 1);
    wm_arcade_special_lists_init(&l);
    wm_arcade_special_obj_init(&p1);
    wm_arcade_special_obj_init(&p2);
    wm_arcade_special_obj_init(&pie);
    wm_arcade_spawn_taker_spirit(&l, &p1, &a1);
    wm_arcade_spawn_taker_reaper(&l, &p2, &a2);
    wm_arcade_special_reaper_finish_grow(&p2);
    wm_arcade_special_reaper_enable_collision(&p2);
    p1.x_fixed = p2.x_fixed = 0; p1.y_fixed = p2.y_fixed = 0; p1.z_fixed = p2.z_fixed = 0;
    wm_arcade_special_set_all_boxes(&l);
    wm_arcade_combat_runtime_init(&rt);
    cr = wm_arcade_object_collisions(&l, NULL, 0, &rt, &sc);
    assert(cr.object_object_hit == 1);
    assert(!p1.in_list && !p2.in_list);
    assert(p1.anim == WM_SP_ANIM_SPIRIT_SPLAT && p2.anim == WM_SP_ANIM_REAPER_SPLAT);

    /* A cold process slot starts at ID zero; Doink's constructor does not
       overwrite it, so this particular cold-slot collision follows ID0. */
    wm_arcade_special_lists_init(&l);
    actor_init(&victim, 0, 97, 0, 1); victim.facing_dir = WM_MOVE_RIGHT;
    actors[0] = &victim;
    wm_arcade_spawn_doink_pie(&l, &pie, &a1);
    pie.x_fixed = FX16(0); pie.y_fixed = FX16(97); pie.z_fixed = FX16(0);
    wm_arcade_special_set_boxes(&pie);
    cr = wm_arcade_object_collisions(&l, actors, 1, &rt, &sc);
    assert(cr.wrestler_hits == 1);
    assert(victim.immobilize_time == 60);
    assert(pie.anim == WM_SP_ANIM_SPIRIT_SPLAT);
}

static void test_unchecked_special_hit_id_hazard(void)
{
    wm_arcade_special_lists_t l;
    wm_arcade_special_obj_t bad, good;
    wm_arcade_actor_t p1, p2;

    actor_init(&p1, 0, 0, 0, 0);
    actor_init(&p2, 0, 0, 0, 1);
    wm_arcade_special_lists_init(&l);
    wm_arcade_special_obj_init(&bad);
    wm_arcade_special_obj_init(&good);

    /* Model a recycled process slot whose unwritten SP_ID is stale/out of range. */
    bad.id = 7;
    wm_arcade_spawn_doink_pie(&l, &bad, &p1);
    assert(bad.id == 7);
    wm_arcade_spawn_taker_spirit(&l, &good, &p2);

    /* SPECIAL.ASM special_hit has no range check.  The portable port preserves
       the known deletion-before-lookup side effect, then reports the source
       hazard instead of fabricating an animation pointer from adjacent memory. */
    assert(!wm_arcade_special_hit(&l, &bad, &good));
    assert(!bad.in_list && bad.source_unchecked_splat_id);
    assert(good.in_list);
}

int main(void)
{
    test_lists_and_spawn_state();
    test_set_boxes_flip_and_velocity();
    test_spirit_and_reaper_hit();
    test_salt_block_and_nonblock();
    test_object_collision_order_and_default_id_quirk();
    test_unchecked_special_hit_id_hazard();
    puts("Stage 24 special-object/projectile combat tests: PASS");
    return 0;
}
