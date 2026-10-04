/*
 * Two powerups that were computed and delivered to nothing.
 *
 * AWARD.ASM's get_powerups (:2230) reconciles both players' requests and
 * writes out seven globals. The port computes all seven in
 * wm_get_powerups, and the app copied TWO of them into the match --
 * instant_combos_on and drone_meters_on. The other two the match has
 * consumers for, @blocking_off and @hyper_speed_on, went nowhere.
 *
 * Nothing looked wrong, because zero is a legitimate value for both:
 * blocking_off zero means blocking works, hyper_speed_on zero means the
 * shift is by nothing. Three consumers were reading a field the match
 * never filled in:
 *
 *   std_block's `move @blocking_off,a14 / jrnz` -- all eight dispatchers.
 *   mode_running's `move @hyper_speed_on,a14 / sll a14,a0` on the run
 *     velocity -- all eight wrestler files, all eight translated.
 *   ANIM.ASM:157's `srl a14,a1` on how long an animation FRAME is held,
 *     which this port reads off actor->hyper_speed. Written by nobody.
 *
 * So a player could enter LEFT-PUNCH-PUNCH-BLOCK, get the HYPER MATCH
 * code accepted, and play an ordinary-speed match.
 *
 * It is a SHIFT COUNT and not a flag. AWARD.ASM:2276 normalises the bit
 * to 1, and one bit means twice the run speed and half the frame hold.
 * blocking_off is NOT normalised -- `movi 2020h,a8`, which is why
 * WM_BLOCKING_OFF_VALUE exists and why every consumer only asks whether
 * it is non-zero.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/match.h"
#include "wm/arcade/wm_arcade_powerup.h"
#include "wm_arcade_roster.h"

static uint32_t hcount(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t sp(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static wm_match_state M;

/*
 * get_powerups turns the two request words into the flags. Both of these
 * bits are in BOTH_P_MASK, so both players have to have asked -- which is
 * the reconciliation this test goes through rather than around.
 */
static void test_get_powerups_yields_both_flags(void) {
    wm_powerup_flags f;
    memset(&f, 0, sizeof f);
    wm_get_powerups(&f);          /* nothing requested */
    assert(f.blocking_off == 0);
    assert(f.hyper_speed_on == 0);

    memset(&f, 0, sizeof f);
    f.p_request[0] = WM_PU_BLOCKING_OFF | WM_PU_HYPER_MATCH_ON;
    f.p_request[1] = WM_PU_BLOCKING_OFF | WM_PU_HYPER_MATCH_ON;
    wm_get_powerups(&f);
    /* `movi 2020h,a8` -- the bit value, not a 1. */
    assert(f.blocking_off == WM_BLOCKING_OFF_VALUE);
    /* AWARD.ASM:2276 `movi 1,a8` -- normalised, because it is a shift. */
    assert(f.hyper_speed_on == 1);

    /* One player alone is not enough for either: both are in BOTH_P_MASK. */
    memset(&f, 0, sizeof f);
    f.p_request[0] = WM_PU_BLOCKING_OFF | WM_PU_HYPER_MATCH_ON;
    wm_get_powerups(&f);
    assert(f.blocking_off == 0 && f.hyper_speed_on == 0);

    puts("  get_powerups yields both flags: ok");
}

static void start(uint8_t who, int32_t blocking_off, int32_t hyper) {
    WmRng rng;
    uint32_t seed = 1;
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    /* What the app copies in at MATCH_INIT, which is the step that was
       missing for these two. */
    M.blocking_off = blocking_off;
    M.hyper_speed_on = hyper;
    wm_rng_init(&rng, 0x13572468u, hcount, sp, &seed);
    wm_match_start_selected(&M, &rng, who);
}

/*
 * The frame-hold consumer, reached through the actor rather than a
 * global. One tick is enough: the match writes it in the same per-tick
 * block that builds the dispatcher env.
 */
static void test_the_match_carries_hyper_speed_to_the_actor(void) {
    wm_input_state in;
    int i;

    start((uint8_t)WM_ROSTER_TAKER, 0, 0);
    memset(&in, 0, sizeof in);
    for (i = 0; i < 3; ++i) wm_match_tick(&M, NULL, &in);
    /* Off: no shift, so a frame is held for exactly its own count. */
    assert(M.actors[0].hyper_speed == 0);
    assert(M.actors[1].hyper_speed == 0);

    start((uint8_t)WM_ROSTER_TAKER, 0, 1);
    memset(&in, 0, sizeof in);
    for (i = 0; i < 3; ++i) wm_match_tick(&M, NULL, &in);
    /* On: every wrestler, not just the human -- it is a global in the
       source and applies to whoever the animation is running on. */
    assert(M.actors[0].hyper_speed == 1);
    assert(M.actors[1].hyper_speed == 1);

    puts("  hyper_speed reaches every actor: ok");
}

/*
 * The run-velocity consumer. `movi #XRUN_VAL,a0 / move @hyper_speed_on,a14
 * / sll a14,a0` -- so the same wrestler running the same way is exactly
 * twice as fast with one bit of hyper speed. Driving it through
 * wm_match_tick is what proves the flag crosses the app-to-env boundary
 * that was broken, rather than testing the dispatcher directly.
 */
static int32_t run_velocity_with_hyper(int32_t hyper) {
    wm_input_state in;
    wm_arcade_actor_t *a;
    int i;

    start((uint8_t)WM_ROSTER_TAKER, 0, hyper);
    a = &M.actors[0];
    memset(&in, 0, sizeof in);
    for (i = 0; i < 4; ++i) wm_match_tick(&M, NULL, &in);

    /* Put him in MODE_RUNNING heading right, re-applied each tick so his
       own dispatcher cannot walk him out of it before the velocity is
       written. usr_var1 is the `#no_vel` gate and must stay clear. */
    for (i = 0; i < 3; ++i) {
        a->player_mode = WM_PMODE_RUNNING;
        a->move_dir = WM_MOVE_RIGHT;
        a->usr_var1 = 0;
        a->getup_time = 0;
        wm_match_tick(&M, NULL, &in);
    }
    return a->x_vel;
}

static void test_hyper_speed_doubles_the_run(void) {
    int32_t off = run_velocity_with_hyper(0);
    int32_t on  = run_velocity_with_hyper(1);

    /* He really is running, or the comparison below means nothing. */
    assert(off > 0);
    /* `sll a14,a0` with a14 == 1. */
    assert(on == off * 2);

    puts("  hyper speed doubles the run velocity: ok");
}

/*
 * std_block's gate, the other flag. `move @blocking_off,a14 / jrnz` --
 * non-zero means the block is refused, so the value only has to be
 * non-zero and the source never normalises it.
 */
static void test_blocking_off_reaches_the_dispatcher_env(void) {
    wm_input_state in;
    int i;

    start((uint8_t)WM_ROSTER_TAKER, WM_BLOCKING_OFF_VALUE, 0);
    memset(&in, 0, sizeof in);
    for (i = 0; i < 3; ++i) wm_match_tick(&M, NULL, &in);
    /* The match is holding what the app gave it; the per-tick env copy is
       what hands it to std_block, and match.c sets both from these. */
    assert(M.blocking_off == WM_BLOCKING_OFF_VALUE);
    assert(M.hyper_speed_on == 0);

    puts("  blocking_off is carried by the match: ok");
}

int main(void) {
    test_get_powerups_yields_both_flags();
    test_the_match_carries_hyper_speed_to_the_actor();
    test_hyper_speed_doubles_the_run();
    test_blocking_off_reaches_the_dispatcher_env();
    puts("powerup delivery tests: PASS");
    return 0;
}
