/*
 * WRESTLE.ASM:3886 auto_pin_check -- the auto-pin -- and the reason it
 * had never run: there were two translations of its caller and the
 * live loop used the wrong one.
 *
 * wm_arcade_move_wrestler (wm/arcade/wm_arcade_move_dispatch.h) is the
 * faithful move_wrestler: the @HALT check, the SPECIAL_MOVE_ADDR drain,
 * auto_pin_check, then the per-character dispatch. It was called only
 * from a test. wm_arcade_move_ported_wrestler is that last step alone,
 * and it is what the match loop called -- so the whole prologue was
 * missing, nothing drained SPECIAL_MOVE_ADDR, and auto_pin_check never
 * ran at all.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_auto_pin.h"
#include "wm/arcade/wm_arcade_move_dispatch.h"

static void beaten(wm_arcade_actor_t *me, wm_arcade_actor_t *opp) {
    memset(me, 0, sizeof *me);
    memset(opp, 0, sizeof *opp);
    me->active = 1;  me->player_side = 0; me->player_mode = WM_PMODE_NORMAL;
    opp->active = 1; opp->player_side = 1; opp->player_mode = WM_PMODE_DEAD;
}

static wm_auto_pin_env_t env_for(wm_arcade_actor_t *opp,
                                 wm_arcade_actor_t *const *roster,
                                 size_t n) {
    wm_auto_pin_env_t e;
    memset(&e, 0, sizeof e);
    e.opponent = opp;
    e.actors = roster;
    e.actor_count = n;
    return e;
}

/* Three seconds, which is what the code says and not what either
   comment beside it says. */
static void test_the_countdown_is_three_seconds_not_four(void) {
    wm_arcade_actor_t me, opp;
    wm_arcade_actor_t *roster[2];
    wm_auto_pin_env_t e;
    wm_auto_pin_result_t r;
    int t;

    assert(WM_AUTO_PIN_TICKS == 53 * 3);
    assert(WM_AUTO_PIN_TICKS == 159);

    beaten(&me, &opp);
    roster[0] = &me; roster[1] = &opp;
    e = env_for(&opp, roster, 2u);

    memset(&r, 0, sizeof r);
    for (t = 1; t <= WM_AUTO_PIN_TICKS; ++t) {
        r = wm_arcade_auto_pin_check(&me, &e);
        assert(r.counted);
        assert(me.auto_pin_cntdown == t);
        if (t < WM_AUTO_PIN_TICKS) {
            assert(!r.became_drone);
            assert(me.plyr_type == WM_PTYPE_PLAYER);
        }
    }
    /* The 159th tick is the one that hands him to the drone AI. */
    assert(r.became_drone);
    assert(me.plyr_type == WM_PTYPE_DRONE);
}

/*
 * The guards divide into two kinds, and that is the content of the
 * routine: two of them RESET the countdown and three only hold it.
 */
