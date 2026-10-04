/*
 * The quiet bouts, one defect at a time.
 *
 * test_live_fight.c used to record that one bout in twenty (Bret v Bret,
 * seed 1) produced no damage at all, and called whether that was arcade
 * behaviour or a defect "NOT established". It was a defect, and under it
 * were five more, each of which left a live attract bout stuck in one
 * state for the rest of the match:
 *
 *   1. drn_ontb threw away the stick drone_seekxz had written
 *      (DRONE.ASM:3078 `move a0,*a13(DRN_JOY)`), so a drone sent to
 *      climb a corner stood at it forever.
 *   2. start_run_flung's two ANI_CODEs (WRESTLE2.ASM:3505 #x_flip,
 *      :3488 #ok2) were untranslated, so a flung wrestler kept his throw
 *      velocity, never reached WAITANIM, and slid off the arena.
 *   3. A torso animation's ANI_SETMODE wrote the body's mode word, which
 *      ANIM.ASM:84 says lives in ANIMODE2, wiping MODE_UNINT mid-grab.
 *   4. ANI_DAMAGE / ANI_DAMAGEOPP called adjust_health without attract
 *      mode, so an animation could kill in attract (LIFEBAR.ASM:1578,
 *      "if we're in attract mode, don't die!").
 *   5. The drone layer read the actor's in_ring (1 = inside) as the
 *      source's INRING (0 = inside), and drn_ontb's #ering was missing.
 *
 * Plus one configuration fact: the drone world was told every match was
 * the ladder's first rung, which attract never is (ATTRACT.ASM:596-602).
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wm/match.h"
#include "wm/anim_program.h"
#include "wm/bret_backend.h"
#include "wm/wrestler_backend.h"
#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wm_arcade_drone_data.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wm_arcade_start_run.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm_arcade_roster.h"

static WmRng g_rng;

static void actor_at(wm_arcade_actor_t *a, int32_t x, int32_t z)
{
    memset(a, 0, sizeof *a);
    a->active = 1;
    a->life = 163;
    a->in_ring = 1;
    a->x_int = x;
    a->z_int = z;
    a->x_fixed = x << 16;
    a->z_fixed = z << 16;
    a->facing_dir = WM_MOVE_RIGHT;
}

/*
 * 1. DRONE.ASM:2671-2678. drone_seekxz to within 32 of the corner, and
 * once it is there, drone_seekxz again with range 0 at RING_TOP-10 --
 * "Push into turnbuckle". The second call's answer is the stick: it is
 * what leans him into the corner so the climb check fires.
 */
static void test_ontb_pushes_into_the_turnbuckle(void)
{
    wm_arcade_actor_t self, opp;
    wm_arcade_drone_state_t d;
    wm_arcade_drone_callbacks_t cb = wm_arcade_drone_data_callbacks(&g_rng);
    int32_t corner = WM_RING_X_CENTER - 225;

    actor_at(&self, corner, WM_RING_TOP);
    actor_at(&opp, corner + 400, WM_RING_TOP + 200);
    self.closest_xdist = 400;
    self.closest_zdist = 200;
    wm_arcade_drone_init(&d, 15);
    d.script = "drn_ontb";

    assert(cb.script_seek != NULL);
    (void)cb.script_seek(&self, &opp, &d, "drn_ontb", cb.user);
    /* On the corner's X already, 10 below RING_TOP-10: push UP. */
    assert(d.joy == WM_MOVE_UP);
}

/*
 * 5b. drn_ontb's #ering: `move *a13(INRING),a0 / jrnz #ering` -- a drone
 * outside the ring cannot climb a corner post, so it goes back in first.
 * And #ny: Yokozuna will not climb while his opponent is outside.
 */
