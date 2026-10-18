/*
 * DRONE.ASM:121 -- how drone_main finds out who it is fighting.
 *
 *      move  *a13(CLOSEST_NUM),a14
 *      X32   a14
 *      addi  process_ptrs,a14
 *      move  *a14,a8,L          ;A8=*Closest's proc
 *
 * CLOSEST_NUM is PLYR.EQU:72, "number of closest opponent" -- a PLYRNUM,
 * written by calc_closest's `#accept` (WRESTLE.ASM:4322) and resolved
 * against process_ptrs, which is what the drone world's actor list is.
 *
 * The port used to take the opponent from smart_target instead. That is
 * the SMART_ATTACK lock: match.c sets it once as a fixed pair at match
 * start and wm_arcade_ani_attack_off zeroes it every time an attack
 * animation ends. It was NULL on 95.3% of actor-ticks, so drone_main's
 * `if (!opp) return IDLE` bailed out and the drone AI was off from tick
 * 927 to the end of the match.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/match.h"
#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wm_arcade_closest.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm_arcade_roster.h"

static uint32_t g_r;
static uint32_t rnd_up(uint32_t max, void *u) { (void)u; g_r += 0x9e37u; return max ? (g_r % (max + 1u)) : 0u; }

/* Records which script drone_main picked, so the test can watch the
   code BEHIND the `if (!opp)` bail rather than guess at a step result:
   the NORMAL path has its own PCNT gates and random rolls and may
   legitimately idle with an opponent in hand. */
static const char *g_picked;
static void on_script(wm_arcade_actor_t *a, const char *label, void *u) {
    (void)a; (void)u; g_picked = label;
}

/* calc_closest writes CLOSEST_NUM beside the four distances, because
   that is where `#accept` writes it. */
static void test_calc_closest_records_the_plyrnum(void)
{
    wm_arcade_actor_t a, b;
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    a.player_num = 2;
    b.player_num = 3;
    a.x_int = 100; b.x_int = 160;
    a.closest_num = WM_CLOSEST_NUM_NONE;

    wm_arcade_calc_closest(&a, &b);
    assert(a.closest_num == 3);
    /* It is the opponent's PLYRNUM, not his slot and not our own. */
    assert(a.closest_num != a.player_num);
    assert(a.closest_xdist == 60);
}

/*
 * A fresh wrestler has NO closest opponent, not player one's.
 *
 * init_actor_life memsets, and 0 is player one's real PLYRNUM, so a
 * plain zero would mean "my closest opponent is player one" before
 * calc_closest had run once -- accidentally right in a singles match
 * and wrong the moment buddy mode puts four men in the ring.
 */
static void test_a_fresh_actor_has_no_closest(void)
{
    static wm_match_state m;
    static WmRng rng;
    size_t i;
    wm_rng_init(&rng, 1u, 0, 0, 0);
    wm_match_init(&m);
    wm_match_start_attract(&m, &rng);
    assert(m.actor_count >= 2u);
    for (i = 0; i < m.actor_count; ++i)
        assert(m.actors[i].closest_num == WM_CLOSEST_NUM_NONE);
    assert(WM_CLOSEST_NUM_NONE != 0);
}

/*
 * THE REGRESSION. drone_main must find its opponent through CLOSEST_NUM
 * and the world's actor list, with no smart_target and no
 * closest_actor callback -- which is the state a live match is in for
 * 95% of its ticks.
 *
 * PLYRMODE ONGROUND is the probe, because its arm of `#doact` selects
 * drn_roll unconditionally -- no PCNT gate, no roll. So the script
 * being picked means the opponent was resolved, and nothing else in
 * the routine can produce it.
 */
