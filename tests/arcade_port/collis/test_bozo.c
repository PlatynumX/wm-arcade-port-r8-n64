/*
 * DOINK.ASM:3316 bozo_check -- the last of the routines the ledger had
 * open, and the game's answer to a player who has stopped wrestling and
 * started mashing.
 *
 * The routine is thirty lines. What took the work was that six of the
 * eight ported dispatchers never called it: the seam existed in
 * wm/arcade/wm_arcade_roster.h, Razor's and Bret's modules called it,
 * and the other six had no bozo branch at all -- so the head-hold power
 * move and the head-held reversal were missing outright rather than
 * merely inert.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_bozo.h"
#include "wm/wrestler_backend.h"
#include "wm/bret_backend.h"
#include "wm/arcade/wm_arcade_doink.h"

static int endless_kills;
static void kill_endless(wm_arcade_actor_t *a, void *user) {
    (void)a; (void)user;
    ++endless_kills;
}

static void pair(wm_arcade_actor_t *me, wm_arcade_actor_t *him) {
    memset(me, 0, sizeof *me);
    memset(him, 0, sizeof *him);
    me->active = him->active = 1;
    me->player_side = 0; him->player_side = 1;
    me->player_mode = WM_PMODE_HEADHELD;
    him->player_mode = WM_PMODE_HEADHOLD;
    me->who_hit_me = him;
}

/* ---- the count, and which counters are in it --------------------- */

static void test_which_buttons_count(void) {
    wm_arcade_actor_t me, him;
    wm_bozo_env_t env;

    memset(&env, 0, sizeof env);

    /* Nothing pressed, nothing doing. */
    pair(&me, &him);
    assert(!wm_arcade_bozo_check(&me, &env));

    /* One short of the threshold. `cmpi 18,a2 / jrlt #no_bozo` is a
       strict less-than, so seventeen is not enough and eighteen is. */
    pair(&me, &him);
    me.spunchb_count = 17;
    assert(!wm_arcade_bozo_check(&me, &env));
    me.spunchb_count = 18;
    assert(wm_arcade_bozo_check(&me, &env));

    /*
     * THE ORDINARY ATTACK COUNTERS ARE NOT IN THE SUM. PLYR.EQU:152-156
     * declares five in one "keep ordered" block, and this routine adds
     * only SPUNCHB_COUNT, SKICKB_COUNT and BLOCKB_COUNT -- "lots of
     * super buttons and blocks have been hit". Mashing punch and kick
     * will never trip it, however hard.
     */
    pair(&me, &him);
    me.punchb_count = 500;
    me.kickb_count = 500;
    assert(!wm_arcade_bozo_check(&me, &env));

    /* And the three that do count are summed, not compared. */
    pair(&me, &him);
    me.spunchb_count = 6;
    me.skickb_count = 6;
    me.blockb_count = 5;
    assert(!wm_arcade_bozo_check(&me, &env));
    me.blockb_count = 6;
    assert(wm_arcade_bozo_check(&me, &env));
}

/* ---- the two refusals are not the same shape --------------------- */

static void test_the_refusals(void) {
    wm_arcade_actor_t me, him;
    wm_bozo_env_t env;

    memset(&env, 0, sizeof env);

    /*
     * "Do reversal unless I have been immobilized!" A frozen wrestler
     * does not get to reverse out of it -- and the count is NOT cleared
     * on the way past, so the moment he thaws it still stands. That is
     * the difference between this refusal and the too-few one, and it
     * is why the order of the two tests matters.
     */
    pair(&me, &him);
    me.spunchb_count = 20;
    me.immobilize_time = 1;
    assert(!wm_arcade_bozo_check(&me, &env));
    assert(me.spunchb_count == 20);
    assert(!(me.status_flags & WM_STATUS_SMART_ATTACK));
    assert(him.immobilize_time == 0);

    me.immobilize_time = 0;
    assert(wm_arcade_bozo_check(&me, &env));

    assert(!wm_arcade_bozo_check(NULL, &env));
}

/* ---- what success does ------------------------------------------- */

static void test_what_it_does(void) {
    wm_arcade_actor_t me, him;
    wm_bozo_env_t env;

    memset(&env, 0, sizeof env);
    env.find_and_kill_endless = kill_endless;
    endless_kills = 0;

    pair(&me, &him);
    me.spunchb_count = 18;
    assert(wm_arcade_bozo_check(&me, &env));

    /*
     * SMRTTGT is a macro with TWO halves (JJXM.H:37) and both of them
     * land: the flag, and the target. "target WHOHITME -- don't hit
     * anyone else."
     */
    assert(me.status_flags & WM_STATUS_SMART_ATTACK);
    assert(me.smart_target == &him);

    /* The man who did it to him is frozen for 32 ticks. */
    assert(him.immobilize_time == WM_BOZO_IMMOBILIZE);

    /* And FIND_AND_KILL_ENDLESS, once. */
    assert(endless_kills == 1);

    /*
     * The one guard that is this port's rather than the source's. The
     * source reads WHOHITME and stores through it without asking
     * whether it is zero; that is survivable on the arcade and a null
     * dereference here. Everything that is his OWN state still happens.
     */
    pair(&me, &him);
    me.who_hit_me = NULL;
    me.spunchb_count = 18;
    endless_kills = 0;
    assert(wm_arcade_bozo_check(&me, &env));
    assert(me.status_flags & WM_STATUS_SMART_ATTACK);
    assert(me.smart_target == NULL);
    assert(endless_kills == 1);

    /* The env is optional; the decision does not depend on it. */
    pair(&me, &him);
    me.spunchb_count = 18;
    assert(wm_arcade_bozo_check(&me, NULL));
    assert(him.immobilize_time == WM_BOZO_IMMOBILIZE);
}

