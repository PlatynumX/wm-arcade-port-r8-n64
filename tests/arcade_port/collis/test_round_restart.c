/*
 * A decided round used to be a dead end: the score moved and nothing
 * else did, so the match kept ticking with two wrestlers frozen where
 * they fell. LIFEBAR.ASM:3098's WRESTLERS_RESET is what should happen
 * instead, and this checks that it does, in a real match.
 */
#include <assert.h>
#include <string.h>

#include "wm/match.h"
#include "wm_arcade_roster.h"
#include "wm_arcade_round_reset.h"

static uint32_t hc(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t spf(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static wm_match_state M;

/*
 * Knock one side out and hold it there until the round is awarded.
 *
 * It watches the ROUND COUNT rather than round_state.decided, because
 * the reset clears `decided` on the same tick it is set -- which is
 * the point of the reset, and is exactly what a caller polling that
 * flag would get wrong. WRESTLE2.ASM:4198's countdown is 5*TSEC, so a
 * KO takes 265 ticks to be called.
 */
static void kill_side(int side, int max_ticks) {
    wm_input_state in;
    int32_t before = M.score.p1rounds + M.score.p2rounds;
    int i;
    memset(&in, 0, sizeof in);
    for (i = 0; i < max_ticks; ++i) {
        unsigned k;
        for (k = 0; k < M.actor_count; ++k)
            if ((int)M.actors[k].player_side == side) {
                M.actors[k].player_mode = (uint16_t)WM_PMODE_DEAD;
                M.actors[k].life = 0;
            }
        wm_match_tick(&M, NULL, &in);
        if (M.score.p1rounds + M.score.p2rounds != before) return;
    }
    assert(0 && "the round was never called");
}

static void test_camera_starts_where_the_source_puts_it(void) {
    WmRng r;
    uint32_t t = 1;
    wm_rng_init(&r, 0x12345678u, hc, spf, &t);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_match_start_selected(&M, &r, (uint8_t)WM_ROSTER_TAKER);

    /* WRESTLE.ASM:6688 init_scroller. It was left at the origin, which
       put the ring off the right of the first frame. */
    assert(M.scroll.worldtlx == (int32_t)(1074 - 200) << 16);
    assert(M.scroll.worldtly == (int32_t)0xffe50000);
}

static void test_round_actually_restarts(void) {
    WmRng r;
    uint32_t t = 1;
    wm_input_state in;
    wm_arcade_actor_t *p1, *p2;
    int i;

    wm_rng_init(&r, 0x2468ACE0u, hc, spf, &t);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_match_start_selected(&M, &r, (uint8_t)WM_ROSTER_TAKER);
    p1 = &M.actors[0];
    p2 = &M.actors[1];
    memset(&in, 0, sizeof in);

    /* Settle, then shove both wrestlers somewhere they should not be
       at the start of a round and dirty the state a round leaves. */
    for (i = 0; i < 4; ++i) wm_match_tick(&M, NULL, &in);
    p1->status_flags |= WM_STATUS_DID_PIN | WM_STATUS_PINNED |
                        WM_STATUS_KOD | WM_STATUS_TEMP_PAL;
    p1->consecutive_hits = 7;
    p1->special_move_addr = 0x1234;
    p1->in_ring = 0;
    p1->life = 3;

    assert(M.score.p1rounds == 0 && M.score.p2rounds == 0);
    assert(M.current_round == 0);

    kill_side(1, 400);                     /* side 1 loses the round */

    assert(M.score.p1rounds == 1);
    assert(M.score.match_winner == 0);     /* best of three, not over */

    /*
     * The reset is OWED on the deciding tick, not done on it -- the
     * source reaches WRESTLERS_RESET from deep inside
     * announce_rnd_winner, long after the round is called, and
     * deciding-tick state has to survive for anything watching it.
     */
    assert(M.round_reset_pending);
    assert(M.round_state.decided);
    assert(M.current_round == 0);
    {
        wm_input_state idle;
        memset(&idle, 0, sizeof idle);
        wm_match_tick(&M, NULL, &idle);
    }
    assert(!M.round_reset_pending);

    /* reset_for_round put them back on their marks. */
    assert(M.current_round == 1);
    assert(p1->x_int == wm_round_team_starts[0][0].x);
    assert(p1->z_int == wm_round_team_starts[0][0].z);
    assert(p1->facing_dir == wm_round_team_starts[0][0].facing);
    assert(p2->x_int == wm_round_team_starts[1][0].x);
    assert(p2->facing_dir == wm_round_team_starts[1][0].facing);
    /* Standing on the mat, not falling to it. */
    assert(p1->y_int == WM_MAT_Y && p1->y_vel == 0);
    /* And back inside the ring -- `clr INRING` means IN. */
    assert(p1->in_ring == 1);
    assert(p1->life == WM_ARCADE_LIFE_MAX);
    /* "Don't allow meters for the first x seconds of round". One tick
       of it has already been spent by the tick the reset landed on. */
    assert(p1->delay_meter >= 14 * 60 - 1 && p1->delay_meter <= 14 * 60);

    /* reset_for_round2 cleared the per-round state and kept exactly
       the two bits SF_RESET_MASK protects. */
    assert(p1->consecutive_hits == 0);
    assert(p1->special_move_addr == 0);
    assert(p1->immobilize_time >= 29 && p1->immobilize_time <= 30);
    assert(!(p1->status_flags & WM_STATUS_DID_PIN));
    assert(!(p1->status_flags & WM_STATUS_PINNED));
    assert(!(p1->status_flags & WM_STATUS_KOD));
    assert(p1->status_flags & WM_STATUS_TEMP_PAL);

    /* The camera went back too. */
    assert(M.scroll.worldtlx == (int32_t)(1074 - 200) << 16);

    /* And the round can be decided again, which is the whole point:
       without the reset, round_state.decided stayed true forever. */
    assert(!M.round_state.decided);
    kill_side(1, 400);
    assert(M.score.p1rounds == 2);
    /* Two rounds is the match (LIFEBAR.ASM's best-of-three). */
    assert(M.score.match_winner == 1);
    /* The match being over stops the reset -- the source goes to its
       own end-of-match path from here, which this port does not have.
       So nothing is owed and the round counter stays put. */
    assert(!M.round_reset_pending);
    {
        wm_input_state idle;
        int k;
        memset(&idle, 0, sizeof idle);
        for (k = 0; k < 10; ++k) wm_match_tick(&M, NULL, &idle);
    }
    assert(M.current_round == 1);
}

/* The loser of a round must be able to fight the next one: a KO'd
   wrestler is woken (PTIME 1) and his KOD bit cleared. */
static void test_loser_comes_back(void) {
    WmRng r;
    uint32_t t = 1;
    wm_arcade_actor_t *p2;

    wm_rng_init(&r, 0x13572468u, hc, spf, &t);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_match_start_selected(&M, &r, (uint8_t)WM_ROSTER_TAKER);
    p2 = &M.actors[1];

    {
        wm_input_state in;
        int i;
        memset(&in, 0, sizeof in);
        for (i = 0; i < 4; ++i) wm_match_tick(&M, NULL, &in);
    }
    p2->status_flags |= WM_STATUS_KOD;

    kill_side(1, 400);
    {
        wm_input_state idle;
        memset(&idle, 0, sizeof idle);
        wm_match_tick(&M, NULL, &idle);    /* the reset lands here */
    }

    assert(!(p2->status_flags & WM_STATUS_KOD));
    assert(p2->ptime == 1);
    assert(p2->player_mode == WM_PMODE_NORMAL);
    assert(p2->life == WM_ARCADE_LIFE_MAX);
}

int main(void) {
    test_camera_starts_where_the_source_puts_it();
    test_round_actually_restarts();
    test_loser_comes_back();
    return 0;
}
