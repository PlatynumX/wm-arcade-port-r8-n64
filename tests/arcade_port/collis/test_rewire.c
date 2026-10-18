/*
 * LIFEBAR.ASM:880 rewire_monitor -- which wrestler the second life
 * bar is showing, in each of its three completely different loops.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_lifebar.h"

#define N 4

static void mkactors(wm_rewire_actor_t a[N]) {
    int i;
    memset(a, 0, sizeof(wm_rewire_actor_t) * N);
    for (i = 0; i < N; ++i) {
        a[i].active = true;
        a[i].plyrnum = i;
        a[i].closest_num = -1;
        a[i].plyr_side = (i & 1) ? 1 : 0;
        a[i].dead = false;
    }
}

/* Run one poll's worth of ticks and return the rewires it made. */
static size_t poll(wm_rewire_t *r, const wm_rewire_actor_t *a, int32_t pcnt) {
    int i;
    size_t n = 0;
    for (i = 0; i < WM_REWIRE_POLL_TICKS; ++i) {
        assert(wm_rewire_tick(r, a, N, pcnt));
        if (r->rewire_count) n = r->rewire_count;
    }
    return n;
}

static void test_mode_choice(void) {
    /* Buddy wins outright, then rumble, then the two exits. */
    assert(wm_rewire_mode(true, true, 3, 1) == WM_REWIRE_MODE_BUDDY);
    assert(wm_rewire_mode(false, true, 3, 1) == WM_REWIRE_MODE_RUMBLE);
    assert(wm_rewire_mode(false, false, 3, 4) == WM_REWIRE_MODE_DIE);
    assert(wm_rewire_mode(false, false, 1, 1) == WM_REWIRE_MODE_DIE);
    assert(wm_rewire_mode(false, false, 1, 4) == WM_REWIRE_MODE_NORMAL);
}

static void test_die_does_nothing(void) {
    wm_rewire_t r;
    wm_rewire_actor_t a[N];
    mkactors(a);
    assert(!wm_rewire_begin(&r, WM_REWIRE_MODE_DIE, a, N));
    assert(!wm_rewire_tick(&r, a, N, 0));
    assert(r.rewire_count == 0);
}

/* NORMAL: no live process at all and the monitor dies at once. */
static void test_normal_needs_a_process(void) {
    wm_rewire_t r;
    wm_rewire_actor_t a[N];
    int i;
    mkactors(a);
    for (i = 0; i < N; ++i) a[i].active = false;
    assert(!wm_rewire_begin(&r, WM_REWIRE_MODE_NORMAL, a, N));
}

/*
 * NORMAL: the display follows the human's CLOSEST_NUM, but not
 * within #LATENCY of the last rewire.
 */
static void test_normal_latency(void) {
    wm_rewire_t r;
    wm_rewire_actor_t a[N];
    int32_t pcnt = 1000;

    mkactors(a);
    a[0].closest_num = 1;            /* the human is fighting 1 */
    assert(wm_rewire_begin(&r, WM_REWIRE_MODE_NORMAL, a, N));
    assert(r.watcher == 0);
    /* Its starting idea of who is shown comes from the same read. */
    assert(r.showing == 1);

    /* Nothing has changed: no rewire, ever. */
    assert(poll(&r, a, pcnt) == 0);
    assert(poll(&r, a, pcnt) == 0);

    /* The human turns on wrestler 2. The first rewire is free --
       there has been no previous one to be too soon after. */
    a[0].closest_num = 2;
    assert(poll(&r, a, pcnt) == 1);
    assert(r.rewires[0].wrestler == 2);
    assert(r.rewires[0].display == a[2].plyr_side);
    assert(r.showing == 2);

    /* He turns straight back. Too soon: the bar stays on 2. */
    a[0].closest_num = 1;
    assert(poll(&r, a, pcnt + 1) == 0);
    assert(poll(&r, a, pcnt + WM_REWIRE_LATENCY) == 0);
    assert(r.showing == 2);

    /* One past the deadline and it moves. The compare is `jrle`,
       so equal is still too soon. */
    assert(poll(&r, a, pcnt + WM_REWIRE_LATENCY + 1) == 1);
    assert(r.rewires[0].wrestler == 1);
    assert(r.showing == 1);
}

/* The latency is waived when the DISPLAYED wrestler is dead. */
static void test_normal_death_waives_the_latency(void) {
    wm_rewire_t r;
    wm_rewire_actor_t a[N];
    int32_t pcnt = 500;

    mkactors(a);
    a[0].closest_num = 1;
    assert(wm_rewire_begin(&r, WM_REWIRE_MODE_NORMAL, a, N));

    a[0].closest_num = 2;
    assert(poll(&r, a, pcnt) == 1);      /* rewires to 2, sets the clock */
    assert(r.showing == 2);

    /* The human turns away and 2 dies. Normally too soon; not now. */
    a[0].closest_num = 3;
    a[2].dead = true;
    assert(poll(&r, a, pcnt + 1) == 1);
    assert(r.rewires[0].wrestler == 3);

    /* And with the displayed wrestler alive again, the latency is
       back -- the same tick offset now does nothing. */
    a[0].closest_num = 1;
    assert(poll(&r, a, pcnt + 2) == 0);
}

