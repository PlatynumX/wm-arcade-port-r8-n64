#ifndef WMANIA_RING_CLIMB_H
#define WMANIA_RING_CLIMB_H

#include "wmania_ring_geometry.h"
#include "wmania_ring_player_fields.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Direct GAME.EQU movement values used by WRESTLE2.ASM.
 */
enum {
    WM_RING_MOVE_ZIP = 0,
    WM_RING_MOVE_UP = 1,
    WM_RING_MOVE_DOWN = 2,
    WM_RING_MOVE_LEFT = 4,
    WM_RING_MOVE_UP_LEFT = 5,
    WM_RING_MOVE_DOWN_LEFT = 6,
    WM_RING_MOVE_RIGHT = 8,
    WM_RING_MOVE_UP_RIGHT = 9,
    WM_RING_MOVE_DOWN_RIGHT = 10
};

enum {
    WM_RING_MOVE_UP_BIT = 0,
    WM_RING_MOVE_DOWN_BIT = 1,
    WM_RING_MOVE_LEFT_BIT = 2,
    WM_RING_MOVE_RIGHT_BIT = 3
};

#define WM_RING_IDIOT_COUNT 21u

typedef struct {
    bool active;

    uint8_t wrestler_num;
    uint8_t player_num;
    int16_t player_side;

    int16_t player_mode;
    uint16_t anim_mode;
    uint16_t status_flags;

    int16_t inring;
    int16_t climbing_thru;
    int16_t getup_time;

    int16_t x_int;
    int16_t z_int;
    int32_t z_fp16;
    int16_t coll_x1;
    int16_t coll_x2;

    uint16_t move_dir;
    uint16_t stick_val_cur;
    uint16_t but_val_cur;

    uint16_t facing_dir;
    uint16_t new_facing_dir;

    uint32_t climb_start;
    uint32_t climb_last;

    /* Source animation label currently in ANIBASE, for zombie duplicate check. */
    const char *animbase_label;
} WmRingClimbPlayer;

typedef enum {
    WM_RING_CLIMB_ACTION_NONE = 0,
    WM_RING_CLIMB_ACTION_START_ANIMATION,
    WM_RING_CLIMB_ACTION_ROTATE_THEN_CONTINUE,
    WM_RING_CLIMB_ACTION_SOURCE_NULL_ANIMATION,
    WM_RING_CLIMB_ACTION_SOURCE_QUIRK_INPUT_REQUIRED
} WmRingClimbAction;

typedef enum {
    WM_RING_CLIMB_CONT_NONE = 0,
    WM_RING_CLIMB_CONT_TURNBUCKLE,
    WM_RING_CLIMB_CONT_OUT_SIDE,
    WM_RING_CLIMB_CONT_IN_SIDE
} WmRingClimbContinuation;

typedef struct {
    WmRingClimbAction action;
    WmRingClimbContinuation continuation;
    const char *source_animation_label;
    uint16_t target_facing;
} WmRingClimbResult;

/*
 * The current WRESTLE2.ASM ck_climb_out_side contains a shipped source
 * quirk:
 *
 *   callr any_opp_outside
 *   ...
 *   move *a0(CLIMBING_THRU),a0
 *
 * any_opp_outside explicitly trashes a0 and leaves it as the post-incremented
 * process_ptrs cursor, not a wrestler process pointer. Reproducing this exact
 * memory read requires the merge/runtime to provide the word read from:
 *
 *   process_ptrs[next_slot_after_found] + CLIMBING_THRU bit offset
 *
 * No "intended" replacement is invented here.
 */
typedef bool (*WmRingSourceQuirkReadFn)(
    void *user,
    size_t next_process_slot,
    int16_t *value_out);

bool wm_ring_any_opp_outside(
    const WmRingClimbPlayer *self,
    const WmRingClimbPlayer *players,
    size_t player_count,
    size_t *source_a0_next_slot);

bool wm_ring_idiot_check(
    WmRingClimbPlayer *player,
    uint32_t pcnt);

WmRingClimbResult wm_ring_climb_turnbuckle(
    WmRingClimbPlayer *player,
    const WmRingClimbPlayer *players,
    size_t player_count,
    int16_t rope_x);

WmRingClimbResult wm_ring_ck_climb_out_bot(
    WmRingClimbPlayer *player,
    const WmRingClimbPlayer *players,
    size_t player_count,
    uint32_t pcnt);

WmRingClimbResult wm_ring_ck_climb_in_top(
    WmRingClimbPlayer *player,
    uint32_t pcnt);

WmRingClimbResult wm_ring_ck_climb_out_top(
    WmRingClimbPlayer *player,
    const WmRingClimbPlayer *players,
    size_t player_count,
    uint32_t pcnt);

WmRingClimbResult wm_ring_ck_climb_in_bot(
    WmRingClimbPlayer *player,
    uint32_t pcnt);

WmRingClimbResult wm_ring_ck_climb_out_side(
    WmRingClimbPlayer *player,
    const WmRingClimbPlayer *players,
    size_t player_count,
    uint32_t pcnt,
    int16_t rope_x,
    WmRingSourceQuirkReadFn source_quirk_read,
    void *source_quirk_user);

WmRingClimbResult wm_ring_ck_climb_in_side(
    WmRingClimbPlayer *player,
    uint32_t pcnt,
    int16_t calc_line_x_result);

/*
 * Execute the source CODE_ADDR continuation after a rotate animation ends.
 */
WmRingClimbResult wm_ring_climb_continue(
    WmRingClimbPlayer *player,
    WmRingClimbContinuation continuation);

/* Exact source table labels. */
extern const uint8_t wm_ring_face_turnbuckle[10];
extern const char *const wm_ring_climb_up_anims[10];
extern const char *const wm_ring_climbthru_bot_anims[10];
extern const char *const wm_ring_climbthru_top_anims[10];
extern const char *const wm_ring_climbin_bot_anims[10];
extern const char *const wm_ring_climbin_top_anims[10];
extern const char *const wm_ring_climbthru_side_anims[10];
extern const char *const wm_ring_climbin_side_anims[10];
extern const char *const wm_ring_rollthru_top_anims[9];

#ifdef __cplusplus
}
#endif

#endif