static void test_ontb_from_outside_goes_back_into_the_ring(void)
{
    wm_arcade_actor_t self, opp;
    wm_arcade_drone_state_t d;
    wm_arcade_drone_callbacks_t cb = wm_arcade_drone_data_callbacks(&g_rng);

    actor_at(&self, 820, 962);
    actor_at(&opp, 1100, 1200);
    self.in_ring = 0;                         /* on the floor outside */
    self.closest_xdist = 280;
    self.closest_zdist = 238;
    wm_arcade_drone_init(&d, 15);
    d.script = "drn_ontb";

    (void)cb.script_seek(&self, &opp, &d, "drn_ontb", cb.user);
    assert(d.script != NULL);
    assert(strcmp(d.script, "drn_enterring") == 0);

    /* drn_enterring itself walks him: out, facing a man who is in, so
       neither guard ("Opp out?", "In ring?") stops it. Between the ring
       ends and above the near side: the near side's centre, down-right. */
    d.joy = 0;
    assert(cb.script_seek(&self, &opp, &d, "drn_enterring", cb.user) != 0);
    assert(d.joy == (WM_MOVE_RIGHT | WM_MOVE_DOWN));
    assert(d.script != NULL);
    /* ...and stops, pressing nothing, once the opponent is out too. */
    opp.in_ring = 0;
    (void)cb.script_seek(&self, &opp, &d, "drn_enterring", cb.user);
    assert(d.joy == 0);
    assert(d.script == NULL);
    opp.in_ring = 1;

    /* #ny: Yokozuna, in the ring, opponent out -- #x, nothing pushed. */
    actor_at(&self, 900, 1100);
    actor_at(&opp, 900, 1700);
    opp.in_ring = 0;
    self.wrestler_num = WM_ROSTER_YOKO;
    self.closest_xdist = 0;
    self.closest_zdist = 600;
    wm_arcade_drone_init(&d, 15);
    d.script = "drn_ontb";
    d.joy = 0;
    assert(cb.script_seek(&self, &opp, &d, "drn_ontb", cb.user) == 0);
    assert(d.joy == 0);
}

/*
 * DS_JMP reads the new script at once (DRONE.ASM's interpreter jumps
 * straight back to its own fetch loop), so the ONE interpreter step that
 * runs drn_ontb's code block and takes #ering must also run
 * drn_enterring's guard and seek -- the drone is already walking back
 * to the ring on that tick, not the next one.
 */
static void test_ering_hands_over_in_the_same_tick(void)
{
    wm_arcade_actor_t self, opp;
    wm_arcade_drone_state_t d;
    wm_arcade_drone_callbacks_t cb = wm_arcade_drone_data_callbacks(&g_rng);
    const wm_arcade_drone_script_t *ontb;

    actor_at(&self, 820, 962);
    actor_at(&opp, 1100, 1200);
    self.in_ring = 0;
    self.closest_xdist = 280;
    self.closest_zdist = 238;
    wm_arcade_drone_init(&d, 15);
    d.script = "drn_ontb";
    d.script_pc = 0;
    d.script_mode = WM_PMODE_NORMAL;
    d.joy = 0;
    ontb = cb.resolve_script("drn_ontb", cb.user);
    assert(ontb != NULL);

    (void)wm_arcade_drone_script_step(&self, &opp, &d, ontb, &cb);
    assert(d.script != NULL);
    assert(strcmp(d.script, "drn_enterring") == 0);
    assert(d.joy == (WM_MOVE_RIGHT | WM_MOVE_DOWN));
}

/* drone_main's pick, recorded. */
static const char *g_picked;
static void on_script(wm_arcade_actor_t *a, const char *label, void *u)
{
    (void)a; (void)u;
    g_picked = label;
}

/*
 * 5a. DRONE.ASM:466-474: only a drone who is OUT, facing an opponent
 * who is IN, selects drn_enterring. The port's in_ring is 1 inside, so
 * the source's two tests flip; they were copied the source's way round
 * and sent the drone who was INSIDE off to "enter" the ring.
 */
