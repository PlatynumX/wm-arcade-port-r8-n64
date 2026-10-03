/*
 * WRESTLE.ASM:663 plyr_strtb1 / plyr_strtb2 -- somebody pressed
 * start.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_plyr_start.h"

static wm_plyr_start_state_t fresh(void) {
    wm_plyr_start_state_t s;
    s.winstreak = 7;
    s.winstreakd = 3;
    s.entered_inits = 0x11223344;
    s.process_ptr = 0x0f0f0f0f;
    s.pin_time_total = 4321;
    return s;
}

static void test_guards(void) {
    /* Any negative GAMSTATE is the diagnostic menu. */
    assert(!wm_plyr_start_allowed(WM_GAMSTATE_INDIAG, 0, 0));
    assert(!wm_plyr_start_allowed(-99, 0, 0));
    /* The winner's party. */
    assert(!wm_plyr_start_allowed(WM_GAMSTATE_INPARTY, 0, 0));
    /* Already playing: the bit for THIS player, not either. */
    assert(!wm_plyr_start_allowed(WM_GAMSTATE_INGAME, 0x1, 0));
    assert(wm_plyr_start_allowed(WM_GAMSTATE_INGAME, 0x1, 1));
    assert(!wm_plyr_start_allowed(WM_GAMSTATE_INGAME, 0x2, 1));
    assert(wm_plyr_start_allowed(WM_GAMSTATE_INGAME, 0x2, 0));
    /* Nobody playing: both may join. */
    assert(wm_plyr_start_allowed(WM_GAMSTATE_INAMODE, 0, 0));
    assert(wm_plyr_start_allowed(WM_GAMSTATE_INAMODE, 0, 1));
}

/* A guard that fails touches nothing at all. */
static void test_a_blocked_start_changes_nothing(void) {
    wm_plyr_start_state_t s = fresh();
    wm_plyr_start_state_t before = s;
    wm_plyr_start_result_t r =
        wm_plyr_start(0, WM_GAMSTATE_INPARTY, 0, 0, true, &s);

    assert(r.outcome == WM_PLYR_START_DIE);
    assert(!r.cleared_state);
    assert(!r.awards_reset_due);
    assert(!r.took_credit);
    assert(memcmp(&s, &before, sizeof s) == 0);
}

/*
 * The state is wiped BEFORE the credit is checked, so a player with
 * no credit still loses his win streak and his pin times.
 */
static void test_no_credit_still_wipes_the_state(void) {
    wm_plyr_start_state_t s = fresh();
    wm_plyr_start_result_t r =
        wm_plyr_start(0, WM_GAMSTATE_INAMODE, 0, 0, false, &s);

    assert(r.outcome == WM_PLYR_START_DIE);
    assert(!r.took_credit);
    /* And yet: */
    assert(r.cleared_state);
    assert(r.awards_reset_due);
    assert(r.dufus_msgs_reset_due);
    assert(r.icon_total_clear_due);
    assert(s.winstreak == 0);
    assert(s.entered_inits == 0);
    assert(s.process_ptr == 0);
    assert(s.pin_time_total == 0);
}

/* A NEGATIVE winstreakd survives; anything else is cleared. */
static void test_negative_winstreakd_survives(void) {
    wm_plyr_start_state_t s = fresh();
    s.winstreakd = -5;
    (void)wm_plyr_start(0, WM_GAMSTATE_INAMODE, 0, 0, true, &s);
    assert(s.winstreakd == -5);

    s = fresh();
    s.winstreakd = 0;
    (void)wm_plyr_start(0, WM_GAMSTATE_INAMODE, 0, 0, true, &s);
    assert(s.winstreakd == 0);

    s = fresh();
    s.winstreakd = 1;
    (void)wm_plyr_start(0, WM_GAMSTATE_INAMODE, 0, 0, true, &s);
    assert(s.winstreakd == 0);
}

/* The seven-way exit. */
static void test_dispatch(void) {
    struct { int32_t gamstate; wm_plyr_start_outcome_t want; } cases[] = {
        { WM_GAMSTATE_INAMODE,    WM_PLYR_START_AMODE },
        { WM_GAMSTATE_INGAMEOVER, WM_PLYR_START_GAMEOVER },
        { WM_GAMSTATE_INSELECT,   WM_PLYR_START_SELECT },
        { WM_GAMSTATE_INPREGAME,  WM_PLYR_START_PREGAME },
        /* Both of these go to the same place. */
        { WM_GAMSTATE_INPREGAME2, WM_PLYR_START_MIDGAME },
        { WM_GAMSTATE_INGAME,     WM_PLYR_START_MIDGAME },
        /* 5 and 8 are GAMSTATE values the dispatch does not name. */
        { 5,                      WM_PLYR_START_IMPOSSIBLE },
        { 8,                      WM_PLYR_START_IMPOSSIBLE },
    };
    size_t i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        wm_plyr_start_state_t s = fresh();
        wm_plyr_start_result_t r =
            wm_plyr_start(1, cases[i].gamstate, 0, 0, true, &s);
        assert(r.outcome == cases[i].want);
        assert(r.player == 1);
        /* Every one of them took the credit first. */
        assert(r.took_credit);
    }
}

/*
 * OLD_PSTATUS means he was playing a moment ago: a continue, which
 * takes no credit -- and anywhere but the select screen the source
 * LOCKUPs rather than guessing.
 */
static void test_continue_path(void) {
    wm_plyr_start_state_t s = fresh();
    wm_plyr_start_result_t r =
        wm_plyr_start(0, WM_GAMSTATE_INSELECT, 0, 0x1, true, &s);
    assert(r.outcome == WM_PLYR_START_WAITCONT);
    assert(!r.took_credit);
    assert(r.cleared_state);

    /* Same OLD_PSTATUS, wrong screen. */
    s = fresh();
    r = wm_plyr_start(0, WM_GAMSTATE_INGAME, 0, 0x1, true, &s);
    assert(r.outcome == WM_PLYR_START_IMPOSSIBLE);
    assert(!r.took_credit);

    /* It is this player's bit that matters, not the other's. */
    s = fresh();
    r = wm_plyr_start(0, WM_GAMSTATE_INAMODE, 0, 0x2, true, &s);
    assert(r.outcome == WM_PLYR_START_AMODE);
    assert(r.took_credit);
}

/* The two entry points differ only in which player they are. */
static void test_both_players(void) {
    int p;
    for (p = 0; p < 2; ++p) {
        wm_plyr_start_state_t s = fresh();
        wm_plyr_start_result_t r =
            wm_plyr_start(p, WM_GAMSTATE_INGAME, 0, 0, true, &s);
        assert(r.player == p);
        assert(r.outcome == WM_PLYR_START_MIDGAME);
        assert(s.process_ptr == 0);
    }
    /* A player number outside 0..1 is not a player. */
    {
        wm_plyr_start_state_t s = fresh();
        wm_plyr_start_result_t r =
            wm_plyr_start(2, WM_GAMSTATE_INGAME, 0, 0, true, &s);
        assert(r.outcome == WM_PLYR_START_DIE);
        assert(!r.cleared_state);
    }
}

int main(void) {
    test_guards();
    test_a_blocked_start_changes_nothing();
    test_no_credit_still_wipes_the_state();
    test_negative_winstreakd_survives();
    test_dispatch();
    test_continue_path();
    test_both_players();
    printf("plyr start ok\n");
    return 0;
}
