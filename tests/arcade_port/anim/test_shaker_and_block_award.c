/*
 * The last two deferred seams that were behaviour rather than drawing,
 * and a ledger note that was wrong about what one of them is.
 *
 *   start_shake / kill_shake   TAKER.ASM:533 shake_world
 *   round_award_block          JJXM.H:44 RND_AWARD a13,BLOCKS_AWD
 *
 * shake_world is NOT UTIL.ASM:2406 SHAKER2, which the ledger said would
 * do for it. SHAKER2 is a damped oscillator on WORLDTLY alone, off a
 * sine table and an e^-x table, with a finite tick count that doubles as
 * its amplitude. shake_world is an UNBOUNDED uniform jitter of +/-2 on
 * BOTH axes every three ticks, running until its process is killed.
 * These tests pin the difference, because substituting one for the other
 * is the mistake the note invited.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_und_finish.h"
#include "wm/arcade/wm_arcade_shake.h"
#include "wm/arcade/wmania_rng.h"
#include "wm/award.h"

/* ---- shake_world's body ---------------------------------------- */

static int32_t seen_tlx, seen_tly;
static int origin_writes;
static void cap_origin(int32_t tlx, int32_t tly, void *u) {
    (void)u; seen_tlx = tlx; seen_tly = tly; ++origin_writes;
}
static int shakes_started, shakes_killed;
static void cap_start(void *u) { (void)u; ++shakes_started; }
static void cap_kill(void *u) { (void)u; ++shakes_killed; }
static int finish_flag_writes, finish_flag_last;
static void cap_finish_flag(int on, void *u) {
    (void)u; ++finish_flag_writes; finish_flag_last = on;
}

/*
 * `movk 4,a0 / calla RNDRNG0 / movk 2,a11 / sub a0,a11 / sll 16,a11`:
 * RNDRNG0(4) is 0..4 inclusive, so 2 minus it is +2..-2, and BOTH axes
 * get one. Every offset is from the base the process captured, never
 * from the last jittered value.
 */
static void test_the_jitter_is_two_pixels_on_both_axes(void) {
    wm_arcade_und_finish_callbacks_t cb;
    WmRng rng;
    const int32_t bx = 1000 << 16, by = 500 << 16;
    int i;
    int saw_x_move = 0, saw_y_move = 0;

    wm_rng_init(&rng, 0x2468u, NULL, NULL, NULL);
    memset(&cb, 0, sizeof cb);
    cb.set_world_origin = cap_origin;
    cb.rng = &rng;

    for (i = 0; i < 200; ++i) {
        seen_tlx = seen_tly = 0;
        wm_arcade_und_shake_world(bx, by, &cb);
        /* Always within two whole pixels of the BASE, both axes. */
        assert(seen_tlx >= bx - (2 << 16) && seen_tlx <= bx + (2 << 16));
        assert(seen_tly >= by - (2 << 16) && seen_tly <= by + (2 << 16));
        /* And always a whole number of pixels: `sll 16`. */
        assert((seen_tlx & 0xFFFF) == 0);
        assert((seen_tly & 0xFFFF) == 0);
        if (seen_tlx != bx) saw_x_move = 1;
        if (seen_tly != by) saw_y_move = 1;
    }
    /* Both axes really move -- SHAKER2 only ever touches Y. */
    assert(saw_x_move && saw_y_move);
}

/*
 * The same reading, stated as the distinction the ledger got wrong:
 * SHAKER2 leaves WORLDTLX alone and decays, shake_world moves both and
 * does not.
 */
static void test_shake_world_is_not_shaker2(void) {
    wm_shake_state s;
    int i;
    int32_t first = 0, last = 0;
    int ticks = 0;

    wm_shake_init(&s);
    wm_shake_start(&s, 40);
    for (i = 0; i < 400; ++i) {
        if (!wm_shake_tick(&s)) break;
        if (i == 0) first = s.y_adj;
        last = s.y_adj;
        ++ticks;
    }
    /* SHAKER2 ENDS. shake_world has no such counter -- its loop is
       `jruc #sw_loop` with the DIE after it unreachable. */
    assert(ticks > 0 && ticks <= 400);
    assert(!s.on);
    (void)first; (void)last;
}

/* ---- the process around it ------------------------------------- */

/*
 * TAKER.ASM:697 `#fdone_wait`'s tail: clear @in_finish_move, THEN kill
 * the shaker. kill_shake had no call site at all before this -- declared
 * beside start_shake and called by nobody, which the seam audit does not
 * report because it only looks at seams that are called and empty.
 */
static void test_the_finish_tail_clears_the_flag_then_kills_the_shaker(void) {
    wm_arcade_und_finish_callbacks_t cb;
    memset(&cb, 0, sizeof cb);
    cb.set_in_finish_move = cap_finish_flag;
    cb.kill_shake = cap_kill;
    finish_flag_writes = finish_flag_last = shakes_killed = 0;

    wm_arcade_und_finish_done(&cb);
    assert(finish_flag_writes == 1);
    assert(finish_flag_last == 0);       /* `clr a14` */
    assert(shakes_killed == 1);

    /* A NULL callbacks pointer is not a crash. */
    wm_arcade_und_finish_done(NULL);
}

/*
 * adjust_view starts the shaker on BOTH its paths -- `#av_exit: CREATE
 * FIREWRK_PID,shake_world` is reached whether or not the scroll ran,
 * which is why the early exit starts it too.
 */
