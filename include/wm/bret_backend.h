#ifndef WM_BRET_BACKEND_H
#define WM_BRET_BACKEND_H

#include "wm/arcade/wm_arcade_bret.h"
#include "wm/bret_frame_geometry.h"
#include "wm/movement.h"
#include "wm/visual.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The "merge adapter" wm_arcade_bret.h's own comment anticipates: resolves
 * BRET.ASM's wm_arcade_bret_anim_id_t tokens to the native wm_visual_sequence
 * data already extracted from the arcade assets (src/generated/bret_visuals.c,
 * bret_attacks.c), and drives two wm_visual_state tracks (body + torso) from
 * wm_arcade_bret_callbacks_t.
 *
 * Coverage is deliberately partial, matching what tools/wlanim.py has
 * actually extracted from HRTSEQ1-4.ASM so far:
 *   - WM_BRET_ANIM_STAND2/STAND4/TORSO2/TORSO4 (idle stance + torso)
 *   - WM_BRET_ANIM_PUNCH2/PUNCH4 (light punch, HRTSEQ2.ASM hrt_2/4_punch_anim)
 *   - WM_BRET_ANIM_SUPER_PUNCH2_4 (power punch, hrt_4_super_punch_anim --
 *     hrt_2_super_punch_anim does not exist in the source tree, so
 *     WM_BRET_ANIM_SUPER_PUNCH2_2/SUPER_PUNCH4 stay unmapped)
 *   - WM_BRET_ANIM_KICK2/KICK4 (light kick, hrt_2/4_kick_anim)
 *   - WM_BRET_ANIM_SUPER_KICK2/SUPER_KICK4 (power kick, hrt_2_super_kick_anim
 *     -- hrt_4_super_kick_anim, HRTSEQ2.ASM:1335, is a literal SUBR alias of
 *     it, same address, not distinct artwork, so both ids map to the same
 *     extracted wm_bret_power_kick_anim data)
 *
 * Every other wm_arcade_bret_anim_id_t (grapples, pins, finishers, running
 * moves, turnbuckle moves, ...) resolves to NULL and wm_bret_backend_change_*
 * no-ops for it rather than substituting a placeholder animation -- BRET.ASM
 * itself still picks the correct id; only its visual result is missing.
 *
 * wm_arcade_bret_callbacks_t.execute_walk is wired to wm/movement.h's
 * wm_execute_walk()+wm_bret_velocity_table (real MOVE_DIR/OBJ_CONTROL/
 * velocity behavior) AND WRESTLE.ASM::change_walk_anim's leg-cycle
 * reselection: hrt_leg_anims_table (BRET.ASM:2897, transcribed
 * value-for-value as wm_bret_leg_anim's leg_table) is indexed by MOVE_DIR/
 * FACING_DIR compass, all 12 of its hrt_walkM_fF_anim sequences are now
 * extracted, and change_anim1's own "restart only on change or END" rule is
 * preserved by start_if_new. FACING_DIR itself is real, not substituted:
 * wm/arcade/wm_arcade_closest.h's wm_arcade_update_newfacing translates
 * WRESTLE.ASM's update_newfacing (live NEW_FACING_DIR toward the opponent,
 * every tick, for every wrestler) and wm/movement.h's wm_execute_walk
 * translates set_rotate_anim's literal FACING_DIR=NEW_FACING_DIR catch-up
 * for the #zip/stance case -- exactly like the source, FACING_DIR stays
 * frozen at its last idle value while actually walking (change_walk_anim's
 * leg half never writes it), so facing one way while walking another is now
 * representable, just still using whichever leg sprite FACING_DIR/MOVE_DIR
 * already resolve to.
 *
 * change_walk_anim's TORSO reselection (hrt_torso_anims_table) is now fully
 * wired too, including all 12 off-diagonal turn-transition entries (e.g.
 * walking past an opponent whose relative side flips) -- see
 * wm_bret_torso_anim's own comment. Each of those 12 carries one or two
 * real ANI_SETFACING commands (HRTSEQ1.ASM), which is what actually
 * promotes FACING_DIR to NEW_FACING_DIR mid-walk; wm_bret_backend_tick
 * fires them at their real, hand-traced frame indices.
 *
 * set_rotate_anim's own turn-animation *pick* for the #zip/idle-turn case
 * (hrt_rotate_anims_table) is now wired too, via wm_bret_rotate_anim --
 * idle facing changes genuinely animate a turn now, not just update
 * FACING_DIR silently. wm_bret_backend_execute_walk also translates
 * execute_walk's own INTURN freeze (WRESTLE.ASM:5222-5252): movement and
 * reselection are held while a turn (leg or torso) is still playing, which
 * both matches the source and is load-bearing here -- without it, the
 * instant FACING_DIR catch-up would truncate every turn animation almost
 * immediately. Not translated: ANI_XFLIP inside the leg's own turn anims
 * (a purely cosmetic sprite-mirror mid-turn, no state consequence this
 * port tracks) and hrt_stand6_anim/hrt_stand8_anim as distinct symbols --
 * both are literal SUBR aliases of hrt_stand4_anim/hrt_stand2_anim
 * (HRTSEQ1.ASM:50,75) and rotate_table simply reuses those two pointers,
 * same reasoning already established for hrt_torso6/8_anim.
 */

