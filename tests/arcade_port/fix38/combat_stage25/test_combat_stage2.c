#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_anim_combat.h"
#include "wm_arcade_react.h"

struct test_ctx {
    int adjust_calls;
    int last_delta;
    int first_awards;
    int first_msgs;
    int bonus_msgs;
    int reaction_calls;
    int breakout_calls;
    int ditch_calls;
    int good_run;
};

static void adjust_cb(wm_arcade_actor_t *victim, int16_t delta,
                      wm_arcade_actor_t *source, void *user)
{
    struct test_ctx *t = user;
    (void)victim; (void)source;
    t->adjust_calls++;
    t->last_delta = delta;
}
static void first_award(wm_arcade_actor_t *a, void *u) { (void)a; ((struct test_ctx*)u)->first_awards++; }
static void first_msg(wm_arcade_actor_t *a, void *u) { (void)a; ((struct test_ctx*)u)->first_msgs++; }
static void bonus_msg(wm_arcade_actor_t *a, void *u) { (void)a; ((struct test_ctx*)u)->bonus_msgs++; }
static int good_run(wm_arcade_actor_t *a, wm_arcade_actor_t *v, void *u)
{ (void)a; (void)v; return ((struct test_ctx*)u)->good_run; }
static void reaction_cb(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                        wm_arcade_reaction_id_t r, int16_t *d, int16_t *m,
                        void *u)
{ (void)a; (void)v; (void)r; (void)d; (void)m; ((struct test_ctx*)u)->reaction_calls++; }
static void reaction_zero_stomp(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                                wm_arcade_reaction_id_t r, int16_t *d, int16_t *m,
                                void *u)
{ (void)a; (void)v; (void)m; ((struct test_ctx*)u)->reaction_calls++; if (r == WM_RXN_STOMP) *d = 0; }
static void breakout_cb(wm_arcade_actor_t *p, wm_arcade_partner_breakout_t k, void *u)
{ (void)p; (void)k; ((struct test_ctx*)u)->breakout_calls++; }
static void ditch_cb(wm_arcade_actor_t *v, void *u)
{ (void)v; ((struct test_ctx*)u)->ditch_calls++; }

static wm_arcade_react_callbacks_t callbacks(struct test_ctx *t)
{
    wm_arcade_react_callbacks_t c;
    memset(&c, 0, sizeof(c));
    c.good_run_hit = good_run;
    c.reaction = reaction_cb;
    c.adjust_health = adjust_cb;
    c.round_first_hit_award = first_award;
    c.first_hit_message = first_msg;
    c.bonus_message = bonus_msg;
    c.partner_breakout = breakout_cb;
    c.ditch_getup_meter = ditch_cb;
    c.user = t;
    return c;
}

static void actor_init(wm_arcade_actor_t *a)
{
    memset(a, 0, sizeof(*a));
    a->wrestler_num = 0;
    a->player_mode = WM_PMODE_NORMAL;
}

static void test_damage_table_and_50_tick_window(void)
{
    wm_arcade_actor_t a, v;
    wm_arcade_combat_runtime_t rt;
    wm_arcade_wrestler_hit_result_t r;
    struct test_ctx t = {0};
    wm_arcade_react_callbacks_t cb = callbacks(&t);
    int16_t f, rd;

    assert(wm_arcade_attack_damage_pair(WM_AMODE_PUNCH, &f, &rd));
    assert(f == 8 && rd == 5);
    assert(wm_arcade_attack_damage_pair(WM_AMODE_SUPER_KICK, &f, &rd));
    assert(f == 17 && rd == 11);

    actor_init(&a); actor_init(&v); wm_arcade_combat_runtime_init(&rt);
    a.attack_mode = WM_AMODE_PUNCH;
    rt.pcnt = 1000;
    v.last_damage = 951; /* 49 ticks -> reduced */
    r = wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(r.status == WM_WRESTLER_HIT_OK);
    /* 5 * 345 * 256 >> 16 = 6 */
    assert(r.damage_before_reaction == -6);
    assert(t.last_delta == -6);
}

