#ifndef WM_ARCADE_ROLL_H
#define WM_ARCADE_ROLL_H

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE2.ASM:1290 SUBR do_roll -- what a knocked-down wrestler does
 * while the player holds up or down: he rolls along Z, showing frames off
 * his own per-wrestler table (wm/roll_frames.h).
 *
 * Returns non-zero if he rolled, zero if he did not. The source returns it
 * in the Z flag and ANIM.ASM:3031's `jrz #getup` reads it: NOT rolling is
 * what makes him get up. Letting go of the stick, or reaching Z_BOUND, is
 * how the player ends the roll.
 *
 * Two things it writes beyond the frame: OBJ_ZVEL (negated when rolling
 * up), and ROLL_POS, the running 8-bit position that picks the frame.
 */
int wm_arcade_do_roll(wm_arcade_actor_t *a);

/*
 * WRESTLE.ASM:2538-2618 -- the getup meter, run once per wrestler per tick
 * from the main loop. While GETUP_TIME is set the wrestler is stuck on the
 * ground, and this is what unsticks him:
 *
 *   - DELAY_METER clears it outright ("don't want to allow getup time to
 *     be set this close to last time!"),
 *   - otherwise it counts down one per tick,
 *   - and ANY button or stick press this tick, or the release edge of one
 *     (STATUS_FLAGS' M_PRESS_LAST), takes THREE more off. That is the
 *     mash-to-get-up mechanic; the source's own comment is "As long as
 *     getup_time has a value, he is stuck."
 *   - Reaching zero clears PLYR_DIZZY and STARS_FLAG.
 *
 * Not translated: the `match_time` expiry that also clears it (this port
 * has no round-timer countdown -- see wm/arcade/wm_arcade_round.h), and
 * the two `.if DEBUG` stay_down branches, which are not in the shipped
 * build.
 */
void wm_arcade_tick_getup_time(wm_arcade_actor_t *a);

/*
 * WRESTLE.ASM:2503-2535 -- the five per-wrestler countdown timers the main
 * loop runs immediately before the getup meter, each one `jrz` guarded so
 * it stops at zero rather than going negative:
 *
 *   DELAY_BUTNS     "delaying the reading of buttons just after regaining
 *                    control from being flung"
 *   SAFE_TIME       "delaying collisions when a player gets up"
 *   DELAY_METER     "delaying the reappearance of a getup meter"
 *   IMMOBILIZE_TIME "disallowing movement by wrestler"
 *   WALK_FAST       the walk-fast powerup -- and uniquely, this one is
 *                   also guarded on being POSITIVE (`jrn #skp6`), because
 *                   it is written negative elsewhere as a flag.
 *
 * Every one of these was being SET all over this port -- React1-5, the
 * special moves, the attach opcodes -- and counted down by nothing, so an
 * immobilize lasted the rest of the match and
 * wm_arcade_try_attack_hit rejected an immobilized attacker forever.
 */
void wm_arcade_tick_wrestler_timers(wm_arcade_actor_t *a);

#ifdef __cplusplus
}
#endif
#endif