typedef struct {
    wm_visual_state visual;
    wm_visual_state torso_visual;
    /* Set by the caller before each wm_arcade_move_bret() call; read back
       by the execute_walk callback, whose own signature has no opponent
       parameter. NULL is safe (wm_execute_walk treats it as "no opponent
       to check ONGROUND/DEAD against"). */
    wm_arcade_actor_t *opponent;
    /* Last id passed to wm_bret_backend_change_anim, and whether
       wm_bret_backend_tick has an ANIM.ASM ATTACK_ON active for it right
       now -- see wm_bret_backend_tick's own comment. */
    wm_arcade_bret_anim_id_t current_id;
    bool attack_active;
    /* Set once by the caller at match creation (wm/match.h's
       init_bret_backends: !has_human) and read by the adjust_health
       callback -- LIFEBAR.ASM adjust_health's "attract mode never dies"
       rule, see wm/arcade/wm_arcade_lifebar.h. */
    bool attract_mode;
    /* Set by the caller every tick (alongside wm_arcade_bret_env_t.pcnt,
       the same value) and read by the adjust_health callback for
       LIFEBAR.ASM adjust_health's LAST_DAMAGE timestamp update. */
    uint32_t pcnt;
} wm_bret_backend_actor;

void wm_bret_backend_init(wm_bret_backend_actor *bva);

/* NULL for any wm_arcade_bret_anim_id_t not listed above. */
const wm_visual_sequence *wm_bret_anim_sequence(wm_arcade_bret_anim_id_t id);

/* BRET.ASM:2897 hrt_leg_anims_table[move_compass][facing_compass] (both
   wm_convert_facing() 0-7 results). NULL if either is out of 0-7 range
   (in particular wm_convert_facing's -1 "zip" result). */
const wm_visual_sequence *wm_bret_leg_anim(int move_compass, int facing_compass);

/*
 * BRET.ASM:2981 hrt_torso_anims_table[FACING_DIR][NEW_FACING_DIR], both
 * indices folded to a 0-3 "diagonal" (wm_convert_facing() 0-7 result >> 1).
 * Fully wired: the table's diagonal (facing_diag==new_facing_diag, i.e.
 * FACING_DIR and NEW_FACING_DIR in the same compass quadrant -- not
 * mid-turn) resolves to hrt_torso2_anim/hrt_torso4_anim, and all 12
 * off-diagonal turn-transition entries resolve to their real
 * hrt_N_to_M_turn2_anim sequence (src/core/bret_backend.c's torso_table,
 * transcribed value-for-value including its real SUBR aliasing). NULL only
 * if either compass is out of 0-7 range (in particular wm_convert_facing's
 * -1 "zip"/never-moved result).
 */
const wm_visual_sequence *wm_bret_torso_anim(int facing_compass, int new_facing_compass);

/*
 * BRET.ASM:2871 hrt_rotate_anims_table[old FACING_DIR][NEW_FACING_DIR],
 * both indices folded to a 0-3 diagonal like wm_bret_torso_anim above.
 * Diagonal resolves to hrt_stand2_anim/hrt_stand4_anim (hrt_stand8_anim/
 * hrt_stand6_anim's own real SUBR aliases); all 12 off-diagonal entries
 * resolve to their real hrt_N_to_M_turn_anim sequence. Played only from
 * the #zip/idle-turn case (wm_bret_backend_execute_walk), using FACING_DIR
 * as it was *before* that tick's catch-up, not after -- see
 * wm_bret_backend_execute_walk's own comment. NULL if either compass is
 * out of 0-7 range.
 */
const wm_visual_sequence *wm_bret_rotate_anim(int old_facing_compass, int new_facing_compass);

