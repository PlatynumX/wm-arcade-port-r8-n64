/*
 * The head of the wrestler loop, on the live match.
 *
 * WRESTLE.ASM:2411-2421 runs, after move_wrestler and before the SLEEP:
 * ARE_WE_IN_RING, set_collision_boxes, confine_wrestler and its fix2,
 * update_newfacing, and drone_main. ARE_WE_IN_RING (SPECIAL.ASM:4565)
 * had been translated and called by nothing, so a wrestler could stay
 * outside the ring for good at no cost, and RING_TIME -- which the
 * Undertaker's finishing move reads (TAKER.ASM:652) -- was a stand-in
 * derived from INRING. And drone_main ran at the TOP of the port's tick,
 * so a drone's decision reached its stick in the same tick rather than
 * after the sleep, through update_joystat.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/match.h"
#include "wm_arcade_roster.h"
#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wm_arcade_drone_data.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wm_arcade_powerup.h"
#include "wm/arcade/wmania_ring_geometry.h"

static uint32_t g_tick;
static uint32_t hc(void *u) { (void)u; return g_tick * 7u + 13u; }
static uint32_t spf(void *u) { (void)u; return 0x1234u + g_tick; }

static wm_match_state M;
static WmRng R;
static unsigned SOUNDS_C4;

static void count_sound(void *user, uint16_t id) {
    (void)user;
    if (id == 0xc4u) ++SOUNDS_C4;
}

/* Taker for the human (idle, inside); the drawn opponent is the one
   put outside. No drone callbacks, so nobody walks him back in. */
static unsigned start(void) {
    unsigned opp = 1;
    g_tick = 0;
    wm_rng_init(&R, 7u, hc, spf, NULL);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    M.anim_sound = count_sound;
    SOUNDS_C4 = 0;
    wm_match_start_selected(&M, &R, (uint8_t)WM_ROSTER_TAKER);
    assert(M.actor_count == 2 && M.actors[opp].player_side == 1);
    return opp;
}

static void hold_outside(wm_arcade_actor_t *a) {
    a->in_ring = 0;
    a->x_int = WM_RING_X_CENTER + 500;
    a->z_int = WM_RING_Z_CENTER;
}

static void tick(void) {
    wm_input_state in;
    memset(&in, 0, sizeof in);
    wm_match_tick(&M, NULL, &in);
    ++g_tick;
}

#define OUT_TIME (WM_TSEC * 7)

/*
 * RING_TIME counts up inside and down outside; past seven seconds out,
 * with the man inside past seven seconds in, it costs a point of life
 * on every eighth tick.
 */
static void test_seven_seconds_outside_costs_life(void) {
    unsigned opp = start(), t;
    int32_t life_at_threshold = -1;
    wm_arcade_actor_t *o = &M.actors[opp];

    hold_outside(o);
    for (t = 0; t < (unsigned)OUT_TIME + 200u; ++t) {
        hold_outside(o);
        tick();
        assert(M.actors[0].ring_time > 0);      /* the man inside */
        assert(o->ring_time < 0);               /* the man outside */
        if (o->ring_time == -OUT_TIME) life_at_threshold = o->life;
        if (o->ring_time > -OUT_TIME) assert(o->life == WM_ARCADE_LIFE_MAX);
    }
    assert(life_at_threshold == WM_ARCADE_LIFE_MAX);
    /* About 200 ticks past it: one point in eight. adjust_health's own
       85% scaling takes a single point to (-1*217)>>8 = -1. */
    assert(o->life < WM_ARCADE_LIFE_MAX);
    assert(WM_ARCADE_LIFE_MAX - o->life >= 200 / 8 - 1);
    assert(WM_ARCADE_LIFE_MAX - o->life <= 200 / 8 + 1);
}

/* Nothing is taken while the man inside has not been in seven seconds. */
static void test_no_hurt_while_the_other_man_is_out_too(void) {
    unsigned opp = start(), t;
    for (t = 0; t < (unsigned)OUT_TIME + 200u; ++t) {
        hold_outside(&M.actors[opp]);
        hold_outside(&M.actors[0]);
        M.actors[0].x_int = WM_RING_X_CENTER - 500;
        tick();
    }
    assert(M.actors[opp].life == WM_ARCADE_LIFE_MAX);
    assert(M.actors[0].life == WM_ARCADE_LIFE_MAX);
}

/* DEATH BY RING-OUT: SETMODE DEAD, CREATE_DISQUAL's 0c4h, and the
   announcer for the round. */
static void test_a_ring_out_death_disqualifies(void) {
    unsigned opp = start(), t;
    wm_arcade_actor_t *o = &M.actors[opp];
    hold_outside(o);
    for (t = 0; t < (unsigned)OUT_TIME + 40u && o->player_mode != WM_PMODE_DEAD; ++t) {
        hold_outside(o);
        if (o->ring_time < -OUT_TIME + 2) o->life = 1;
        tick();
    }
    assert(o->player_mode == WM_PMODE_DEAD);
    assert(SOUNDS_C4 == 1);
    assert(M.round_announce.phase != 0);
    /* Dead outside: RING_TIME restarts at 1 and counts up. */
    tick();
    assert(o->ring_time == 2 || o->ring_time == 1);
}

