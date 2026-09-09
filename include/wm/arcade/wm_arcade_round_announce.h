#ifndef WM_ARCADE_ROUND_ANNOUNCE_H
#define WM_ARCADE_ROUND_ANNOUNCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_round.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DNKSEQ2.ASM:5202 win_announce and the LIFEBAR.ASM process it starts.
 *
 * win_announce is what a pin animation calls the instant the pin sticks,
 * and it is five instructions: CREATE an ANNC_PID running
 * announce_rnd_winner, and KILL_PIN_HIM. Everything interesting is in the
 * process, which is where a round actually ends when somebody is pinned
 * rather than when the pin-idiot-check countdown in
 * wm/arcade/wm_arcade_round.h runs out.
 *
 * What is translated here is that process down to the round award and the
 * end-of-match announcement, plus the two buckoff helpers it uses:
 *
 *   LIFEBAR.ASM:2642  announce_rnd_winner  (the round-ending core)
 *   LIFEBAR.ASM:3603  anyone_bucking
 *   LIFEBAR.ASM:3630  set_all_buckoffs
 *   LIFEBAR.ASM:5063  set_winner           (the winner search, KO branch)
 *   DCSSOUND.ASM:3989 KILL_PIN_HIM
 *
 * NOT translated, and each for the same reason -- it belongs to a
 * subsystem this port has not got: ZFLIP_POS_VAR (the scroll's own
 * variable), calc_match_time_1/2 (there is no on-screen match clock),
 * MATCH_AWARD TWO_RND_AWD / is_perfect + PERFECT_AWD / CREATE_PERFECT
 * (match awards are a per-credit display system, distinct from the
 * round_award seam the animation VM already reaches), increment_wincount
 * and replace_wins (the win-count text), the winner token and its float,
 * and the button-press wait at the end. The `is_8_on_1` and royal-rumble
 * +2 overrides do not apply: neither mode exists here, exactly as
 * wm_arcade_match_score_award_round already records.
 */

/* LIFEBAR.ASM:2771's `MOVI CROWD_VICTORY_LOOP,A3 / CALLA SNDSND`, and
   the bare `movi 27,a3 / calla SNDSND` before CALL_MATCH_OVER. */
#define WM_ARW_VICTORY_SOUND 2058          /* SOUND.EQU CROWD_VICTORY_LOOP */
#define WM_ARW_VICTORY_TICKS 200           /* SOUND.EQU D_CROWD_VICTORY_LOOP */
#define WM_ARW_TOKEN_SOUND 27

/* The process's own sleeps, in source ticks. */
#define WM_ARW_FINISH_POLL 10              /* #fini_wait's SLEEPK 10 */
#define WM_ARW_BUCKOFF_SLEEP 90            /* the SLEEP 90 before arw_bwait */
#define WM_ARW_PRE_TOKEN_SLEEP 30          /* SLEEPK 30 */

typedef enum {
    WM_ARW_IDLE = 0,      /* no process: win_announce has not been reached */
    WM_ARW_FINISH_WAIT,   /* #fini_wait: in_finish_move is set */
    WM_ARW_BUCKOFF_WAIT,  /* the SLEEP 90 that ends at arw_bwait */
    WM_ARW_PRE_TOKEN,     /* the round is awarded; SLEEPK 30 before the call */
    WM_ARW_FINISHED       /* the process reached CALL_MATCH_OVER and died */
} wm_arcade_arw_phase_t;

typedef struct {
    uint8_t phase;             /* wm_arcade_arw_phase_t */
    bool done;                 /* @annc_rnd_winner_done */
    int32_t sleep_left;
    /* @round_end_time, stamped at #nobuck. */
    uint32_t round_end_time;
    /* set_winner's answer, held for the CALL_MATCH_OVER that follows. */
    wm_arcade_actor_t *winner;
    /* How many times KILL_PIN_HIM was reached. The PIN_HIM_PROC nag it
       kills is a DCSSOUND process this port does not run, so the kill is
       recorded rather than performed -- and it is recorded because
       win_announce does it unconditionally, before either guard, so it
       happens even on the calls where the process itself declines. */
    uint32_t pin_him_kills;
} wm_arcade_round_announce_t;

typedef struct {
    uint32_t pcnt;                 /* @PCNT */
    bool royal_rumble;             /* @royal_rumble, for anyone_bucking */
    bool in_finish_move;           /* @in_finish_move, for #fini_wait */
    wm_arcade_match_score_t *score;
    void *user;
    /* SNDSND. `ticks` is the sound's own length where the source has one
       (CROWD_VICTORY_LOOP), 0 where it does not. */
    void (*sound)(void *user, int sound, int ticks);
    /* DCSSOUND.ASM CALL_MATCH_OVER, given the winner's PLYR_SIDE. */
    void (*match_over)(void *user, int winner_side);
} wm_arcade_round_announce_ctx_t;

void wm_arcade_round_announce_init(wm_arcade_round_announce_t *st);

/*
 * LIFEBAR.ASM:3603 anyone_bucking -- the first wrestler trying a buckoff,
 * or NULL. A wrestler counts when DO_BUCKOFF is set and either
 * NEW_BUCKOFF is set (he only just asked) or DID_BUCKOFF is not (he has
 * not had his turn). No buckoffs at all in a royal rumble.
 */
wm_arcade_actor_t *wm_arcade_anyone_bucking(wm_arcade_actor_t *const *actors,
                                            size_t actor_count,
                                            bool royal_rumble);

/* LIFEBAR.ASM:3630 set_all_buckoffs -- everyone's NO_BUCKOFF flag. */
void wm_arcade_set_all_buckoffs(wm_arcade_actor_t *const *actors,
                                size_t actor_count);

/*
 * LIFEBAR.ASM:5063 set_winner's KO branch, as its own function: the first
 * live wrestler, preferring one whose DID_PIN is set. The source falls
 * back to the first *active* wrestler when it finds nobody alive -- its
 * own comment calls that "really bad and should never happen" -- and that
 * fallback is translated too, because a pin can end a round with the
 * pinned man's whole side dead and the caller still needs an answer.
 * NULL only when there are no active wrestlers at all.
 */
wm_arcade_actor_t *wm_arcade_set_winner(wm_arcade_actor_t *const *actors,
                                        size_t actor_count);

/*
 * DNKSEQ2.ASM:5202 win_announce. KILL_PIN_HIM always; the CREATE only
 * takes if no ANNC_PID is already running and annc_rnd_winner_done is
 * clear -- the process's own first two tests, which it makes before doing
 * anything else, so they are made here instead of starting a process that
 * would immediately DIE. Returns true when the process actually started.
 */
bool wm_arcade_win_announce(wm_arcade_round_announce_t *st);

/*
 * One tick of announce_rnd_winner. No-op unless a process is running.
 * Returns true on the tick the round is awarded (#nobuck), which is the
 * edge a caller wants: it is where round_end_time is stamped and the
 * score changes.
 */
bool wm_arcade_round_announce_tick(wm_arcade_round_announce_t *st,
                                   wm_arcade_actor_t *const *actors,
                                   size_t actor_count,
                                   const wm_arcade_round_announce_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
#endif
