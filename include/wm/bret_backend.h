#ifndef WM_BRET_BACKEND_H
#define WM_BRET_BACKEND_H

#include "wm/arcade/wm_arcade_bret.h"
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
 *   - WM_BRET_ANIM_SUPER_KICK2 (power kick, hrt_2_super_kick_anim --
 *     hrt_4_super_kick_anim exists in HRTSEQ2.ASM:1335 but has not been
 *     extracted yet, so WM_BRET_ANIM_SUPER_KICK4 stays unmapped)
 *
 * Every other wm_arcade_bret_anim_id_t (grapples, pins, finishers, running
 * moves, turnbuckle moves, ...) resolves to NULL and wm_bret_backend_change_*
 * no-ops for it rather than substituting a placeholder animation -- BRET.ASM
 * itself still picks the correct id; only its visual result is missing.
 *
 * wm_arcade_bret_callbacks_t.execute_walk is wired to wm/movement.h's
 * wm_execute_walk()+wm_bret_velocity_table (real MOVE_DIR/OBJ_CONTROL/
 * velocity behavior) AND, while actually moving, WRESTLE.ASM::
 * change_walk_anim's leg-cycle reselection: hrt_leg_anims_table
 * (BRET.ASM:2897, transcribed value-for-value as wm_bret_leg_anim's
 * leg_table) is indexed by MOVE_DIR/FACING_DIR compass, all 12 of its
 * hrt_walkM_fF_anim sequences are now extracted, and change_anim1's own
 * "restart only on change or END" rule is preserved by start_if_new. See
 * wm_bret_backend_execute_walk's own comment for the one real gap: real
 * FACING_DIR tracking needs a not-yet-located shared "compute
 * NEW_FACING_DIR" routine plus set_rotate_anim's copy step, neither
 * ported, so FACING_DIR is substituted with MOVE_DIR while moving --
 * correct for straight walking, the only case reachable today.
 *
 * change_walk_anim's TORSO reselection (hrt_torso_anims_table) and
 * set_rotate_anim/change_anim1 (the #zip/idle-turn case, using
 * hrt_rotate_anims_table's 12 turn-transition sequences plus
 * hrt_stand6/8_anim) are still not translated -- the torso stays on
 * whatever wm_bret_backend_ani_init set, and idle facing changes don't
 * animate a turn.
 */

typedef struct {
    wm_visual_state visual;
    wm_visual_state torso_visual;
    /* Set by the caller before each wm_arcade_move_bret() call; read back
       by the execute_walk callback, whose own signature has no opponent
       parameter. NULL is safe (wm_execute_walk treats it as "no opponent
       to check ONGROUND/DEAD against"). */
    wm_arcade_actor_t *opponent;
} wm_bret_backend_actor;

void wm_bret_backend_init(wm_bret_backend_actor *bva);

/* NULL for any wm_arcade_bret_anim_id_t not listed above. */
const wm_visual_sequence *wm_bret_anim_sequence(wm_arcade_bret_anim_id_t id);

/* BRET.ASM:2897 hrt_leg_anims_table[move_compass][facing_compass] (both
   wm_convert_facing() 0-7 results). NULL if either is out of 0-7 range
   (in particular wm_convert_facing's -1 "zip" result). */
const wm_visual_sequence *wm_bret_leg_anim(int move_compass, int facing_compass);

/* wm_arcade_bret_callbacks_t.change_anim body. */
void wm_bret_backend_change_anim(wm_arcade_actor_t *actor,
                                 wm_arcade_bret_anim_id_t id, void *user);
/* wm_arcade_bret_callbacks_t.change_torso_anim body. */
void wm_bret_backend_change_torso_anim(wm_arcade_actor_t *actor,
                                       wm_arcade_bret_anim_id_t id, void *user);

/* Builds a callbacks struct with change_anim/change_torso_anim/execute_walk/
   user populated; every other BRET.ASM callback (sound, secret moves, ...)
   is intentionally left NULL -- see the file comment. Set bva->opponent
   before calling wm_arcade_move_bret() with this. */
wm_arcade_bret_callbacks_t wm_bret_backend_callbacks(wm_bret_backend_actor *bva);

/* wm_arcade_bret_callbacks_t.execute_walk body: wm_execute_walk(actor,
   bva->opponent, wm_bret_velocity_table) plus leg-cycle reselection --
   see the file comment for exactly what that does and does not cover. */
void wm_bret_backend_execute_walk(wm_arcade_actor_t *actor, void *user);

/* Advances both visual tracks by one source tick. Does not move the actor
   -- see wm_bret_backend_tick_position(). */
void wm_bret_backend_tick(wm_bret_backend_actor *bva);

/* Not a source routine (see wm/movement.h's wm_integrate_position): applies
   actor->x_vel/z_vel to its position for one tick. */
void wm_bret_backend_tick_position(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif

#endif
