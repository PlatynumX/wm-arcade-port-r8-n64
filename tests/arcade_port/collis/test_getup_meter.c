/*
 * WRESTLE2.ASM:950 getup_meter, :1180 slide_offscr, :1235
 * ditch_getup_meter_a9, :1242 ditch_getup_meter.
 *
 * Three of the five routines the ledger still had open, and one real
 * gap: nothing in this port wrote DELAY_METER's eighteen seconds, which
 * is slide_offscr's first instruction and a gate two other translated
 * routines already read.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_getup_meter.h"
#include "wm/match.h"
#include "wm_arcade_roster.h"

static void be(wm_arcade_actor_t *a, int side, int num, int32_t type) {
    memset(a, 0, sizeof *a);
    a->active = 1;
    a->player_side = side;
    a->player_num = num;
    a->plyr_type = type;
    a->player_mode = WM_PMODE_NORMAL;
}

/* ---- the eligibility decision ------------------------------------ */

static void test_who_gets_a_meter(void) {
    wm_arcade_actor_t me, mate;
    wm_arcade_actor_t *all[2];

    all[0] = &me; all[1] = &mate;

    /* "humans get getup meters" -- before any of the rest is asked. */
    be(&me, 0, 0, WM_PTYPE_PLAYER);
    be(&mate, 0, 1, WM_PTYPE_PLAYER);
    assert(wm_arcade_getup_meter_eligible(&me, all, 2, false, 9) ==
           WM_GETUP_METER_YES);

    /* A drone with a human teammate: `#die`, and METER_PROC cleared. */
    be(&me, 0, 2, WM_PTYPE_DRONE);
    be(&mate, 0, 0, WM_PTYPE_PLAYER);
    me.meter_proc = &me;
    assert(wm_arcade_getup_meter_eligible(&me, all, 2, true, 1) ==
           WM_GETUP_METER_NO);
    assert(me.meter_proc == NULL);

    /* The same human on the OTHER team is skipped, so he is a lone
       drone and the powerup decides. */
    be(&me, 0, 2, WM_PTYPE_DRONE);
    be(&mate, 1, 0, WM_PTYPE_PLAYER);
    assert(wm_arcade_getup_meter_eligible(&me, all, 2, false, 1) ==
           WM_GETUP_METER_NO);
    assert(wm_arcade_getup_meter_eligible(&me, all, 2, true, 1) ==
           WM_GETUP_METER_YES);

    /* `cmpi 2,a1 / jrge #die`: two or more opponents and no meter, even
       with the powerup on. */
    assert(wm_arcade_getup_meter_eligible(&me, all, 2, true, 2) ==
           WM_GETUP_METER_NO);

    /* A drone teammate is not a human teammate -- the loop runs to the
       end and the powerup still decides. */
    be(&me, 0, 2, WM_PTYPE_DRONE);
    be(&mate, 0, 3, WM_PTYPE_DRONE);
    assert(wm_arcade_getup_meter_eligible(&me, all, 2, true, 1) ==
           WM_GETUP_METER_YES);

    /* And an INACTIVE human teammate is skipped, like any empty slot. */
    be(&me, 0, 2, WM_PTYPE_DRONE);
    be(&mate, 0, 0, WM_PTYPE_PLAYER);
    mate.active = 0;
    assert(wm_arcade_getup_meter_eligible(&me, all, 2, true, 1) ==
           WM_GETUP_METER_YES);
}

/* ---- the royal-rumble hack and the geometry ---------------------- */

