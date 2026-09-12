#ifndef WM_ROLL_FRAMES_H
#define WM_ROLL_FRAMES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE2.ASM:1354 #roll_table -- nine slots in WRESTLERNUM order
 * (GAME.EQU W_*), Adam Bomb's cut seventh written as a plain `.long 0`.
 */
#define WM_ROLL_SLOTS 9

/*
 * One `xxx_roll_frames` table: WRESTLE2.ASM:1290 do_roll adds `speed` to
 * ROLL_POS (mod 256), multiplies the result by `multiplier` and shifts
 * right 16 to index `frames`. The frame lists are NOT in ascending order
 * -- Bret's runs FR1 then FR13 down to FR2, because that is the direction
 * his artwork rolls.
 *
 * `multiplier` is written in the source as an expression, e.g.
 * `10000h*12/255`, and the assembler's divide truncates: Bret's comes out
 * at 3084, so the largest index 255 can reach is 11 and his thirteenth
 * frame is unreachable. That is the shipped data, kept as it is.
 */
typedef struct {
    int32_t speed;
    int32_t zvel;                 /* 16.16, applied to OBJ_ZVEL */
    int32_t multiplier;
    const char *const *frames;
    size_t frame_count;
} wm_roll_table;

extern const wm_roll_table wm_roll_tables[WM_ROLL_SLOTS];

#ifdef __cplusplus
}
#endif
#endif
