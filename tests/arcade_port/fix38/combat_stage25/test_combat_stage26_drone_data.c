#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_drone.h"
#include "wm_arcade_drone_data.h"
#include "wm/match.h"

/* ---- Interpreter-extension unit tests (skip/abort/redirect/cross-script
   jump/DS_SLP1-yield) -- exercised directly against wm_arcade_drone_script_
   step, independent of the real Bret data. ---- */

typedef struct {
    int calls;
    int last_result;
} CallCtx;

static int call_skip(wm_arcade_actor_t *a, wm_arcade_actor_t *o,
                     wm_arcade_drone_state_t *d, const char *l, void *u) {
    (void)a; (void)o; (void)d; (void)l;
    ((CallCtx *)u)->calls++;
    return WM_DRONE_CALL_SKIP_NEXT;
}
static int call_abort(wm_arcade_actor_t *a, wm_arcade_actor_t *o,
                      wm_arcade_drone_state_t *d, const char *l, void *u) {
    (void)a; (void)o; (void)d; (void)l;
    ((CallCtx *)u)->calls++;
    return WM_DRONE_CALL_ABORT;
}
static int call_redirect(wm_arcade_actor_t *a, wm_arcade_actor_t *o,
                         wm_arcade_drone_state_t *d, const char *l, void *u) {
    (void)a; (void)o; (void)l; (void)u;
    d->script = "redirected";
    d->script_pc = 0;
    return WM_DRONE_CALL_REDIRECTED;
}

static const wm_arcade_drone_script_op_t redirected_ops[] = {
    {WM_DRONE_SC_INPUT, WM_BTN_KICK, 5, 0, 0, NULL, NULL}
};
static const wm_arcade_drone_script_t redirected_script = {
    "redirected", redirected_ops, 1
};
static const wm_arcade_drone_script_t *resolve_redirected(const char *l, void *u) {
    (void)u;
    if (l && strcmp(l, "redirected") == 0) return &redirected_script;
    return NULL;
}

static void actor_pair(wm_arcade_actor_t *a, wm_arcade_actor_t *b) {
    memset(a, 0, sizeof(*a));
    memset(b, 0, sizeof(*b));
    a->active = b->active = 1;
    a->player_mode = b->player_mode = WM_PMODE_NORMAL;
    a->facing_dir = WM_MOVE_RIGHT;
    a->smart_target = b;
    b->smart_target = a;
}

static void test_skip_next(void) {
    wm_arcade_actor_t a, b;
    wm_arcade_drone_state_t d;
    wm_arcade_drone_callbacks_t cb;
    CallCtx ctx = {0, 0};
    /* op0: CALL_CODE (skip) op1: INPUT (skipped) op2: INPUT (reached) */
    static const wm_arcade_drone_script_op_t ops[] = {
        {WM_DRONE_SC_CALL_CODE, 0, 0, 0, 0, "x", NULL},
        {WM_DRONE_SC_INPUT, WM_BTN_PUNCH, 9, 0, 0, NULL, NULL},
        {WM_DRONE_SC_INPUT, WM_BTN_KICK, 3, 0, 0, NULL, NULL}
    };
    static const wm_arcade_drone_script_t script = {"s", ops, 3};

    actor_pair(&a, &b);
    memset(&cb, 0, sizeof(cb));
    cb.script_call = call_skip;
    cb.user = &ctx;
    wm_arcade_drone_init(&d, 0);
    d.script = "s"; d.script_pc = 0; d.script_mode = WM_PMODE_NORMAL;

    assert(wm_arcade_drone_script_step(&a, &b, &d, &script, &cb) == WM_DRONE_STEP_INPUT);
    assert(ctx.calls == 1);
    assert(d.but == WM_BTN_KICK && d.delay == 3);
    puts("drone_data skip_next: PASS");
}

static void test_abort_result(void) {
    wm_arcade_actor_t a, b;
    wm_arcade_drone_state_t d;
    wm_arcade_drone_callbacks_t cb;
    CallCtx ctx = {0, 0};
    static const wm_arcade_drone_script_op_t ops[] = {
        {WM_DRONE_SC_CALL_CODE, 0, 0, 0, 0, "x", NULL},
        {WM_DRONE_SC_INPUT, WM_BTN_PUNCH, 9, 0, 0, NULL, NULL}
    };
    static const wm_arcade_drone_script_t script = {"s", ops, 2};

    actor_pair(&a, &b);
    memset(&cb, 0, sizeof(cb));
    cb.script_call = call_abort;
    cb.user = &ctx;
    wm_arcade_drone_init(&d, 0);
    d.script = "s"; d.script_pc = 0; d.script_mode = WM_PMODE_NORMAL;

    assert(wm_arcade_drone_script_step(&a, &b, &d, &script, &cb) == WM_DRONE_STEP_ABORT_SCRIPT);
    assert(d.script == NULL);
    puts("drone_data call_abort: PASS");
}

