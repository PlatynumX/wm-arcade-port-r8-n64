#ifndef WM_ARCADE_CONFINE_H
#define WM_ARCADE_CONFINE_H

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE.ASM:3074 SUBRP confine_wrestler. wm_arcade_confine_wrestler runs
 * one pass of it per call.
 *
 * Not translated: the source actually runs confine_wrestler twice per tick
 * (WRESTLE.ASM:3743-3752 confine_wrestler_fix1/fix2 -- "for whatever
 * reason, confine_wrestler happens twice per tick", the source's own
 * comment -- OR-ing the two calls' CAN_MOVE_DIR results together, since
 * the first call's own position clamp can make the second call's collision
 * check no longer trigger). That only converges because OBJ_COLLX1/
 * OBJ_COLLX2 get refreshed from the corrected position before the second
 * call -- which requires locating the second confine_wrestler call site
 * (outside the one visible in the main per-process loop) and confirming
 * what refreshes the collision box in between. Neither is established
 * here, so a second pass reusing the same, now-stale hurt_box is not
 * simulated -- doing so was tried and confirmed to diverge (a stale
 * collision box re-applies nearly the same correction a second time,
 * overshooting past the boundary in the opposite direction) rather than
 * converge, which would be strictly worse than the single real pass below.
 *
 * Translated (the in-ring branch only -- see below for why the out-of-ring
 * branch is never reached in this port):
 *   - MODE_NOCONFINE and PLYRMODE==ATTACHED both early-out to "can move in
 *     all directions, no clamp" exactly as the source's #no_confine does.
 *   - Z bounds: RING_TOP/RING_BOT (wm/arcade/wmania_ring_geometry.h),
 *     clamping z_int/z_fixed and setting WM_MOVE_UP/WM_MOVE_DOWN in
 *     can_move_dir.
 *   - X bounds: the real left/right rope boundary lines
 *     (WM_RING_BOUNDARY_LEFT_ROPE/RIGHT_ROPE, wm_ring_calc_line_x), using
 *     actor->hurt_box.x1/x2 exactly where the source reads OBJ_COLLX1/
 *     OBJ_COLLX2 -- wm_arcade_set_hurt_box already computes byte-identical
 *     values (same set_collision_boxes translation, COLLIS.ASM:260-354).
 *     Clamps x_int/x_fixed so the collision-box edge lands exactly on the
 *     rope line, and sets WM_MOVE_LEFT/WM_MOVE_RIGHT. The near-range
 *     pre-check the source does before calling calc_line_x
 *     (WRESTLE.ASM:3136-3139 etc) is a pure performance short-circuit --
 *     mathematically always agrees with the real per-Z check once Z is
 *     already clamped into [RING_TOP,RING_BOT] by the step above, which it
 *     always is by this point -- so it's not reproduced separately.
 *
 * NOT translated, all real and all skipped for the same reason: this
 * port's Bret actor is the only one with real, moving position and a real
 * hurt_box (see wm/bret_backend.h), and it never grapples/attaches to an
 * opponent (ATTACH_PROC) or leaves the ring (in_ring only ever starts and
 * stays 1/true -- wm/match.h's place_wrestler, no ring-out/knockback
 * system is wired to ever clear it), so:
 *   - ATTACH_PROC paired-actor movement and the rope-wobble bounce
 *     velocity/sound effects (ROPE_BOUNCEIO, triple_sound) on first
 *     contact -- audio/visual and two-actor physics, not reachable here.
 *   - ck_climb_out_top/bot/side (WRESTLE2.ASM, already translated as pure
 *     logic in wm/arcade/wmania_ring_climb.h but not wired to any real
 *     animation system yet) -- triggering a climb is a presentation
 *     concern layered on top of the confinement this function computes,
 *     not part of computing CAN_MOVE_DIR or the position clamp itself.
 *   - The #outring branch entirely (ARENA_TOP/BOT, the fence lines, and
 *     the further mat2-edge climb-in overlap check that also touches
 *     CAN_MOVE_DIR) -- genuinely unreached while in_ring never leaves 1.
 *   - The PLYRMODE==RUNNING "hit a gate, take damage, crash animation,
 *     zombie transform" tail (WRESTLE.ASM:3656-3730) -- besides also being
 *     under the unreached #outring path, it needs FACETBL/FACE24TBL
 *     animation-table dispatch this port hasn't translated.
 */
void wm_arcade_confine_wrestler(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif

#endif
