#ifndef WM_ARCADE_COMBO_H
#define WM_ARCADE_COMBO_H

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The combo meter: ANIM.ASM's three combo opcodes and the LIFEBAR.ASM
 * routine behind them. There are TWO separate counters and they are easy
 * to confuse:
 *
 *   COMBO_COUNT     per wrestler, bumped by ANI_INC_COMBO. This is what
 *                   adjust_health reads, and a hit landed while it is
 *                   non-zero does FIXED damage of -max(10-COMBO_COUNT, 4)
 *                   instead of the normal scaled amount. Nothing in this
 *                   port incremented it before -- wm_arcade_lifebar.h said
 *                   so in its own notes -- so the whole combo damage rule
 *                   was dead code with a correct implementation.
 *
 *   PLT_COMBO_SIZE  per player, grown by ANI_ADD_MOVE through
 *                   LIFEBAR.ASM:750 ADD_TO_COMBO_COUNT. This is the meter
 *                   BAR, and at 16 it trips the super-combo flash.
 */

/*
 * LIFEBAR.ASM:750 SUBR ADD_TO_COMBO_COUNT, called by ANIM.ASM:4247
 * _ani_add_move with the move's identity bits.
 *
 * The source's own comment on it is "Adds first value each time! [why?]"
 * and the reason is worth recording, because it looks like a bug and is
 * not one to work around: the routine sets a3 to 1, and its "already
 * added" branch only skips re-setting a3 from a5 -- which `movk 1,a5` has
 * just made 1 as well. So COMBO_SIZE grows by exactly 1 every call
 * whether or not the move was already counted, and the COMBO_START
 * bookkeeping affects nothing but COMBO_START. Translated as written.
 *
 * Not translated: the royal-rumble branch that halves the amount and gives
 * it to player 0 (a mode this port has no equivalent of), and everything
 * from `#do_check` on -- picking a bar image out of WHICH_SIZE_BAR and
 * spawning the FLASH_COMBO process -- which is pure rendering. The
 * threshold itself IS translated, as combo_flash.
 */
void wm_arcade_add_to_combo_count(wm_arcade_actor_t *a, int32_t move_bits);

/* LIFEBAR.ASM's `movk 16,a6` super-combo threshold. */
#define WM_COMBO_SUPER_SIZE 16

#ifdef __cplusplus
}
#endif
#endif