static void test_the_two_kinds_of_guard(void) {
    wm_arcade_actor_t me, opp;
    wm_arcade_actor_t *roster[2];
    wm_auto_pin_env_t e;

    beaten(&me, &opp);
    roster[0] = &me; roster[1] = &opp;
    e = env_for(&opp, roster, 2u);

    /* Wind it up a bit. */
    (void)wm_arcade_auto_pin_check(&me, &e);
    (void)wm_arcade_auto_pin_check(&me, &e);
    (void)wm_arcade_auto_pin_check(&me, &e);
    assert(me.auto_pin_cntdown == 3);

    /* RESET #1: the opponent gets up. `jrne #alive`. */
    opp.player_mode = WM_PMODE_NORMAL;
    (void)wm_arcade_auto_pin_check(&me, &e);
    assert(me.auto_pin_cntdown == 0);

    /* RESET #2: he is MODE_DEAD but a ZOMBIE -- dead and coming back,
       so not beaten. Also `jrnz #alive`. */
    opp.player_mode = WM_PMODE_DEAD;
    (void)wm_arcade_auto_pin_check(&me, &e);
    (void)wm_arcade_auto_pin_check(&me, &e);
    assert(me.auto_pin_cntdown == 2);
    opp.status_flags |= WM_STATUS_ZOMBIE;
    (void)wm_arcade_auto_pin_check(&me, &e);
    assert(me.auto_pin_cntdown == 0);
    opp.status_flags &= (uint32_t)~WM_STATUS_ZOMBIE;

    /* HOLD #1: this wrestler has already pinned. A plain `rets`, so the
       count stands where it was rather than starting over. */
    (void)wm_arcade_auto_pin_check(&me, &e);
    (void)wm_arcade_auto_pin_check(&me, &e);
    (void)wm_arcade_auto_pin_check(&me, &e);
    (void)wm_arcade_auto_pin_check(&me, &e);
    assert(me.auto_pin_cntdown == 4);
    me.status_flags |= WM_STATUS_DID_PIN;
    {
        wm_auto_pin_result_t r = wm_arcade_auto_pin_check(&me, &e);
        assert(!r.counted);
        assert(me.auto_pin_cntdown == 4);      /* held, not reset */
    }
    me.status_flags &= (uint32_t)~WM_STATUS_DID_PIN;

    /* HOLD #2: somebody is attempting a buckoff. */
    opp.status_flags |= WM_STATUS_DO_BUCKOFF | WM_STATUS_NEW_BUCKOFF;
    {
        wm_auto_pin_result_t r = wm_arcade_auto_pin_check(&me, &e);
        assert(!r.counted);
        assert(me.auto_pin_cntdown == 4);
    }
    opp.status_flags &=
        (uint32_t)~(WM_STATUS_DO_BUCKOFF | WM_STATUS_NEW_BUCKOFF);

    /* And it resumes from where it was held. */
    (void)wm_arcade_auto_pin_check(&me, &e);
    assert(me.auto_pin_cntdown == 5);
}

/*
 * HOLD #3, and the one the holding is FOR: a man who reaches the
 * threshold mid-move waits there and drops into a drone the instant
 * his animation becomes interruptible. Reset semantics would make that
 * impossible, which is why the distinction above matters.
 */
static void test_he_waits_for_the_unint_bit(void) {
    wm_arcade_actor_t me, opp;
    wm_arcade_actor_t *roster[2];
    wm_auto_pin_env_t e;
    wm_auto_pin_result_t r;
    int t;

    beaten(&me, &opp);
    roster[0] = &me; roster[1] = &opp;
    e = env_for(&opp, roster, 2u);
    me.anim_mode = WM_MODE_UNINT;

    for (t = 1; t <= WM_AUTO_PIN_TICKS + 50; ++t) {
        r = wm_arcade_auto_pin_check(&me, &e);
        assert(r.counted);                  /* it keeps counting past it */
        assert(!r.became_drone);
    }
    assert(me.auto_pin_cntdown == WM_AUTO_PIN_TICKS + 50);
    assert(me.plyr_type == WM_PTYPE_PLAYER);

    /* The move ends. */
    me.anim_mode = 0;
    r = wm_arcade_auto_pin_check(&me, &e);
    assert(r.became_drone);
    assert(me.plyr_type == WM_PTYPE_DRONE);
}

