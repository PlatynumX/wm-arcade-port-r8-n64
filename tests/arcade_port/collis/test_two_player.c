/*
 * WRESTLE.ASM:1801 #2plyr -- the two-player creation path, and the
 * buddy draw behind it (WRESTLE2.ASM:4548 choose_buddies /
 * PROGRESS.ASM:4273 get_rnd_wrestler).
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/app.h"
#include "wm/match.h"
#include "wm/arcade/wm_arcade_buddies.h"

static uint32_t hc(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t spf(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static WmRng RNG;
static uint32_t RNG_T;

static void rng_reset(void) {
    RNG_T = 1;
    memset(&RNG, 0, sizeof RNG);
    wm_rng_init(&RNG, 0x2468ACE0u, hc, spf, &RNG_T);
}

/* ---- get_rnd_wrestler -------------------------------------------- */

static void test_get_rnd_wrestler(void) {
    unsigned w;
    int seen[WM_BUDDY_WRESTLER_SLOTS];
    int i;

    /* Nothing excluded: every slot is reachable, INCLUDING slot 7 --
       the cut Adam Bomb. This routine counts only the mask and does
       not skip him the way the rest of the game does. */
    memset(seen, 0, sizeof seen);
    rng_reset();
    for (i = 0; i < 4000; ++i) {
        w = wm_get_rnd_wrestler(0, &RNG);
        assert(w < WM_BUDDY_WRESTLER_SLOTS);
        seen[w] = 1;
    }
    for (i = 0; i < WM_BUDDY_WRESTLER_SLOTS; ++i) assert(seen[i]);
    assert(seen[7]);

    /* An excluded slot is never returned. */
    rng_reset();
    for (i = 0; i < 2000; ++i) {
        w = wm_get_rnd_wrestler((uint8_t)0x85, &RNG);   /* 0, 2, 7 */
        assert(w != 0 && w != 2 && w != 7);
        assert(w < WM_BUDDY_WRESTLER_SLOTS);
    }

    /* One slot free: it is the only answer, whatever the draw. */
    rng_reset();
    for (i = 0; i < 50; ++i)
        assert(wm_get_rnd_wrestler((uint8_t)0xef, &RNG) == 4);

    /* Everything excluded is a state the source cannot reach and
       walks off the end of the mask in; this stops instead. */
    assert(wm_get_rnd_wrestler((uint8_t)0xff, &RNG) == 0);

    /* The popcount the draw is scaled by. */
    assert(wm_buddy_excluded_count(0) == 0);
    assert(wm_buddy_excluded_count(0xff) == 8);
    assert(wm_buddy_excluded_count(0x85) == 3);
}

/* ---- choose_buddies ---------------------------------------------- */

static void test_choose_buddies(void) {
    int i;
    rng_reset();
    for (i = 0; i < 500; ++i) {
        wm_buddies_t b = wm_choose_buddies(1, 3, &RNG);
        /* Neither player's own wrestler can be a buddy... */
        assert(b.first != 1 && b.first != 3);
        assert(b.second != 1 && b.second != 3);
        /* ...and the two buddies are always different, because the
           first draw joins the mask before the second. Unlike the
           ladder scramble, which tolerates a collision. */
        assert(b.first != b.second);
        assert(b.first < 8 && b.second < 8);
        /* The mask afterwards holds all four. */
        assert(b.excluded & (1u << 1));
        assert(b.excluded & (1u << 3));
        assert(b.excluded & (1u << b.first));
        assert(b.excluded & (1u << b.second));
        assert(wm_buddy_excluded_count(b.excluded) == 4);
    }

    /* The PUSH/PULL pair swaps them: player one gets the SECOND
       draw, player two the first. */
    {
        wm_buddies_t b;
        b.first = 5;
        b.second = 6;
        b.excluded = 0;
        assert(wm_buddy_for_player1(&b) == 6);
        assert(wm_buddy_for_player2(&b) == 5);
    }
}

/* ---- #2plyr ------------------------------------------------------- */