static void test_drone_main_resolves_the_opponent_without_smart_target(void)
{
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *arr[2];
    wm_arcade_drone_state_t d;
    wm_arcade_drone_world_t w;
    wm_arcade_drone_callbacks_t cb;

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    memset(&cb, 0, sizeof cb);
    arr[0] = &a; arr[1] = &b;
    a.active = b.active = 1;
    a.in_ring = b.in_ring = 1;
    a.life = b.life = 163;
    a.player_mode = WM_PMODE_ONGROUND;
    b.player_mode = WM_PMODE_NORMAL;
    a.player_num = 2; b.player_num = 3;
    a.player_side = 0; b.player_side = 1;
    a.wrestler_num = WM_ROSTER_BRET; b.wrestler_num = WM_ROSTER_BRET;
    a.x_int = 900; b.x_int = 1100;
    a.z_int = b.z_int = 1200;
    a.facing_dir = WM_MOVE_RIGHT;
    /* Exactly the live state: the SMART_ATTACK lock is clear. */
    a.smart_target = 0;
    b.smart_target = 0;
    cb.rnd_upto = rnd_up;
    cb.rndrng0_upto = rnd_up;
    cb.script_selected = on_script;
    /* and no closest_actor callback to fall back on. */
    assert(cb.closest_actor == 0);

    memset(&w, 0, sizeof w);
    w.actors = arr; w.actor_count = 2; w.pcnt = 16; w.round_tickcount = 16;
    wm_arcade_drone_init(&d, 15);

    /* With no CLOSEST_NUM there is nothing to resolve and nothing to
       fall back on, so drone_main cannot get past `if (!opp)`. */
    g_picked = 0;
    d.script = 0;
    a.closest_num = WM_CLOSEST_NUM_NONE;
    (void)wm_arcade_drone_main(&a, &d, &w, &cb);
    assert(g_picked == 0);

    /* Given CLOSEST_NUM it finds him in the actor list and gets there. */
    g_picked = 0;
    d.script = 0;
    a.closest_num = b.player_num;
    (void)wm_arcade_drone_main(&a, &d, &w, &cb);
    assert(g_picked != 0);
    assert(strcmp(g_picked, "drn_roll") == 0);

    /* It is the actor list that resolves him, not the pointer: with the
       list withheld, the same CLOSEST_NUM resolves to nobody. */
    g_picked = 0;
    d.script = 0;
    w.actors = 0;
    (void)wm_arcade_drone_main(&a, &d, &w, &cb);
    assert(g_picked == 0);
    w.actors = arr;
}

/* A PLYRNUM that names nobody resolves to nobody rather than to slot
   zero: the source indexes process_ptrs and an empty slot is NULL. */
static void test_an_unknown_plyrnum_resolves_to_nobody(void)
{
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *arr[2];
    wm_arcade_drone_state_t d;
    wm_arcade_drone_world_t w;
    wm_arcade_drone_callbacks_t cb;

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    memset(&cb, 0, sizeof cb);
    arr[0] = &a; arr[1] = &b;
    a.active = b.active = 1;
    a.player_num = 2; b.player_num = 3;
    a.smart_target = 0;
    cb.rnd_upto = rnd_up;
    cb.rndrng0_upto = rnd_up;
    memset(&w, 0, sizeof w);
    w.actors = arr; w.actor_count = 2;
    wm_arcade_drone_init(&d, 15);

    a.player_mode = WM_PMODE_ONGROUND;
    cb.script_selected = on_script;

    g_picked = 0;
    a.closest_num = 9;   /* nobody in this ring */
    (void)wm_arcade_drone_main(&a, &d, &w, &cb);
    assert(g_picked == 0);

    /* And it never resolves to SELF, even when CLOSEST_NUM is our own
       PLYRNUM -- `p == self` is skipped, as the source's own
       `cmp a11,a13 / jreq #skip` does in every pair loop. */
    g_picked = 0;
    d.script = 0;
    a.closest_num = a.player_num;
    (void)wm_arcade_drone_main(&a, &d, &w, &cb);
    assert(g_picked == 0);
}

int main(void)
{
    test_calc_closest_records_the_plyrnum();
    test_a_fresh_actor_has_no_closest();
    test_drone_main_resolves_the_opponent_without_smart_target();
    test_an_unknown_plyrnum_resolves_to_nobody();
    printf("drone closest: CLOSEST_NUM resolves the opponent, not smart_target\n");
    return 0;
}
