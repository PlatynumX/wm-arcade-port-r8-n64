#ifndef WMANIA_RING_ONSCREEN_H
#define WMANIA_RING_ONSCREEN_H

#include "wmania_ring_geometry.h"
#include "wmania_ring_player_fields.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Recovered from the rev 1.30 arcade program ROM because the source body is
 * absent from the historical source snapshot.
 *
 * TMS34010 bit addresses:
 */
#define WM_RING_KEEP_ONSCREEN_CALLSITE       0xFF85CAB0u
#define WM_RING_KEEP_ONSCREEN_ROM_ENTRY      0xFF8711C0u
#define WM_RING_KEEP_ONSCREEN_PLAYER_HELPER  0xFF871500u
#define WM_RING_KEEP_ONSCREEN_RUN_HELPER     0xFF86E4B0u
#define WM_RING_KEEP_ONSCREEN_GETUP_ENTRY    0xFF86D7D0u

/* Source/ROM constants. */
#define WM_RING_KEEP_REQUIRED_OLD_PSTATUS 3
#define WM_RING_KEEP_WORLD_HALF_WIDTH     200
#define WM_RING_KEEP_SAFE_HALF_WIDTH      185
#define WM_RING_KEEP_RING_CENTER_COMPARE  WM_RING_X_CENTER

/* GAME.EQU: GETUP_PID .equ 12Bh */
#define WM_RING_KEEP_GETUP_PID 0x012Bu

typedef struct {
    /* OBJ_XPOSINT / OBJ_XVEL */
    int16_t x_int;
    int32_t xvel_fp;

    /* Ring/player state */
    int16_t inring;
    int16_t player_mode;
    uint16_t animode;
    int16_t climbing_thru;

    /*
     * Fields read only by the run-stop helper at 0xFF86E4B0:
     * GETUP_TIME, PLYR_DIZZY, METER_PROC.
     */
    int16_t getup_time;
    int16_t player_dizzy;

    bool meter_proc_exists;

    /*
     * Saved A8/A9/A10 copied from the existing METER_PROC process before
     * CREATE(GETUP_PID, 0xFF86D7D0).
     */
    uint32_t meter_saved_a8;
    uint32_t meter_saved_a9;
    uint32_t meter_saved_a10;
} WmRingOnscreenPlayer;

typedef struct {
    /*
     * Global OLD_PSTATUS. The ROM routine returns unless it equals 3.
     */
    int16_t old_pstatus;

    /*
     * Integer half of DISPLAY.ASM WORLDTLX (16:16).
     * The ROM adds 200 to obtain current world-screen center X.
     */
    int16_t worldtlx_int;

    /*
     * Global allow_offscrn.
     * If nonzero, the ROM decrements it. It returns while the decremented
     * value is still nonzero; value 1 therefore becomes 0 and confinement
     * resumes on that same call.
     */
    uint16_t allow_offscrn;

    /*
     * ROM reads process_ptrs[0] and process_ptrs[1] only.
     * OLD_PSTATUS==3 is expected to guarantee valid arcade pointers.
     */
    WmRingOnscreenPlayer *p1;
    WmRingOnscreenPlayer *p2;
} WmRingOnscreenState;

typedef struct {
    bool create;

    uint16_t pid;
    uint32_t entry_bit_address;

    uint32_t a8;
    uint32_t a9;
    uint32_t a10;
} WmRingGetupSpawn;

typedef struct {
    int32_t screen_center_x;
    int32_t left_limit;
    int32_t right_limit;

    /*
     * The shipped ROM compares center against RING_X_CENTER (0x432), but
     * both branch paths execute the same +/-0xB9 arithmetic. This records
     * which raw branch would have been taken without changing behavior.
     */
    bool source_center_gt_ring_center_branch;

    bool stopped_p1;
    bool stopped_p2;

    WmRingGetupSpawn p1_getup_spawn;
    WmRingGetupSpawn p2_getup_spawn;
} WmRingOnscreenEvents;

/*
 * Exact functional bounds recovered from ROM:
 *
 * center = WORLDTLX.int + 200
 * left   = center - 185
 * right  = center + 185
 *
 * This leaves a 15-pixel margin inside a 400-pixel-wide view.
 */
void wm_ring_keep_onscreen_bounds(
    int16_t worldtlx_int,
    int32_t *center_out,
    int32_t *left_out,
    int32_t *right_out,
    bool *source_center_gt_ring_center_branch_out);

/*
 * Direct semantic translation of the ROM routine at 0xFF8711C0.
 *
 * Important: it does NOT clamp player X position. It only zeroes outward
 * X velocity when a player has reached/passed the safe X limit.
 */
WmRingOnscreenEvents wm_ring_keep_onscreen_tick(
    WmRingOnscreenState *state);

#ifdef __cplusplus
}
#endif

#endif