/*
 * RUMBLE: a9 starts at 3 and the routine jumps straight to #toggle,
 * so its very first act is to put wrestler 2 up.
 */
static void test_rumble_starts_by_toggling(void) {
    wm_rewire_t r;
    wm_rewire_actor_t a[N];
    mkactors(a);
    assert(wm_rewire_begin(&r, WM_REWIRE_MODE_RUMBLE, a, N));
    assert(r.rumble_at == 3);

    assert(poll(&r, a, 0) == 1);
    assert(r.rewires[0].wrestler == 2);
    assert(r.rewires[0].display == 1);   /* always display 1 */
    assert(r.rumble_at == 2);
    assert(r.rumble_hold == WM_REWIRE_RUMBLE_HOLD);
}

/* It holds each wrestler for a fixed number of POLLS, then swaps. */
static void test_rumble_holds_then_swaps(void) {
    wm_rewire_t r;
    wm_rewire_actor_t a[N];
    int i;
    mkactors(a);
    assert(wm_rewire_begin(&r, WM_REWIRE_MODE_RUMBLE, a, N));
    assert(poll(&r, a, 0) == 1);         /* -> 2 */

    /* The hold is counted in polls, not ticks. */
    for (i = 0; i < WM_REWIRE_RUMBLE_HOLD - 1; ++i) {
        assert(poll(&r, a, 0) == 0);
    }
    assert(poll(&r, a, 0) == 1);
    assert(r.rewires[0].wrestler == 3);
    assert(r.rumble_at == 3);
}

/* A dead wrestler ends the hold at once, and it refuses to toggle
   onto a dead one. */
static void test_rumble_and_death(void) {
    wm_rewire_t r;
    wm_rewire_actor_t a[N];
    int i;

    mkactors(a);
    assert(wm_rewire_begin(&r, WM_REWIRE_MODE_RUMBLE, a, N));
    assert(poll(&r, a, 0) == 1);         /* -> 2 */

    a[2].dead = true;
    assert(poll(&r, a, 0) == 1);         /* dies -> swaps early */
    assert(r.rumble_at == 3);

    /* Now 3 dies too, and 2 is still dead: nothing to toggle to,
       so the bar keeps showing 3 rather than going blank. */
    a[3].dead = true;
    for (i = 0; i < 5; ++i) {
        assert(poll(&r, a, 0) == 0);
        assert(r.rumble_at == 3);
    }
}

/*
 * BUDDY: display 0 follows wrestler 0 or, dead, wrestler 2; display
 * 1 follows 1 or 3. Both can change on the same pass.
 */
static void test_buddy(void) {
    wm_rewire_t r;
    wm_rewire_actor_t a[N];

    mkactors(a);
    assert(wm_rewire_begin(&r, WM_REWIRE_MODE_BUDDY, a, N));
    assert(r.buddy_show0 == 0 && r.buddy_show1 == 1);

    /* Everyone alive and each display already on its primary. */
    assert(poll(&r, a, 0) == 0);

    /* Both humans die in the same frame: both bars switch, in one
       pass, in display order. */
    a[0].dead = true;
    a[1].dead = true;
    assert(poll(&r, a, 0) == 2);
    assert(r.rewires[0].display == 0 && r.rewires[0].wrestler == 2);
    assert(r.rewires[1].display == 1 && r.rewires[1].wrestler == 3);
    assert(r.buddy_show0 == 2 && r.buddy_show1 == 3);

    /* Steady state again. */
    assert(poll(&r, a, 0) == 0);

    /* Player 1 comes back: display 0 returns to him. */
    a[0].dead = false;
    assert(poll(&r, a, 0) == 1);
    assert(r.rewires[0].display == 0 && r.rewires[0].wrestler == 0);

    /* Both the primary and the fallback dead: the bar keeps showing
       the dead fallback rather than blanking. */
    a[1].dead = true;
    a[3].dead = true;
    assert(poll(&r, a, 0) == 0);
    assert(r.buddy_show1 == 3);
}

/* All three loops poll every tenth tick, not every tick. */
static void test_poll_rate(void) {
    wm_rewire_t r;
    wm_rewire_actor_t a[N];
    int i;
    int rewire_ticks = 0;

    mkactors(a);
    assert(wm_rewire_begin(&r, WM_REWIRE_MODE_RUMBLE, a, N));
    for (i = 0; i < WM_REWIRE_POLL_TICKS * 3; ++i) {
        assert(wm_rewire_tick(&r, a, N, 0));
        if (r.rewire_count) {
            /* Every rewire lands on a multiple of the poll rate. */
            assert(i % WM_REWIRE_POLL_TICKS == 0);
            rewire_ticks++;
        }
    }
    assert(rewire_ticks >= 1);
}

int main(void) {
    test_mode_choice();
    test_die_does_nothing();
    test_normal_needs_a_process();
    test_normal_latency();
    test_normal_death_waives_the_latency();
    test_rumble_starts_by_toggling();
    test_rumble_holds_then_swaps();
    test_rumble_and_death();
    test_buddy();
    test_poll_rate();
    printf("rewire ok\n");
    return 0;
}
