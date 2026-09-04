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
 * 1429-1670), shared by every real caller of it in this port: the REACT1.ASM
 * hit path (wm_arcade_wrestler_hit -> wm_match's wrestler_hit callback) and
 * BRET.ASM mode_normal's own "I_WILL_DIE amidst a combo" self-death case
 * (wm_arcade_move_bret, via wm_arcade_bret_callbacks_t.adjust_health) --
 * both call the same shared SUBR in the original, so this is one real
 * function rather than two copies.
 *
 * Translated:
 *   - LIFEBAR.ASM:1429-1466, applied to delta before anything else below:
 *     if damage_source's COMBO_COUNT is nonzero, delta is replaced entirely
 *     (not scaled) by -max(10-COMBO_COUNT, 4) and *dam_mult is cleared --
 *     "doing a combo" damage, unrelated to the original hit's own damage
 *     value. Otherwise, if *dam_mult is nonzero, delta is scaled by
 *     (delta*(1+*dam_mult))>>1 (DAM_MULT 2/3/4+ => x1.5/x2/x2.5, source's
 *     own repeated-addition-then-halve loop) and *dam_mult is cleared.
 *     dam_mult may be NULL (no DAM_MULT tracking in this call context --
 *     wm_bret_backend_callbacks' self-death path has none; only
 *     wm_arcade_wrestler_hit ever actually sets a runtime's dam_mult to
 *     2 or 4, via wm_match's wm_arcade_combat_runtime_t).
 *   life = victim->life + delta (the possibly-transformed value above),
 *   clamped to [0, LIFE_MAX], except:
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
 *       port: nothing yet increments combo_count anywhere (which also
 *       means the delta-override branch above is currently unreachable the
 *       same way).
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
 * NOT translated: LIFEBAR.ASM's damage_mod_table/speed_adjustment scaling
 * (both happen earlier in adjust_health, before delta reaches this
 * function), CHECK_COMBO_GO (an unlocated global "is the combo mechanic
 * enabled" gate -- this function always allows both combo_count branches
 * above, stricter than the source in a way that currently never matters
 * since combo_count is always 0), the lifebar flash-warning process,
 * ACTUAL_PLYRNUM/royal-rumble teammate propagation, 8-on-1 wrestler_count
 * bookkeeping, and everything past SETMODE DEAD (death sound, wrestler-type
 * death-animation dispatch, ROLL_POS reset, flash_red) -- all rendering/
 * team-mode features this port doesn't have.
 */
void wm_arcade_adjust_health(wm_arcade_actor_t *victim, int16_t delta,
                             wm_arcade_actor_t *damage_source,
                             bool attract_mode, uint32_t pcnt,
                             int32_t *dam_mult);

#ifdef __cplusplus
}
#endif

#endif