static void test_the_side_hack(void) {
    /* Outside a rumble, PLYR_SIDE and nothing else. */
    assert(wm_arcade_getup_meter_side(0, 1, false) == 0);
    assert(wm_arcade_getup_meter_side(1, 1, false) == 1);
    /* In one, player 1 alone is moved to team 1 "where it belongs". */
    assert(wm_arcade_getup_meter_side(0, 1, true) == 1);
    assert(wm_arcade_getup_meter_side(0, 0, true) == 0);
    assert(wm_arcade_getup_meter_side(0, 2, true) == 0);

    /*
     * `dec a10 / neg a10`. The decrement is on the whole long, so it
     * costs a fraction and not a pixel: side 0 parks at integer -221
     * with a fraction of 1, not at -220.
     */
    assert(wm_arcade_getup_meter_offscr_x(1) == (int32_t)(221 << 16));
    assert(wm_arcade_getup_meter_offscr_x(0) == -(int32_t)((221 << 16) - 1));
    assert((wm_arcade_getup_meter_offscr_x(0) >> 16) == -221);
    assert((wm_arcade_getup_meter_offscr_x(0) & 0xffff) == 1);

    /* #set_x's targets: 199 plus or minus the mark. */
    assert((wm_arcade_getup_meter_target_x(1, true) >> 16) == 199 + 173);
    assert((wm_arcade_getup_meter_target_x(0, true) >> 16) == 199 - 173);
    assert((wm_arcade_getup_meter_target_x(1, false) >> 16) == 199 + 221);

    /* And it eases rather than snaps: a quarter of what is left, and
       arithmetic, so it works from either direction. */
    assert(wm_arcade_getup_meter_x_step(400, 0) == 100);
    assert(wm_arcade_getup_meter_x_step(0, 400) == -100);
    /* `sra` on a small negative remainder: it does not stall at zero
       the way a divide would, it steps to -1. */
    assert(wm_arcade_getup_meter_x_step(0, 1) == -1);
    assert(wm_arcade_getup_meter_x_step(1, 0) == 0);
}

/* ---- the come-out gate, and the check that decides nothing ------- */

static int32_t health_hook(const wm_arcade_actor_t *a, void *user) {
    (void)a;
    return *(int32_t *)user;
}

static void test_the_come_out_gate(void) {
    wm_arcade_actor_t me, hitter;
    wm_getup_meter_env_t env;
    int32_t health;
    int32_t h;

    memset(&env, 0, sizeof env);
    env.get_health = health_hook;
    env.user = &health;
    health = 100;

    /* Nothing to get up from. */
    be(&me, 0, 0, WM_PTYPE_PLAYER);
    assert(!wm_arcade_getup_meter_may_show(&me, &env));

    /* Something to get up from. */
    me.getup_time = 90;
    assert(wm_arcade_getup_meter_may_show(&me, &env));

    /* The man who hit him is mid-combo. */
    be(&hitter, 1, 1, WM_PTYPE_DRONE);
    hitter.combo_count = 2;
    me.who_hit_me = &hitter;
    assert(!wm_arcade_getup_meter_may_show(&me, &env));
    hitter.combo_count = 0;
    assert(wm_arcade_getup_meter_may_show(&me, &env));

    /* DELAY_METER -- the gate this whole change exists to write. */
    me.delay_meter = 1;
    assert(!wm_arcade_getup_meter_may_show(&me, &env));
    me.delay_meter = 0;

    /* And a dead man does not get up. */
    me.player_mode = (uint16_t)WM_PMODE_DEAD;
    assert(!wm_arcade_getup_meter_may_show(&me, &env));
    me.player_mode = (uint16_t)WM_PMODE_NORMAL;

    /*
     * THE HEALTH CHECK DECIDES NOTHING, demonstrated rather than
     * asserted. The source's comment says "If health meter is down low,
     * don't have getup meter come out. Unless it was a fling!", but the
     * low-health arm falls through into the same GETUP_TIME test, so
     * every health value gives the same answer -- for a flung getup
     * time and for any other.
     */
    for (h = 0; h <= 100; h += 5) {
        health = h;
        me.getup_time = WM_FLUNG_TIME;
        assert(wm_arcade_getup_meter_may_show(&me, &env));
        me.getup_time = 90;
        assert(wm_arcade_getup_meter_may_show(&me, &env));
        me.getup_time = 0;
        assert(!wm_arcade_getup_meter_may_show(&me, &env));
    }

    /* Including with no health hook at all, which is what a caller that
       cannot see the life data passes. */
    me.getup_time = 90;
    assert(wm_arcade_getup_meter_may_show(&me, NULL));
    me.getup_time = 0;
    assert(!wm_arcade_getup_meter_may_show(&me, NULL));
}

/* ---- the bar arithmetic ------------------------------------------ */

