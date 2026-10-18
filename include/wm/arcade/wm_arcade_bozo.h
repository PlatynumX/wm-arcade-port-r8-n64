#ifndef WM_ARCADE_BOZO_H
#define WM_ARCADE_BOZO_H

/*
 * DOINK.ASM:3316 bozo_check -- the game's answer to a player who has
 * given up on wrestling and is just mashing.
 *
 * Its own comment is the whole design: "Bozo check ... Lots of super
 * buttons and blocks have been hit! Reverse out. Do reversal unless I
 * have been immobilized! If not, set immobilize time for opponent and
 * reverse." A wrestler held in a head hold, or holding one, who has run
 * up eighteen presses across the two SUPER buttons and block gets a free
 * power move out of it, and the man who did it to him is frozen for
 * half a second.
 *
 * Every wrestler file calls it TWICE -- once at the top of mode_headhold
 * and once at the top of mode_headheld -- and every one of those sixteen
 * call sites reads the carry flag it sets.
 */

#include <stdbool.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * `cmpi 18,a2 / jrlt #no_bozo`, over a sum of exactly three counters.
 *
 * WHICH three is the content. PLYR.EQU:152-156 declares five in a block
 * whose comment says "keep ordered" -- PUNCHB_COUNT, BLOCKB_COUNT,
 * SPUNCHB_COUNT, KICKB_COUNT, SKICKB_COUNT -- and this routine adds
 * SPUNCHB_COUNT, SKICKB_COUNT and BLOCKB_COUNT and leaves the two
 * ORDINARY attack counters out. Mashing punch and kick will never trip
 * it; mashing the super buttons and block will.
 */
#define WM_BOZO_BUTTON_TOTAL 18

/* `movk 32,a14 / move a14,*a0(IMMOBILIZE_TIME)` on WHOHITME. */
#define WM_BOZO_IMMOBILIZE 32

typedef struct {
    /*
     * ANIM.ASM's FIND_AND_KILL_ENDLESS, which bozo_check reaches
     * unconditionally on the way out. Optional; the routine's decision
     * does not depend on it.
     */
    void (*find_and_kill_endless)(wm_arcade_actor_t *actor, void *user);
    void *user;
} wm_bozo_env_t;

/*
 * True where the source does `setc`, which is what the sixteen
 * `jrnc #fail` call sites branch on.
 *
 * The two refusals are not symmetrical and the source says why in the
 * order it tests them: too few presses is "not a bozo yet", while
 * IMMOBILIZE_TIME is "ignore" -- a wrestler who is himself frozen does
 * not get to reverse out of it, and the count is left standing so he
 * can the moment he thaws.
 *
 * On success it does three things besides returning true. It sets
 * M_SMART_ATTACK and points SMART_TARGET at WHOHITME -- both halves of
 * JJXM.H's SMRTTGT macro, and the target matters: "target WHOHITME --
 * don't hit anyone else". It freezes WHOHITME for 32 ticks. And it calls
 * FIND_AND_KILL_ENDLESS.
 *
 * ONE GUARD IS THIS PORT'S AND NOT THE SOURCE'S. `move *a13(WHOHITME),
 * a0,L` is followed straight away by a store through a0, with nothing
 * asking whether it is zero; on the arcade that writes into the bottom
 * of RAM and is survivable, and here it would be a null dereference. The
 * freeze is skipped when nobody has hit him. Everything else still
 * happens, because everything else is his own state -- and a wrestler in
 * a head hold with no WHOHITME is not a case the source can reach
 * anyway, since the hold is what sets it.
 */
bool wm_arcade_bozo_check(wm_arcade_actor_t *actor, const wm_bozo_env_t *env);

#ifdef __cplusplus
}
#endif
#endif
