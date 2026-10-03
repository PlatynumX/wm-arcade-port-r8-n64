/*
 * WRESTLE.ASM:1726 #ndrone -- the real drone team.
 *
 * The ladder's later rungs are not one-on-one: rung 4 and 5 are
 * two-on-one and rung 6, the final battle, is three-on-one. This
 * port drew a single random opponent for every one of them.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/app.h"
#include "wm/match.h"
#include "wm/pregame.h"

static uint32_t hc(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t spf(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static WmRng RNG;
static uint32_t RNG_T;
static void rng_reset(void) {
    RNG_T = 1;
    memset(&RNG, 0, sizeof RNG);
    wm_rng_init(&RNG, 0x2468ACE0u, hc, spf, &RNG_T);
}

/*
 * The ladder really does escalate. If this ever comes back all-ones
 * the team code below is testing nothing.
 */
static void test_the_ladder_has_teams(void) {
    wm_pregame_state s;
    unsigned counts[7];
    int i;

    rng_reset();
    wm_pregame_init(&s, 6, 0, &RNG);
    for (i = 0; i < 7; ++i)
        counts[i] = wm_pregame_num_of_opps(s.ladder[i].packed);

    /* Four singles, two doubles, then the three-on-one final. */
    for (i = 0; i < 4; ++i) assert(counts[i] == 1);
    assert(counts[4] == 2);
    assert(counts[5] == 2);
    assert(counts[6] == 3);

    /* Three opponents plus the human is exactly the actor cap. */
    assert(counts[6] + 1u == WM_MATCH_MAX_ACTORS);
}

/* One opponent behaves exactly as the old single-opponent path. */
static void test_one_opponent(void) {
    wm_match_state m;
    const uint8_t opps[1] = { 4 };

    wm_match_init(&m);
    rng_reset();
    wm_match_start_one_player_team(&m, &RNG, 1, 6, 0, opps, 1);

    assert(m.actor_count == 2);
    assert(m.actors[0].wrestler_num == 6);
    assert(m.actors[0].player_num == 0);
    assert(m.actors[0].player_side == 0);
    assert(m.actors[0].plyr_type == WM_PTYPE_PLAYER);
    assert(m.actors[1].wrestler_num == 4);
    /* `MOVK 2,A10` -- drone PLYRNUMs start at two, not one. */
    assert(m.actors[1].player_num == 2);
    assert(m.actors[1].player_side == 1);
    assert(m.actors[1].plyr_type == WM_PTYPE_DRONE);
    assert(m.opponent_wrestler == 4);
}

/* Three-on-one: the final battle, which used to be one-on-one. */
static void test_three_on_one(void) {
    wm_match_state m;
    const uint8_t opps[3] = { 3, 5, 6 };
    unsigned i;

    wm_match_init(&m);
    rng_reset();
    wm_match_start_one_player_team(&m, &RNG, 1, 0, 0, opps, 3);

    assert(m.actor_count == 4);
    /* The human alone on side 0, all three drones on side 1. */
    assert(m.actors[0].player_side == 0);
    assert(m.actors[0].plyr_type == WM_PTYPE_PLAYER);
    for (i = 1; i < 4; ++i) {
        assert(m.actors[i].player_side == 1);
        assert(m.actors[i].plyr_type == WM_PTYPE_DRONE);
        /* PLYRNUM counts up from two. */
        assert(m.actors[i].player_num == (int32_t)(1 + i));
        assert(m.actors[i].life == 163);
    }
    /* In the ladder's own order. */
    assert(m.actors[1].wrestler_num == 3);
    assert(m.actors[2].wrestler_num == 5);
    assert(m.actors[3].wrestler_num == 6);

    /* #set0 rows: each drone is the Nth on its side, so they take
       #team2_starts rows 0, 1, 2 -- not three men in one spot. */
    assert(m.actors[1].x_int == 1074 + 85);
    assert(m.actors[2].x_int == 1074 + 150);
    assert(m.actors[3].x_int == 1074 + 20);
    assert(m.actors[1].x_int != m.actors[2].x_int);
    assert(m.actors[2].x_int != m.actors[3].x_int);
}

/* `btst 0,a14` -- the drones take the side opposite the human. */
static void test_drones_oppose_the_human(void) {
    wm_match_state m;
    const uint8_t opps[2] = { 1, 6 };

    wm_match_init(&m);
    rng_reset();
    wm_match_start_one_player_team(&m, &RNG, 2, 0, 0, opps, 2);   /* human is p2 */

    assert(m.actors[0].player_side == 1);
    assert(m.actors[1].player_side == 0);
    assert(m.actors[2].player_side == 0);
}

/* A team match must still tick, and a drone must never be handed a
   team-mate as its opponent. */
static void test_a_team_match_runs(void) {
    wm_match_state m;
    const uint8_t opps[3] = { 3, 5, 6 };
    unsigned t, i;

    wm_match_init(&m);
    rng_reset();
    wm_match_start_one_player_team(&m, &RNG, 1, 0, 0, opps, 3);

    for (t = 0; t < 200; ++t) wm_match_tick(&m, NULL, NULL);
    assert(m.active);

    /* Every drone's chosen target is the human, because he is the
       only wrestler on the other side. */
    for (i = 1; i < m.actor_count; ++i) {
        if (m.actors[i].smart_target)
            assert(m.actors[i].smart_target->player_side !=
                   m.actors[i].player_side);
    }
}

/* The app hands the ladder over rather than drawing. */
static void test_app_uses_the_ladder(void) {
    wm_app app;
    wm_input_state p1;
    unsigned i;

    wm_app_init(&app);
    for (i = 0; i < 20000u && app.mode != WM_APP_MODE_MATCH; ++i) {
        memset(&p1, 0, sizeof p1);
        p1.start = true;
        p1.light_punch = (i % 7) == 0;
        wm_app_tick_dual(&app, &p1, NULL);
    }
    assert(app.mode == WM_APP_MODE_MATCH);

    /* One human plus however many the rung calls for. */
    assert(app.match.actor_count == 1u + app.pregame.opponent_count);
    /* And the opponent is the ladder's, not a draw. */
    assert(app.match.actors[1].wrestler_num ==
           (int32_t)wm_pregame_opponent_at(&app.pregame, 0));
}

int main(void) {
    test_the_ladder_has_teams();
    test_one_opponent();
    test_three_on_one();
    test_drones_oppose_the_human();
    test_a_team_match_runs();
    test_app_uses_the_ladder();
    printf("ladder team ok\n");
    return 0;
}