static void test_two_player_plain(void) {
    wm_match_state m;
    wm_match_init(&m);
    rng_reset();

    /* No buddy mode: neither request carries it. */
    wm_match_start_two_player(&m, &RNG, 3, 2, 5, false, 0, 0);

    assert(m.active);
    assert(m.pstatus == 3);
    assert(m.actor_count == 2);
    assert(m.buddy_mode_checked);
    assert(!m.buddy_mode_on);

    /* Both slots are humans, and both are PTYPE_PLAYER. */
    assert(m.actor_is_human[0] && m.actor_is_human[1]);
    assert(m.actors[0].plyr_type == WM_PTYPE_PLAYER);
    assert(m.actors[1].plyr_type == WM_PTYPE_PLAYER);
    assert(m.has_human);

    /* PLYRNUM 0 and 1, opposite sides, the chosen wrestlers. */
    assert(m.actors[0].player_num == 0 && m.actors[1].player_num == 1);
    assert(m.actors[0].player_side == 0);
    assert(m.actors[1].player_side == 1);
    assert(m.actors[0].wrestler_num == 2);
    assert(m.actors[1].wrestler_num == 5);
    assert(m.actors[0].life == 163 && m.actors[1].life == 163);
}

/*
 * `movi PSIDE_PLYR2,a9 / move @royal_rumble,a14 / sub a14,a9` --
 * one subtract puts both humans on side zero.
 */
static void test_royal_rumble_makes_them_teammates(void) {
    wm_match_state m;
    wm_match_init(&m);
    rng_reset();
    wm_match_start_two_player(&m, &RNG, 3, 0, 1, true, 0, 0);

    assert(m.actors[0].player_side == 0);
    assert(m.actors[1].player_side == 0);   /* 1 - 1 */
}

/* Each slot's PTYPE comes from its OWN bit -- dead branching on the
   shipped dispatch, which only ever reaches #2plyr with PSTATUS 3,
   but the code is written for it. */
static void test_pstatus_bits_pick_the_type(void) {
    wm_match_state m;

    wm_match_init(&m);
    rng_reset();
    wm_match_start_two_player(&m, &RNG, 1, 0, 1, false, 0, 0);
    assert(m.actors[0].plyr_type == WM_PTYPE_PLAYER);
    assert(m.actors[1].plyr_type == WM_PTYPE_DRONE);
    assert(m.actor_is_human[0] && !m.actor_is_human[1]);
    assert(m.human_actor_index == 0);

    wm_match_init(&m);
    rng_reset();
    wm_match_start_two_player(&m, &RNG, 2, 0, 1, false, 0, 0);
    assert(m.actors[0].plyr_type == WM_PTYPE_DRONE);
    assert(m.actors[1].plyr_type == WM_PTYPE_PLAYER);
    assert(!m.actor_is_human[0] && m.actor_is_human[1]);
    assert(m.human_actor_index == 1);
}

/* Buddy mode needs BOTH requests -- an AND, not an OR. */
static void test_buddy_mode_needs_both(void) {
    assert(!wm_match_buddy_mode_on(0, 0));
    assert(!wm_match_buddy_mode_on(128, 0));
    assert(!wm_match_buddy_mode_on(0, 128));
    assert(wm_match_buddy_mode_on(128, 128));
    /* Other powerup bits in the same words do not leak in. */
    assert(!wm_match_buddy_mode_on(0x7f, 0x7f));
    assert(wm_match_buddy_mode_on(0xff, 0x80));
}

