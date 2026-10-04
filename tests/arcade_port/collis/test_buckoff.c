/*
 * DOINK.ASM:2920 mode_dead and the buckoff -- the button-mashing
 * comeback that brings a dying wrestler back as a zombie.
 *
 * This port collapsed the whole routine to "set NO_BUCKOFF and stop",
 * on an argument with three legs. One still stands (@royal_rumble
 * really is always zero here). The other two had gone stale:
 * is_8_on_1 is real now that the championship ladder's last rung is an
 * eight-on-one, and CHECK_COMBO_GO is answerable now that
 * add_to_combo_count writes COMBO_SIZE.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_mode_dead.h"
#include "wm/arcade/wm_arcade_combo.h"
#include "wm/arcade/wm_arcade_lifebar.h"

/* A wrestler who has just died with a lit meter, inside the ring. */
static void dying(wm_arcade_actor_t *a) {
    memset(a, 0, sizeof *a);
    a->active = 1;
    a->in_ring = 1;
    a->player_mode = WM_PMODE_DEAD;
    a->player_num = 0;
    a->player_side = 0;
    a->combo_size = WM_COMBO_SUPER_SIZE;
    a->life = 0;
}

/* The env for a wrestler who has lost a round and should qualify. */
static wm_mode_dead_env_t ok_env(wm_arcade_actor_t *const *roster,
                                 size_t n) {
    wm_mode_dead_env_t e;
    memset(&e, 0, sizeof e);
    e.rounds[1] = 1;        /* side 1 has won one, so side 0 has lost one */
    e.actors = roster;
    e.actor_count = n;
    return e;
}

/*
 * LIFEBAR.ASM:718 CHECK_COMBO_GO. It answers through the flags --
 * COMBO_SIZE minus the threshold -- and every caller reads a negative
 * as "not lit".
 */
static void test_check_combo_go(void) {
    wm_arcade_actor_t a;

    memset(&a, 0, sizeof a);
    a.combo_size = 0;
    assert(wm_arcade_check_combo_go(&a, 0) < 0);
    a.combo_size = 15;
    assert(wm_arcade_check_combo_go(&a, 0) < 0);
    a.combo_size = WM_COMBO_SUPER_SIZE;
    assert(wm_arcade_check_combo_go(&a, 0) == 0);   /* `jrlt` lets 0 pass */
    a.combo_size = 20;
    assert(wm_arcade_check_combo_go(&a, 0) > 0);

    /* instant_combos_on drops the threshold to zero: every meter lit. */
    a.combo_size = 0;
    assert(wm_arcade_check_combo_go(&a, 1) == 0);
    assert(wm_arcade_check_combo_go(NULL, 0) < 0);
}

/* The meter really is filled by the routine that fills it. */
static void test_the_meter_is_actually_tracked(void) {
    wm_arcade_actor_t a;
    int i;

    memset(&a, 0, sizeof a);
    assert(wm_arcade_check_combo_go(&a, 0) < 0);
    for (i = 0; i < WM_COMBO_SUPER_SIZE; ++i)
        wm_arcade_add_to_combo_count(&a, 1 << (i & 7));
    assert(a.combo_size == WM_COMBO_SUPER_SIZE);
    assert(wm_arcade_check_combo_go(&a, 0) >= 0);
    /* SET_FLASHING_COMBO_GOING's own threshold is the same one. */
    assert(a.combo_flash);

    wm_arcade_clear_combo_meter(&a);
    assert(wm_arcade_check_combo_go(&a, 0) < 0);
}

