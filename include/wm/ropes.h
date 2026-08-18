#ifndef WM_ROPES_H
#define WM_ROPES_H
#include <stdbool.h>
#include <stdint.h>
#include "wm/fixed.h"

/* The original ROPES.ASM has four rope process groups: front/back/left/right. */
typedef enum {
    WM_ROPE_FRONT = 0,
    WM_ROPE_BACK  = 1,
    WM_ROPE_LEFT  = 2,
    WM_ROPE_RIGHT = 3,
    WM_ROPE_GROUP_COUNT = 4
} wm_rope_group;

typedef enum {
    WM_ROPE_CMD_0 = 0,
    WM_ROPE_CMD_1 = 1,
    WM_ROPE_CMD_2 = 2,
    WM_ROPE_CMD_3 = 3,
    WM_ROPE_CMD_4 = 4,
    WM_ROPE_CMD_5 = 5,
    WM_ROPE_COMMAND_COUNT = 6
} wm_rope_command_id;

typedef struct {
    bool present;
    uint8_t action;
    uint8_t position_or_magnitude;
    wm_fix16 wrestler_z;
    uint32_t generation;
} wm_rope_state;

typedef struct {
    wm_rope_state group[WM_ROPE_GROUP_COUNT];
} wm_rope_system;

void wm_ropes_init(wm_rope_system *r);
bool wm_rope_command(wm_rope_system *r, wm_rope_group group,
                     uint8_t action, uint8_t position_or_magnitude,
                     wm_fix16 wrestler_z);

#endif
