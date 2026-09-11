/*
 * WRESTLE2.ASM:4058 init_smoves, MACROS.H:652 WAITSWITCH_DWN, and the
 * three monitors this port can complete. Every expectation is the
 * source's own arithmetic, not the port's output recorded back.
 */
#include <assert.h>
#include <string.h>

#include "wm_arcade_smove.h"
#include "wm_arcade_pin.h"
#include "wm_arcade_roster.h"
#include "wm/wrestler_anim_tables.h"

/* ---- WAITSWITCH_DWN ---------------------------------------------- */

static void test_switch_word(void) {
    /* `sll 4,a0` on the buttons, then OR the relative stick in.
       GAME.EQU's B_* carry the same shift, so they compare directly. */
    assert(wm_smove_switch_word(WM_BTN_PUNCH, 0) == WM_B_PUNCH);
    assert(wm_smove_switch_word(WM_BTN_BLOCK, 0) == WM_B_BLOCK);
    assert(wm_smove_switch_word(0, WM_J_UP) == WM_J_UP);
    assert(wm_smove_switch_word(WM_BTN_BLOCK, WM_J_UP) ==
           (WM_B_BLOCK | WM_J_UP));
    /* J_ALL is the whole stick, bits 0-3 plus the real-LR pair. */
    assert(WM_J_ALL == (0x0f | (WM_MOVE_LEFT << 8) | (WM_MOVE_RIGHT << 8)));
}

static void test_waitswitch(void) {
    int32_t c;

    /* Nothing new: WAIT, and the countdown still ticks. */
    c = 5;
    assert(wm_smove_waitswitch(&c, 0, 0, 0, WM_J_UP, 0) == WM_SMOVE_WAIT);
    assert(c == 4);

    /* The right input: MATCH. */
    c = 5;
    assert(wm_smove_waitswitch(&c, 0, 0, WM_J_UP, WM_J_UP, 0)
           == WM_SMOVE_MATCH);

    /* The WRONG input FAILS rather than waiting -- the opposite of
       AWARD.ASM's PUPWAITSWITCH, which loops on junk. */
    c = 5;
    assert(wm_smove_waitswitch(&c, 0, 0, WM_J_DOWN, WM_J_UP, 0)
           == WM_SMOVE_FAIL);

    /* Already doing a special move: FAIL, whatever the input. */
    c = 5;
    assert(wm_smove_waitswitch(&c, 0x1234, 0, WM_J_UP, WM_J_UP, 0)
           == WM_SMOVE_FAIL);

    /* The deadline: `dec a11 / jrz`, so a countdown of 1 fails on the
       very next tick. */
    c = 1;
    assert(wm_smove_waitswitch(&c, 0, 0, WM_J_UP, WM_J_UP, 0)
           == WM_SMOVE_FAIL);

    /*
     * And a countdown of ZERO never fails: `dec 0` is -1, which is not
     * zero, so the opening input of every monitor waits forever. Run
     * it a few thousand ticks to show it really is not a deadline.
     */
    c = 0;
    {
        int i;
        for (i = 0; i < 5000; ++i)
            assert(wm_smove_waitswitch(&c, 0, 0, 0, WM_J_UP, 0)
                   == WM_SMOVE_WAIT);
        assert(c == -5000);
    }

    /* The mask hides bits from BOTH the test and the compare: with
       B_BLOCK masked off, a held block does not spoil a stick step. */
    c = 5;
    assert(wm_smove_waitswitch(&c, 0, WM_BTN_BLOCK, WM_J_UP, WM_J_UP,
                               WM_B_BLOCK) == WM_SMOVE_MATCH);
    /* ...and with no mask, the same input is the wrong word. */
    c = 5;
    assert(wm_smove_waitswitch(&c, 0, WM_BTN_BLOCK, WM_J_UP, WM_J_UP, 0)
           == WM_SMOVE_FAIL);
}

/* ---- the registry and init_smoves -------------------------------- */