static void test_redirect_same_tick(void) {
    wm_arcade_actor_t a, b;
    wm_arcade_drone_state_t d;
    wm_arcade_drone_callbacks_t cb;
    static const wm_arcade_drone_script_op_t ops[] = {
        {WM_DRONE_SC_CALL_FUNCTION, 0, 0, 0, 0, "x", NULL}
    };
    static const wm_arcade_drone_script_t script = {"s", ops, 1};

    actor_pair(&a, &b);
    memset(&cb, 0, sizeof(cb));
    cb.script_call = call_redirect;
    cb.resolve_script = resolve_redirected;
    wm_arcade_drone_init(&d, 0);
    d.script = "s"; d.script_pc = 0; d.script_mode = WM_PMODE_NORMAL;

    /* The redirect must land on the *new* script's own op, same tick --
       not merely yield and require a second step() call. */
    assert(wm_arcade_drone_script_step(&a, &b, &d, &script, &cb) == WM_DRONE_STEP_INPUT);
    assert(d.but == WM_BTN_KICK && d.delay == 5);
    assert(d.script && strcmp(d.script, "redirected") == 0);
    puts("drone_data redirect_same_tick: PASS");
}

static void test_cross_script_jump(void) {
    wm_arcade_actor_t a, b;
    wm_arcade_drone_state_t d;
    wm_arcade_drone_callbacks_t cb;
    static const wm_arcade_drone_script_op_t src_ops[] = {
        {WM_DRONE_SC_JUMP, 0, 0, 0, 0, NULL, "redirected"}
    };
    static const wm_arcade_drone_script_t src_script = {"src", src_ops, 1};

    actor_pair(&a, &b);
    memset(&cb, 0, sizeof(cb));
    cb.resolve_script = resolve_redirected;
    wm_arcade_drone_init(&d, 0);
    d.script = "src"; d.script_pc = 0; d.script_mode = WM_PMODE_NORMAL;

    assert(wm_arcade_drone_script_step(&a, &b, &d, &src_script, &cb) == WM_DRONE_STEP_INPUT);
    assert(d.but == WM_BTN_KICK && d.delay == 5);
    assert(d.script && strcmp(d.script, "redirected") == 0);
    puts("drone_data cross_script_jump: PASS");
}

static void test_done_yields_not_aborts(void) {
    wm_arcade_actor_t a, b;
    wm_arcade_drone_state_t d;
    wm_arcade_drone_callbacks_t cb;
    /* DS_SLP1 then a following op -- must yield (pc advances, script stays
       set), not abort. */
    static const wm_arcade_drone_script_op_t ops[] = {
        {WM_DRONE_SC_DONE, 0, 0, 0, 0, NULL, NULL},
        {WM_DRONE_SC_INPUT, WM_BTN_SKICK, 1, 0, 0, NULL, NULL}
    };
    static const wm_arcade_drone_script_t script = {"s", ops, 2};

    actor_pair(&a, &b);
    memset(&cb, 0, sizeof(cb));
    wm_arcade_drone_init(&d, 0);
    d.script = "s"; d.script_pc = 0; d.script_mode = WM_PMODE_NORMAL;

    assert(wm_arcade_drone_script_step(&a, &b, &d, &script, &cb) == WM_DRONE_STEP_SCRIPT);
    assert(d.script != NULL); /* not aborted */
    assert(d.script_pc == 1);

    /* Next tick resumes right after the DS_SLP1 marker. */
    assert(wm_arcade_drone_script_step(&a, &b, &d, &script, &cb) == WM_DRONE_STEP_INPUT);
    assert(d.but == WM_BTN_SKICK && d.delay == 1);
    puts("drone_data done_yields_not_aborts: PASS");
}

/* ---- Table spot-checks (SKLM-derived tables) ---- */

static void test_tables(void) {
    assert(wm_arcade_drone_block_base_pct(0) == 10);
    assert(wm_arcade_drone_block_base_pct(29) == 75);
    assert(wm_arcade_drone_block_attack_pct(0) == 0);
    assert(wm_arcade_drone_block_attack_pct(9) == 50);
    assert(wm_arcade_drone_block_attack_pct(20) == 50); /* clamp */
    assert(wm_arcade_drone_headhold_delay_max(0) == 150);
    assert(wm_arcade_drone_headhold_delay_max(29) == 5);
    assert(wm_arcade_drone_headheld_delay_max(0) == 150);
    assert(wm_arcade_drone_headheld_delay_max(29) == 5);
    assert(wm_arcade_drone_repeat_pct(0) == 20);
    assert(wm_arcade_drone_repeat_pct(29) == 102);
    puts("drone_data tables: PASS");
}

/* ---- Integration: a real CPU opponent actually fights ---- */

