#ifndef WM_ARCADE_SHAKE_H
#define WM_ARCADE_SHAKE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * UTIL.ASM:2406 SHAKER2 -- "Shake screen as if an earthquake is in effect".
 *
 * This is not a renderer. It is a damped oscillator that adds an offset to
 * WORLDTLY once a tick and takes it off again the next, and every number in
 * it is in the source: a ten-entry sine table at 36-degree steps, and a
 * 64-entry table of e^-x over 0..7 scaled by 1024 (the source's own note:
 * "values of e^(-x) for values from 0 to 7, in 64 divisions... all values
 * are multiplied by 1024"). What a camera does with WORLDTLY belongs to a
 * DISPLAY.ASM port; producing the number does not.
 *
 * The operand is one value doing two jobs -- the source's own comment says
 * "A10 = # ticks to shake and power of shake" -- because the amplitude is
 * multiplied by the total tick count at the end.
 *
 * TMS34010 MPYS/MPYU place a 64-bit product in a register pair; with an
 * ODD destination register only the low 32 bits are kept, in that
 * register. Both multiplies here have an odd destination (a1), so both are
 * ordinary 32-bit products -- unlike RNDRNG0's `mpyu a1,a0`, whose EVEN
 * destination keeps the HIGH half (see wm/arcade/wmania_rng.h).
 */

/* UTIL.ASM:2383 `#last_entry equ 9`. The walk runs 9, 8, ... 1 and wraps
   back to 9, so entry 0 is only ever the table's own first value. */
#define WM_SHAKE_SINE_ENTRIES 10
#define WM_SHAKE_EXP_ENTRIES 64

extern const int16_t wm_shake_sine[WM_SHAKE_SINE_ENTRIES];
extern const int16_t wm_shake_exp[WM_SHAKE_EXP_ENTRIES];

typedef struct {
    /* `#SHK_ON`: is a shake in progress. */
    bool on;
    /* `#Y_ADJ`: the offset currently added to WORLDTLY. */
    int32_t y_adj;
    int32_t ticks_left;      /* a10 */
    int32_t total;           /* a11, the original tick count */
    int32_t index;           /* a9, into the sine table */
    /* @WORLDTLY, which the shake is the only thing here that moves. A
       camera would read this; nothing in this port does yet. */
    int32_t world_tly;
} wm_shake_state;

void wm_shake_init(wm_shake_state *s);

/*
 * SHAKER2 itself. A non-positive `ticks` does nothing (`jrn`/`jrz #done`).
 * A shake already running is ABORTED first -- its process killed and its
 * outstanding Y_ADJ taken back off WORLDTLY -- and then the new one starts,
 * so two shakes never accumulate.
 */
void wm_shake_start(wm_shake_state *s, int32_t ticks);

/*
 * One tick of the `#shaker` process: compute the offset, add it to
 * WORLDTLY, and on the NEXT tick take it off again before computing the
 * new one. Returns true while a shake is still running.
 */
bool wm_shake_tick(wm_shake_state *s);

#ifdef __cplusplus
}
#endif
#endif
