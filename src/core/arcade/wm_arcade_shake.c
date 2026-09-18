/*
 * UTIL.ASM:2406 SHAKER2 and its `#shaker` process. See
 * wm/arcade/wm_arcade_shake.h for what this is and is not.
 */
#include "wm/arcade/wm_arcade_shake.h"

#include <string.h>

/* UTIL.ASM:2372 #sine_table, 36 degrees a step. */
const int16_t wm_shake_sine[WM_SHAKE_SINE_ENTRIES] = {
    -601, -973, -973, -601, 0, 602, 974, 974, 602, 0
};

/* UTIL.ASM:2388 #exp_table -- e^-x over 0..7 in 64 steps, times 1024. */
const int16_t wm_shake_exp[WM_SHAKE_EXP_ENTRIES] = {
    1024, 945, 873, 807, 745, 688, 636, 587,
     542, 501, 463, 427, 395, 364, 337, 311,
     287, 265, 245, 226, 209, 193, 178, 165,
     152, 140, 130, 120, 110, 102,  94,  87,
      80,  74,  68,  63,  58,  54,  50,  46,
      42,  39,  36,  33,  31,  28,  26,  24,
      22,  20,  19,  17,  16,  15,  14,  13,
      12,  11,  10,   9,   8,   8,   7,   6
};

void wm_shake_init(wm_shake_state *s) {
    if (s) memset(s, 0, sizeof(*s));
}

void wm_shake_start(wm_shake_state *s, int32_t ticks) {
    if (!s) return;
    if (ticks <= 0) return;              /* `jrn #done` / `jrz #done` */

    if (s->on) {
        /*
         * "abort shake currently in progress": KIL1C the process and put
         * WORLDTLY back where it was, so a second shake starting mid-way
         * through the first does not leave the camera offset behind.
         */
        s->world_tly -= s->y_adj;
        s->y_adj = 0;
    }
    s->on = true;
    s->ticks_left = ticks;
    s->total = ticks;                    /* `move a10,a11` */
    s->index = WM_SHAKE_SINE_ENTRIES - 1;   /* `movi #last_entry,a9` */
}

bool wm_shake_tick(wm_shake_state *s) {
    int32_t sine, exp_i, amp;
    if (!s || !s->on) return false;

    /* The tail of the previous pass: "undo it" before this one's "apply
       it", which is what makes the offset a one-tick pulse. */
    if (s->y_adj) {
        s->world_tly -= s->y_adj;
        s->y_adj = 0;
        if (--s->ticks_left <= 0) {      /* `dsj a10,#loop` falling out */
            s->on = false;
            s->index = 0;
            return false;
        }
    }

    sine = wm_shake_sine[s->index];

    /* "index is 64 - (64 * a10 / a11)" -- full power at the start, where
       a10 still equals a11, damping to nothing as a10 counts down. */
    exp_i = 64 - (s->ticks_left * 64) / s->total;
    if (exp_i < 0) exp_i = 0;
    if (exp_i >= WM_SHAKE_EXP_ENTRIES) exp_i = WM_SHAKE_EXP_ENTRIES - 1;

    /* `mpys a0,a1 / sra 5,a1 / mpyu a11,a1` -- both destinations are odd,
       so both are plain 32-bit products. */
    amp = (sine * wm_shake_exp[exp_i]) >> 5;
    amp = amp * s->total;

    s->y_adj = amp;

    /* `dsj a9,#table_ok / movi #last_entry,a9` -- 9, 8, ... 1, then 9
       again. The step that would reach 0 resets instead. */
    if (--s->index == 0) s->index = WM_SHAKE_SINE_ENTRIES - 1;

    s->world_tly += amp;
    return true;
}
