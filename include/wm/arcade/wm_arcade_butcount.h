#ifndef WM_ARCADE_BUTCOUNT_H
#define WM_ARCADE_BUTCOUNT_H

#include "wm/arcade/wm_arcade_combat.h"

/*
 * WRESTLE.ASM:4681 count_button_presses and :4667 clear_button_presses --
 * the button-mash counters the animation VM branches on.
 *
 * The arcade counts, per wrestler, how many times each of the five buttons
 * has been newly pressed, and animations read those counts to decide
 * whether to keep going:
 *
 *     WWWL ANI_IF_BUTCOUNT_LT,KICKB_COUNT,1,#exit
 *
 * is "if he has not pressed kick since the last ANI_CLR_BUTCOUNT, stop
 * here". That is the whole mash mechanic -- repeated knees to the head, the
 * combo strings, and holding a grapple all run on it, 239 times across the
 * eight playable wrestlers' sequence files.
 *
 * The source counts newly-pressed buttons only (wres_get_but_val_down), so
 * holding a button down does not tick the counter up; each press is one.
 * The counters are reset by ANIM.ASM's own ANI_CLR_BUTCOUNT, which is what
 * makes a count meaningful per animation span rather than per round.
 *
 * Called once per wrestler per tick, immediately after that wrestler's
 * input is committed and before the animation VM runs -- WRESTLE.ASM:2452
 * calls `update_joystat` then `count_button_presses`, both before
 * `animate_wrestler`, and an animation reading a count must see this tick's
 * press.
 */
void wm_arcade_count_button_presses(wm_arcade_actor_t *actor);

/* WRESTLE.ASM:4667 clear_button_presses: all five back to zero. */
void wm_arcade_clear_button_presses(wm_arcade_actor_t *actor);

/*
 * One counter, by its PLYR.EQU:152-156 index: 0 punch, 1 block, 2 super
 * punch, 3 kick, 4 super kick. That index is what ANI_IF_BUTCOUNT_GE/LT
 * carry as their first operand -- in the source it is a struct offset, and
 * the fields are adjacent WORDs in exactly this order.
 */
int32_t wm_arcade_button_count(const wm_arcade_actor_t *actor, int index);

#endif
