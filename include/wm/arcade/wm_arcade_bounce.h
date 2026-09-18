/*
 * WRESTLE.ASM:5115 SUBR bounce_off_ropes -- the rope rebound.
 *
 * What makes a whip into the ropes come back. Every wrestler's own
 * mode_running calls it (BRET.ASM:1935, BAM.ASM:1880, DNK.ASM:1970,
 * DOINK.ASM:2337 and the rest), and all eight of this port's
 * dispatchers already had the callback seam for it with nothing behind
 * it -- so a running wrestler crossed the ropes and kept going.
 *
 * The animation is the visible half. The half that matters is RISK:
 *
 *      MOVE    *A13(GETUP_TIME),A0
 *      JRNZ    ALREADY_DONE_RISK_MESS
 *      move    *a13(RISK),A0
 *      JRNZ    ALREADY_DONE_RISK_MESS
 *      MOVI    60,A0                   ;Time to execute high-risk move!
 *      MOVE    A0,*A13(RISK)
 *
 * a sixty-tick window in which the next hit counts as a high-risk move.
 * Its READER is already translated -- src/core/arcade/wm_arcade_react.c
 * reads attacker->risk, sets any_hits, and on WM_ARCADE_RISK_HIGH_BIT
 * multiplies the damage and fires the bonus message -- and other
 * producers exist (the taunt-style path). This was the missing one, so
 * until now a run into the ropes could never open the window at all.
 *
 * Note which bit this producer does NOT set: it writes a plain 60, so
 * WM_ARCADE_RISK_HIGH_BIT is clear and a rope-bounce hit gets any_hits
 * without the multiplier. The 4x belongs to the taunt path.
 */
#ifndef WM_ARCADE_BOUNCE_H
#define WM_ARCADE_BOUNCE_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* `MOVI 60,A0 / MOVE A0,*A13(RISK)`. */
#define WM_BOUNCE_RISK_TICKS 60

/*
 * WRESTLE.ASM:5185 `#bounce_xoffsets`, one WORD per roster slot.
 *
 * How far INSIDE the rope line the bounce triggers, and it is per
 * wrestler rather than uniform: -20 for most of them, 0 for Shawn
 * Michaels and the Referee, and -30 for Bam Bam, who is the widest man
 * on the roster and turns earliest.
 *
 * The sign reads backwards until you follow both branches. On the left
 * the source does `sub a14,a0` and on the right `add a14,a0` with the
 * same value, so a negative offset moves the trigger line inward on
 * both sides -- 20 pixels before the rope, not past it.
 *
 * This is a `.word` table rather than a table of animation labels, so
 * tools/wlrostertbl.py does not extract it (it reads `.long` label
 * tables); it is transcribed here and pinned by a test.
 */
#define WM_BOUNCE_SLOTS 10
extern const int16_t wm_bounce_xoffsets[WM_BOUNCE_SLOTS];

typedef struct {
    /* The `#bounce` path ran. PLYRMODE is already WM_PMODE_BOUNCING. */
    bool bounced;
    /*
     * `#bounce_anims[WRESTLERNUM]` for the `calla change_anim1a`, or
     * NULL. The table is real and extracted (WRESTLE.ASM:5196); every
     * slot is filled, Adam Bomb and the Referee both getting Doink's,
     * so a NULL here means the roster number was out of range.
     */
    const char *bounce_anim;
    /*
     * RISK was set to WM_BOUNCE_RISK_TICKS on this call. False when the
     * two guards above it refused: he is getting up (GETUP_TIME), or a
     * window is already open. The bounce itself still happens -- the
     * label the source jumps to is ALREADY_DONE_RISK_MESS, which is
     * past the store and before the animation.
     */
    bool opened_risk_window;
} wm_bounce_result_t;

/*
 * One call of the routine. Reads INRING, MOVE_DIR, the hurt box and Z
 * off the actor; writes RISK and PLYRMODE to it and reports the
 * animation, which this module cannot start.
 *
 * `INRING` is 1 for OUTSIDE in the source (`jrnz #outside`) while this
 * port's in_ring is the ordinary boolean, so a wrestler outside the
 * ring returns an empty result -- he has no ropes to bounce off.
 */
wm_bounce_result_t wm_arcade_bounce_off_ropes(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_BOUNCE_H */