static void test_enterring_is_for_the_man_outside(void)
{
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *arr[2];
    wm_arcade_drone_state_t d;
    wm_arcade_drone_world_t w;
    wm_arcade_drone_callbacks_t cb = wm_arcade_drone_data_callbacks(&g_rng);
    int k;

    cb.script_selected = on_script;
    for (k = 0; k < 2; ++k) {
        actor_at(&a, 900, 1500);
        actor_at(&b, 1100, 1200);
        arr[0] = &a; arr[1] = &b;
        a.player_num = 2; b.player_num = 3;
        a.player_side = 0; b.player_side = 1;
        a.closest_num = 3;
        a.closest_xdist = 200; a.closest_zdist = 300; a.closest_dist = 300;
        /* k == 0: a is outside, b inside. k == 1: the reverse. */
        a.in_ring = k == 0 ? 0 : 1;
        b.in_ring = k == 0 ? 1 : 0;
        memset(&w, 0, sizeof w);
        w.actors = arr; w.actor_count = 2; w.pcnt = 15;
        w.round_tickcount = 15;
        wm_arcade_drone_init(&d, 15);
        d.mode = -1;
        g_picked = NULL;
        (void)wm_arcade_drone_main(&a, &d, &w, &cb);
        if (k == 0) {
            assert(g_picked != NULL);
            assert(strcmp(g_picked, "drn_enterring") == 0);
        } else {
            assert(g_picked == NULL || strcmp(g_picked, "drn_enterring") != 0);
        }
    }
}

/*
 * 2. start_run_flung, run on the real program. Its two ANI_CODEs turn
 * him round (#x_flip, `xori >0C`) and then (#ok2 into #contx) clear
 * DELAY_METER, park #dorun_flung in CODE_ADDR, SETMODE WAITANIM, halve
 * OBJ_XVEL with an arithmetic shift and zero OBJ_ZVEL. ANI_GETUP gives
 * him FLUNG_TIME and ANI_END marks the animation ended.
 */
static void test_a_flung_wrestler_is_handed_back(void)
{
    const wm_anim_program *prog = wm_anim_program_find("start_run_flung");
    wm_anim_exec ex;
    wm_arcade_actor_t a;

    assert(prog != NULL);
    actor_at(&a, 1000, 1200);
    a.wrestler_num = WM_ROSTER_TAKER;
    a.player_mode = WM_PMODE_PUPPET;
    a.facing_dir = WM_MOVE_RIGHT;
    a.new_facing_dir = WM_MOVE_RIGHT | WM_MOVE_DOWN;
    a.x_vel = 4 << 16;
    a.z_vel = 3 << 16;
    a.delay_meter = 7;
    memset(&ex, 0, sizeof ex);
    wm_anim_exec_start(&ex, prog, &a, 0, NULL);

    assert(a.player_mode == WM_PMODE_WAITANIM);
    assert(a.code_addr == (uintptr_t)WM_CODE_ADDR_DORUN_FLUNG);
    assert(a.x_vel == 2 << 16);
    assert(a.z_vel == 0);
    assert(a.delay_meter == 0);
    /* SETFACING took NEW_FACING_DIR, then #x_flip swapped left/right. */
    assert(a.facing_dir == (WM_MOVE_LEFT | WM_MOVE_DOWN));
    assert(a.getup_time == 120);              /* FLUNG_TIME, 170-20-30 */
    assert(a.anim_mode & WM_MODE_END);

    /* `sra 1`: a leftward fling stays leftward, rounding toward -inf. */
    actor_at(&a, 1000, 1200);
    a.player_mode = WM_PMODE_PUPPET;
    a.x_vel = -3;
    wm_anim_exec_start(&ex, prog, &a, 0, NULL);
    assert(a.x_vel == -2);

    /* MACROS.H SETMODE never overwrites MODE_DEAD. */
    actor_at(&a, 1000, 1200);
    a.player_mode = WM_PMODE_DEAD;
    wm_anim_exec_start(&ex, prog, &a, 0, NULL);
    assert(a.player_mode == WM_PMODE_DEAD);
}

