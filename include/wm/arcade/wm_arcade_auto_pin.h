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

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_AUTO_PIN_H */