static void test_next_damage_rules(void)
{
    wm_arcade_actor_t a, v;
    wm_arcade_combat_runtime_t rt;
    struct test_ctx t = {0};
    wm_arcade_react_callbacks_t cb = callbacks(&t);
    wm_arcade_wrestler_hit_result_t r;

    actor_init(&a); actor_init(&v); wm_arcade_combat_runtime_init(&rt);
    a.attack_mode = WM_AMODE_PUNCH; /* base 8 */
    a.next_damage = 6;
    a.special_damage_time = 500;
    rt.pcnt = 400;
    r = wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(r.damage_before_reaction == -8); /* 6 * 345 / 256 = 8 */
    assert(a.next_damage == 0);

    t.adjust_calls = 0;
    actor_init(&a); actor_init(&v);
    a.attack_mode = WM_AMODE_PUNCH;
    a.next_damage = 20; /* larger than base -> don't use, but clear */
    a.special_damage_time = 500;
    r = wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(r.damage_before_reaction == -10); /* 8*345/256 */
    assert(a.next_damage == 0);

    actor_init(&a); actor_init(&v);
    a.attack_mode = WM_AMODE_GRABTHROW; /* zero damage keeps NEXT_DAMAGE */
    a.next_damage = 7;
    a.special_damage_time = 500;
    r = wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(r.damage_before_reaction == 0);
    assert(a.next_damage == 7);
}

static void test_block_exceptions_and_bstomp2_nonexception(void)
{
    wm_arcade_actor_t a, v;
    wm_arcade_combat_runtime_t rt;
    struct test_ctx t = {0};
    wm_arcade_react_callbacks_t cb = callbacks(&t);
    wm_arcade_wrestler_hit_result_t r;

    actor_init(&a); actor_init(&v); wm_arcade_combat_runtime_init(&rt);
    v.player_mode = WM_PMODE_BLOCK;
    a.attack_mode = WM_AMODE_PUNCH;
    r = wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(r.damage_before_reaction == -1);

    actor_init(&a); actor_init(&v); v.player_mode = WM_PMODE_BLOCK;
    a.attack_mode = WM_AMODE_BSTOMP;
    r = wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(r.damage_before_reaction < -1);

    actor_init(&a); actor_init(&v); v.player_mode = WM_PMODE_BLOCK;
    a.attack_mode = WM_AMODE_BLBOWDROP;
    r = wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(r.damage_before_reaction < -1);

    actor_init(&a); actor_init(&v); v.player_mode = WM_PMODE_BLOCK;
    a.attack_mode = WM_AMODE_BSTOMP2;
    r = wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(r.damage_before_reaction == -1);
}

static void test_reaction_can_cancel_damage(void)
{
    wm_arcade_actor_t a, v;
    wm_arcade_combat_runtime_t rt;
    struct test_ctx t = {0};
    wm_arcade_react_callbacks_t cb = callbacks(&t);
    wm_arcade_wrestler_hit_result_t r;

    cb.reaction = reaction_zero_stomp;
    actor_init(&a); actor_init(&v); wm_arcade_combat_runtime_init(&rt);
    a.attack_mode = WM_AMODE_STOMP;
    r = wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(r.damage_before_reaction < 0);
    assert(r.damage_after_reaction == 0);
    assert(t.adjust_calls == 0);
}