static void test_registry(void) {
    const wm_smove_monitor_t *m;

    /* Three hand-written, plus the head-hold family read out of the
       source by tools/wlsmove.py. */
    assert(wm_smove_monitor_count() == 3 + wm_smove_hdhold_count);

    m = wm_smove_monitor_find("und_finish_move1");
    assert(m && m->steps == 3);
    assert(m->step[0].switches == WM_J_UP && m->step[0].mask == 0);
    assert(m->step[1].switches == WM_J_DOWN);
    assert(m->step[2].switches == WM_B_PUNCH && m->step[2].mask == WM_J_ALL);
    assert(m->timeout == 53);        /* TAKER.ASM:610, `.equ TSEC` */
    assert(m->one_shot);

    /* Both DOINK monitors are a full eight-way stick rotation, and
       both go the same way round -- they just start at a different
       point on the circle. */
    m = wm_smove_monitor_find("std_walk_fast");
    assert(m && m->steps == 8 && m->timeout == 61);
    assert(m->step[0].switches == WM_J_AWAY);
    assert(m->step[7].switches == WM_J_UP_AWAY);
    assert(!m->humans_only);

    m = wm_smove_monitor_find("std_taunt");
    assert(m && m->steps == 8 && m->timeout == 61);
    assert(m->step[0].switches == WM_J_UP);
    assert(m->step[7].switches == WM_J_UP_AWAY);
    assert(m->humans_only);          /* `PLYR_TYPE != 0 -> SUCIDE` */
    {
        size_t i;
        for (i = 0; i < m->steps; ++i)
            assert(m->step[i].mask == WM_B_BLOCK);
    }

    /* A head-hold monitor resolves now, and carries its row. */
    {
        const wm_smove_monitor_t *hh =
            wm_smove_monitor_find("und_hdhold_neckbrk");
        assert(hh && hh->hdhold);
    }
    /* One of the twenty still missing does not. */
    assert(wm_smove_monitor_find("und_grab_toss_air") == NULL);
    assert(wm_smove_monitor_find(NULL) == NULL);
}

static void test_init_smoves(void) {
    wm_smove_run_t runs[WM_SMOVE_MAX_PER_WRESTLER];
    size_t n, missing;

    /* The Undertaker's table is twelve long and this port has three of
       its entries -- und_finish_move1, std_walk_fast, std_taunt. */
    n = wm_smove_init(WM_ROSTER_TAKER, false, runs,
                      WM_SMOVE_MAX_PER_WRESTLER, &missing);
    /* His twelve, less the ones still missing. The sum is what
       matters: every entry is either made or counted. */
    assert(n + missing == (size_t)wm_wrestler_smoves[WM_ROSTER_TAKER].count);
    assert(n > 3);

    /* A drone gets no std_taunt: its first two instructions kill it. */
    {
        size_t human = n;
        n = wm_smove_init(WM_ROSTER_TAKER, true, runs,
                          WM_SMOVE_MAX_PER_WRESTLER, &missing);
        assert(n == human - 1);        /* exactly std_taunt is gone */
    }

    /* Bret has no finishing move -- GAME.EQU:580 zeroes his switch --
       so his table's two shared entries are all this port can run. */
    n = wm_smove_init(WM_ROSTER_BRET, false, runs,
                      WM_SMOVE_MAX_PER_WRESTLER, &missing);
    assert(n + missing == (size_t)wm_wrestler_smoves[WM_ROSTER_BRET].count);
    assert(n > 2);

    /* Slot 7 is Adam Bomb, cut: a zero table pointer, no watchdogs. */
    n = wm_smove_init(7, false, runs, WM_SMOVE_MAX_PER_WRESTLER, &missing);
    assert(n == 0 && missing == 0);
    n = wm_smove_init(-1, false, runs, WM_SMOVE_MAX_PER_WRESTLER, &missing);
    assert(n == 0);
}

/* ---- driving a monitor ------------------------------------------- */

