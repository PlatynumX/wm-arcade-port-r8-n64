#ifndef WM_ARCADE_ROUND_H
#define WM_ARCADE_ROUND_H

#include <stdbool.h>
#include <stddef.h>
#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE2.ASM::get_live_bits: returns which PLYR_SIDE teams still have a
 * live member. Bit 0 (1) = a member of side 0 is live; bit 1 (2) = a member
 * of side 1 is live. "Live" matches the source definition exactly:
 * player_mode != WM_PMODE_DEAD, or dead but WM_STATUS_ZOMBIE is set
 * (nothing in this port sets that flag yet, but the check is real and
 * costs nothing to keep). Inactive actor slots are skipped, matching
 * process_ptrs' own jrz-skip-inactive loop.
 */
int wm_arcade_get_live_bits(wm_arcade_actor_t *const *actors, size_t actor_count);

/* WRESTLE2.ASM #1tmded's 5*TSEC ("5*53") pin-idiot-check countdown. */
#define WM_ARCADE_PIN_TIMEOUT_TICKS (5 * 53)

/*
 * WRESTLE2.ASM::match_timer's #loop/#1tmded body (WRESTLE2.ASM:4155-4250):
 * the real trigger for a round ending on a knockout, not just the round
 * clock running out. Every tick this checks get_live_bits(); once one side
 * has no live member, a 5-second "pin idiot check" countdown starts (a
 * live opponent gets that long to score a pin bonus before the round is
 * awarded to them anyway) -- when it reaches 0, the round is decided.
 *
 * Translated: the live-bits check and the 5-second countdown-to-decided
 * state machine itself.
 *
 * NOT translated (this is match_timer's whole KO-detection loop, not the
 * whole routine): the CREATE PINHIM_ANIM_PID pin-prompt process, the
 * buckoff/reversal window announce_rnd_winner itself grants afterward
 * (WRESTLE2.ASM/LIFEBAR.ASM's own later stages), 8-on-1/8-on-2 team-size
 * bookkeeping and reduce_bog/crowd-wake (no team system exists in this
 * port), match_time's on-screen countdown display and its ADJSPEED
 * difficulty-based decrement rate (SUBR match_timer's own preamble, a
 * separate presentation feature this function doesn't touch), and
 * everything downstream once a round is actually awarded: set_winner's
 * pin-based winner selection, DO_ROUNDS/p1rounds/p2rounds best-of-3
 * tracking, match_over, and postgame_audits. A double-KO (both sides dying
 * the same tick) resolves as a draw (decided_winner_side=-1) here rather
 * than guessing at set_winner's real pin-priority tie-break, which this
 * port cannot reach anyway without a translated PIN system.
 */
typedef struct {
    /* 0 before any all-dead condition and after a round is decided;
       counts down from WM_ARCADE_PIN_TIMEOUT_TICKS otherwise. */
    int32_t pin_timeout;
    bool decided;
    /* Valid only once decided is true: the surviving PLYR_SIDE (0 or 1),
       or -1 for a simultaneous double-KO draw. */
    int decided_winner_side;
} wm_arcade_round_state_t;

void wm_arcade_round_state_init(wm_arcade_round_state_t *rs);

/* One source tick. No-ops once rs->decided is already true. */
void wm_arcade_round_tick(wm_arcade_round_state_t *rs,
                          wm_arcade_actor_t *const *actors, size_t actor_count);

/*
 * LIFEBAR.ASM:5063 SUBRP set_winner, its KO branch only (LIFEBAR.ASM:
 * 5070-5147) -- the #tmout branch (LIFEBAR.ASM:5149+, "clock ran out":
 * award by average team life, tied-break by last hit) is unreached in this
 * port, which has no round-timer/match_time countdown to expire in the
 * first place.
 *
 * Real source semantics for the KO branch, specialized to the one case
 * this port can ever produce: wm_arcade_round_state_t.decided_winner_side
 * already picked the winning PLYR_SIDE (get_live_bits/pin_timeout, see
 * above); the source's own "find the first live wrestler on that side,
 * preferring one with DID_PIN set" search is moot here since this port's
 * fixed 1-on-1 match never has more than one wrestler per side to find.
 * decided_winner_side==-1 (a simultaneous double-KO) awards no round,
 * matching how the source's own equivalent "found nobody alive" case
 * (LIFEBAR.ASM:5099 "trouble... this should never happen") is explicitly
 * not a normal path.
 *
 * p1rounds/p2rounds increment by 1 for a normal round win. The source's
 * is_8_on_1/royal_rumble +2 override for a final match does not apply --
 * neither mode exists in this port. match_winner is set (1 = side 0, 2 =
 * side 1) the moment either count reaches LIFEBAR.ASM's real best-of-3
 * threshold of 2, and this function then no-ops (matching the ONE-shot
 * nature of a round award: calling it again for the same decided round
 * would double-count, so the caller -- wm_match_tick -- calls this only on
 * the false-to-true edge of round_state.decided).
 *
 * NOT translated: DO_ROUNDS and everything a real next round needs (life/
 * position reset, a restarted round_state, on-screen round announcement) --
 * this port has no round-2 restart at all, so match_winner becoming
 * nonzero is currently a dead end: wm_match_tick keeps ticking exactly as
 * it did before the round was decided.
 */
typedef struct {
    int32_t p1rounds;
    int32_t p2rounds;
    /* 0 = match not yet decided; 1 = side 0 (PSIDE_PLYR1) has won the
       match; 2 = side 1 (PSIDE_PLYR2). */
    int32_t match_winner;
} wm_arcade_match_score_t;

void wm_arcade_match_score_init(wm_arcade_match_score_t *score);

/* winner_side: 0 or 1 (wm_arcade_round_state_t.decided_winner_side once
   decided is true), or -1 for a draw (no-op, awards nothing). */
void wm_arcade_match_score_award_round(wm_arcade_match_score_t *score, int winner_side);

#ifdef __cplusplus
}
#endif

#endif