/* The three globals, each refusing outright. */
static void test_the_globals_refuse(void) {
    wm_arcade_actor_t me, opp;
    wm_arcade_actor_t *roster[2];
    wm_auto_pin_env_t e;
    int t;

    beaten(&me, &opp);
    roster[0] = &me; roster[1] = &opp;

    e = env_for(&opp, roster, 2u);
    e.in_finish_move = true;
    for (t = 0; t < WM_AUTO_PIN_TICKS + 5; ++t)
        assert(!wm_arcade_auto_pin_check(&me, &e).became_drone);
    assert(me.auto_pin_cntdown == 0);

    e = env_for(&opp, roster, 2u);
    e.finish_completed = true;
    for (t = 0; t < WM_AUTO_PIN_TICKS + 5; ++t)
        assert(!wm_arcade_auto_pin_check(&me, &e).became_drone);
    assert(me.auto_pin_cntdown == 0);

    e = env_for(&opp, roster, 2u);
    e.royal_rumble = true;
    for (t = 0; t < WM_AUTO_PIN_TICKS + 5; ++t)
        assert(!wm_arcade_auto_pin_check(&me, &e).became_drone);
    assert(me.auto_pin_cntdown == 0);

    /* No opponent reads as "not dead", which resets -- the source would
       have read through a null process pointer and there is no honest
       value for that. */
    e = env_for(NULL, roster, 2u);
    me.auto_pin_cntdown = 40;
    (void)wm_arcade_auto_pin_check(&me, &e);
    assert(me.auto_pin_cntdown == 0);

    assert(!wm_arcade_auto_pin_check(NULL, &e).became_drone);
    assert(!wm_arcade_auto_pin_check(&me, NULL).became_drone);
}

/* ---------------------------------------------------------------- */
/* move_wrestler's own order, which is what puts the routine in play. */
/* ---------------------------------------------------------------- */
typedef struct { int special, autopin, character; uintptr_t token; } trace_t;

static void t_special(wm_arcade_actor_t *a, uintptr_t tok, void *u) {
    trace_t *t = (trace_t *)u; (void)a; t->special++; t->token = tok;
}
static void t_autopin(wm_arcade_actor_t *a, void *u) {
    trace_t *t = (trace_t *)u; (void)a; t->autopin++;
}
static void t_character(wm_arcade_actor_t *a, wm_arcade_move_handler_id_t w,
                        void *u) {
    trace_t *t = (trace_t *)u; (void)a; (void)w; t->character++;
}

static void test_the_dispatcher_drains_and_then_checks(void) {
    wm_arcade_actor_t a;
    trace_t t;
    wm_arcade_move_callbacks_t cb;

    memset(&cb, 0, sizeof cb);
    cb.change_anim_special = t_special;
    cb.auto_pin_check = t_autopin;
    cb.character_move = t_character;
    cb.user = &t;

    /*
     * A queued special wins and RETURNS: no auto-pin check and no
     * character move that tick. That is the source's `jruc #rets`, and
     * it is the ordering the match loop did not have.
     */
    memset(&a, 0, sizeof a);
    a.wrestler_num = 0;
    a.special_move_addr = (uintptr_t)0x1234;
    memset(&t, 0, sizeof t);
    assert(wm_arcade_move_wrestler(&a, 0, &cb) == WM_MOVE_DISPATCH_SPECIAL);
    assert(t.special == 1 && t.token == (uintptr_t)0x1234);
    assert(t.autopin == 0 && t.character == 0);
    /* And the field is DRAINED -- the thing nothing was doing, which
       left SPECIAL_MOVE_ADDR latched for the rest of the match. */
    assert(a.special_move_addr == (uintptr_t)0);

    /* With nothing queued: auto_pin_check first, then the character. */
    memset(&t, 0, sizeof t);
    assert(wm_arcade_move_wrestler(&a, 0, &cb) == WM_MOVE_DISPATCH_CHARACTER);
    assert(t.special == 0);
    assert(t.autopin == 1 && t.character == 1);

    /* @HALT stops everything, before even the drain. */
    a.special_move_addr = (uintptr_t)0x99;
    memset(&t, 0, sizeof t);
    assert(wm_arcade_move_wrestler(&a, 1, &cb) == WM_MOVE_DISPATCH_HALTED);
    assert(t.special == 0 && t.autopin == 0 && t.character == 0);
    assert(a.special_move_addr == (uintptr_t)0x99);
}

int main(void) {
    test_the_countdown_is_three_seconds_not_four();
    test_the_two_kinds_of_guard();
    test_he_waits_for_the_unint_bit();
    test_the_globals_refuse();
    test_the_dispatcher_drains_and_then_checks();
    printf("auto pin ok\n");
    return 0;
}
