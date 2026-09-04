#ifndef WM_ARCADE_LIFEBAR_H
#define WM_ARCADE_LIFEBAR_H

#include <stdbool.h>
#include <stdint.h>
#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* LIFEBAR.ASM:135 LIFE_MAX equ 163 (green pixels in life bar). */
#define WM_ARCADE_LIFE_MAX 163

/*
 * LIFEBAR.ASM::adjust_health's damage-application tail (LIFEBAR.ASM:
 * 1547-1670), shared by every real caller of it in this port: the REACT1.ASM
 * hit path (wm_arcade_wrestler_hit -> wm_match's wrestler_hit callback) and
 * BRET.ASM mode_normal's own "I_WILL_DIE amidst a combo" self-death case
 * (wm_arcade_move_bret, via wm_arcade_bret_callbacks_t.adjust_health) --
 * both call the same shared SUBR in the original, so this is one real
 * function rather than two copies.
 *
 * Translated:
 *   life = victim->life + delta, clamped to [0, LIFE_MAX], except:
 *     - LIFEBAR.ASM:1561-1569 "fudge": a killing hit of 20+ points that
 *       doesn't overkill by 10 or more points bumps life to 5 instead of 0.
 *     - LIFEBAR.ASM:1578-1581 "if we're in attract mode, don't die!":
 *       attract_mode snaps life back to LIFE_MAX instead of letting it
 *       reach 0.
 *     - LIFEBAR.ASM:1659-1670: when life would genuinely reach 0 (neither
 *       fudged nor attract-mode-saved) and damage_source's COMBO_COUNT is
 *       nonzero, death is deferred: life becomes 1 and victim->i_will_die
 *       is set instead, matching mode_normal's own I_WILL_DIE resolution
 *       (BRET.ASM:1325-1350, translated in every wm_arcade_move_* wrestler
 *       dispatcher already). This is real but currently unreachable in this
 *       port: nothing yet increments combo_count anywhere.
 *     - LIFEBAR.ASM:1723-1725: a genuine death (life reaches 0, not
 *       deferred) sets player_mode to WM_PMODE_DEAD and turns off further
 *       hit checks (wm_arcade_wrestler_collisions_off, matching SETMODE
 *       DEAD + calla wres_collis_off).
 *   - LIFEBAR.ASM:1593-1595 "update LAST_DAMAGE": unconditionally, on every
 *     call (fudged, attract-saved, genuinely dying, or a plain clamp
 *     alike), victim->last_damage is stamped with pcnt. This is what makes
 *     wm_arcade_wrestler_hit's own reduced_damage window real: it already
 *     read last_damage (REACT1.ASM's "elapsed_word(pcnt, last_damage)<=50"
 *     rapid-hit check) but nothing ever wrote it before this, so repeated
 *     attacks always dealt full_damage regardless of timing.
 *
 * NOT translated: LIFEBAR.ASM's DAM_MULT/COMBO_COUNT-based damage
 * multiplier and damage_mod_table/speed_adjustment scaling (both happen
 * earlier in adjust_health, before the range check this function starts
 * from), CHECK_COMBO_GO (an unlocated global "is the combo mechanic
 * enabled" gate -- this function always allows the combo-revival branch
 * when combo_count is nonzero, stricter than the source in a way that
 * currently never matters since combo_count is always 0), the lifebar
 * flash-warning process, ACTUAL_PLYRNUM/royal-rumble teammate propagation,
 * 8-on-1 wrestler_count bookkeeping, and everything past SETMODE DEAD
 * (death sound, wrestler-type death-animation dispatch, ROLL_POS reset,
 * flash_red) -- all rendering/team-mode features this port doesn't have.
 */
void wm_arcade_adjust_health(wm_arcade_actor_t *victim, int16_t delta,
                             wm_arcade_actor_t *damage_source,
                             bool attract_mode, uint32_t pcnt);

#ifdef __cplusplus
}
#endif

#endif