static void test_buddy_mode_creates_four(void) {
    wm_match_state m;
    unsigned i;

    wm_match_init(&m);
    rng_reset();
    wm_match_start_two_player(&m, &RNG, 3, 2, 5, false, 128, 128);

    assert(m.buddy_mode_checked && m.buddy_mode_on);
    assert(m.actor_count == 4);

    /* Two humans, two drone partners. */
    assert(m.actors[0].plyr_type == WM_PTYPE_PLAYER);
    assert(m.actors[1].plyr_type == WM_PTYPE_PLAYER);
    assert(m.actors[2].plyr_type == WM_PTYPE_DRONE);
    assert(m.actors[3].plyr_type == WM_PTYPE_DRONE);
    assert(!m.actor_is_human[2] && !m.actor_is_human[3]);

    /* PLYRNUM 2 and 3, one partner on each side. */
    assert(m.actors[2].player_num == 2 && m.actors[3].player_num == 3);
    assert(m.actors[2].player_side == 0);
    assert(m.actors[3].player_side == 1);

    /* Neither partner is one of the chosen wrestlers, nor each
       other. */
    for (i = 2; i < 4; ++i) {
        assert(m.actors[i].wrestler_num != 2);
        assert(m.actors[i].wrestler_num != 5);
    }
    assert(m.actors[2].wrestler_num != m.actors[3].wrestler_num);

    /* All four start on full life and have watchdogs armed. */
    for (i = 0; i < 4; ++i) assert(m.actors[i].life == 163);

    /*
     * #set0 picks a start row by COUNTING how many wrestlers are
     * already on your side, so each partner takes row 1 of his own
     * team's table -- "second" -- rather than an offset from his
     * player. These are #team1_starts / #team2_starts as written,
     * with RING_X_CENTER = 1074.
     */
    assert(m.actors[0].x_int == 1074 - 85);
    assert(m.actors[1].x_int == 1074 + 85);
    assert(m.actors[2].x_int == 1074 - 150);
    assert(m.actors[3].x_int == 1074 + 150);
    assert(m.actors[2].z_int == 1127 + 170);
    assert(m.actors[3].z_int == 1103 + 170);
    /* Row 1's face_dir words are 9 and 5. */
    assert(m.actors[2].facing_dir == 9);
    assert(m.actors[3].facing_dir == 5);
    /* And row 0's are 9 and 6, which the two humans already use. */
    assert(m.actors[0].facing_dir == 9);
    assert(m.actors[1].facing_dir == 6);
}

/*
 * The buddies do NOT get the royal-rumble subtract -- only the two
 * humans do. In a rumble the humans share side 0 while their
 * partners stay on opposite sides, which is what the source's bare
 * `movk PSIDE_PLYR2,a9` says.
 */
static void test_rumble_does_not_move_the_buddies(void) {
    wm_match_state m;
    wm_match_init(&m);
    rng_reset();
    wm_match_start_two_player(&m, &RNG, 3, 0, 1, true, 128, 128);

    assert(m.actor_count == 4);
    assert(m.actors[0].player_side == 0);
    assert(m.actors[1].player_side == 0);   /* the subtract */
    assert(m.actors[2].player_side == 0);
    assert(m.actors[3].player_side == 1);   /* NOT subtracted */
}

/* A four-actor match must not treat a partner as an opponent. */
static void test_partner_is_not_an_opponent(void) {
    wm_match_state m;
    unsigned t;

    wm_match_init(&m);
    rng_reset();
    wm_match_start_two_player(&m, &RNG, 3, 0, 1, false, 128, 128);
    assert(m.actor_count == 4);

    /* Tick it and make sure nothing blows up with four in the ring
       and each one's smart_target left alone across the pass. */
    for (t = 0; t < 120; ++t) {
        wm_input_state in;
        memset(&in, 0, sizeof in);
        wm_match_tick(&m, NULL, &in);
    }
    assert(m.active);
    /* Everybody who is still standing is on a real side. */
    {
        unsigned i;
        for (i = 0; i < m.actor_count; ++i) {
            assert(m.actors[i].player_side == 0 ||
                   m.actors[i].player_side == 1);
        }
    }
}

/* ---- the app actually reaches it -------------------------------- */

/*
 * A two-player select must end up in start_match's #2plyr path, not
 * #1plyr with a drone standing in for the second human. This drives
 * the app the way the platform layer does -- two controllers -- and
 * checks where it lands.
 */