/*
 * ...and mode_waitanim's `call a0` lands on #dorun_flung, in both
 * backends: his own run animation, MODE RUNNING, DELAY_BUTNS = 1. Unlike
 * #dorun it does NOT clear GETUP_TIME.
 */
static void test_dorun_flung_starts_his_run(void)
{
    wm_wrestler_backend_actor st;
    wm_bret_backend_actor bva;
    wm_arcade_roster_callbacks_t roster_cb;
    wm_arcade_bret_callbacks_t bret_cb;
    wm_arcade_actor_t a;

    memset(&st, 0, sizeof st);
    st.wrestler_num = WM_ROSTER_TAKER;
    roster_cb = wm_wrestler_roster_callbacks(&st);

    actor_at(&a, 1000, 1200);
    a.wrestler_num = WM_ROSTER_TAKER;
    a.player_mode = WM_PMODE_WAITANIM;
    a.code_addr = (uintptr_t)WM_CODE_ADDR_DORUN_FLUNG;
    a.facing_dir = WM_MOVE_LEFT;
    a.getup_time = 90;
    a.run_time = 5;
    roster_cb.code_addr(&a, (uint32_t)a.code_addr, roster_cb.user);
    assert(a.player_mode == WM_PMODE_RUNNING);
    assert(a.code_addr == 0);
    assert(a.delay_butns == 1);
    assert(a.run_time == 0);
    assert(a.move_dir == WM_MOVE_LEFT);
    assert(a.getup_time == 90);
    assert(st.current_label != NULL);
    assert(strcmp(st.current_label, "und_run_anim") == 0);

    wm_bret_backend_init(&bva);
    bret_cb = wm_bret_backend_callbacks(&bva);
    actor_at(&a, 1000, 1200);
    a.wrestler_num = WM_ROSTER_BRET;
    a.player_mode = WM_PMODE_WAITANIM;
    a.code_addr = (uintptr_t)WM_CODE_ADDR_DORUN_FLUNG;
    bret_cb.code_addr(&a, (uint32_t)a.code_addr, bret_cb.user);
    assert(a.player_mode == WM_PMODE_RUNNING);
    assert(a.code_addr == 0);
}

/* A two-op program: whatever `first` is, then ANI_END. */
static wm_anim_op g_ops[2];
static wm_anim_program g_prog;
static const wm_anim_program *two_op(wm_anim_op first)
{
    memset(g_ops, 0, sizeof g_ops);
    g_ops[0] = first;
    g_ops[0].target = -1;
    g_ops[1].op = WM_AOP_END;
    g_ops[1].target = -1;
    memset(&g_prog, 0, sizeof g_prog);
    g_prog.source_label = "test_two_op";
    g_prog.source_file = "TEST.ASM";
    g_prog.ops = g_ops;
    g_prog.op_count = 2;
    return &g_prog;
}

/*
 * 3. ANIM.ASM:371 _ani_setmode writes `*a10(OANIMODE)`, and a10 is
 * a13+ANIMODE2 on the secondary channel (:84). A torso SETMODE must not
 * touch the body's word -- it used to, and cleared MODE_UNINT under a
 * grab the tick it connected. The STATUS_FLAGS half is a13-relative,
 * so it happens on either channel.
 */
