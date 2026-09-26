/*
 * DRONE.ASM:3091 drone_calcskill -- everything that decides how hard a
 * drone plays, in one sum, run once at the start of each round.
 *
 * Worth testing rather than eyeballing because the sum has two
 * asymmetries that read like bugs and are not: the win-streak term is
 * six times a win and only twice a loss, and the source's own comment
 * about the difficulty term describes a default the shipped machine
 * does not use.
 */
#include "wm/arcade/wm_arcade_drone.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* A neutral starting point: round 1 is rerolled, so these tests pin
   skill_rndm by asking for a later round wherever the roll would be
   in the way. */
static wm_arcade_drone_skill_inputs_t base(void)
{
    wm_arcade_drone_skill_inputs_t in;
    memset(&in, 0, sizeof(in));
    in.plyr_type = 1;          /* a drone */
    in.current_round = 2;      /* past the reroll */
    in.ladder_index = 0;
    in.win_streak = 0;
    in.rounds_won = 0;
    in.adj_difficulty = 5;     /* AUDIT.ASM:2923's shipped default */
    return in;
}

static void test_a_human_is_left_alone(void)
{
    wm_arcade_drone_skill_inputs_t in = base();
    int32_t rndm = 99;
    in.plyr_type = 0;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == -1);
    assert(rndm == 99);        /* not even the reroll happens */
    assert(wm_arcade_drone_calcskill(NULL, &rndm, NULL) == -1);
    assert(wm_arcade_drone_calcskill(&in, NULL, NULL) == -1);
}

static void test_the_shipped_default(void)
{
    /*
     * Round 2, bottom of the ladder, no wins, no rounds, ADJDIFF 5:
     *   ladder 0 + 0/2 = 0
     *   + skill_rndm 0
     *   - 2*(2-1)     = -2
     *   + 2*(5-2)     = +6
     * The source's comment on that last term says "+8 default", which
     * would need ADJDIFF 6; AUDIT.ASM:2923 ships 5.
     */
    wm_arcade_drone_skill_inputs_t in = base();
    int32_t rndm = 0;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == 4);

    in.adj_difficulty = 6;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == 6);
}

static void test_the_ladder_and_the_round(void)
{
    wm_arcade_drone_skill_inputs_t in = base();
    int32_t rndm = 0;

    /* `move a5,a1 / sra 1,a1 / add a1,a5` -- ladder + ladder/2, and the
       shift truncates, so 3 contributes 4 and not 4.5. */
    in.ladder_index = 3;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == 4 + 4);
    in.ladder_index = 6;                 /* the top: 6 + 3 = 9 */
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == 4 + 9);

    /* -2 per round past the first. */
    in.ladder_index = 0;
    in.current_round = 3;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == 4 - 2);
}

static void test_the_win_streak_is_not_symmetric(void)
{
    /*
     * `jrlt #loser` jumps PAST the first doubling and its add, so a
     * winner's streak counts six times and a loser's only twice. One
     * win is worth as much as three losses, not one.
     */
    wm_arcade_drone_skill_inputs_t in = base();
    int32_t rndm = 0;
    int neutral;

    neutral = wm_arcade_drone_calcskill(&in, &rndm, NULL);
    assert(neutral == 4);

    in.win_streak = 1;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == neutral + 6);
    in.win_streak = 2;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == neutral + 12);

    in.win_streak = -1;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == neutral - 2);
    in.win_streak = -2;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == neutral - 4);
}

static void test_rounds_won_count_three(void)
{
    /* `add a0,a5 / X2 a0 / add a0,a5` -- r then 2r. */
    wm_arcade_drone_skill_inputs_t in = base();
    int32_t rndm = 0;
    in.rounds_won = 1;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == 4 + 3);
    in.rounds_won = 3;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == 4 + 9);
}

static void test_the_clamps(void)
{
    wm_arcade_drone_skill_inputs_t in = base();
    int32_t rndm = 0;

    /* `jrge #minok / clr a5` -- the floor is zero, not a negative skill. */
    in.adj_difficulty = 1;
    in.current_round = 3;
    in.win_streak = -10;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == 0);

    /* `cmpi 29,a5 / jrle #maxok / movk 29,a5`. */
    in = base();
    in.ladder_index = 6;
    in.win_streak = 9;
    in.rounds_won = 5;
    in.adj_difficulty = 10;
    assert(wm_arcade_drone_calcskill(&in, &rndm, NULL) == 29);
}

static void test_the_handicap_is_rolled_once_a_match(void)
{
    /*
     * `move @current_round,a3 / subk 1,a3 / jrgt #n1st`: the roll
     * happens on the first round and the value is carried for the rest
     * of the match, so a drone does not get a fresh handicap every
     * round.
     */
    wm_arcade_drone_skill_inputs_t in = base();
    WmRng rng;
    int32_t rndm = 7;           /* an impossible carried value */
    int32_t first;

    wm_rng_init(&rng, 0x13572468u, NULL, NULL, NULL);
    wm_rng_set_latched_inputs(&rng, 0x1234u, 0x5678u);

    in.current_round = 1;
    (void)wm_arcade_drone_calcskill(&in, &rndm, &rng);
    first = rndm;
    assert(first >= -2 && first <= 2);   /* RNDRNG0(4) - 2 */

    in.current_round = 2;
    (void)wm_arcade_drone_calcskill(&in, &rndm, &rng);
    assert(rndm == first);
    in.current_round = 3;
    (void)wm_arcade_drone_calcskill(&in, &rndm, &rng);
    assert(rndm == first);

    /* And it really does move the answer. */
    rndm = 2;
    in.current_round = 2;
    assert(wm_arcade_drone_calcskill(&in, &rndm, &rng) == 6);
    rndm = -2;
    assert(wm_arcade_drone_calcskill(&in, &rndm, &rng) == 2);
}

int main(void)
{
    test_a_human_is_left_alone();
    test_the_shipped_default();
    test_the_ladder_and_the_round();
    test_the_win_streak_is_not_symmetric();
    test_rounds_won_count_three();
    test_the_clamps();
    test_the_handicap_is_rolled_once_a_match();
    printf("drone_calcskill: all checks passed\n");
    return 0;
}