static void test_the_bar(void) {
    int32_t start, v;

    /* #update_meter's average, and the crop that follows it. */
    assert(wm_arcade_getup_meter_smooth(80, 0) == 40);
    assert(wm_arcade_getup_meter_smooth(40, 0) == 20);
    assert(wm_arcade_getup_meter_smooth(0, 80) == 40);
    assert(wm_arcade_getup_meter_crop(80) == 0);
    assert(wm_arcade_getup_meter_crop(0) == WM_GETUP_SIZE);
    assert(wm_arcade_getup_meter_crop(20) == 60);
    /* Both clamps. */
    assert(wm_arcade_getup_meter_crop(-5) == WM_GETUP_SIZE);
    assert(wm_arcade_getup_meter_crop(1000) == 0);

    /* The plain scale: half the starting getup time is half a bar. */
    start = 100;
    assert(wm_arcade_getup_meter_scale(50, WM_GETUP_SIZE, &start) == 40);
    assert(start == 100);

    /* First guard: a current getup above the starting one raises the
       starting one to it, and the bar reads full. */
    start = 100;
    assert(wm_arcade_getup_meter_scale(150, WM_GETUP_SIZE, &start) ==
           WM_GETUP_SIZE);
    assert(start == 150);

    /*
     * Second guard, transcribed and not repaired. Scaling 50 against
     * 100 gives 40; with DISPLAY_VAL at 10 that is "incremented", so the
     * source sets a11 to the SCALED 40 and divides by it -- pegging the
     * bar at GETUP_SIZE and leaving the starting value at 40, a scaled
     * number that every later tick then divides by.
     */
    start = 100;
    v = wm_arcade_getup_meter_scale(50, 10, &start);
    assert(v == WM_GETUP_SIZE);
    assert(start == 40);
    /* Which is why the next tick reads the way it does: 39 against a
       "starting" 40 is very nearly a full bar. */
    assert(wm_arcade_getup_meter_scale(39, WM_GETUP_SIZE, &start) == 78);

    /* A zero starting time divides by nothing. */
    start = 0;
    assert(wm_arcade_getup_meter_scale(0, 0, &start) == 0);
}

/* ---- the process ------------------------------------------------- */

static void test_the_process(void) {
    wm_arcade_actor_t me;
    wm_getup_meter_t st;
    wm_getup_meter_frame_t f;
    wm_arcade_actor_t *all[1];
    int i;

    all[0] = &me;

    /* A drone who is not allowed one: no process, and it says so. */
    be(&me, 0, 2, WM_PTYPE_DRONE);
    assert(!wm_arcade_getup_meter_start(&st, &me, all, 1, false, 1, false));
    assert(st.phase == (uint8_t)WM_GETUP_PHASE_NONE);
    assert(me.meter_proc == NULL);
    /* And it ticks to nothing rather than crashing. */
    wm_arcade_getup_meter_tick(&st, &me, NULL, &f);
    assert(!f.visible && !f.announce_sound);

    /* A human: the process exists, METER_PROC points at it, and
       slide_offscr's eighteen seconds are on the clock. */
    be(&me, 0, 0, WM_PTYPE_PLAYER);
    assert(wm_arcade_getup_meter_start(&st, &me, all, 1, false, 1, false));
    assert(st.phase == (uint8_t)WM_GETUP_PHASE_OFFSCR);
    assert(me.meter_proc == &st);
    assert(me.delay_meter == WM_GETUP_METER_DELAY);

    /*
     * The ten-tick grace is a ONE-OFF and not a period: a11 counts down
     * to zero and is never reloaded. With DELAY_METER cleared and a
     * getup time to serve, the meter comes out on the eleventh tick and
     * not before -- and that is the eleventh, not every eleventh.
     */
    me.delay_meter = 0;
    me.getup_time = 100;
    for (i = 0; i < WM_GETUP_METER_GRACE; ++i) {
        wm_arcade_getup_meter_tick(&st, &me, NULL, &f);
        assert(!f.visible);
        assert(st.phase == (uint8_t)WM_GETUP_PHASE_OFFSCR);
    }
    wm_arcade_getup_meter_tick(&st, &me, NULL, &f);
    assert(st.phase == (uint8_t)WM_GETUP_PHASE_ONSCR);
    assert(f.visible);
    assert(f.announce_sound);            /* triple_sound 0BDh, once */
    assert(st.start_getup == 100);
    assert(st.display_val == WM_GETUP_SIZE);

    /* Onscreen, the bar drains as he mashes. */
    wm_arcade_getup_meter_tick(&st, &me, NULL, &f);
    assert(f.visible && !f.announce_sound);
    assert(f.green_height == WM_GETUP_SIZE);   /* full, and staying full */
    me.getup_time = 20;
    wm_arcade_getup_meter_tick(&st, &me, NULL, &f);
    /* 20 of 100 is 16 of 80, averaged with the 80 it was at. */
    assert(st.display_val == (16 + WM_GETUP_SIZE) / 2);

    /* On his feet: the averaged value reaches zero and the meter goes
       back to sliding off -- which re-arms the eighteen seconds. */
    me.getup_time = 0;
    me.delay_meter = 0;
    for (i = 0; i < 40 && st.phase == (uint8_t)WM_GETUP_PHASE_ONSCR; ++i)
        wm_arcade_getup_meter_tick(&st, &me, NULL, &f);
    assert(st.phase == (uint8_t)WM_GETUP_PHASE_OFFSCR);
    assert(me.delay_meter == WM_GETUP_METER_DELAY);

    /* A death takes it away too, without waiting for the bar. */
    be(&me, 0, 0, WM_PTYPE_PLAYER);
    assert(wm_arcade_getup_meter_start(&st, &me, all, 1, false, 1, false));
    me.delay_meter = 0;
    me.getup_time = 100;
    for (i = 0; i <= WM_GETUP_METER_GRACE; ++i)
        wm_arcade_getup_meter_tick(&st, &me, NULL, &f);
    assert(st.phase == (uint8_t)WM_GETUP_PHASE_ONSCR);
    me.player_mode = (uint16_t)WM_PMODE_DEAD;
    wm_arcade_getup_meter_tick(&st, &me, NULL, &f);
    assert(st.phase == (uint8_t)WM_GETUP_PHASE_OFFSCR);
}

