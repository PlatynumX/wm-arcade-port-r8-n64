#ifndef WMANIA_ROPE_COMMAND_H
#define WMANIA_ROPE_COMMAND_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Direct GAME.EQU values. */
typedef enum {
    WM_ROPE_FRONT = 0,
    WM_ROPE_BACK  = 1,
    WM_ROPE_LEFT  = 2,
    WM_ROPE_RIGHT = 3
} WmRopeBank;

typedef enum {
    WM_ROPE_TOP    = 0,
    WM_ROPE_MIDDLE = 1,
    WM_ROPE_BOTTOM = 2
} WmRopeStrand;

typedef enum {
    WM_ROPE_Z_HIGH = 0,
    WM_ROPE_Z_NORM = 1
} WmRopeZAction;

typedef enum {
    WM_ROPE_BOUNCE_UD = 0,
    WM_ROPE_BOUNCE_IO = 1,
    WM_ROPE_SIDE_SPRING = 2,
    WM_ROPE_DOWN_SPRING = 3,
    WM_ROPE_SIDE_SPRING_RELEASE = 4,
    WM_ROPE_DOWN_SPRING_RELEASE = 5,
    WM_ROPE_COMMAND_COUNT = 6
} WmRopeAction;

/* Direct ROPES.ASM priorities. */
#define WM_ROPE_SIDE_SPRING_PRIORITY 10u
#define WM_ROPE_DOWN_SPRING_PRIORITY 9u
#define WM_ROPE_SHAKE_PRIORITY 5u

/* ROPES.ASM #LANE_WIDTH = (RING_BOT-RING_TOP)/5 = 64. */
#define WM_ROPE_LANE_WIDTH 64

/* set_rope_z hardcoded "high" second-half Z. */
#define WM_ROPE_HIGH_SECOND_HALF_Z 0x15a9u

typedef struct {
    WmRopeBank bank;
    WmRopeAction action;

    /*
     * Literal script-table label selected by rope_command.
     * This is a source identity, not a recreated animation.
     */
    const char *source_script_table;

    /* 5 for shake, 9 for down spring, 10 for side spring. */
    uint16_t priority;

    /*
     * For side/down spring only: source threshold row 0..4.
     * 0xff for commands that do not use a Z lane.
     */
    uint8_t lane;

    /*
     * Exact a2 input retained for merge/debug:
     * - 0..3 magnitude for BOUNCE_UD
     * - 0..5 rope position for side/down spring
     */
    uint8_t selector;
} WmRopeCommand;

/*
 * Direct translation of the table-routing part of rope_command.
 *
 * wrestler_z_fp16 is the arcade fixed-point Z used by the side/down spring
 * threshold table.  For non-spring commands it is ignored.
 *
 * Returns false for the same table-invalid cases:
 * - front/back action other than BOUNCE_UD
 * - invalid magnitude/position
 * - the source's NULL sspring entry at selector 5
 */
bool wm_rope_resolve_command(
    WmRopeBank bank,
    WmRopeAction action,
    uint8_t selector,
    int32_t wrestler_z_fp16,
    WmRopeCommand *out);

/*
 * Direct source threshold behavior.  Threshold comparisons are strict:
 * z < threshold picks that row; equality advances to the next row.
 */
uint8_t wm_rope_side_lane_from_z_fp16(int32_t wrestler_z_fp16);

/*
 * new_command_wake priority rule:
 * existing priority > incoming priority => skip;
 * incoming >= existing => replace.
 */
bool wm_rope_priority_accepts(
    uint16_t existing_priority,
    uint16_t incoming_priority);

/*
 * Direct set_rope_z result for the second rope half.
 * RZ_HIGH -> 0x15a9
 * RZ_NORM -> copy first-half current OZPOS
 */
uint16_t wm_rope_second_half_z(
    uint16_t first_half_current_z,
    WmRopeZAction action);

#ifdef __cplusplus
}
#endif

#endif
