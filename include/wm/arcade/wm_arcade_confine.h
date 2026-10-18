#ifndef WM_ARCADE_CONFINE_H
#define WM_ARCADE_CONFINE_H

#include <stdbool.h>
#include <stddef.h>

#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wmania_ring_climb.h"

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
     * The other half of what the climb checks can decide, and the half
     * that used to be dropped on the floor here.
     *
     * Three routines reach the rope while the wrestler is facing the
     * WRONG WAY, and the source does not climb on the spot. It turns
     * him first. Two of them are in this routine -- ck_climb_out_side
     * (WRESTLE2.ASM:578) and ck_climb_in_side (:702); the third,
     * climb_turnbuckle (:200), is called from each wrestler's own
     * mode_normal instead and has its own entry point below.
     *
     *      calla   set_rotate_anim
     *      calla   change_anim1a
     *      movi    #climb,a0
     *      move    a0,*a13(CODE_ADDR),L    ;when the rotate anim
     *      SETMODE WAITANIM                ;finishes
     *
     * and mode_waitanim -- BRET.ASM:2550, copied verbatim into every
     * other wrestler file -- `call`s CODE_ADDR once that rotate
     * animation ends. The climb itself is the continuation, which
     * wm_ring_climb_continue is.
     *
     * WM_RING_CLIMB_CONT_NONE means no rotate is pending. Anything else
     * is a continuation the caller must (a) start the rotate animation
     * for, (b) park in WM_PMODE_WAITANIM, and (c) keep until the
     * animation ends -- wm_arcade_actor_t::code_addr is where it goes,
     * because CODE_ADDR is the field the source uses for exactly this.
     *
     * Dropping it is not a cosmetic loss. Both of the two here set
     * CLIMBING_THRU=1 on this path as well, and that flag is cleared
     * only by an ANI_INRING/ANI_NOTINRING op inside the climbthru
     * animation (ANIM.ASM:405, :4471). So a wrestler who walked into
     * the side ropes facing away used to come out of the check flagged
     * as climbing through, with no animation running to ever clear it,
     * and ck_climb_in_side's own `if (climbing_thru) return` then
     * refused him the ropes for the rest of the match.
     */
    WmRingClimbContinuation climb_rotate_then;

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
 * WRESTLE2.ASM:103 SUBR climb_turnbuckle -- the fourth climb check, and
 * the one that is not inside confine_wrestler.
 *
 * Every wrestler's own mode_normal calls it directly (BRET.ASM:1453,
 * BAM.ASM:1410, DOINK.ASM:1828 and the rest) with
 * STICK_VAL_CUR in a0, and all eight of this port's dispatchers already
 * had the callback seam for it -- with nothing behind it, the same way
 * ck_climb_out_* had no caller before the ring-out work. So nobody could
 * climb a turnbuckle.
 *
 * It is here rather than beside the module for the reason
 * wm_arcade_climb_continue is: this file owns the one mapping between a
 * wm_arcade_actor_t and a WmRingClimbPlayer, INRING's inverted polarity
 * included. `rope_x` comes from wm_ring_get_rope_x (WRESTLE.ASM:5789),
 * which is what the source's own `calla get_rope_x` computes at each of
 * the two comparison sites.
 *
 * `handled` is the source's carry flag: `setc` on both climb paths,
 * `clrc` at #not_top/#no_climb, and the dispatcher stops processing the
 * tick when it is set.
 */
typedef struct {
    bool handled;
    /* `calla change_anim1a` on #climb_anims[WRESTLERNUM], or NULL when
       the turn has to come first (or for a wrestler whose row in that
       table is a real 0). */
    const char *anim;
    /* CODE_ADDR: WM_RING_CLIMB_CONT_TURNBUCKLE when the wrestler has to
       turn to the corner first, WM_RING_CLIMB_CONT_NONE otherwise. */
    WmRingClimbContinuation rotate_then;
    uint16_t facing;                 /* NEW_FACING_DIR */
} wm_climb_turnbuckle_result_t;

wm_climb_turnbuckle_result_t wm_arcade_climb_turnbuckle(
    wm_arcade_actor_t *actor,
    wm_arcade_actor_t *const *actors,
    size_t actor_count);

/*
 * mode_waitanim's `call a0`, for the climb continuations.
 *
 * BRET.ASM:2550, and the identical copy in every other wrestler file:
 *
 *      mode_waitanim   ;12
 *              move    *a13(ANIMODE),a0
 *              btst    MODE_END_BIT,a0
 *              jrz     #not_ended
 *              move    *a13(CODE_ADDR),a0,L
 *              call    a0
 *
 * There is no NULL check on a0 there, because the source never enters
 * WAITANIM without having written CODE_ADDR in the instruction before
 * the SETMODE. This port carries the token in
 * wm_arcade_actor_t::code_addr and decodes it here.
 *
 * `cont` is the WmRingClimbContinuation the confine pass reported in
 * wm_confine_result_t::climb_rotate_then. Returns the source animation
 * label the continuation wants started (`change_anim1a`), or NULL --
 * for WM_RING_CLIMB_CONT_NONE, and for a wrestler whose entry in the
 * continuation's animation table is a real 0 (the referee's, and Adam
 * Bomb's in rollthru_top_anims). PLYRMODE and CLIMBING_THRU are
 * applied to the actor either way, exactly as the continuation sets
 * them: that is what takes him back out of WAITANIM.
 */
const char *wm_arcade_climb_continue(wm_arcade_actor_t *actor,
                                     WmRingClimbContinuation cont);

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
