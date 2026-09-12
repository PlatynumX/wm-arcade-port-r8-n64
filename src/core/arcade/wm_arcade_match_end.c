/*
 * LIFEBAR.ASM's end-of-match path -- see
 * wm/arcade/wm_arcade_match_end.h.
 */
#include "wm/arcade/wm_arcade_match_end.h"

#include <stddef.h>
#include <string.h>

bool wm_match_end_reached(const wm_arcade_match_score_t *score) {
    if (!score) return false;
    return score->p1rounds >= WM_MATCH_ROUNDS_TO_WIN ||
           score->p2rounds >= WM_MATCH_ROUNDS_TO_WIN;
}

unsigned wm_match_increment_wincount(wm_match_streaks_t *st,
                                     int32_t match_winner, int32_t pstatus) {
    int32_t won;
    unsigned cleared = 0;

    if (!st) return 0;

    /* ";save old streaks" -- before anything else, because loser_snd
       reads this pair to tell a broken streak from no streak. */
    st->p1oldwinstreak = st->p1winstreak;
    st->p2oldwinstreak = st->p2winstreak;

    /* `move @match_winner,a0 / move @PSTATUS,a14 / and a14,a0`. */
    won = match_winner & pstatus;

    /* `inc a1 / btst 0,a0 / jrnz #p1ok / clr a1` -- the increment is
       computed first and discarded on a loss, so losing resets to
       zero rather than stepping back. */
    st->p1winstreak = (won & 1) ? st->p1winstreak + 1 : 0;
    if (st->p1winstreak == 0) cleared |= 1u;

    st->p2winstreak = (won & 2) ? st->p2winstreak + 1 : 0;
    if (st->p2winstreak == 0) cleared |= 2u;

    return cleared;
}

int wm_match_end_round_index(const wm_arcade_match_score_t *score) {
    if (!score) return 0;
    /* `#won_match: movk 2,a9` when either side has two. */
    if (score->p1rounds == WM_MATCH_ROUNDS_TO_WIN ||
        score->p2rounds == WM_MATCH_ROUNDS_TO_WIN)
        return 2;
    /* `add a14,a9 / dec a9` -- the rounds played so far, less one. */
    return (int)(score->p1rounds + score->p2rounds) - 1;
}

bool wm_match_tip_due(int32_t p1winstreak, int32_t p2winstreak,
                      int32_t match_cnt) {
    int32_t streak;

    /*
     * `move @p1winstreak,a1 / jrnz #tip_chk` then `#ck_p2: move
     * @p2winstreak,a1 / jrz #ck_mtch_num`. p2's streak is only ever
     * looked at when p1's is zero, so two players on streaks means
     * only p1's is tested.
     */
    streak = p1winstreak ? p1winstreak : p2winstreak;
    if (streak != 0) {
        /* `modu a0,a1 / jrz #do_tip` -- unsigned remainder. */
        if (((uint32_t)streak % (uint32_t)WM_MATCH_TIP_WINS) == 0)
            return true;
        /* ...and a miss falls THROUGH to the match count rather than
           giving up, which is the easy thing to read wrong. */
    }
    return ((uint32_t)match_cnt % (uint32_t)WM_MATCH_TIP_MATCHES) == 0;
}

int wm_match_wrestler_tune(int32_t wrestler_num) {
    /* `#wrestler_tunes .word 5,2,1,7,6,4,8,0,3`. Slot 7 is Adam Bomb,
       cut, and its 0 is the table's own content. */
    static const int TUNES[9] = { 5, 2, 1, 7, 6, 4, 8, 0, 3 };
    if (wrestler_num < 0 || wrestler_num >= 9) return 0;
    return TUNES[wrestler_num];
}

void wm_match_end_init(wm_match_end_t *st) {
    if (!st) return;
    memset(st, 0, sizeof(*st));
    st->phase = (uint8_t)WM_MEND_IDLE;
    st->tune = -1;
}

