/*
 * LIFEBAR.ASM:1712 `#grnd` -- the convulse a wrestler dies with when he
 * was ALREADY on the ground when the killing blow landed, instead of
 * the fall-back he gets when he was on his feet.
 *
 * This port took the catch-all for both, behind a note saying #grnd was
 * "only reachable when player_mode is already ONGROUND or DEAD, which
 * this port's Bret dispatcher never sets -- and no hrt_hitonground_anim
 * data has been extracted either". Four live sites set ONGROUND and all
 * nine wrestlers' hitonground animations are in the generated programs,
 * so both halves of that were false.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_lifebar.h"
#include "wm/arcade/wm_arcade_roster_anims.h"
#include "wm/arcade/wm_arcade_react1_core.h"

static wm_arcade_react1_anim_group_t LAST_ANIM;
static int ANIM_CALLS;

static void record(wm_arcade_actor_t *a, wm_arcade_react1_anim_group_t g,
                   void *user) {
    (void)a; (void)user;
    LAST_ANIM = g;
    ++ANIM_CALLS;
}

static void kill_from_mode(wm_arcade_actor_t *v, wm_arcade_actor_t *src,
                           uint16_t mode) {
    wm_arcade_death_anim_callback_t cb;
    memset(v, 0, sizeof *v);
    v->active = 1;
    v->life = 5;
    v->player_mode = mode;
    memset(src, 0, sizeof *src);
    src->active = 1;
    src->x_int = 100;
    v->x_int = 200;

    cb.change_anim = record;
    cb.user = NULL;
    LAST_ANIM = (wm_arcade_react1_anim_group_t)0;
    ANIM_CALLS = 0;
    wm_arcade_adjust_health(v, -50, src, false, 1000u, NULL, &cb);
}

/*
 * The source tests MODE_DEAD first and MODE_ONGROUND second, and both
 * land on #grnd -- so a wrestler hit again after he is already dead
 * convulses too.
 */
static void test_a_man_already_down_convulses(void) {
    wm_arcade_actor_t v, src;

    kill_from_mode(&v, &src, WM_PMODE_ONGROUND);
    assert(ANIM_CALLS == 1);
    assert(LAST_ANIM == WM_R1_ANIM_HIT_ON_GROUND);
    assert(v.player_mode == WM_PMODE_DEAD);
    /* `#grnd` touches no velocity: the knockback belongs to #fallbk. */
    assert(v.x_vel == 0);

    kill_from_mode(&v, &src, WM_PMODE_DEAD);
    assert(ANIM_CALLS == 1);
    assert(LAST_ANIM == WM_R1_ANIM_HIT_ON_GROUND);
}

/* On his feet, he still falls back and is still knocked away. */
static void test_a_man_on_his_feet_falls_back(void) {
    wm_arcade_actor_t v, src;

    kill_from_mode(&v, &src, WM_PMODE_NORMAL);
    assert(ANIM_CALLS == 1);
    assert(LAST_ANIM == WM_R1_ANIM_FALL_BACK);
    /* Killer is to his LEFT (x 100 vs 200), so he is knocked right. */
    assert(v.x_vel == (2 << 16));

    kill_from_mode(&v, &src, WM_PMODE_DIZZY);
    assert(LAST_ANIM == WM_R1_ANIM_FALL_BACK);
}

/*
 * `#will_die`: HEADHELD does not die now, it dies in three seconds --
 * and it reaches `#skip`, which is BELOW the SETMODE DEAD, so the mode
 * is deliberately NOT changed.
 */
static void test_headheld_defers_instead_of_dying(void) {
    wm_arcade_actor_t v, src;

    kill_from_mode(&v, &src, WM_PMODE_HEADHELD);
    assert(ANIM_CALLS == 0);            /* no animation on this path */
    assert(v.i_will_die == 3 * 60);
    assert(v.player_mode == WM_PMODE_HEADHELD);   /* NOT dead */
}

/* Anything else still takes the catch-all: velocities zeroed, no anim. */
static void test_the_catch_all_is_unchanged(void) {
    wm_arcade_actor_t v, src;

    kill_from_mode(&v, &src, WM_PMODE_ATTACHED);
    assert(ANIM_CALLS == 0);
    assert(v.x_vel == 0 && v.y_vel == 0 && v.z_vel == 0);
    assert(v.player_mode == WM_PMODE_DEAD);
}

/*
 * convulse_t (LIFEBAR.ASM:1863) is its own table, not a borrowed copy
 * of REACT1's hitonground_tbl. It was missed by the table generator
 * because a `#hitonground` local label sits between the SUBR and its
 * data. Both are extracted now, and they agree -- which is worth
 * asserting rather than assuming, since that agreement is the only
 * reason borrowing would ever have been safe.
 */
static void test_convulse_t_is_extracted_and_agrees(void) {
    const wm_roster_anim_table *convulse = wm_roster_anim_find("convulse_t");
    const wm_roster_anim_table *react1 = wm_roster_anim_find("hitonground_tbl");
    const wm_roster_anim_table *fallbacks = wm_roster_anim_find("fallbacks_t");
    const wm_roster_anim_table *wrestle = wm_roster_anim_find("fall_back_tbl");
    int w;

    assert(convulse && react1 && fallbacks && wrestle);
    for (w = 0; w < 9; ++w) {
        const char *a = wm_roster_anim_for(convulse, w);
        const char *b = wm_roster_anim_for(react1, w);
        if (a == NULL || b == NULL) { assert(a == b); continue; }
        assert(strcmp(a, b) == 0);
    }
    /* Same for LIFEBAR's fallbacks_t against WRESTLE's fall_back_tbl. */
    for (w = 0; w < 9; ++w) {
        const char *a = wm_roster_anim_for(fallbacks, w);
        const char *b = wm_roster_anim_for(wrestle, w);
        if (a == NULL || b == NULL) { assert(a == b); continue; }
        assert(strcmp(a, b) == 0);
    }
    /* Slot 7 is the cut Adam Bomb -- a hole in both. */
    assert(wm_roster_anim_for(convulse, 7) == NULL);
    /* And Bret's own row is the plain animation, NOT the facedown one. */
    assert(strcmp(wm_roster_anim_for(convulse, 0), "hrt_hitonground_anim") == 0);
}

int main(void) {
    test_a_man_already_down_convulses();
    test_a_man_on_his_feet_falls_back();
    test_headheld_defers_instead_of_dying();
    test_the_catch_all_is_unchanged();
    test_convulse_t_is_extracted_and_agrees();
    printf("death convulse ok\n");
    return 0;
}