static void test_the_dufus_message(void) {
    wm_arcade_actor_t me;
    wm_getup_meter_t st;
    wm_getup_meter_frame_t f;
    wm_arcade_actor_t *all[1];
    int i, fired = 0;

    all[0] = &me;
    be(&me, 0, 0, WM_PTYPE_PLAYER);
    assert(wm_arcade_getup_meter_start(&st, &me, all, 1, false, 1, false));
    me.delay_meter = 0;
    me.getup_time = 300;
    for (i = 0; i <= WM_GETUP_METER_GRACE; ++i)
        wm_arcade_getup_meter_tick(&st, &me, NULL, &f);
    assert(st.phase == (uint8_t)WM_GETUP_PHASE_ONSCR);

    /*
     * `subk 1,a6 / jrnz #dont_bother` -- once, on the 120th onscreen
     * tick, and never again because a6 keeps counting past zero. He is
     * not mashing at all here, so nothing has been burned off and the
     * game offers to explain the buttons.
     */
    for (i = 0; i < 300; ++i) {
        wm_arcade_getup_meter_tick(&st, &me, NULL, &f);
        if (f.dufus_message) { ++fired; assert(i == WM_GETUP_DUFUS_TICK - 1); }
    }
    assert(fired == 1);

    /* And a player who IS mashing does not get told how. */
    be(&me, 0, 0, WM_PTYPE_PLAYER);
    assert(wm_arcade_getup_meter_start(&st, &me, all, 1, false, 1, false));
    me.delay_meter = 0;
    me.getup_time = 400;
    for (i = 0; i <= WM_GETUP_METER_GRACE; ++i)
        wm_arcade_getup_meter_tick(&st, &me, NULL, &f);
    fired = 0;
    for (i = 0; i < WM_GETUP_DUFUS_TICK; ++i) {
        me.getup_time -= 2;               /* 240 burned off, well over 175 */
        wm_arcade_getup_meter_tick(&st, &me, NULL, &f);
        if (f.dufus_message) ++fired;
    }
    assert(fired == 0);
}

/* ---- ditch_getup_meter, and its a9 twin -------------------------- */

static void test_the_ditch(void) {
    wm_arcade_actor_t me;
    wm_getup_meter_t st;
    wm_arcade_actor_t *all[1];

    all[0] = &me;
    be(&me, 0, 0, WM_PTYPE_PLAYER);
    assert(wm_arcade_getup_meter_start(&st, &me, all, 1, false, 1, false));

    /* "makes your getup meter go away if you've got one out" -- but it
       does nothing at all unless he has a getup time to lose. */
    me.delay_meter = 0;
    me.getup_time = 0;
    assert(!wm_arcade_ditch_getup_meter(&me, &st));
    assert(me.delay_meter == 0);

    /* Nor while he is dizzy. */
    me.getup_time = 100;
    me.plyr_dizzy = 1;
    assert(!wm_arcade_ditch_getup_meter(&me, &st));
    assert(me.delay_meter == 0);

    /* With both right, the meter is transferred back to slide_offscr,
       which is what stamps the delay. */
    me.plyr_dizzy = 0;
    assert(wm_arcade_ditch_getup_meter(&me, &st));
    assert(me.delay_meter == WM_GETUP_METER_DELAY);
    assert(st.phase == (uint8_t)WM_GETUP_PHASE_OFFSCR);

    /* And a wrestler with no meter has nothing to ditch. */
    me.delay_meter = 0;
    me.meter_proc = NULL;
    assert(!wm_arcade_ditch_getup_meter(&me, &st));
    assert(me.delay_meter == 0);

    /*
     * ditch_getup_meter_a9 is the same routine against whoever a9
     * holds instead of the process's own a13 -- that is its entire
     * content, and its one caller is WRESTLE2.ASM:2271.
     */
    be(&me, 0, 0, WM_PTYPE_PLAYER);
    assert(wm_arcade_getup_meter_start(&st, &me, all, 1, false, 1, false));
    me.delay_meter = 0;
    me.getup_time = 100;
    assert(wm_arcade_ditch_getup_meter_a9(&me, &st));
    assert(me.delay_meter == WM_GETUP_METER_DELAY);
    assert(!wm_arcade_ditch_getup_meter_a9(NULL, &st));
}