/* ---- the seams, in all three callback structs -------------------- */

static void test_the_seams_are_wired(void) {
    wm_wrestler_backend_actor st;
    wm_bret_backend_actor bva;
    wm_arcade_roster_callbacks_t roster_cb;
    wm_arcade_razor_callbacks_t razor_cb;
    wm_arcade_bret_callbacks_t bret_cb;
    wm_arcade_actor_t me, him;

    memset(&st, 0, sizeof st);
    memset(&bva, 0, sizeof bva);
    roster_cb = wm_wrestler_roster_callbacks(&st);
    razor_cb = wm_wrestler_razor_callbacks(&st);
    bret_cb = wm_bret_backend_callbacks(&bva);

    /* bozo_check had nothing behind it in any of the three, and
       find_and_kill_endless -- which bozo_check itself calls, and which
       fifteen other call sites NULL-check -- had nothing behind it
       either. */
    assert(roster_cb.bozo_check && razor_cb.bozo_check && bret_cb.bozo_check);
    assert(roster_cb.find_and_kill_endless);
    assert(razor_cb.find_and_kill_endless);
    assert(bret_cb.find_and_kill_endless);

    /* And the wired one answers the way the routine does. */
    pair(&me, &him);
    assert(!roster_cb.bozo_check(&me, roster_cb.user));
    me.spunchb_count = 18;
    assert(roster_cb.bozo_check(&me, roster_cb.user));
    assert(him.immobilize_time == WM_BOZO_IMMOBILIZE);
    assert(bret_cb.bozo_check(&me, bret_cb.user));
}

/* ---- and the branch six dispatchers did not have ----------------- */

static const char *last_anim;
static const char *last_sound;
static int reversals, reversal_messages;
static void cap_anim(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; last_anim = l;
}
static void cap_sound(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; last_sound = l;
}
static int always_bozo(wm_arcade_actor_t *a, void *u) { (void)a; (void)u; return 1; }
static int never_bozo(wm_arcade_actor_t *a, void *u) { (void)a; (void)u; return 0; }
static void cap_reversal(wm_arcade_actor_t *a, void *u) { (void)a; (void)u; ++reversals; }
static void cap_reversal_msg(wm_arcade_actor_t *a, void *u) {
    (void)a; (void)u; ++reversal_messages;
}

static void test_the_dispatcher_branch(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_doink_callbacks_t cb;
    wm_arcade_doink_env_t env;

    memset(&cb, 0, sizeof cb);
    cb.change_anim_label = cap_anim;
    cb.sound_label = cap_sound;
    cb.do_reversal = cap_reversal;
    cb.do_reversal_message = cap_reversal_msg;
    memset(&env, 0, sizeof env);

    /*
     * mode_headhold, "Bozo power move". The animation alternates on
     * PCNT's low bit: `move @PCNT,a14 / btst 0,a14 / jrz #tag` keeps
     * the FIRST label when the bit is clear and falls into the second
     * `movi` when it is set.
     */
    cb.bozo_check = always_bozo;
    pair(&me, &him);
    me.player_mode = WM_PMODE_HEADHOLD;
    him.player_mode = WM_PMODE_HEADHELD;
    last_anim = last_sound = NULL;
    env.pcnt = 0;
    (void)wm_arcade_move_doink(&me, &him, &env, &cb);
    assert(last_anim && strcmp(last_anim, "dnk_3_head_slam_anim") == 0);
    assert(last_sound && strcmp(last_sound, "FLYKICK") == 0);

    last_anim = NULL;
    env.pcnt = 1;
    (void)wm_arcade_move_doink(&me, &him, &env, &cb);
    assert(last_anim && strcmp(last_anim, "dnk_3_pile_driver_anim") == 0);

    /* Refused, and the ordinary head-hold path runs instead. */
    cb.bozo_check = never_bozo;
    last_anim = NULL;
    me.but_val_down = 0;
    (void)wm_arcade_move_doink(&me, &him, &env, &cb);
    assert(last_anim == NULL || strcmp(last_anim, "dnk_3_head_slam_anim") != 0);

    /*
     * mode_headheld, "Bozo reversal" -- the same move, with DO_REVERSAL
     * and DO_REVERSAL_MESS in front of it. Those two are the whole
     * difference between the two call sites.
     */
    cb.bozo_check = always_bozo;
    pair(&me, &him);
    reversals = reversal_messages = 0;
    last_anim = NULL;
    env.pcnt = 0;
    (void)wm_arcade_move_doink(&me, &him, &env, &cb);
    assert(reversals == 1 && reversal_messages == 1);
    assert(last_anim && strcmp(last_anim, "dnk_3_head_slam_anim") == 0);
}

int main(void) {
    test_which_buttons_count();
    test_the_refusals();
    test_what_it_does();
    test_the_seams_are_wired();
    test_the_dispatcher_branch();
    printf("bozo ok\n");
    return 0;
}