/* One tick of input, as read_switches would have cached it. */
static void press(wm_arcade_actor_t *a, uint16_t but_down, uint16_t stick) {
    a->but_val_down = but_down;
    a->stick_rel_new = stick;
}

static void test_taunt_sequence(void) {
    static const uint16_t ROTATION[8] = {
        WM_J_UP, WM_J_UP_TOWARD, WM_J_TOWARD, WM_J_DOWN_TOWARD,
        WM_J_DOWN, WM_J_DOWN_AWAY, WM_J_AWAY, WM_J_UP_AWAY
    };
    wm_arcade_actor_t taker, victim;
    wm_smove_run_t run;
    wm_smove_env_t env;
    wm_smove_fire_t fire;
    int i;

    memset(&taker, 0, sizeof taker);
    memset(&victim, 0, sizeof victim);
    memset(&run, 0, sizeof run);
    memset(&env, 0, sizeof env);
    taker.wrestler_num = WM_ROSTER_TAKER;
    taker.player_mode = WM_PMODE_BLOCK;
    taker.but_val_cur = WM_BTN_BLOCK;
    victim.player_mode = WM_PMODE_NORMAL;
    env.victim = &victim;
    run.monitor = wm_smove_monitor_find("std_taunt");
    assert(run.monitor);

    /* The first tick is the SLEEPK before the first wait: the gate
       passes and the sequence arms, but nothing is read yet. */
    press(&taker, 0, 0);
    assert(!wm_smove_tick(&run, &taker, &env, &fire));
    assert(run.armed && run.at == 0);

    for (i = 0; i < 8; ++i) {
        press(&taker, 0, ROTATION[i]);
        if (i < 7) {
            assert(!wm_smove_tick(&run, &taker, &env, &fire));
            assert(run.at == (size_t)(i + 1));
            /* The one `movi #TIMEOUT,a11`, after the first step and
               never again -- the rest of the rotation shares it. */
            if (i == 0) assert(run.countdown == 61);
        } else {
            assert(wm_smove_tick(&run, &taker, &env, &fire));
        }
    }

    assert(fire.anim &&
           strcmp(fire.anim, wm_wrestler_taunt_anims[WM_ROSTER_TAKER]) == 0);
    assert(fire.risk == (uint16_t)(0x8000u + 12u * 60u));
    assert(fire.walk_fast == 0);
    /* `DIE` -- it does not come round again. */
    assert(run.dead);
    memset(&fire, 0, sizeof fire);
    assert(!wm_smove_tick(&run, &taker, &env, &fire));
}

static void test_taunt_refuses(void) {
    wm_arcade_actor_t taker, victim;
    wm_smove_run_t run;
    wm_smove_env_t env;
    wm_smove_fire_t fire;

    memset(&taker, 0, sizeof taker);
    memset(&victim, 0, sizeof victim);
    memset(&run, 0, sizeof run);
    memset(&env, 0, sizeof env);
    taker.wrestler_num = WM_ROSTER_TAKER;
    taker.but_val_cur = WM_BTN_BLOCK;
    env.victim = &victim;
    run.monitor = wm_smove_monitor_find("std_taunt");

    /* The gate is MODE_BLOCK: not blocking, never arms. */
    taker.player_mode = WM_PMODE_NORMAL;
    press(&taker, 0, 0);
    assert(!wm_smove_tick(&run, &taker, &env, &fire));
    assert(!run.armed);

    /* Arm it, then let go of BLOCK after the opening UP: the mid gate
       (`btst PLAYER_BLOCK_BIT / jrz #lp0`) throws it back. */
    taker.player_mode = WM_PMODE_BLOCK;
    press(&taker, 0, 0);
    (void)wm_smove_tick(&run, &taker, &env, &fire);
    assert(run.armed);
    taker.but_val_cur = 0;
    press(&taker, 0, WM_J_UP);
    assert(!wm_smove_tick(&run, &taker, &env, &fire));
    assert(!run.armed);

    /* A wrong direction mid-rotation restarts it, rather than being
       ignored the way a powerup code would ignore it. */
    taker.but_val_cur = WM_BTN_BLOCK;
    press(&taker, 0, 0);
    (void)wm_smove_tick(&run, &taker, &env, &fire);
    press(&taker, 0, WM_J_UP);
    (void)wm_smove_tick(&run, &taker, &env, &fire);
    assert(run.at == 1);
    press(&taker, 0, WM_J_DOWN);            /* not UP_TOWARD */
    assert(!wm_smove_tick(&run, &taker, &env, &fire));
    assert(!run.armed);
}

