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

#ifdef __cplusplus
}
#endif

#endif