/* wm_arcade_bret_callbacks_t.change_anim body. */
void wm_bret_backend_change_anim(wm_arcade_actor_t *actor,
                                 wm_arcade_bret_anim_id_t id, void *user);
/* wm_arcade_bret_callbacks_t.change_torso_anim body. */
void wm_bret_backend_change_torso_anim(wm_arcade_actor_t *actor,
                                       wm_arcade_bret_anim_id_t id, void *user);

/* Builds a callbacks struct with change_anim/change_torso_anim/execute_walk/
   adjust_health/user populated; every other BRET.ASM callback (sound,
   secret moves, ...) is intentionally left NULL -- see the file comment.
   Set bva->opponent and bva->attract_mode before calling
   wm_arcade_move_bret() with this. adjust_health only fires from
   mode_normal's I_WILL_DIE self-death case (BRET.ASM:1325-1350), which
   this port cannot currently reach on its own -- see
   wm/arcade/wm_arcade_lifebar.h. */
wm_arcade_bret_callbacks_t wm_bret_backend_callbacks(wm_bret_backend_actor *bva);

/* wm_arcade_bret_callbacks_t.execute_walk body: wm_execute_walk(actor,
   bva->opponent, wm_bret_velocity_table) plus leg-cycle and torso
   reselection -- see the file comment for exactly what that does and does
   not cover. */
void wm_bret_backend_execute_walk(wm_arcade_actor_t *actor, void *user);

/*
 * Advances both visual tracks by one source tick (does not move the actor
 * -- see wm_bret_backend_tick_position()), then fires ANIM.ASM's real
 * ATTACK_ON/ATTACK_ON_Z/ATTACK_OFF (wm/arcade/wm_arcade_anim_combat.h,
 * real and ctest-verified since fix38) for whichever of the 6 mapped
 * attack animations (wm_bret_anim_sequence) is playing, using literal
 * args hand-verified against each one's HRTSEQ2.ASM source (not the
 * shipped wm_visual_sequence frame data, which -- like every wlanim.py
 * --slice extraction -- linearly concatenates branchy source regardless
 * of which path a real hit/block/miss takes; the frame index the attack
 * box turns on at was traced against the raw source directly). actor may
 * be NULL (attack activation is then simply skipped).
 *
 * This sets a real actor's real attack_mode/attack_xoff/.../anim_mode
 * (WM_MODE_CHECKHIT) exactly as the source opcodes do.
 *
 * Also calls wm_arcade_set_hurt_box() every tick using
 * wm_bret_hurt_box_for_frame() below -- see that function's comment for
 * exactly what data backs it and what it does not claim to be.
 */
void wm_bret_backend_tick(wm_bret_backend_actor *bva, wm_arcade_actor_t *actor,
                          uint16_t round_tickcount);

/*
 * SYS.EQU's per-frame ANI3 hurt-box header (IANI3X/Y/Z/ID) is compiled from
 * WIMP artist source by a build step whose output (bretimg.tbl/bret.seq/
 * imgtbl.glo) is absent from the checked-in tree -- confirmed unrecoverable
 * from the ASM source and from the WIMP .IMG directory format alike (its
 * image entries carry xani/yani/width/height, never an authored hit-box
 * rectangle). This substitutes the frame's own real WIMP image geometry as
 * the hurt box, using the exact xani/yani/width/height
 * wm_bret_frame_geometry_find() resolves (wm/bret_frame_geometry.h -- the
 * same real numbers wm/bret_sprites.h's wm_source_sprite carries for
 * rendering, without that struct's pixel/palette payload):
 *   iani3x = -xani, iani3z = width   (wm_arcade_set_hurt_box's x1=x_int+
 *     iani3x, x2=x1+iani3z then reproduces the sprite's own screen-space
 *     footprint around the actor's x_int anchor, and mirrors correctly
 *     under WM_OBJ_FLIPH since xani is anchor-to-left-edge in image space)
 *   iani3y = -yani, iani3id = height (same construction on the Y axis)
 * This is a deliberate, documented substitute for the original hand-tuned
 * hit-box, not a recovery of it -- expect looser/tighter hit windows than
 * the arcade original per frame. Returns an all-zero box (a box exactly at
 * the actor's own point, effectively unhittable) if source_frame doesn't
 * resolve to a known sprite.
 */
wm_arcade_frame_box_t wm_bret_hurt_box_for_frame(const char *source_frame);

/* Not a source routine (see wm/movement.h's wm_integrate_position): applies
   actor->x_vel/z_vel to its position for one tick. */
void wm_bret_backend_tick_position(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif

#endif
