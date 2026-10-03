/*
 * AWARD.ASM:2198 powerup_check / :2182 player_powerup_checker, run
 * for real. Every piece of this was translated a while back and
 * nothing ever called it, so no code could be entered and buddy
 * mode was unreachable in play.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/app.h"
#include "wm/arcade/wm_arcade_powerup.h"

/* The compile-time bound must match the real table. */
static void test_code_slots(void) {
    assert(wm_powerup_code_count <= WM_PUP_CODE_SLOTS);
    assert(wm_powerup_code_count == 8);
}

/*
 * no_block is Block x3 and buddy_mode is Block x5, so the shorter
 * code is a PREFIX of the longer one: you cannot enter buddy mode
 * without also turning blocking off on the way through. That is the
 * table as written, not a quirk of this driver.
 */
static void test_buddy_code_contains_no_block(void) {
    const wm_powerup_code *nb = NULL, *bm = NULL;
    int i, k;

    for (i = 0; i < wm_powerup_code_count; ++i) {
        if (strcmp(wm_powerup_codes[i].name, "no_block") == 0)
            nb = &wm_powerup_codes[i];
        if (strcmp(wm_powerup_codes[i].name, "buddy_mode") == 0)
            bm = &wm_powerup_codes[i];
    }
    assert(nb && bm);
    assert(nb->steps == 3 && bm->steps == 5);
    for (k = 0; k < nb->steps; ++k) assert(nb->step[k] == bm->step[k]);
    for (k = 0; k < bm->steps; ++k) assert(bm->step[k] == WM_PUP_BLOCK);
}

/* Drive the app to a two-player match, tapping Block five times
   during the pregame on both pads. */
static void run_to_match(wm_app *app, bool enter_buddy_code) {
    wm_input_state p1, p2;
    unsigned i;
    unsigned taps = 0;
    bool down = false;

    wm_app_init(app);
    for (i = 0; i < 40000u && app->mode != WM_APP_MODE_MATCH; ++i) {
        memset(&p1, 0, sizeof p1);
        memset(&p2, 0, sizeof p2);
        p1.start = true;
        p2.start = true;
        if (app->mode == WM_APP_MODE_PREGAME) {
            /*
             * No filler input during the pregame: the codes are
             * listening, and four punches spell move_names. That
             * is not a flaw in the test harness, it is the feature
             * -- any button pressed in this window is code input.
             */
            if (enter_buddy_code && taps < 5u) {
                /* Tap, not hold: PUPWAITSWITCH wants switches-DOWN. */
                down = !down;
                p1.block = down;
                p2.block = down;
                if (down) taps++;
            }
        } else {
            p1.light_punch = (i % 7) == 0;
            p2.light_punch = (i % 11) == 0;
        }
        wm_app_tick_dual(app, &p1, &p2);
    }
}

/* Without the code, nothing is requested and buddy mode is off --
   the same answer as before, but now because it was ASKED. */
static void test_no_code_no_buddies(void) {
    wm_app app;
    run_to_match(&app, false);
    assert(app.mode == WM_APP_MODE_MATCH);
    /* Two players here, so drone_meters' gate fails and nothing at
       all is granted. */
    assert(app.match.pstatus == 3);
    assert(app.powerups.p_request[0] == 0);
    assert(app.powerups.p_request[1] == 0);
    assert(app.match.buddy_mode_checked);
    assert(!app.match.buddy_mode_on);
    assert(app.match.actor_count == 2);
}

/*
 * drone_meters is not a code. AWARD.ASM:2016 has its three-Kick
 * sequence commented out, so in a one-player one-on-one game it is
 * simply ON, with nothing entered.
 */
static void test_drone_meters_is_automatic(void) {
    wm_app app;
    wm_input_state p1;
    unsigned i;

    wm_app_init(&app);
    for (i = 0; i < 40000u && app.mode != WM_APP_MODE_MATCH; ++i) {
        memset(&p1, 0, sizeof p1);
        p1.start = true;
        if (app.mode != WM_APP_MODE_PREGAME)
            p1.light_punch = (i % 7) == 0;
        wm_app_tick_dual(&app, &p1, NULL);   /* one player */
    }
    assert(app.mode == WM_APP_MODE_MATCH);
    assert(app.match.pstatus == 1);
    /* The first ladder rung is one-on-one, so both gates pass. */
    assert(app.pregame.opponent_count == 1);
    assert(app.powerups.p_request[0] & WM_PU_D_METERS_ON);
    /* Nothing else was entered. */
    assert(!(app.powerups.p_request[0] & WM_PU_BUDDY_MODE));
}

/* With it, both players request it and the match fields four. */
static void test_buddy_code_turns_it_on(void) {
    wm_app app;
    run_to_match(&app, true);

    assert(app.mode == WM_APP_MODE_MATCH);
    /* Both players entered it, so both request words carry the bit
       and get_powerups' AND keeps it. */
    assert(app.powerups.p_request[0] & WM_PU_BUDDY_MODE);
    assert(app.powerups.p_request[1] & WM_PU_BUDDY_MODE);
    /* And no_block came along for the ride, being a prefix. */
    assert(app.powerups.p_request[0] & WM_PU_BLOCKING_OFF);

    /* Two humans, so #2plyr, so buddy mode really applies. */
    assert(app.match.pstatus == 3);
    assert(app.match.buddy_mode_checked);
    assert(app.match.buddy_mode_on);
    assert(app.match.actor_count == 4);
    assert(app.match.actors[2].plyr_type == WM_PTYPE_DRONE);
    assert(app.match.actors[3].plyr_type == WM_PTYPE_DRONE);
}

/* A HELD button is one press, not many -- the edge detection. */
static void test_holding_block_is_one_press(void) {
    wm_app app;
    wm_input_state p1, p2;
    unsigned i;

    wm_app_init(&app);
    for (i = 0; i < 40000u && app.mode != WM_APP_MODE_MATCH; ++i) {
        memset(&p1, 0, sizeof p1);
        memset(&p2, 0, sizeof p2);
        p1.start = true;
        p2.start = true;
        p1.light_punch = (i % 7) == 0;
        p2.light_punch = (i % 11) == 0;
        if (app.mode == WM_APP_MODE_PREGAME) {
            p1.block = true;          /* held down the whole time */
            p2.block = true;
            p1.light_punch = false;
            p2.light_punch = false;
        }
        wm_app_tick_dual(&app, &p1, &p2);
    }
    assert(app.mode == WM_APP_MODE_MATCH);
    /* One press cannot finish a three- or five-press code. */
    assert(!(app.powerups.p_request[0] & WM_PU_BUDDY_MODE));
    assert(!(app.powerups.p_request[0] & WM_PU_BLOCKING_OFF));
    assert(!app.match.buddy_mode_on);
}

int main(void) {
    test_code_slots();
    test_buddy_code_contains_no_block();
    test_no_code_no_buddies();
    test_drone_meters_is_automatic();
    test_buddy_code_turns_it_on();
    test_holding_block_is_one_press();
    printf("powerup entry ok\n");
    return 0;
}