static void test_cpu_opponent_fights(void) {
    wm_match_state m;
    WmRng rng;
    wm_arcade_drone_callbacks_t cb;
    unsigned tick;
    int saw_button = 0;
    int saw_stick = 0;
    int32_t start_x0, start_z0, start_x1, start_z1;
    int moved = 0;

    wm_rng_init(&rng, 0x1234u, NULL, NULL, NULL);
    wm_match_init(&m);
    wm_match_start_attract(&m, &rng);

    /* Force a real CPU-vs-CPU Bret matchup: both actors are drone-driven
       in attract mode, and only Bret has a real move/animation backend. */
    m.actors[0].wrestler_num = WM_ROSTER_BRET;
    m.actors[1].wrestler_num = WM_ROSTER_BRET;

    cb = wm_arcade_drone_data_callbacks(&rng);

    start_x0 = m.actors[0].x_int; start_z0 = m.actors[0].z_int;
    start_x1 = m.actors[1].x_int; start_z1 = m.actors[1].z_int;

    for (tick = 0; tick < 2000 && m.active; ++tick) {
        wm_match_tick(&m, &cb, NULL);
        if (m.actors[0].but_val_cur != 0 || m.actors[1].but_val_cur != 0)
            saw_button = 1;
        if (m.actors[0].stick_val_cur != 0 || m.actors[1].stick_val_cur != 0)
            saw_stick = 1;
        if (m.actors[0].x_int != start_x0 || m.actors[0].z_int != start_z0 ||
            m.actors[1].x_int != start_x1 || m.actors[1].z_int != start_z1)
            moved = 1;
    }

    assert(saw_button && "CPU opponent never pressed a button");
    assert(saw_stick && "CPU opponent never pushed the stick");
    assert(moved && "CPU opponent never moved");
    puts("drone_data cpu_opponent_fights: PASS");
}

/* ---- Every wrestler, not just Bret, runs its real dispatcher ---- */

static void test_all_wrestlers_move(void) {
    static const int ids[8] = {
        WM_ROSTER_BRET, WM_ROSTER_RAZOR, WM_ROSTER_TAKER, WM_ROSTER_YOKO,
        WM_ROSTER_SHAWN, WM_ROSTER_BAM, WM_ROSTER_DOINK, WM_ROSTER_LEX
    };
    int k;

    /* Doink is the one wrestler whose own ASM uses a different #VEL/#DVEL. */
    assert(wm_wrestler_velocity_table(WM_ROSTER_DOINK) !=
           wm_wrestler_velocity_table(WM_ROSTER_BRET));
    assert(wm_wrestler_velocity_table(WM_ROSTER_LEX) ==
           wm_wrestler_velocity_table(WM_ROSTER_BRET));

    for (k = 0; k < 8; ++k) {
        wm_match_state m;
        WmRng rng;
        wm_arcade_drone_callbacks_t cb;
        unsigned tick;
        int32_t sx0, sz0, sx1, sz1;
        int moved = 0, attacked = 0, left_block = 0;

        wm_rng_init(&rng, 0x2000u + (uint32_t)k, NULL, NULL, NULL);
        wm_match_init(&m);
        wm_match_start_attract(&m, &rng);
        m.actors[0].wrestler_num = ids[k];
        m.actors[1].wrestler_num = ids[k];
        cb = wm_arcade_drone_data_callbacks(&rng);

        sx0 = m.actors[0].x_int; sz0 = m.actors[0].z_int;
        sx1 = m.actors[1].x_int; sz1 = m.actors[1].z_int;

        for (tick = 0; tick < 2000 && m.active; ++tick) {
            uint16_t but;
            wm_match_tick(&m, &cb, NULL);
            if (m.actors[0].x_int != sx0 || m.actors[0].z_int != sz0 ||
                m.actors[1].x_int != sx1 || m.actors[1].z_int != sz1)
                moved = 1;
            /* A real attack button (not just BLOCK) means this wrestler's
               own AI script list produced a real action script. */
            but = (uint16_t)(m.actors[0].but_val_cur | m.actors[1].but_val_cur);
            if (but & (WM_BTN_PUNCH | WM_BTN_SPUNCH | WM_BTN_KICK | WM_BTN_SKICK))
                attacked = 1;
            /* Nothing may get permanently stuck in MODE_BLOCK: the block
               animation's own ANI_WAITRELEASE releases it. */
            if (m.actors[0].player_mode != WM_PMODE_BLOCK) left_block = 1;
        }
        assert(moved && "wrestler never moved through its own dispatcher");
        assert(attacked && "wrestler's own AI script list never produced an attack");
        assert(left_block && "wrestler never left MODE_BLOCK");
    }
    puts("drone_data all_wrestlers_move: PASS");
}

int main(void) {
    test_skip_next();
    test_abort_result();
    test_redirect_same_tick();
    test_cross_script_jump();
    test_done_yields_not_aborts();
    test_tables();
    test_cpu_opponent_fights();
    test_all_wrestlers_move();
    puts("stage26 real CPU opponent (DRONE.ASM data + callbacks) tests: PASS");
    return 0;
}