static void test_a_torso_setmode_leaves_the_body_alone(void)
{
    wm_anim_op op;
    wm_anim_exec torso, body;
    wm_arcade_actor_t a;
    const uint16_t held = WM_MODE_UNINT | WM_MODE_NOAUTOFLIP | WM_MODE_OVERLAP;

    memset(&op, 0, sizeof op);
    op.op = WM_AOP_SETMODE;
    op.a = 0;

    actor_at(&a, 1000, 1200);
    a.anim_mode = held;
    a.status_flags = WM_STATUS_KOD | WM_STATUS_PUSH | WM_STATUS_ZOMBIE;
    memset(&torso, 0, sizeof torso);
    wm_anim_exec_start_secondary(&torso, two_op(op), &a, 0, NULL);
    assert(a.anim_mode == held);
    /* SF_CLEAR_BITS cleared, everything else kept. */
    assert((a.status_flags & WM_STATUS_SF_CLEAR_BITS) == 0);
    assert(a.status_flags & WM_STATUS_ZOMBIE);

    /* The primary channel's SETMODE is still absolute. */
    op.a = WM_MODE_NOAUTOFLIP;
    memset(&body, 0, sizeof body);
    wm_anim_exec_start(&body, two_op(op), &a, 0, NULL);
    assert((a.anim_mode & (uint16_t)~WM_MODE_END) == WM_MODE_NOAUTOFLIP);
}

/* 4. ANI_DAMAGEOPP through the env's adjust_health. */
static int g_calls;
static int16_t g_delta;
static wm_arcade_actor_t *g_victim, *g_source;
static void count_health(void *user, wm_arcade_actor_t *victim,
                         int16_t delta, wm_arcade_actor_t *source)
{
    (void)user;
    ++g_calls;
    g_delta = delta;
    g_victim = victim;
    g_source = source;
}

static void test_an_animation_blow_is_the_matchs_blow(void)
{
    wm_anim_op op;
    wm_anim_exec ex;
    wm_anim_env env;
    wm_arcade_actor_t att, vic;

    memset(&op, 0, sizeof op);
    op.op = WM_AOP_DAMAGEOPP;
    op.a = 10;                                /* FULL */
    op.b = 5;                                 /* REDUCED */

    actor_at(&att, 1000, 1200);
    actor_at(&vic, 1040, 1200);
    att.attach_proc = &vic;
    memset(&env, 0, sizeof env);
    env.adjust_health = count_health;
    env.pcnt = 500;
    g_calls = 0;
    memset(&ex, 0, sizeof ex);
    wm_anim_exec_start(&ex, two_op(op), &att, 0, &env);
    assert(g_calls == 1);
    assert(g_delta == -10);
    assert(g_victim == &vic);
    assert(g_source == &att);
    assert(vic.life == 163);                  /* the seam did the work */

    /* With no seam, the plain routine still applies the blow. */
    env.adjust_health = NULL;
    wm_anim_exec_start(&ex, two_op(op), &att, 0, &env);
    assert(vic.life < 163);
}

/*
 * The first-rung fact. ATTRACT.ASM:596-602 picks CURRENT_LADDER from
 * rungs 1 to 6, never LADDER itself, so an attract bout is never the
 * first rung and its drones may roll aggressive modes.
 */
static void test_attract_is_never_the_first_rung(void)
{
    static wm_match_state m;
    static WmRng rng;
    wm_rng_init(&rng, 1u, 0, 0, 0);
    wm_match_init(&m);
    wm_match_start_attract(&m, &rng);
    assert(!m.first_ladder);

    wm_match_init(&m);
    wm_match_start_one_player(&m, &rng, 1, 0, 0);
    assert(m.first_ladder);
}

int main(void)
{
    wm_rng_init(&g_rng, 7u, 0, 0, 0);
    test_ontb_pushes_into_the_turnbuckle();
    test_ontb_from_outside_goes_back_into_the_ring();
    test_ering_hands_over_in_the_same_tick();
    test_enterring_is_for_the_man_outside();
    test_a_flung_wrestler_is_handed_back();
    test_dorun_flung_starts_his_run();
    test_a_torso_setmode_leaves_the_body_alone();
    test_an_animation_blow_is_the_matchs_blow();
    test_attract_is_never_the_first_rung();
    printf("quiet bout: corner push, flung recovery, torso mode, "
           "animation damage, INRING polarity, first rung\n");
    return 0;
}