/*
 * @ring_out_on (AWARD.ASM:2262): going over the ropes starts
 * kill_when_hit_ground, which takes 150 when he is on the ground.
 */
static void test_ring_outs_on_kill_when_he_lands(void) {
    unsigned opp = start(), t;
    wm_arcade_actor_t *o = &M.actors[opp];
    M.ring_out_on = 1;
    for (t = 0; t < 10; ++t) tick();              /* inside, counting up */
    assert(o->ring_time > 0);
    assert(!M.kill_when_hit_ground[opp]);
    hold_outside(o);
    o->y_int = o->ground_y + 40;                  /* in the air */
    tick();
    assert(o->ring_time == -1);
    assert(M.kill_when_hit_ground[opp]);
    assert(o->life == WM_ARCADE_LIFE_MAX);
    /* Down he comes. */
    for (t = 0; t < 120 && M.kill_when_hit_ground[opp]; ++t) {
        hold_outside(o);
        tick();
    }
    assert(!M.kill_when_hit_ground[opp]);
    assert(o->life < WM_ARCADE_LIFE_MAX - 100);
}

/*
 * drone_main runs at the loop head, after the tick's body: it sees the
 * drone where this tick's velocities and confine put him. It used to run
 * at the top of the tick and see him where the last tick left him.
 * Every drone callback samples the drone's position; the last sample in
 * a tick has to be his position when the tick ends.
 */
static wm_arcade_drone_callbacks_t REAL;
static int32_t SEEN_X, SEEN_Z;
static int SAMPLED;
static unsigned DRONE;

static void sample(void) {
    SEEN_X = M.actors[DRONE].x_int;
    SEEN_Z = M.actors[DRONE].z_int;
    SAMPLED = 1;
}
static uint32_t w_rnd_upto(uint32_t n, void *u) { sample(); return REAL.rnd_upto(n, u); }
static uint32_t w_rndrng0_upto(uint32_t n, void *u) { sample(); return REAL.rndrng0_upto(n, u); }
static const wm_arcade_drone_script_t *w_resolve(const char *l, void *u) {
    sample(); return REAL.resolve_script(l, u);
}

static void test_drone_main_sees_the_end_of_the_tick(void) {
    wm_arcade_drone_callbacks_t cb;
    wm_input_state in;
    unsigned t, sampled = 0, moved = 0, seen_at_end = 0;
    unsigned i;
    g_tick = 0;
    wm_rng_init(&R, 5u, hc, spf, NULL);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_match_start_selected(&M, &R, (uint8_t)WM_ROSTER_TAKER);
    for (i = 0; i < M.actor_count; ++i)
        if (!M.actor_is_human[i]) DRONE = i;
    assert(!M.actor_is_human[DRONE]);
    REAL = wm_arcade_drone_data_callbacks(&R);
    cb = REAL;
    if (cb.rnd_upto) cb.rnd_upto = w_rnd_upto;
    if (cb.rndrng0_upto) cb.rndrng0_upto = w_rndrng0_upto;
    if (cb.resolve_script) cb.resolve_script = w_resolve;
    memset(&in, 0, sizeof in);
    for (t = 0; t < 1500 && !M.match_over; ++t) {
        int32_t x0 = M.actors[DRONE].x_int, z0 = M.actors[DRONE].z_int;
        SAMPLED = 0;
        wm_match_tick(&M, &cb, &in);
        ++g_tick;
        if (!SAMPLED) continue;
        ++sampled;
        if (M.actors[DRONE].x_int == x0 && M.actors[DRONE].z_int == z0)
            continue;
        ++moved;
        /* At the top of the tick he would have been seen at (x0, z0). */
        if (SEEN_X == M.actors[DRONE].x_int && SEEN_Z == M.actors[DRONE].z_int)
            ++seen_at_end;
    }
    /*
     * The main loop's own pass -- check_collisions, final_confine
     * (WRESTLE.ASM:2063-2064) -- runs after every wrestler process and can
     * still move him after his drone_main, so a few ticks differ by that.
     * Seen from the top of the tick, every tick on which his own body
     * moved him would differ.
     */
    assert(sampled > 100);
    assert(moved > 50);
    assert(seen_at_end * 100u >= moved * 95u);
}

int main(void) {
    test_seven_seconds_outside_costs_life();
    test_no_hurt_while_the_other_man_is_out_too();
    test_a_ring_out_death_disqualifies();
    test_ring_outs_on_kill_when_he_lands();
    test_drone_main_sees_the_end_of_the_tick();
    printf("ring out live ok\n");
    return 0;
}