static void test_walk_fast(void) {
    static const uint16_t ROTATION[8] = {
        WM_J_AWAY, WM_J_DOWN_AWAY, WM_J_DOWN, WM_J_DOWN_TOWARD,
        WM_J_TOWARD, WM_J_UP_TOWARD, WM_J_UP, WM_J_UP_AWAY
    };
    wm_arcade_actor_t a;
    wm_smove_run_t run;
    wm_smove_env_t env;
    wm_smove_fire_t fire;
    int i;

    memset(&a, 0, sizeof a);
    memset(&run, 0, sizeof run);
    memset(&env, 0, sizeof env);
    a.wrestler_num = WM_ROSTER_DOINK;
    a.player_mode = WM_PMODE_NORMAL;
    run.monitor = wm_smove_monitor_find("std_walk_fast");

    press(&a, 0, 0);
    (void)wm_smove_tick(&run, &a, &env, &fire);
    for (i = 0; i < 8; ++i) {
        press(&a, 0, ROTATION[i]);
        if (i < 7) assert(!wm_smove_tick(&run, &a, &env, &fire));
        else assert(wm_smove_tick(&run, &a, &env, &fire));
    }
    assert(fire.walk_fast == 15 * 60);
    assert(fire.anim == NULL);      /* the rest of it is presentation */

    /* "One time per match": with WALK_FAST already set the gate never
       lets it arm again. */
    a.walk_fast = fire.walk_fast;
    memset(&run, 0, sizeof run);
    run.monitor = wm_smove_monitor_find("std_walk_fast");
    press(&a, 0, 0);
    assert(!wm_smove_tick(&run, &a, &env, &fire));
    assert(!run.armed);
}

/*
 * The one that matters: the Undertaker's coffin trigger. UP, DOWN,
 * PUNCH -- and then the seven guards, which are the reason it almost
 * never fires.
 */
static void test_und_finish_monitor(void) {
    wm_arcade_actor_t taker, victim;
    wm_smove_run_t run;
    wm_smove_env_t env;
    wm_smove_fire_t fire;
    wm_arcade_und_finish_callbacks_t cb;

    memset(&taker, 0, sizeof taker);
    memset(&victim, 0, sizeof victim);
    memset(&run, 0, sizeof run);
    memset(&env, 0, sizeof env);
    memset(&cb, 0, sizeof cb);

    taker.wrestler_num = WM_ROSTER_TAKER;
    taker.player_mode = WM_PMODE_NORMAL;
    taker.x_int = 100;
    victim.player_mode = WM_PMODE_DEAD;
    victim.x_int = 100;
    victim.status_flags = WM_STATUS_DO_BUCKOFF;

    env.victim = &victim;
    env.my_pins = 2;             /* his second pin attempt */
    env.ring_time = 1;           /* inside the ring */
    env.und_cb = &cb;
    run.monitor = wm_smove_monitor_find("und_finish_move1");

    press(&taker, 0, 0);
    (void)wm_smove_tick(&run, &taker, &env, &fire);
    press(&taker, 0, WM_J_UP);
    assert(!wm_smove_tick(&run, &taker, &env, &fire));
    assert(run.countdown == 53);
    press(&taker, 0, WM_J_DOWN);
    assert(!wm_smove_tick(&run, &taker, &env, &fire));
    press(&taker, WM_BTN_PUNCH, 0);
    assert(wm_smove_tick(&run, &taker, &env, &fire));

    assert(fire.anim && strcmp(fire.anim, "und_2_raise_dead_anim") == 0);
    assert(fire.in_finish_move);
    /* und_finish_move1's own side effect on the victim. */
    assert(!(victim.status_flags & WM_STATUS_DO_BUCKOFF));
    assert(victim.status_flags & WM_STATUS_NO_BUCKOFF);
    assert(run.dead);

    /* One pin attempt is not enough: `cmpi 2,a0 / jrlt #reset`. */
    memset(&run, 0, sizeof run);
    run.monitor = wm_smove_monitor_find("und_finish_move1");
    env.my_pins = 1;
    victim.status_flags = WM_STATUS_DO_BUCKOFF;
    press(&taker, 0, 0);
    (void)wm_smove_tick(&run, &taker, &env, &fire);
    press(&taker, 0, WM_J_UP);
    (void)wm_smove_tick(&run, &taker, &env, &fire);
    press(&taker, 0, WM_J_DOWN);
    (void)wm_smove_tick(&run, &taker, &env, &fire);
    press(&taker, WM_BTN_PUNCH, 0);
    assert(!wm_smove_tick(&run, &taker, &env, &fire));
    assert(!run.dead);
    /* The guards refusing does NOT consume the victim's buckoff. */
    assert(victim.status_flags & WM_STATUS_DO_BUCKOFF);
}