void wm_match_end_start(wm_match_end_t *st) {
    if (!st || st->phase != (uint8_t)WM_MEND_IDLE) return;
    st->phase = (uint8_t)WM_MEND_SETTLE;
    st->sleep_left = WM_MEND_SETTLE_TICKS;
}

bool wm_match_end_tick(wm_match_end_t *st, const wm_match_end_ctx_t *ctx) {
    if (!st || !ctx) return false;
    if (st->phase == (uint8_t)WM_MEND_IDLE ||
        st->phase == (uint8_t)WM_MEND_DONE)
        return false;

    if (st->sleep_left > 0) { st->sleep_left -= 1; return false; }

    switch ((wm_match_end_phase_t)st->phase) {
    case WM_MEND_SETTLE:
        /* DO_WAIT is reached only when somebody has two rounds; the
           other path went to #go0 and the between-round reset. */
        if (!wm_match_end_reached(ctx->score)) {
            st->phase = (uint8_t)WM_MEND_IDLE;
            return false;
        }
        st->phase = (uint8_t)WM_MEND_WINCOUNT;
        st->sleep_left = 0;
        break;

    case WM_MEND_WINCOUNT: {
        unsigned cleared = 0;
        if (ctx->streaks)
            cleared = wm_match_increment_wincount(
                ctx->streaks,
                ctx->score ? ctx->score->match_winner : 0, ctx->pstatus);
        if (cleared && ctx->reset_winstreak_rows)
            ctx->reset_winstreak_rows(ctx->user, cleared);
        /* replace_wins draws the win-count text here; not translated. */
        st->round_index = wm_match_end_round_index(ctx->score);
        st->phase = (uint8_t)WM_MEND_GRAPHICS;
        /* "wait 1 sec or until a press" -- the press half belongs to
           the caller, which owns the switch latch. */
        st->sleep_left = WM_MEND_WINCOUNT_WAIT;
        break;
    }

    case WM_MEND_GRAPHICS:
        /* `movi 0c4h,a0 / calla triple_sound`, then the three
           CREATE_END_ROUND_* processes 15, 10 and 20 ticks apart. The
           graphics are the caller's; the sound and the timing are
           here. */
        if (ctx->sound) ctx->sound(ctx->user, WM_MEND_ROUND_SOUND);
        st->phase = (uint8_t)WM_MEND_AWARDS;
        st->sleep_left = WM_MEND_TOP_TICKS + WM_MEND_BOT_TICKS +
                         WM_MEND_ICON_TICKS;
        break;

    case WM_MEND_AWARDS:
        /* The four award routines and CLEAR_SPEECH_REPEAT, all in the
           source's order, all already translated. */
        if (ctx->run_awards) ctx->run_awards(ctx->user);
        st->phase = (uint8_t)WM_MEND_AWARD_BAR;
        st->sleep_left = 0;
        break;

    case WM_MEND_AWARD_BAR:
        /* `#end`: check_for_award_for_winstreak, then the award bar,
           then `#wait_awards_dead` -- a poll on @award_ok_to_die
           reaching 3, which the caller answers. */
        if (ctx->run_winstreak_award) ctx->run_winstreak_award(ctx->user);
        if (!ctx->awards_done) return false;      /* still polling */
        st->phase = (uint8_t)WM_MEND_TIP;
        st->sleep_left = WM_MEND_AWARD_WAIT;
        break;

    case WM_MEND_TIP:
        st->tip_due = ctx->streaks
            ? wm_match_tip_due(ctx->streaks->p1winstreak,
                               ctx->streaks->p2winstreak, ctx->match_cnt)
            : false;
        st->tune = wm_match_wrestler_tune(ctx->winner_wrestler_num);
        /* `MOVK 2,A0 / move a0,@match_over`, the 4Dh sound, and HALT
           cleared. */
        st->match_over = 2;
        if (ctx->sound) ctx->sound(ctx->user, WM_MEND_FINAL_SOUND);
        st->phase = (uint8_t)WM_MEND_DONE;
        return true;

    case WM_MEND_IDLE:
    case WM_MEND_DONE:
        break;
    }
    return false;
}