/* Each gate in the chain, refused one at a time. */
static void test_the_gates(void) {
    wm_arcade_actor_t a;
    wm_arcade_actor_t *roster[1];
    wm_mode_dead_env_t e;
    wm_mode_dead_result_t r;

    memset(&r, 0, sizeof r);

    roster[0] = &a;

    /* All gates open: DO_BUCKOFF is set and counting starts. */
    dying(&a);
    e = ok_env(roster, 1u);
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.status_flags & WM_STATUS_DO_BUCKOFF);
    assert(!(a.status_flags & WM_STATUS_NO_BUCKOFF));
    assert(!r.bucked_off);

    /* First round of the match: nothing lost yet, so no comeback. */
    dying(&a);
    e = ok_env(roster, 1u);
    e.rounds[1] = 0;
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.status_flags & WM_STATUS_NO_BUCKOFF);

    /* Meter not lit. */
    dying(&a);
    a.combo_size = WM_COMBO_SUPER_SIZE - 1;
    e = ok_env(roster, 1u);
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.status_flags & WM_STATUS_NO_BUCKOFF);

    /* ...unless the combos_on powerup is up. */
    dying(&a);
    a.combo_size = 0;
    e = ok_env(roster, 1u);
    e.instant_combos_on = 1;
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.status_flags & WM_STATUS_DO_BUCKOFF);

    /* Outside the ring. */
    dying(&a);
    a.in_ring = 0;
    e = ok_env(roster, 1u);
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.status_flags & WM_STATUS_NO_BUCKOFF);

    /* "Buckoff is NOT allowed if undertaker started his finish move or
       has completed his finish move!!!!" -- either one. */
    dying(&a);
    e = ok_env(roster, 1u);
    e.in_finish_move = true;
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.status_flags & WM_STATUS_NO_BUCKOFF);

    dying(&a);
    e = ok_env(roster, 1u);
    e.finish_completed = true;
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.status_flags & WM_STATUS_NO_BUCKOFF);

    /* A rumble refuses WITHOUT setting the flag, so it is re-asked
       every tick and never gets anywhere -- `jrnz #done`, not #nobuck. */
    dying(&a);
    e = ok_env(roster, 1u);
    e.royal_rumble = true;
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(!(a.status_flags & WM_STATUS_NO_BUCKOFF));
    assert(!(a.status_flags & WM_STATUS_DO_BUCKOFF));

    /* One per match, and one check per round. */
    dying(&a);
    a.status_flags = WM_STATUS_DID_BUCKOFF;
    e = ok_env(roster, 1u);
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(!(a.status_flags & WM_STATUS_DO_BUCKOFF));

    dying(&a);
    a.status_flags = WM_STATUS_NO_BUCKOFF;
    e = ok_env(roster, 1u);
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(!(a.status_flags & WM_STATUS_DO_BUCKOFF));

    /* A zombie is somebody else's problem now (the final-battle tail). */
    dying(&a);
    a.status_flags = WM_STATUS_ZOMBIE;
    e = ok_env(roster, 1u);
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.status_flags == WM_STATUS_ZOMBIE);
}

/*
 * `#ck81`: in the eight-on-one the round-count test is skipped in
 * favour of "only the player is allowed to buckoff" -- PLYRNUM under 2.
 */
static void test_the_eight_on_one_rule(void) {
    wm_arcade_actor_t a;
    wm_arcade_actor_t *roster[1];
    wm_mode_dead_env_t e;
    wm_mode_dead_result_t r;

    memset(&r, 0, sizeof r);

    roster[0] = &a;

    /* The human, first round, which would normally be refused. */
    dying(&a);
    e = ok_env(roster, 1u);
    e.rounds[0] = e.rounds[1] = 0;
    e.eight_on_one = true;
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.status_flags & WM_STATUS_DO_BUCKOFF);

    /* A drone at PLYRNUM 2 gets nothing, however lit his meter. */
    dying(&a);
    a.player_num = 2;
    e = ok_env(roster, 1u);
    e.eight_on_one = true;
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.status_flags & WM_STATUS_NO_BUCKOFF);
}

/*
 * `#count_btns` counts BUTTON PRESSES, not ticks: every set bit in
 * BUT_VAL_DOWN is popped and added, so mashing two at once counts two.
 */
static void test_the_button_count(void) {
    wm_arcade_actor_t a;
    wm_arcade_actor_t *roster[1];
    wm_mode_dead_env_t e;
    wm_mode_dead_result_t r;

    memset(&r, 0, sizeof r);
    int ticks;

    roster[0] = &a;
    dying(&a);
    e = ok_env(roster, 1u);

    /* First call opens the window AND counts this tick's presses. */
    a.but_val_down = 0x3u;                 /* two buttons at once */
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.status_flags & WM_STATUS_DO_BUCKOFF);
    assert(a.buckoff_count == 2);

    for (ticks = 0; ticks < 200 && !r.bucked_off; ++ticks) {
        a.but_val_down = 0x1u;             /* one press a tick */
        wm_arcade_mode_dead_ex(&a, &e, &r);
    }
    assert(r.bucked_off);
    assert(a.buckoff_count >= WM_BUCKOFF_TARGET);
    /* Two on the opening tick, then one each: 48 more ticks. */
    assert(ticks == WM_BUCKOFF_TARGET - 2);

    /* A tick with nothing pressed adds nothing. */
    dying(&a);
    e = ok_env(roster, 1u);
    a.but_val_down = 0u;
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.buckoff_count == 0);
    wm_arcade_mode_dead_ex(&a, &e, &r);
    assert(a.buckoff_count == 0);
    assert(!r.bucked_off);
}