static void test_turnbuckle_dispatch_and_run_gate(void)
{
    wm_arcade_actor_t a, v;
    wm_arcade_combat_runtime_t rt;
    struct test_ctx t = {0};
    wm_arcade_react_callbacks_t cb = callbacks(&t);
    wm_arcade_wrestler_hit_result_t r;

    actor_init(&a); actor_init(&v); wm_arcade_combat_runtime_init(&rt);
    a.attack_mode = WM_AMODE_PUNCH;
    v.player_mode = WM_PMODE_ONTURNBKL;
    r = wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(r.reaction == WM_RXN_ONTURNBUCKLE);

    actor_init(&a); actor_init(&v);
    a.attack_mode = WM_AMODE_PUPPET;
    v.player_mode = WM_PMODE_ONTURNBKL;
    r = wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(r.reaction == WM_RXN_PUPPET);

    actor_init(&a); actor_init(&v);
    a.attack_mode = WM_AMODE_RUN;
    t.good_run = 0;
    r = wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(r.status == WM_WRESTLER_HIT_IGNORED_RUN);
    assert(a.who_i_hit == NULL && v.who_hit_me == NULL);
}

static void test_hit_stuff_cleanup(void)
{
    wm_arcade_actor_t a, v, p;
    struct test_ctx t = {0};
    wm_arcade_react_callbacks_t cb = callbacks(&t);

    actor_init(&a); actor_init(&v); actor_init(&p);
    a.attack_mode = WM_AMODE_PUNCH;
    v.status_flags = WM_STATUS_KOD | WM_STATUS_SMART_ATTACK;
    v.player_mode = WM_PMODE_RUNNING;
    v.attach_proc = &p;
    p.attach_proc = &v;
    p.player_mode = WM_PMODE_PUPPET;
    v.stars_flag = 9; v.debris_x = 8; v.combo_count = 7;
    v.smart_target = &a; v.run_time = 99;

    wm_arcade_hit_stuff(&a, &v, &cb);
    assert((v.status_flags & WM_STATUS_KOD) == 0);
    assert((v.status_flags & WM_STATUS_NO_KO) != 0);
    assert(v.ptime == 1);
    assert(v.stars_flag == 0 && v.debris_x == 0 && v.combo_count == 0);
    assert(v.attach_proc == NULL && p.attach_proc == NULL);
    assert(v.smart_target == NULL);
    assert((v.status_flags & WM_STATUS_SMART_ATTACK) == 0);
    assert(v.run_time == 0);
    assert(t.breakout_calls == 1);
    assert(t.ditch_calls == 1);
}

static void test_anim_attack_ops(void)
{
    wm_arcade_actor_t a;
    wm_arcade_attack_on_args_t on = { WM_AMODE_KICK, -12, 4, 33, 44 };
    wm_arcade_attack_on_z_args_t onz = { WM_AMODE_FLYKICK, 1,2,3,4,5,6 };

    actor_init(&a);
    a.anim_mode = WM_MODE_STATUS | WM_MODE_WAITHITOPP;
    a.status_flags = WM_STATUS_SMART_ATTACK;
    a.smart_target = &a;
    wm_arcade_ani_attack_on(&a, &on);
    assert((a.anim_mode & WM_MODE_STATUS) == 0);
    assert((a.anim_mode & WM_MODE_CHECKHIT) != 0);
    assert(a.attack_mode == WM_AMODE_KICK);
    assert(a.attack_xoff == -12 && a.attack_yoff == 4);
    assert(a.attack_width == 33 && a.attack_height == 44);
    assert(a.attack_zoff == -40 && a.attack_depth == 80);

    a.attach_zoff = 55;
    wm_arcade_ani_attack_on_z(&a, &onz);
    assert(a.attach_zoff == 0);
    assert(a.attack_xoff == 1 && a.attack_yoff == 2 && a.attack_zoff == 3);
    assert(a.attack_width == 4 && a.attack_height == 5 && a.attack_depth == 6);

    wm_arcade_ani_waithitopp(&a);
    wm_arcade_ani_attack_off(&a, 1234);
    assert((a.anim_mode & (WM_MODE_CHECKHIT|WM_MODE_WAITHITOPP)) == 0);
    assert((a.status_flags & WM_STATUS_SMART_ATTACK) == 0);
    assert(a.smart_target == NULL && a.attack_time == 1234);
}