/* ------------------------------------------------------------------
 * And on the live loop, which is where the gap actually was.
 * ------------------------------------------------------------------ */

static wm_match_state MS;
static uint32_t hc_stub(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t spf_stub(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static void test_the_meter_runs_in_a_real_match(void) {
    WmRng r;
    uint32_t t = 1;
    wm_input_state in;
    int i;
    int came_out = -1, went_back = -1, rearms = 0;
    int32_t prev_dm;

    wm_rng_init(&r, 0x2468ACE0u, hc_stub, spf_stub, &t);
    memset(&MS, 0, sizeof MS);
    wm_match_init(&MS);
    wm_match_start_selected(&MS, &r, (uint8_t)WM_ROSTER_TAKER);
    memset(&in, 0, sizeof in);

    /*
     * The human's meter exists from the first tick, and the eighteen
     * seconds are already on his clock -- slide_offscr's own first
     * instruction, and the thing this port was missing.
     */
    assert(MS.actors[0].meter_proc == &MS.getup_meter[0]);
    assert(MS.getup_meter[0].phase == (uint8_t)WM_GETUP_PHASE_OFFSCR);
    assert(MS.actors[0].delay_meter == WM_GETUP_METER_DELAY);

    /*
     * The lone DRONE opposite him has none, and that is the source's
     * answer rather than an omission: he is not a human, he has no human
     * teammate, and the drone_meters_on powerup is off, so his process
     * reaches `#die` and clears its own METER_PROC.
     */
    assert(MS.actors[1].plyr_type == WM_PTYPE_DRONE);
    assert(MS.actors[1].meter_proc == NULL);
    assert(MS.getup_meter[1].phase == (uint8_t)WM_GETUP_PHASE_NONE);

    prev_dm = MS.actors[0].delay_meter;
    for (i = 0; i < 3000; ++i) {
        /* Knock him down twice, the second time well after the first
           meter has gone away again. */
        if (i == 1200 || i == 2500) MS.actors[0].getup_time = 200;
        wm_match_tick(&MS, NULL, &in);
        if (came_out < 0 &&
            MS.getup_meter[0].phase == (uint8_t)WM_GETUP_PHASE_ONSCR)
            came_out = i;
        if (came_out >= 0 && went_back < 0 &&
            MS.getup_meter[0].phase == (uint8_t)WM_GETUP_PHASE_OFFSCR)
            went_back = i;
        if (prev_dm == 0 && MS.actors[0].delay_meter == WM_GETUP_METER_DELAY)
            ++rearms;
        prev_dm = MS.actors[0].delay_meter;
    }

    /*
     * It could not come out before the delay ran down -- which is the
     * whole of the behaviour change, since DELAY_METER set is what makes
     * wm_arcade_tick_getup_time wipe GETUP_TIME outright and stand him
     * straight back up.
     */
    assert(came_out > WM_GETUP_METER_DELAY);
    assert(came_out >= 1200);
    /* And it did come out, on the knockdown, once he was allowed one. */
    assert(came_out <= 1200 + WM_GETUP_METER_GRACE + 2);
    /* Then went away again when he got to his feet. */
    assert(went_back > came_out);
    /* Twice out, twice re-armed. */
    assert(rearms == 2);
}

int main(void) {
    test_who_gets_a_meter();
    test_the_side_hack();
    test_the_come_out_gate();
    test_the_bar();
    test_the_process();
    test_the_dufus_message();
    test_the_ditch();
    test_the_meter_runs_in_a_real_match();
    printf("getup meter ok\n");
    return 0;
}
