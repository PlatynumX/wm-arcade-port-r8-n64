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

/*
 * WRESTLE.ASM:3018 SUBRP update_newfacing, the shared per-wrestler routine
 * that recomputes NEW_FACING_DIR every tick from relative position toward
 * the closest opponent (get_opp_process -- substituted with the fixed
 * actor->smart_target opponent, the same substitution wm_arcade_calc_closest
 * above already makes). Called unconditionally, for every wrestler process,
 * from WRESTLE.ASM's main per-process loop (WRESTLE.ASM:2418 `callr
 * update_newfacing`, right after confine_wrestler/confine_wrestler_fix2 and
 * before the PLYR_TYPE human/zombie/drone_main branch) -- not gated on
 * whether the wrestler is actually moving.
 *
 * Translated verbatim (WRESTLE.ASM:3015-3041):
 *   new_facing_dir = (o->x_int > a->x_int ? MOVE_RIGHT : MOVE_LEFT)
 *                   | (o->z_int > a->z_int ? MOVE_DOWN : MOVE_UP)
 * (the source's `cmp a2,a3` computes a3-a2 then `jrgt` branches on that
 * being >0, i.e. opponent's coordinate strictly greater than self's -- the
 * same CMP/JRGT reading already verified elsewhere in this port.)
 *
 * NOT translated here: WRESTLE.ASM's set_rotate_anim, which is what actually
 * promotes NEW_FACING_DIR into FACING_DIR (and only while the wrestler is
 * stationary -- change_walk_anim never touches FACING_DIR while moving, see
 * wm/movement.h and wm/bret_backend.h). This function only ever writes
 * new_facing_dir; see wm_execute_walk's WM_MOVE_ZIP case for the literal
 * FACING_DIR=NEW_FACING_DIR copy set_rotate_anim performs, and
 * wm/bret_backend.h for what set_rotate_anim's own turn-animation selection
 * still lacks.
 */
void wm_arcade_update_newfacing(wm_arcade_actor_t *a, const wm_arcade_actor_t *o);

#ifdef __cplusplus
}
#endif

#endif