static void test_anim_damageopp_30_ticks_next_damage_and_risk(void)
{
    wm_arcade_actor_t a, v;
    wm_arcade_combat_runtime_t rt;
    struct test_ctx t = {0};
    wm_arcade_react_callbacks_t cb = callbacks(&t);
    wm_arcade_anim_damageopp_result_t r;

    actor_init(&a); actor_init(&v); wm_arcade_combat_runtime_init(&rt);
    a.who_i_hit = &v;
    rt.pcnt = 1000;
    v.last_damage = 971; /* 29 ticks -> reduced */
    r = wm_arcade_ani_damageopp(&a, 20, 7, &rt, &cb);
    assert(r.status == WM_ANI_DAMAGEOPP_OK);
    assert(r.used_reduced_damage == 1);
    assert(r.signed_damage == -7);
    assert(t.last_delta == -7);

    t.adjust_calls = 0;
    actor_init(&a); actor_init(&v); wm_arcade_combat_runtime_init(&rt);
    a.who_i_hit = &v;
    a.next_damage = 25; /* opcode 66 uses it even though larger */
    a.special_damage_time = 2000;
    rt.pcnt = 1000;
    r = wm_arcade_ani_damageopp(&a, 10, 6, &rt, &cb);
    assert(r.used_next_damage == 1 && r.signed_damage == -25);
    assert(a.next_damage == 25); /* source does not clear it here */

    actor_init(&a); actor_init(&v); wm_arcade_combat_runtime_init(&rt);
    a.who_i_hit = &v;
    a.risk = WM_ARCADE_RISK_HIGH_BIT;
    r = wm_arcade_ani_damageopp(&a, 10, 6, &rt, &cb);
    assert(rt.dam_mult == 4 && rt.any_hits == 1);
    assert(a.risk == 0);
    assert(t.bonus_msgs >= 1);
}

static void test_first_hit_and_direct_damage(void)
{
    wm_arcade_actor_t a, v;
    wm_arcade_combat_runtime_t rt;
    struct test_ctx t = {0};
    wm_arcade_react_callbacks_t cb = callbacks(&t);

    actor_init(&a); actor_init(&v); wm_arcade_combat_runtime_init(&rt);
    a.attack_mode = WM_AMODE_PUNCH;
    (void)wm_arcade_wrestler_hit(&a, &v, &rt, &cb);
    assert(rt.any_hits == 1 && rt.dam_mult == 2);
    assert(t.first_awards == 1 && t.first_msgs == 1);

    t.adjust_calls = 0;
    wm_arcade_ani_damage(&v, 13, &cb);
    assert(t.adjust_calls == 1 && t.last_delta == -13);
}


static void test_stage1_bridge(void)
{
    wm_arcade_actor_t a, v;
    wm_arcade_combat_runtime_t rt;
    struct test_ctx t = {0};
    wm_arcade_react_callbacks_t rcb = callbacks(&t);
    wm_arcade_react_bridge_t bridge;

    actor_init(&a); actor_init(&v); wm_arcade_combat_runtime_init(&rt);
    a.attack_mode = WM_AMODE_PUNCH;
    memset(&bridge, 0, sizeof(bridge));
    bridge.runtime = &rt;
    bridge.callbacks = &rcb;
    wm_arcade_wrestler_hit_collision_callback(&a, &v, &bridge);
    assert(bridge.last_result.status == WM_WRESTLER_HIT_OK);
    assert(a.who_i_hit == &v && v.who_hit_me == &a);
}

int main(void)
{
    test_damage_table_and_50_tick_window();
    test_next_damage_rules();
    test_block_exceptions_and_bstomp2_nonexception();
    test_reaction_can_cancel_damage();
    test_turnbuckle_dispatch_and_run_gate();
    test_hit_stuff_cleanup();
    test_anim_attack_ops();
    test_anim_damageopp_30_ticks_next_damage_and_risk();
    test_first_hit_and_direct_damage();
    test_stage1_bridge();
    puts("combat stage2: all tests passed");
    return 0;
}