/* The DOWN must land inside the 53-tick window the UP opened. */
static void test_finish_timeout(void) {
    wm_arcade_actor_t taker;
    wm_smove_run_t run;
    wm_smove_env_t env;
    wm_smove_fire_t fire;
    int i;

    memset(&taker, 0, sizeof taker);
    memset(&run, 0, sizeof run);
    memset(&env, 0, sizeof env);
    taker.wrestler_num = WM_ROSTER_TAKER;
    run.monitor = wm_smove_monitor_find("und_finish_move1");

    press(&taker, 0, 0);
    (void)wm_smove_tick(&run, &taker, &env, &fire);
    press(&taker, 0, WM_J_UP);
    (void)wm_smove_tick(&run, &taker, &env, &fire);
    assert(run.countdown == 53);

    press(&taker, 0, 0);
    for (i = 0; i < 52; ++i)
        assert(!wm_smove_tick(&run, &taker, &env, &fire));
    assert(run.armed);
    assert(!wm_smove_tick(&run, &taker, &env, &fire));
    assert(!run.armed);           /* 53 ticks and it is back at #reset */
}

/* ---- reset_smoves / kill_smove_procs ----------------------------- */

static void test_reset_and_kill(void) {
    wm_smove_run_t runs[4];
    memset(runs, 0, sizeof runs);
    runs[0].monitor = wm_smove_monitor_find("std_taunt");
    runs[0].armed = true;
    runs[0].at = 3;
    runs[0].countdown = 7;
    runs[1].monitor = wm_smove_monitor_find("std_walk_fast");
    runs[1].dead = true;
    runs[1].at = 5;

    wm_smove_reset(runs, 2);
    assert(!runs[0].armed && runs[0].at == 0 && runs[0].countdown == 0);
    /* A process that DIEd is off ACTIVE and reset_smoves cannot reach
       it, so it stays dead and keeps its state. */
    assert(runs[1].dead && runs[1].at == 5);

    wm_smove_kill(runs, 2);
    assert(runs[0].dead && runs[1].dead);
}

/* ---- can_pin and pin_prompt -------------------------------------- */

