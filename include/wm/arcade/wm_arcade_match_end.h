#ifndef WM_ARCADE_MATCH_END_H
#define WM_ARCADE_MATCH_END_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_round.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * LIFEBAR.ASM:2852 `DO_WAIT` -- what happens once somebody has two
 * rounds.
 *
 * The same dead end the between-round reset just fixed, one level up:
 * wm_arcade_match_score_award_round sets match_winner and nothing
 * takes over, so a finished match keeps ticking with two wrestlers
 * standing in a ring that has already been won.
 *
 * The sequence lives inside announce_rnd_winner, below the point
 * WRESTLERS_RESET is reached on the between-rounds path, and it is
 * mostly presentation -- three end-of-round graphics, the award bar,
 * the music, a tip screen. What it DECIDES is here; the drawing is
 * the caller's and is listed against each phase below so nothing goes
 * missing quietly.
 */

/* `move @p1rounds,a0 / cmpi 2,a0 / jrz DO_WAIT` and the same for p2.
   Two rounds is a match; LIFEBAR.ASM has no other threshold. */
bool wm_match_end_reached(const wm_arcade_match_score_t *score);
#define WM_MATCH_ROUNDS_TO_WIN 2

/* ---- increment_wincount (WRESTLE.ASM:1445) ----------------------- */

/*
 * The win-STREAK bookkeeping, which is not the round score and was
 * ledgered as if it were. Four things, in order:
 *
 *   The current streaks are copied to p1oldwinstreak / p2oldwinstreak
 *   FIRST. That pair is what WRESTLE2.ASM's loser_snd reads to decide
 *   whether a streak has just been broken, so the copy has to happen
 *   before the increment or the sound never plays.
 *
 *   `move @match_winner,a0 / move @PSTATUS,a14 / and a14,a0` -- the
 *   winner ANDed with who is actually playing. A player who is not in
 *   the game cannot have won, whatever match_winner says.
 *
 *   Each side's streak is then incremented if its bit survived that
 *   AND, and CLEARED to zero if it did not. Note the increment is
 *   computed before the test and thrown away on a loss, which is why
 *   a loss resets to 0 rather than decrementing.
 *
 *   arm_winstreak_award is called with the NEW value either way
 *   (AWARD.ASM:1121: it arms only on a non-zero multiple of five, and
 *   disarms otherwise), and a streak that came out zero also clears
 *   that player's winstreak award rows.
 */
typedef struct {
    int32_t p1winstreak;
    int32_t p2winstreak;
    /* Written by this routine, read by loser_snd. */
    int32_t p1oldwinstreak;
    int32_t p2oldwinstreak;
} wm_match_streaks_t;

/*
 * `match_winner` is LIFEBAR.ASM's own 1-or-2 value and `pstatus` is
 * the player bitmask. Returns the bitmask of players whose streak was
 * CLEARED, so the caller can reset their award rows
 * (rst_winstreak_awards) without re-deriving it.
 */
unsigned wm_match_increment_wincount(wm_match_streaks_t *st,
                                     int32_t match_winner, int32_t pstatus);

/* ---- the end-of-round graphic index (LIFEBAR.ASM:2872) ----------- */

/*
 * `#won_match` / `#won_fall`: which of the three end-of-round graphics
 * CREATE_END_ROUND_TOP and friends draw.
 *
 * 2 when either side has two rounds -- the match is over. Otherwise
 * p1rounds + p2rounds - 1, so the first round played gives 0 and the
 * second gives 1. It is the round NUMBER, one-based and then made
 * zero-based, not the winner.
 */
int wm_match_end_round_index(const wm_arcade_match_score_t *score);

/* ---- the tip rule (LIFEBAR.ASM:2952) ----------------------------- */

/*
 * "Tips every 25 consecutive wins" and "Tips every 100 matches".
 *
 * Read carefully, because the structure is not the obvious one: the
 * routine checks p1winstreak and falls through to p2winstreak only if
 * p1's is ZERO. So with both players on a streak, only p1's is
 * tested; and if the one it tests is not a multiple of 25 it goes to
 * the match count anyway (`jrz #do_tip` falls through to
 * `#ck_mtch_num`). `modu` is an unsigned remainder, and a streak of 0
 * never reaches the test at all.
 */
bool wm_match_tip_due(int32_t p1winstreak, int32_t p2winstreak,
                      int32_t match_cnt);
#define WM_MATCH_TIP_WINS 25
#define WM_MATCH_TIP_MATCHES 100

/* ---- DO_RIGHT_MUSIC (LIFEBAR.ASM:2996) --------------------------- */

