#ifndef WM_HUMAN_INPUT_H
#define WM_HUMAN_INPUT_H

#include "wm/arcade/wm_arcade_combat.h"
#include "wm/input.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform-input boundary, not a WRESTLE.ASM/BRET.ASM translation: maps the
 * portable wm_input_state (wm/input.h) the N64 frontend already produces
 * from real controller state onto the same but_val_cur/down/up,
 * stick_val_cur/down/up fields wm_arcade_drone_commit_inputs (DRONE.ASM)
 * writes for CPU-controlled actors, using the same GAME.EQU button/move
 * bit layout (wm/arcade/wm_arcade_combat_defs.h) both sides already share.
 *
 * README's existing N64 control table is the mapping: C-Left=light punch
 * (WM_BTN_PUNCH), C-Up=power punch (WM_BTN_SPUNCH), C-Right=light kick
 * (WM_BTN_KICK), C-Down=power kick (WM_BTN_SKICK), R=block (WM_BTN_BLOCK).
 *
 * One deliberate simplification: wm_input_state's four attack fields are
 * already edge-detected by the platform layer (see wm/input.h's own "edge"
 * comment), not "currently held" like arcade BUT_VAL_CUR really is. This
 * commits that edge as a single tick of "held", which correctly fires
 * but_val_down for attack initiation (all BRET.ASM's mode_normal attack
 * dispatch actually reads) but means a physically-held attack button does
 * not keep but_val_cur set for its whole hold duration the way the real
 * cabinet's would.
 */

typedef struct {
    uint16_t but; /* last committed WM_BTN_* bits, for edge detection */
    uint16_t joy; /* last committed WM_MOVE_* bits, for edge detection */
} wm_human_input_state;

void wm_human_input_init(wm_human_input_state *hs);

/* Writes actor->but_val_cur/down/up and stick_val_cur/down/up for one tick. */
void wm_human_input_commit(wm_arcade_actor_t *actor,
                           wm_human_input_state *hs,
                           const wm_input_state *input);

#ifdef __cplusplus
}
#endif

#endif