/* `#dobuck` -- SUCCESS! Everything it puts back. */
static void test_the_revival(void) {
    wm_arcade_actor_t me, pinner, bystander;
    wm_arcade_actor_t *roster[3];
    wm_mode_dead_env_t e;
    wm_mode_dead_result_t r;

    memset(&r, 0, sizeof r);
    int i;

    dying(&me);
    me.status_flags |= WM_STATUS_PINNED | WM_STATUS_PINABLE;
    me.getup_time = 90;
    me.delay_meter = 30;
    me.i_will_die = 1;
    me.my_pal = 7;
    me.obj_pal = 2;

    memset(&pinner, 0, sizeof pinner);
    pinner.active = 1;
    pinner.player_num = 1;
    pinner.player_side = 1;
    pinner.plyr_type = WM_PTYPE_DRONE;
    pinner.status_flags = WM_STATUS_DID_PIN;

    memset(&bystander, 0, sizeof bystander);
    bystander.active = 1;
    bystander.player_num = 2;
    bystander.plyr_type = WM_PTYPE_DRONE;   /* a buddy, and he stays one */
    bystander.status_flags = WM_STATUS_DID_RAISEARM;

    roster[0] = &me; roster[1] = &pinner; roster[2] = &bystander;
    e = ok_env(roster, 3u);

    for (i = 0; i < 200 && !r.bucked_off; ++i) {
        me.but_val_down = 0x1u;
        wm_arcade_mode_dead_ex(&me, &e, &r);
    }
    assert(r.bucked_off);

    /* "back to life..." */
    assert(me.player_mode == WM_PMODE_ONGROUND);
    assert(me.life == WM_BUCKOFF_HEALTH);
    assert(me.getup_time == 0);
    assert(me.delay_meter == 0);
    assert(me.i_will_die == 0);
    assert(me.obj_pal == me.my_pal);
    assert(me.combo_size == 0);            /* `calla clear_combo_meter` */
    assert(me.anim_mode & WM_MODE_NOCOLLIS);

    /* The flag swap, one read and one write on the whole long. */
    assert(me.status_flags & WM_STATUS_DID_BUCKOFF);
    assert(me.status_flags & WM_STATUS_NEW_BUCKOFF);
    assert(!(me.status_flags & WM_STATUS_DO_BUCKOFF));
    assert(!(me.status_flags & WM_STATUS_PINNED));
    assert(!(me.status_flags & WM_STATUS_PINABLE));

    /* The pinner loses DID_PIN and is sent to his buckoff animation. */
    assert(!(pinner.status_flags & WM_STATUS_DID_PIN));
    assert(r.pinner_to_buck == &pinner);
    /* "If anyone has turned into a drone, turn 'em back" -- PLYRNUM 0
       and 1 only. */
    assert(pinner.plyr_type == WM_PTYPE_PLAYER);

    /* Everyone's raisearm is cleared, and PLYRNUM 2 stays a drone. */
    assert(!(bystander.status_flags & WM_STATUS_DID_RAISEARM));
    assert(bystander.plyr_type == WM_PTYPE_DRONE);

    assert(r.convulse);
    assert(r.second_wind_message);
}

/*
 * "if his DID_RAISEARM bit is set, then it was probably taker and he's
 * no longer on top of us, so skip the buckoff" -- the pin is still
 * cleared, but he is not sent anywhere.
 */
static void test_the_undertaker_exception(void) {
    wm_arcade_actor_t me, pinner;
    wm_arcade_actor_t *roster[2];
    wm_mode_dead_env_t e;
    wm_mode_dead_result_t r;

    memset(&r, 0, sizeof r);
    int i;

    dying(&me);
    memset(&pinner, 0, sizeof pinner);
    pinner.active = 1;
    pinner.player_num = 1;
    pinner.status_flags = WM_STATUS_DID_PIN | WM_STATUS_DID_RAISEARM;

    roster[0] = &me; roster[1] = &pinner;
    e = ok_env(roster, 2u);

    for (i = 0; i < 200 && !r.bucked_off; ++i) {
        me.but_val_down = 0x1u;
        wm_arcade_mode_dead_ex(&me, &e, &r);
    }
    assert(r.bucked_off);
    assert(!(pinner.status_flags & WM_STATUS_DID_PIN));
    assert(r.pinner_to_buck == NULL);
}

/* The old no-env entry point still refuses, as it always has. */
static void test_the_bare_entry_point_is_unchanged(void) {
    wm_arcade_actor_t a;
    dying(&a);
    wm_arcade_mode_dead(&a);
    assert(a.status_flags & WM_STATUS_NO_BUCKOFF);
    assert(!(a.status_flags & WM_STATUS_DO_BUCKOFF));
}

int main(void) {
    test_check_combo_go();
    test_the_meter_is_actually_tracked();
    test_the_gates();
    test_the_eight_on_one_rule();
    test_the_button_count();
    test_the_revival();
    test_the_undertaker_exception();
    test_the_bare_entry_point_is_unchanged();
    printf("buckoff ok\n");
    return 0;
}