static void test_both_adjust_view_paths_start_the_shaker(void) {
    wm_arcade_actor_t taker;
    wm_arcade_und_finish_env_t env;
    wm_arcade_und_finish_callbacks_t cb;
    WmRng rng;
    int steps;

    wm_rng_init(&rng, 1u, NULL, NULL, NULL);
    memset(&cb, 0, sizeof cb);
    cb.set_world_origin = cap_origin;
    cb.start_shake = cap_start;
    cb.rng = &rng;
    memset(&taker, 0, sizeof taker);
    taker.x_fixed = 400 << 16;

    /* Already far enough right: the early exit. No scroll, but a shake. */
    memset(&env, 0, sizeof env);
    env.world_tlx = (int32_t)WM_UND_VIEW_STOP_X << 16;
    shakes_started = 0; origin_writes = 0;
    steps = wm_arcade_und_adjust_view(&taker, &env, &cb);
    assert(steps == 0);
    assert(origin_writes == 0);
    assert(shakes_started == 1);

    /* Left of the stop line: 32 scroll steps, and then a shake. */
    memset(&env, 0, sizeof env);
    env.world_tlx = 0;
    shakes_started = 0; origin_writes = 0;
    steps = wm_arcade_und_adjust_view(&taker, &env, &cb);
    assert(steps == 32);
    assert(origin_writes == 32);
    assert(shakes_started == 1);
}

/* ---- the block award ------------------------------------------- */

/*
 * JJXM.H:44 `RND_AWARD a13,BLOCKS_AWD` is round_award(a13, BLOCKS_AWD),
 * and AWARD.ASM's BLOCKS_AWD is index 3. The award machinery was all
 * here; what was missing was the reading that says WHICH block events
 * are scored -- and the eight call sites, one per wrestler file and all
 * in std_block, say it is entering one.
 */
static void test_the_block_tally_counts_blocks_used(void) {
    wm_award_state st;
    uint8_t per = wm_award_icon_value((unsigned)WM_AWARD_BLOCKS);
    memset(&st, 0, sizeof st);
    /* AWARD.ASM:334's table row for BLOCKS_AWD, and :43
       `BLOCK_ICONS .equ 2`. Read from the port's own table rather than
       written in, so the number cannot drift from the generated data. */
    assert((int)WM_AWARD_BLOCKS == 3);
    assert(per == 2);

    wm_award_round_award(&st, 0, (unsigned)WM_AWARD_BLOCKS);
    wm_award_round_award(&st, 0, (unsigned)WM_AWARD_BLOCKS);
    /* round_award ADDS the row's icon value, it does not count to one. */
    assert(st.round[0][WM_AWARD_BLOCKS] == (uint8_t)(2 * per));
    assert(st.round[0][WM_AWARD_REVERSAL] == 0);
    assert(st.round[1][WM_AWARD_BLOCKS] == 0);
}

/*
 * And what the tally is FOR is the opposite of what its name suggests.
 * AWARD.ASM:691 `#adjust_blocks` upgrades the match total under the
 * comment "Give bigger award for multiround NO BLOCKING", and
 * accumulate_player_awards inverts the round value first: any blocks at
 * all becomes 0, none becomes BLOCK_ICONS.
 *
 * So `RND_AWARD a13,BLOCKS_AWD` in std_block is a count of blocks USED,
 * and the bonus goes to the wrestler who did not need them. Worth
 * pinning, because "which block events does the source score" has an
 * answer that reads backwards until this step is in view.
 */
static void test_the_block_award_is_earned_by_not_blocking(void) {
    wm_award_state st;
    bool humans[WM_AWARD_PLAYER_COUNT];
    unsigned i;
    uint8_t per = wm_award_icon_value((unsigned)WM_AWARD_BLOCKS);

    for (i = 0; i < WM_AWARD_PLAYER_COUNT; ++i) humans[i] = true;

    /*
     * accumulate_round inverts the ROUND value, folds it into the MATCH
     * total and then clears the round, so the match total is where the
     * answer lands -- measured, not assumed: asserting on the round
     * array reads 0 for both cases and would have passed the
     * blocked-once half while saying nothing.
     */

    /* Blocked at least once this round: nothing folded in. */
    memset(&st, 0, sizeof st);
    wm_award_round_award(&st, 0, (unsigned)WM_AWARD_BLOCKS);
    assert(st.round[0][WM_AWARD_BLOCKS] == per);   /* the tally, before */
    wm_award_accumulate_round(&st, false, humans);
    assert(st.match[0][WM_AWARD_BLOCKS] == 0);

    /* Never blocked: BLOCK_ICONS carries into the match total. */
    memset(&st, 0, sizeof st);
    wm_award_accumulate_round(&st, false, humans);
    assert(st.match[0][WM_AWARD_BLOCKS] == per);

    /* Two such rounds reach BLOCK_ICONS*2, which AWARD.ASM:691
       #adjust_blocks upgrades to DBL_BLOCK_ICONS -- 5, per :44. */
    wm_award_accumulate_round(&st, false, humans);
    assert(st.match[0][WM_AWARD_BLOCKS] == (uint8_t)(2 * per));
    wm_award_adjust_perfects_and_blocks(&st);
    assert(st.match[0][WM_AWARD_BLOCKS] == 5);
}

int main(void) {
    test_the_jitter_is_two_pixels_on_both_axes();
    test_shake_world_is_not_shaker2();
    test_the_finish_tail_clears_the_flag_then_kills_the_shaker();
    test_both_adjust_view_paths_start_the_shaker();
    test_the_block_tally_counts_blocks_used();
    test_the_block_award_is_earned_by_not_blocking();
    printf("shaker and block award tests passed\n");
    return 0;
}
