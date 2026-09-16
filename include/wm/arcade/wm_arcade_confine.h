#ifndef WM_ARCADE_CONFINE_H
#define WM_ARCADE_CONFINE_H

#include <stdbool.h>
#include <stddef.h>

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
 * Translated, the in-ring branch (WRESTLE.ASM:3090-3425):
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
 * ALSO TRANSLATED, and previously argued away -- the correction is worth
 * stating because the argument was confident and wrong in its premises.
 * This header used to say the out-of-ring half was unreachable because
 * "Bret is the only actor with real, moving position" and "in_ring only
 * ever starts and stays 1/true -- no ring-out/knockback system is wired
 * to ever clear it". All seven wrestlers have had real positions for a
 * while, and the thing that clears INRING was not missing: it is the
 * climbthru animations' own NOT_IN_RING, reached from ck_climb_out_*,
 * which were fully translated in wm/arcade/wmania_ring_climb.h and had
 * NO CALLER. Their call site is inside this routine -- WRESTLE.ASM:3103,
 * :3121 and :3235 -- so the out-of-ring half was dead by construction,
 * and joining them up is what brings it to life:
 *   - ck_climb_out_top/bot/side on the way out and ck_climb_in_top/bot/
 *     side on the way back in, each run immediately after the clamp that
 *     pins the wrestler on the boundary. Immediately matters: every one
 *     of these tests the position for EQUALITY against the boundary, so
 *     they only ever fire on the value the clamp has just written.
 *   - The whole #outring branch: ARENA_TOP/BOT in place of the ropes,
 *     the left/right fence lines, and #cont_x's mat-edge overlap, which
 *     compares the X and Z overlaps and applies whichever correction is
 *     smaller -- push him back off the apron, or stand him on its edge
 *     and offer him the climb in.
 *   - The PLYRMODE==RUNNING gate crash and the MODE_DEAD zombie
 *     transform (WRESTLE.ASM:3656-3730).
 *
 * NOT translated, and now for narrower reasons than before:
 *   - The rope-wobble bounce velocity and sound on first contact
 *     (ROPE_BOUNCEIO, triple_sound) -- audio/visual.
 *   - ck_climb_out_side's shipped source quirk, which reads through the
 *     process_ptrs cursor rather than through a wrestler. No value is
 *     invented for it, so climbing out through the SIDE ropes does not
 *     happen here while top and bottom do; the result reports it.
 *   - Which ANIMATION the gate crash plays (fall_back_tbl or
 *     bncoff_gate) is a FACETBL/FACE24TBL dispatch, so the choice is
 *     reported rather than made.
 */
void wm_arcade_confine_wrestler(wm_arcade_actor_t *actor);

/*
 * What one pass of confine_wrestler decided that it cannot carry out by
 * itself. The routine clamps positions and CAN_MOVE_DIR in place; these
 * three all need something it does not own -- an animation system, a
 * sound queue, or the wrestler's own transform.
 */
/* DAMAGE.EQU:118 `D_GATE_CRASH .equ 20`. */
#define WM_CONFINE_GATE_DAMAGE 20
/* `cmpi TSEC*3,a14 / jrge #bnc` -- three seconds since the last hit. */
#define WM_CONFINE_GATE_REHIT (53 * 3)

typedef struct {
    /*
     * The animation ck_climb_out_top/bot/side or ck_climb_in_top/bot/
     * side started, as a source label, or NULL. `climb_facing` is the
     * NEW_FACING_DIR that came with it.
     *
     * confine_wrestler really does call these itself -- WRESTLE.ASM:3103,
     * :3121, :3235, :3382 for the way out and :3560, :3583, :3604 for
     * the way back in -- right after the clamp has pinned the wrestler
     * on the boundary. Climbing out is the ONLY thing in the game that
     * clears INRING, so until this was wired the whole out-of-ring half
     * of the routine was dead by construction.
     */
    const char *climb_anim;
    uint16_t climb_facing;

    /*
     * ck_climb_out_side reached the shipped source quirk that
     * wm/arcade/wmania_ring_climb.h documents -- a read through the
     * process_ptrs cursor rather than through a wrestler -- and this
     * port supplies no value for it. Reported rather than guessed, so
     * "he did not climb out the side" is visibly a known gap and not a
     * silent no.
     */
    bool climb_needs_source_quirk;

    /*
     * WRESTLE.ASM:3664, the gate crash: a wrestler in MODE_RUNNING who
     * hits the arena fence. The velocities, MODE_NORMAL, RUN_TIME and
     * the damage are applied to the actor; which ANIMATION he plays is
     * a FACETBL/FACE24TBL dispatch, so the choice is reported instead.
     *
     * `gate_fall_back` is the three-second rule: "if we've hit the gate
     * in the last three seconds, fall back instead" of bouncing.
     */
    bool gate_crash;
    bool gate_fall_back;

    /*
     * WRESTLE.ASM:3718 `#dead`: a ZOMBIE with CAN_XFORM who hits the
     * arena edge transforms there and then, which is the second and
     * faster of the two routes out of zombie mode -- the other being
     * mode_dead's ten-second timeout.
     */
    bool zombie_transform;
} wm_confine_result_t;

/*
 * confine_wrestler with everything it actually needs: the roster (the
 * climb checks ask whether any opponent is outside), PCNT (the idiot
 * check and the gate's three-second rule both stamp against it), and
 * somewhere to report what it could not do itself.
 *
 * `actors`/`actor_count` may be NULL/0 and `out` may be NULL; then the
 * climb checks simply do not fire and this is the plain confinement the
 * older entry point has always been.
 */
void wm_arcade_confine_wrestler_ex(wm_arcade_actor_t *actor,
                                   wm_arcade_actor_t *const *actors,
                                   size_t actor_count,
                                   uint32_t pcnt,
                                   wm_confine_result_t *out);

/*
 * WRESTLE.ASM:6163 SUBRP final_confine, called from the main loop at
 * :2064 after every wrestler has moved. It sweeps the process table and
 * re-runs set_collision_boxes and confine_wrestler on each one that has
 * an ATTACH_PROC -- and only those.
 *
 * The reason for the second pass is the order the first one runs in. A
 * puppet is moved by his master, and if the master is later in the
 * table than the puppet then the puppet was confined before he was
 * dragged. Everybody else was confined after their own last move and
 * needs nothing; an attached man might have been pulled through a rope
 * since. So this is not a belt-and-braces repeat, it is the fix for a
 * specific ordering hole, which is why the ATTACH_PROC test is there.
 *
 * Note it does NOT restore a13 per iteration the way the caller might
 * expect -- it pushes once around the whole loop -- so the sweep is
 * over every process, not just the current one's partner.
 */
void wm_arcade_final_confine(wm_arcade_actor_t *const *actors,
                             size_t actor_count);

#ifdef __cplusplus
}
#endif

#endif
