#ifndef WM_ANIM_FRAME_COMMANDS_H
#define WM_ANIM_FRAME_COMMANDS_H

#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"

/*
 * ANIM.ASM's inline motion commands, generated per animation by
 * tools/wlcommands.py with the 0-based index of the frame each one
 * precedes.
 *
 * A wired attack has always played its real frames and set its real attack
 * box, but never moved: these are the commands that move it. Rows are keyed
 * on the animation's own source label rather than a port-side enum, so the
 * table serves any wrestler whose sequences are extracted.
 *
 * Real semantics, from ANIM.ASM:
 *   ZEROVELS      (:1537-ish) x, y and z velocity to 0
 *   ZERO_XZVELS   (:1537)     x and z only, y untouched
 *   SET_XVEL      (:1626)     absolute, or negated when the relevant
 *                             direction bit is clear -- mode picks which
 *                             direction that is (AM_*, ANIM.EQU:167-170)
 *   SET_YVEL      (:430)      absolute; y has no relative form
 *   SET_ZVEL      (:1829)     absolute, or negated per mode
 *   MIN_YVEL      (:23 op)    y = max(y, value); never lowers it
 *   FRICTION      (:22 op)    sets OBJ_FRICTION and turns MODE_FRICTION on
 *   OFFSET        (:21 op)    instant position nudge; x is negated when not
 *                             facing right
 */
typedef enum {
    WM_ANICMD_ZEROVELS = 0,
    WM_ANICMD_ZERO_XZVELS,
    WM_ANICMD_SET_XVEL,
    WM_ANICMD_SET_YVEL,
    WM_ANICMD_SET_ZVEL,
    WM_ANICMD_MIN_YVEL,
    WM_ANICMD_FRICTION,
    WM_ANICMD_OFFSET
} wm_anim_frame_command_kind;

typedef struct {
    const char *source_label;
    uint16_t frame_index;
    uint8_t kind;
    uint8_t mode;      /* ANIM.EQU AM_ABS/AM_FACE_REL/AM_HIT_REL/AM_NEWFACE_REL */
    int32_t a, b, c;
} wm_anim_frame_command;

const wm_anim_frame_command *wm_anim_frame_commands(size_t *count);

/* Apply every command this animation has at `frame_index`. Safe to call for
   an animation with none. */
void wm_anim_apply_frame_commands(wm_arcade_actor_t *actor,
                                  const char *source_label,
                                  size_t frame_index);

#endif
