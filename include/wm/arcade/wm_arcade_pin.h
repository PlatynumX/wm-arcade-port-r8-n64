#ifndef WM_ARCADE_PIN_H
#define WM_ARCADE_PIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The pin, which the whole roster's control code asks about and
 * nothing in this port could answer.
 *
 * Every wrestler's control layer already calls a `can_pin` callback --
 * BRET.ASM:275, TAKER.ASM:147 and six more, all translated -- and no
 * caller ever supplied one, so no wrestler in this port has ever been
 * able to pin anybody. That has a second consequence: @p1pins and
 * @p2pins never move, and und_finish_move1 refuses on "this must be my
 * second pin attempt", so the Undertaker's coffin finish is
 * unreachable even with its own monitor running.
 *
 * Two routines, from two files, both of them the real thing.
 */

/*
 * WRESTLE2.ASM:3925 can_pin. The source's own summary: "Check to make
 * sure your opponent is at rest and staying that way. Check to see if
 * he's in the xxx_dead_anim. And make sure he's in range. Oh, and make
 * sure there aren't any zombies either."
 *
 * Four tests and then five side effects, and the side effects are the
 * half that matters -- this is not a predicate, it is the act of
 * pinning:
 *
 *   No live opponent. Every wrestler on the OTHER side must be
 *   MODE_DEAD and must not be carrying B_ZOMBIE. Teammates are
 *   skipped, and so is an inactive slot.
 *
 *   Range. CLOSEST_DIST <= 70h and CLOSEST_ZDIST <= 50h, both signed
 *   and both `jrgt`, so exactly the boundary passes.
 *
 *   The victim must carry B_PINABLE, which xxx_dead_anim's own
 *   #set_pinable_bit put there -- the "is he in the dead animation"
 *   test, done by flag rather than by asking the animation.
 *
 * Then, on success: PINNED on the victim, WHOPINNEDME pointing at the
 * pinner, all three of the victim's velocities zeroed, his PTIME set
 * to 1 and his KOD bit cleared ("'cuz he's probably been KO'd if he's
 * a drone").
 *
 * `actors` is every wrestler, as the source's process_ptrs sweep is.
 * `victim` is get_opp_process's answer -- CLOSEST_NUM's row -- which
 * this port fixes at match creation and every caller already has in
 * hand, so it is passed rather than re-derived from a CLOSEST_NUM the
 * port does not track.
 *
 * Returns true and performs the side effects, or false and touches
 * nothing.
 */
bool wm_arcade_can_pin(wm_arcade_actor_t *pinner, wm_arcade_actor_t *victim,
                       wm_arcade_actor_t *const *actors, size_t count);

/* The two range limits, the source's own hex. */
#define WM_PIN_MAX_DIST  0x70
#define WM_PIN_MAX_ZDIST 0x50

/*
 * AWARD.ASM:1768 pin_prompt -- the "PIN HIM!" prompt, and the only
 * thing in the game that moves @p1pins and @p2pins.
 * WRESTLE2.ASM:4235 creates it the tick a side is wiped out.
 *
 * This is its DECISION half. What is deliberately not here is its
 * presentation, which is most of its length: END_MATCH_SPEECH, the
 * 0BBh sound, two BEGINOBJ pieces zooming in from off-screen to x=400
 * or x=-100 depending on the side, #flash_pin_txt, two seconds, and
 * the same zoom back out.
 *
 * The tests, in the source's order:
 *
 *   royal_rumble or is_8_on_1, and then only if wrestler_count is
 *   nonzero -- neither mode exists in this port, so the branch is
 *   taken to #not_8_on_1 every time and is not written out.
 *
 *   One prompt at a time: `EXISTP PINHIM_ANIM_PID` and die if one is
 *   already up. The caller owns that, since it owns the process.
 *
 *   A live human on the WINNING team, inside the ring. "only check
 *   humans" -- the sweep is two entries long, not NUM_WRES.
 *
 *   Every wrestler on the dead team MODE_DEAD, not a zombie, and at
 *   least one of them INRING. A live one or a zombie kills the prompt
 *   outright; an out-of-ring corpse is merely skipped.
 *
 * `dead_side` is the a9 WRESTLE2.ASM:4232 computes (`xori 3 / srl 1`
 * of the live-team bits). Returns the pinner -- the live human it
 * found, whose side the counter is credited to -- or NULL.
 *
 * INRING is zero INSIDE the ring in this source; the port keeps the
 * actor's `in_ring` as an ordinary boolean, so the test reads the
 * opposite way round here on purpose.
 */
wm_arcade_actor_t *wm_arcade_pin_prompt(wm_arcade_actor_t *const *actors,
                                        size_t count, int dead_side);

/*
 * @p1pins / @p2pins. AWARD.ASM:1856 and :1868 bump one of the two by
 * the PINNER's PLYR_SIDE -- side 0 is p1 -- as the prompt goes up, and
 * WRESTLE.ASM:1578 clears both at the start of a match, alongside
 * @finish_completed.
 *
 * They are the "second pin attempt" und_finish_move1 counts, which is
 * why they are here rather than left as a display detail.
 */
typedef struct {
    int32_t p1pins;
    int32_t p2pins;
} wm_arcade_pins_t;

void wm_arcade_pins_clear(wm_arcade_pins_t *p);
void wm_arcade_pins_award(wm_arcade_pins_t *p, int plyr_side);
int32_t wm_arcade_pins_for(const wm_arcade_pins_t *p, int plyr_side);

#ifdef __cplusplus
}
#endif
#endif
