/*
 * The auto-pin, its return path, and the victory pose -- four routines
 * that only make sense as one story.
 *
 *   auto_pin_check      WRESTLE.ASM:3886   human -> temp drone
 *   raisearm_check      WRESTLE2.ASM:3683  may he raise his arm?
 *   set_raisearm_bit    WRESTLE2.ASM:4514  he did
 *   drone_change_back   WRESTLE2.ASM:3416  temp drone -> human again
 *
 * Beat everyone and stop doing anything, and auto_pin_check hands you to
 * the drone AI, which walks over and pins. The last thing both the #pin
 * and #raisearm paths do, in every one of the eight wrestler files, is
 * call drone_change_back -- "if we're a temp drone for auto-pin
 * purposes, turn back into a normal player here." Without it the loan
 * is never repaid: a human handed to the AI to finish a pin would stay
 * a drone for the rest of the match.
 *
 * ----------------------------------------------------------------- */

/*
 * WRESTLE.ASM:3886 SUBR auto_pin_check -- the auto-pin.
 *
 * The source's own header on it: "if all opponents are dead, wait four
 * seconds, then wait for the unint bit to clear, then turn into a
 * drone." Beat everyone and stop doing anything, and the game takes
 * over and finishes the pin for you.
 *
 * FOUR SECONDS IS WRONG, TWICE. The header says four and the comment
 * beside the counter says "turn into a drone if the total is >= TSEC*4",
 * and the instruction underneath both of them is `cmpi TSEC*3,a14`.
 * Three seconds is what the arcade does; the two comments disagree with
 * the code and with each other's own arithmetic, and the code wins.
 *
 * Its one caller is move_wrestler (WRESTLE.ASM:3858), one line before
 * the per-character dispatch, every tick, for every wrestler -- and
 * that is where this port calls it from too, through
 * wm_arcade_move_callbacks_t::auto_pin_check.
 */
#ifndef WM_ARCADE_AUTO_PIN_H
#define WM_ARCADE_AUTO_PIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"

/*
 * For an adapter that is kept on purpose while its seam is unwired --
 * see the raisearm_check note in src/core/wrestler_backend.c.
 */
#if defined(__GNUC__)
#define WM_MAYBE_UNUSED __attribute__((unused))
#else
#define WM_MAYBE_UNUSED
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* `cmpi TSEC*3,a14` -- three seconds, not the four both comments claim. */
#define WM_AUTO_PIN_TICKS (53 * 3)

typedef struct {
    /* @in_finish_move and @finish_completed, the same pair mode_dead
       already takes (wm/arcade/wm_arcade_mode_dead.h). */
    bool in_finish_move;
    bool finish_completed;
    /* @royal_rumble. There is no auto-pin in a rumble at all. */
    bool royal_rumble;
    /*
     * get_opp_plyrmode and get_opp_process both read
     * process_ptrs[CLOSEST_NUM], so this is the closest opponent and
     * not an arbitrary one. NULL is read as "not dead", which resets
     * the countdown: the source would have read through a null process
     * pointer, and there is no honest value to reproduce for that.
     */
    wm_arcade_actor_t *opponent;
    /* anyone_bucking's own process_ptrs sweep. */
    wm_arcade_actor_t *const *actors;
    size_t actor_count;
} wm_auto_pin_env_t;

typedef struct {
    /* `movi PTYPE_DRONE,a14 / move a14,*a13(PLYR_TYPE)`. */
    bool became_drone;
    /*
     * The countdown advanced this call. Worth reporting separately
     * because the guards divide into two kinds and the difference is
     * invisible from the outside: see wm_arcade_auto_pin_check.
     */
    bool counted;
} wm_auto_pin_result_t;

/*
 * One call. Writes AUTO_PIN_CNTDOWN and, when it fires, PLYR_TYPE.
 *
 * The guards are not all the same shape, and that is the content. Two
 * of them jump to `#alive`, which ZEROES the countdown: the opponent is
 * not MODE_DEAD, or he is a ZOMBIE (still getting up, so not beaten).
 * The other three just `rets`, leaving the count where it stands: this
 * wrestler has already pinned (B_DID_PIN), somebody is attempting a
 * buckoff, or his own animation is MODE_UNINT. So a man who reaches the
 * threshold mid-move holds there and drops into a drone the instant the
 * uninterruptible bit clears, rather than starting over.
 */
wm_auto_pin_result_t wm_arcade_auto_pin_check(wm_arcade_actor_t *actor,
                                              const wm_auto_pin_env_t *env);


/*
 * WRESTLE2.ASM:3416 drone_change_back -- hand control back.
 *
 * Eight lines, and the whole of it is one guard and one store: a PLYRNUM
 * of 2 or more is a REAL drone and is left alone ("don't check real
 * drones"), and anyone else is made PTYPE_PLAYER. The source does not
 * bother asking whether he was a temp drone first, and says so:
 * "don't bother checking if they're a drone or not. In either case,
 * turning them human again won't hurt."
 *
 * Called at the end of the #pin and #raisearm paths in all eight
 * wrestler files (BRET.ASM:1402 and :1413 are the pair), and from
 * reset_wrestle/reset_wrestle2 between rounds (WRESTLE.ASM:2681,
 * :2822).
 */
bool wm_arcade_drone_change_back(wm_arcade_actor_t *actor);

/*
 * WRESTLE2.ASM:4514 set_raisearm_bit -- one `ori M_DID_RAISEARM` into
 * STATUS_FLAGS, and nothing else. Called right after the arm-raise
 * animation starts; the bit is what stops him doing it twice.
 */
void wm_arcade_set_raisearm_bit(wm_arcade_actor_t *actor);

/*
 * WRESTLE2.ASM:3683 raisearm_check -- may he raise his arm in victory?
 *
 * Returns the source's carry: set is yes. Two parts.
 *
 * FIRST, a royal-rumble hack that the source labels as one. A HUMAN in a
 * rumble may only raise his arm once @FINAL_PTR's queue is empty --
 * `move @FINAL_PTR,a14,L / move *a14,a14 / jrn #hack_done`, a peek that
 * does NOT advance the pointer, which is what wm_final_queue_empty is
 * for. A drone skips the hack entirely (`PLAYER=0`, so a nonzero
 * PLYR_TYPE jumps past it).
 *
 * SECOND, the real test, over every process: every opponent -- anyone
 * whose PLYR_SIDE differs, which is how the sweep skips himself and his
 * teammates in one comparison -- must be MODE_DEAD and not a ZOMBIE, or
 * the answer is no. Dead opponents who are still INSIDE the ring are
 * counted as it goes.
 *
 * Then the geometry, which is the part worth reading twice: if HE is
 * outside the ring the answer is yes regardless, and if he is inside it
 * is yes only when no dead opponent is inside with him. A body on the
 * mat beside him is what stops the pose.
 */
bool wm_arcade_raisearm_check(const wm_arcade_actor_t *actor,
                              wm_arcade_actor_t *const *actors,
                              size_t actor_count,
                              bool royal_rumble,
                              bool final_queue_empty);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_AUTO_PIN_H */