static void test_can_pin(void) {
    wm_arcade_actor_t pinner, victim;
    wm_arcade_actor_t *all[2];

    memset(&pinner, 0, sizeof pinner);
    memset(&victim, 0, sizeof victim);
    all[0] = &pinner;
    all[1] = &victim;
    pinner.active = 1;
    pinner.player_side = 0;
    victim.active = 1;
    victim.player_side = 1;
    victim.player_mode = WM_PMODE_DEAD;
    victim.status_flags = WM_STATUS_PINABLE | WM_STATUS_KOD;
    victim.x_vel = 0x10000;
    victim.y_vel = 0x20000;
    victim.z_vel = 0x30000;
    pinner.closest_dist = 0x70;     /* exactly the limit: `jrgt` allows */
    pinner.closest_zdist = 0x50;

    assert(wm_arcade_can_pin(&pinner, &victim, all, 2));
    assert(victim.status_flags & WM_STATUS_PINNED);
    assert(victim.who_pinned_me == &pinner);
    assert(victim.x_vel == 0 && victim.y_vel == 0 && victim.z_vel == 0);
    assert(!(victim.status_flags & WM_STATUS_KOD));

    /* One pixel further and it refuses. */
    pinner.closest_dist = 0x71;
    assert(!wm_arcade_can_pin(&pinner, &victim, all, 2));
    pinner.closest_dist = 0x70;
    pinner.closest_zdist = 0x51;
    assert(!wm_arcade_can_pin(&pinner, &victim, all, 2));
    pinner.closest_zdist = 0x50;

    /* A zombie on the other team refuses even though he is dead. */
    victim.status_flags |= WM_STATUS_ZOMBIE;
    assert(!wm_arcade_can_pin(&pinner, &victim, all, 2));
    victim.status_flags &= ~(uint32_t)WM_STATUS_ZOMBIE;

    /* And so does one who is not in the dead animation. */
    victim.status_flags &= ~(uint32_t)WM_STATUS_PINABLE;
    assert(!wm_arcade_can_pin(&pinner, &victim, all, 2));
    victim.status_flags |= WM_STATUS_PINABLE;

    victim.player_mode = WM_PMODE_NORMAL;
    assert(!wm_arcade_can_pin(&pinner, &victim, all, 2));
}

static void test_pin_prompt(void) {
    wm_arcade_actor_t live, dead;
    wm_arcade_actor_t *all[2];
    wm_arcade_pins_t pins;

    memset(&live, 0, sizeof live);
    memset(&dead, 0, sizeof dead);
    all[0] = &live;
    all[1] = &dead;
    live.active = 1;
    live.player_side = 0;
    live.player_mode = WM_PMODE_NORMAL;
    live.in_ring = 1;
    dead.active = 1;
    dead.player_side = 1;
    dead.player_mode = WM_PMODE_DEAD;
    dead.in_ring = 1;

    assert(wm_arcade_pin_prompt(all, 2, 1) == &live);

    /* The winner has to be inside the ring. */
    live.in_ring = 0;
    assert(wm_arcade_pin_prompt(all, 2, 1) == NULL);
    live.in_ring = 1;

    /* At least one of the dead team has to be, too. */
    dead.in_ring = 0;
    assert(wm_arcade_pin_prompt(all, 2, 1) == NULL);
    dead.in_ring = 1;

    /* A zombie on the dead team kills the prompt. */
    dead.status_flags |= WM_STATUS_ZOMBIE;
    assert(wm_arcade_pin_prompt(all, 2, 1) == NULL);
    dead.status_flags = 0;

    /* The counter: side 0 is p1, and it is the pinner's side. */
    wm_arcade_pins_clear(&pins);
    assert(wm_arcade_pins_for(&pins, 0) == 0);
    wm_arcade_pins_award(&pins, 0);
    wm_arcade_pins_award(&pins, 0);
    wm_arcade_pins_award(&pins, 1);
    assert(pins.p1pins == 2 && pins.p2pins == 1);
    assert(wm_arcade_pins_for(&pins, 0) == 2);
    assert(wm_arcade_pins_for(&pins, 1) == 1);
    wm_arcade_pins_clear(&pins);
    assert(pins.p1pins == 0 && pins.p2pins == 0);
}

int main(void) {
    test_switch_word();
    test_waitswitch();
    test_registry();
    test_init_smoves();
    test_taunt_sequence();
    test_taunt_refuses();
    test_walk_fast();
    test_und_finish_monitor();
    test_finish_timeout();
    test_reset_and_kill();
    test_can_pin();
    test_pin_prompt();
    return 0;
}