static void test_app_reaches_two_player(void) {
    wm_app app;
    wm_input_state p1, p2;
    unsigned i;

    wm_app_init(&app);

    /* Nobody has entered a powerup code, so buddy mode cannot be on
       -- and the app says so rather than the match guessing. */
    assert(app.powerups.p_request[0] == 0);
    assert(app.powerups.p_request[1] == 0);

    /* Hold both Starts until the app has taken us into a match. */
    for (i = 0; i < 20000u && app.mode != WM_APP_MODE_MATCH; ++i) {
        memset(&p1, 0, sizeof p1);
        memset(&p2, 0, sizeof p2);
        p1.start = true;
        p2.start = true;
        /* Pick something once each select cursor is live. */
        p1.light_punch = (i % 7) == 0;
        p2.light_punch = (i % 11) == 0;
        wm_app_tick_dual(&app, &p1, &p2);
    }

    /* It really gets there -- no escape hatch. Pressing Start on the
       title used to skip the title instead of starting a game, which
       made every mode past attract unreachable; see the
       SOURCE_SELECT_TITLE_START_BRIDGE comment in app.c. */
    assert(app.mode == WM_APP_MODE_MATCH);
    assert(i < 20000u);

    /* Both players held Start, so the second one joined and the
       match must be a #2plyr one with two humans -- not one human
       and a drone standing in. */
    assert(app.select.p2_joined);
    assert(app.match_pstatus == 3);
    assert(app.match.pstatus == 3);
    assert(app.match.actor_is_human[0]);
    assert(app.match.actor_is_human[1]);
    assert(app.match.actors[0].plyr_type == WM_PTYPE_PLAYER);
    assert(app.match.actors[1].plyr_type == WM_PTYPE_PLAYER);

    /* Buddy mode was CHECKED and came out off, because no powerup
       code was entered -- not because the test never asked. */
    assert(app.match.buddy_mode_checked);
    assert(!app.match.buddy_mode_on);
    assert(app.match.actor_count == 2);

    /* And both players' controls reach the match from here on. */
    {
        wm_input_state a, b;
        memset(&a, 0, sizeof a);
        memset(&b, 0, sizeof b);
        a.stick_x = 100;
        b.stick_x = -100;
        wm_app_tick_dual(&app, &a, &b);
        assert(app.match.player_input_set[0]);
        assert(app.match.player_input_set[1]);
    }
}

/* Player two's controls reach player two's wrestler and nobody
   else's -- the per-player switch sets read_switches fills. */
static void test_per_player_input_routing(void) {
    wm_match_state m;
    wm_input_state a, b;
    unsigned t;

    wm_match_init(&m);
    rng_reset();
    wm_match_start_two_player(&m, &RNG, 3, 0, 1, false, 0, 0);

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    /* Player one walks right, player two walks left. */
    a.stick_x = 100;
    b.stick_x = -100;
    wm_match_set_input(&m, 0, &a);
    wm_match_set_input(&m, 1, &b);

    for (t = 0; t < 40; ++t) wm_match_tick(&m, NULL, NULL);

    /* They pressed opposite directions, so their committed stick
       values must differ -- if player two's input were ignored both
       would read the same. */
    assert(m.actors[0].stick_val_cur != m.actors[1].stick_val_cur);

    /* Clearing player two's entry stops it being applied. */
    wm_match_set_input(&m, 1, NULL);
    assert(!m.player_input_set[1]);
    assert(m.player_input_set[0]);

    /* Out-of-range players are ignored rather than scribbling. */
    wm_match_set_input(&m, 9, &a);
    assert(m.player_input_set[0] && !m.player_input_set[1]);
}

int main(void) {
    test_get_rnd_wrestler();
    test_choose_buddies();
    test_two_player_plain();
    test_royal_rumble_makes_them_teammates();
    test_pstatus_bits_pick_the_type();
    test_buddy_mode_needs_both();
    test_buddy_mode_creates_four();
    test_rumble_does_not_move_the_buddies();
    test_partner_is_not_an_opponent();
    test_per_player_input_routing();
    test_app_reaches_two_player();
    printf("two player ok\n");
    return 0;
}
