#ifndef WM_MOVEMENT_H
#define WM_MOVEMENT_H

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE.ASM::execute_walk (WRESTLE.ASM:5210), shared by every wrestler --
 * not Bret-specific, despite living next to wm/bret_backend.h in this pass.
 *
 * Covered, verified against WRESTLE.ASM directly:
 *   - convert_facing (WRESTLE.ASM:4506): WM_MOVE_* -> 8-way compass index.
 *   - set_velocities (WRESTLE.ASM:5435): per-wrestler (X,Z) velocity table
 *     indexed by compass direction, WALK_FAST / opponent-ONGROUND-or-DEAD
 *     1.5x (GRND_MULT) boost, and the 0.9x (MULT) reduction applied when
 *     moving backward relative to facing (X and Z checked independently).
 *   - execute_walk's 9-way MOVE_DIR dispatch (WRESTLE.ASM:5254 walk_table):
 *     sets MOVE_DIR and toggles the M_FLIPH OBJ_CONTROL bit exactly as the
 *     8 real-direction handlers do (up/down do not touch OBJ_CONTROL).
 *   - The #zip/stance case's one line of real state update: set_rotate_anim
 *     (WRESTLE.ASM:5062, called from WRESTLE.ASM:5286 `callr set_rotate_anim
 *     ;or stance`) copies NEW_FACING_DIR into FACING_DIR synchronously and
 *     unconditionally every idle tick, before/regardless of which turn
 *     animation it separately selects -- see wm/arcade/wm_arcade_closest.h's
 *     wm_arcade_update_newfacing for what feeds NEW_FACING_DIR.
 *
 * Covered, but one layer up in wm/bret_backend.h instead of here, since it
 * needs the visual-track state this generic, actor-only function doesn't
 * have:
 *   - execute_walk's own top-of-function INTURN freeze (WRESTLE.ASM:5222-
 *     5252): while a turn animation (leg or torso) is still playing,
 *     movement and reselection are held, "treat it like UNINT". Real and
 *     load-bearing -- see wm_bret_backend_execute_walk's own comment.
 *   - set_rotate_anim's own turn-animation *pick* for the #zip/stance case
 *     (hrt_rotate_anims_table, wm_bret_rotate_anim) and change_walk_anim's
 *     leg/torso reselection for real movement (hrt_leg_anims_table/
 *     hrt_torso_anims_table, including all 12 off-diagonal torso
 *     turn-transition entries and their real ANI_SETFACING promotion) --
 *     all wired in wm/bret_backend.h.
 *
 * NOT covered:
 *   - down_right/down_left's CAN_MOVE_DIR redirect (falls back to pure
 *     #right/#left when downward movement is blocked). CAN_MOVE_DIR is
 *     computed by the unported ring-boundary/keep_onscreen system, so this
 *     always takes the un-redirected diagonal.
 *   - Position integration. execute_walk only ever *sets* OBJ_XVEL/
 *     OBJ_ZVEL; applying that to OBJ_XPOS/OBJ_ZPOS every frame is done by
 *     ANIM.ASM's generic object mover, which this port has not translated.
 *     wm_integrate_position() is therefore NOT a source routine -- it's the
 *     standard 16.16 velocity integration every port needs in its place,
 *     kept separate and clearly not attributed as a WRESTLE.ASM/ANIM.ASM
 *     translation.
 */

/* 0=up, 1=up-right, 2=right, 3=down-right, 4=down, 5=down-left, 6=left,
   7=up-left. -1 for a move_dir_bits value convert_facing's table maps to
   "zip" (0, 3, 11-15): callers should treat that as "no movement". */
int wm_convert_facing(int32_t move_dir_bits);

typedef struct {
    int32_t x, z; /* 16.16 fixed, WRESTLE.ASM order (compass 0-7). */
} wm_move_velocity_entry;

/* BRET.ASM:2848 hrt_velocity_table, using the already-ported
   WM_BRET_WALK_VEL/WM_BRET_WALK_DVEL (wm/arcade/wm_arcade_bret.h). */
extern const wm_move_velocity_entry wm_bret_velocity_table[8];

/* WRESTLE.ASM::set_velocities. table must have 8 entries, compass-ordered. */
void wm_set_velocities(wm_arcade_actor_t *actor,
                       const wm_arcade_actor_t *opponent,
                       const wm_move_velocity_entry table[8]);

/* WRESTLE.ASM::execute_walk, animation reselection excluded -- see above.
   actor->move_dir (already mirrored from stick_val_cur by the caller, as
   BRET.ASM's mode_normal does) selects the walk_table entry. */
void wm_execute_walk(wm_arcade_actor_t *actor,
                     const wm_arcade_actor_t *opponent,
                     const wm_move_velocity_entry table[8]);

/* Not a source routine -- see the file comment. */
void wm_integrate_position(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif

#endif
