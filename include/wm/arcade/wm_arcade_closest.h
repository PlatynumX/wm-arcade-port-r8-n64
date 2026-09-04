#ifndef WM_ARCADE_CLOSEST_H
#define WM_ARCADE_CLOSEST_H

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE.ASM's calc_closest/calc_closest2 (PLYR.EQU CLOSEST_DIST/XDIST/
 * YDIST/ZDIST), scoped to the one case wm_match ever has: a fixed 1-on-1
 * opponent (actor->smart_target), not a multi-wrestler ladder/teammate
 * search.
 *
 * Translated (WRESTLE.ASM's per-candidate delta/distance computation):
 *   closest_xdist = abs(a->x_int - o->x_int)
 *   closest_zdist = abs(a->z_int - o->z_int)
 *   closest_ydist = abs(a->y_int - o->y_int)
 *   closest_dist  = sqrt(closest_xdist^2 + closest_zdist^2 + closest_ydist^2)
 *
 * NOT translated, because it only matters when choosing among more than one
 * live candidate and this port never has one: calc_closest's full ranking
 * loop over every process (biased_range, ONGROUND penalty, WHOIHIT bonus,
 * INRING penalty, previous-closest bonus, combo-zero, Z-penalty, the
 * running/behind-us skip rule, zombie/dead precedence) and CLOSEST_NUM
 * itself (which wrestler is "closest" -- always o here, already fixed at
 * match creation). Also not translated: calc_closest2's "only recalculate
 * every 4th tick, staggered by PLYRNUM" throttle, a pure CPU-cost
 * optimization for scanning many candidates -- irrelevant to a single fixed
 * pair, so this recomputes every tick instead.
 *
 * closest_dist uses a real (if not bit-exact) integer square root rather
 * than SQUARE.ASM's fast quantized table-lookup approximation
 * (square_root, max input 262,143, discards its input's low 5 bits before
 * a table lookup) -- the two callers that actually gate behavior on these
 * fields, BRET.ASM's near()/do_punch and mode_block, only ever read
 * closest_xdist/closest_zdist, which are exact either way.
 */
void wm_arcade_calc_closest(wm_arcade_actor_t *a, const wm_arcade_actor_t *o);

#ifdef __cplusplus
}
#endif

#endif