/*
 * `#wrestler_tunes .word 5,2,1,7,6,4,8,0,3`, indexed by the WINNER's
 * WRESTLERNUM -- each wrestler has his own theme. Slot 7 is Adam
 * Bomb, cut from the game, and carries 0; that is the table's real
 * content, not a gap.
 *
 * DO_RIGHT_MUSIC sleeps 55 first and DO_RIGHT_MUSIC2 does not; they
 * are the same routine entered one line apart, which is why the
 * second exists at all.
 */
int wm_match_wrestler_tune(int32_t wrestler_num);
#define WM_MATCH_MUSIC_SLEEP 55

/* ---- the sequence ------------------------------------------------ */

/*
 * The phases, in LIFEBAR.ASM's order. Each names what the source does
 * in it, so a caller can see what it is not drawing.
 */
typedef enum {
    WM_MEND_IDLE = 0,
    /* `SLEEPK 20` after CALL_MATCH_OVER, then the two-round test. */
    WM_MEND_SETTLE,
    /* DO_WAIT: MUSIC_HAP cleared, increment_wincount, replace_wins
       (the win-count text, not translated), then a one-second wait
       that any button press cuts short. */
    WM_MEND_WINCOUNT,
    /* The 0c4h sound and the three CREATE_END_ROUND_* graphics, 15
       and 10 and 20 ticks apart. */
    WM_MEND_GRAPHICS,
    /* is_it_a_really_quick_win, give_award_if_opponent_is_human,
       check_for_award_for_big_comeback, accumulate_awards,
       CLEAR_SPEECH_REPEAT -- all five already translated, run here in
       the source's order. */
    WM_MEND_AWARDS,
    /* check_for_award_for_winstreak, the award bar
       (create_end_rnd_awards) and the wait for it to finish, then a
       three-second wait a press cuts short. */
    WM_MEND_AWARD_BAR,
    /* The tip check and DO_RIGHT_MUSIC2. */
    WM_MEND_TIP,
    /* `MOVK 2,A0 / move a0,@match_over`, KILL_ALL_CHANNELS, the 4Dh
       sound, HALT cleared, DIE. */
    WM_MEND_DONE
} wm_match_end_phase_t;

typedef struct {
    uint8_t phase;
    int32_t sleep_left;
    /* The graphic index #won_fall computed, held for the caller. */
    int round_index;
    /* @match_over. The source stores the literal 2. */
    int32_t match_over;
    /* Whether the tip check came out true this match. */
    bool tip_due;
    /* The winner's theme, or -1 before WM_MEND_TIP. */
    int tune;
} wm_match_end_t;

void wm_match_end_init(wm_match_end_t *st);

/* `SLEEPK 20` then DO_WAIT. Starts the sequence; the caller should
   only do this once match_winner is set. */
void wm_match_end_start(wm_match_end_t *st);

typedef struct {
    const wm_arcade_match_score_t *score;
    wm_match_streaks_t *streaks;
    int32_t pstatus;
    int32_t match_cnt;
    int32_t winner_wrestler_num;
    /* Whether the caller's award bar has finished
       (@award_ok_to_die >= 3). True when there is none to wait for. */
    bool awards_done;
    void *user;
    /* The five award routines, in the order the source calls them.
       Any may be NULL. */
    void (*run_awards)(void *user);
    void (*run_winstreak_award)(void *user);
    /* rst_winstreak_awards for each player whose streak was cleared. */
    void (*reset_winstreak_rows)(void *user, unsigned player_mask);
    void (*sound)(void *user, int sound);
} wm_match_end_ctx_t;

/* One tick. Returns true on the tick the match finishes (the phase
   reaching WM_MEND_DONE), which is where @match_over becomes 2. */
bool wm_match_end_tick(wm_match_end_t *st, const wm_match_end_ctx_t *ctx);

/* The source's own sleeps, in ticks. */
#define WM_MEND_SETTLE_TICKS 20      /* SLEEPK 20 after CALL_MATCH_OVER */
#define WM_MEND_WINCOUNT_WAIT 53     /* `movi TSEC,a9` at #wl0 */
#define WM_MEND_TOP_TICKS 15         /* SLEEPK 15 */
#define WM_MEND_BOT_TICKS 10         /* SLEEPK 10 */
#define WM_MEND_ICON_TICKS 20        /* SLEEPK 20 */
#define WM_MEND_AWARD_WAIT (53 * 3)  /* `movi TSEC*3,a9` at #wl1 */
/* The two sounds it makes itself. */
#define WM_MEND_ROUND_SOUND 0xc4
#define WM_MEND_FINAL_SOUND 0x4d

#ifdef __cplusplus
}
#endif
#endif
