#ifndef WM_ARCADE_LIFEBAR_H
#define WM_ARCADE_LIFEBAR_H

#include <stdbool.h>
#include <stdint.h>
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_react1_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * wm_arcade_adjust_health's own death-anim hook. Only ever invoked with
 * WM_R1_ANIM_FALL_BACK -- the same wrestler-agnostic id REACT1.ASM's own
 * hit-reaction code already uses for this identical per-wrestler anim
 * (LIFEBAR.ASM's fallbacks_t and REACT1.ASM's fall_back_anim dispatches
 * are the same real animation data, just invoked from two different
 * subroutines), so this doesn't invent a second naming scheme for it. May
 * be NULL (no wired backend for this victim -- true for every wrestler but
 * Bret in this port).
 */
typedef struct wm_arcade_death_anim_callback {
    void (*change_anim)(wm_arcade_actor_t *victim,
                        wm_arcade_react1_anim_group_t anim,
                        void *user);
    void *user;
} wm_arcade_death_anim_callback_t;

/* LIFEBAR.ASM:135 LIFE_MAX equ 163 (green pixels in life bar). */
#define WM_ARCADE_LIFE_MAX 163

/* GAME.EQU _85PCT equ 218 (8.8 fixed, ~0.85*256): LIFEBAR.ASM's
   damage_mod_table[0] entry -- see wm_arcade_adjust_health's comment for
   why this port's fixed 2-actor matches always resolve to that row. */
#define WM_ARCADE_DAMAGE_MOD_85PCT 218

/*
 * LIFEBAR.ASM:183 #timer_table[ADJSPEED-1], Q16.16 fixed. This port has no
 * live operator-settings/CMOS system to read a real ADJSPEED (1-5) from, so
 * it uses the arcade's own factory-shipped default instead of inventing
 * one: AUDIT.ASM's FACTORY_TABLE lists ADJSPEED (adjustment id 25) as 3,
 * exactly matching LIFEBAR.ASM:161's own BADCHK fallback value for an
 * out-of-range read. #timer_table[3-1] (0x10000*1) is the entry literally
 * commented "normal damage (default)" in the source -- i.e. the factory
 * default is precisely 1.0x, a mathematical identity multiply.
 */
#define WM_ARCADE_SPEED_ADJUSTMENT_16_16 0x10000L

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
 *   - LIFEBAR.ASM:1471-1521, applied next, only when delta is still
 *     negative ("unless we're adding life"): delta *= WM_ARCADE_DAMAGE_MOD_
 *     85PCT (218/256, an 8.8 fixed multiply+shift identical to the
 *     unsigned-multiply-then-signed-shift the source uses on a negative
 *     delta) then >>=8. The real source scales this by active-drone-count
 *     and whether the victim is a drone or a player; this port's actors[]
 *     is always the fixed pair wm_match_start_attract/selected create, so
 *     that count is always 0 and both of the table's 0-drones columns are
 *     the same _85PCT anyway -- no PLYR_TYPE branch needed.
 *   - LIFEBAR.ASM:1524-1528, applied next unconditionally (healing and
 *     damage alike, unlike the damage_mod_table step): delta = (delta *
 *     WM_ARCADE_SPEED_ADJUSTMENT_16_16) >> 16. See that macro's own comment
 *     for why its value (the arcade's factory-default ADJSPEED setting) is
 *     exactly 0x10000 -- a 1.0x identity, so this line has no effect today,
 *     but is real and ready for a live value once this port has an operator-
 *     settings system to read one from.
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
 *     - LIFEBAR.ASM:1591-1725: a genuine death (life reaches 0, not
 *       deferred) runs the real death-dispatch tail before finally setting
 *       player_mode to WM_PMODE_DEAD and turning off further hit checks
 *       (wm_arcade_wrestler_collisions_off, matching SETMODE DEAD + calla
 *       wres_collis_off):
 *         - victim->roll_pos is reset to 0 (LIFEBAR.ASM:1608).
 *         - LIFEBAR.ASM:1691-1710's own pre-checks: if damage_source's
 *           attack_mode is WM_AMODE_BLBOWDROP/BSTOMP/BUTTSTOMP, or it's
 *           WM_AMODE_BUZZ and victim's own player_mode (read here, before
 *           it's overwritten to DEAD below) wasn't WM_PMODE_BLOCK, or
 *           victim->status_flags has WM_STATUS_DEAD_ANIM set, the whole
 *           death-animation dispatch below is skipped entirely (velocities
 *           untouched). None of Bret's wired attacks use those attack_mode
 *           ids and nothing in this port ever sets WM_STATUS_DEAD_ANIM on a
 *           path the live match reaches, so this is always false in
 *           practice today -- translated anyway since it's real, cheap, and
 *           correct the moment either changes.
 *         - Otherwise, victim's own player_mode (again, before being
 *           overwritten) selects the dispatch, exactly like LIFEBAR.ASM's
 *           own #fall check: WM_PMODE_NORMAL/RUNNING/INAIR/INAIR2/BOUNCING/
 *           ONTURNBKL/BLOCK/DIZZY/CLIMBTURNBKL (LIFEBAR.ASM's #fallbk) --
 *           every player_mode Bret's own real dispatcher (wm/arcade/
 *           wm_arcade_bret.h) can actually be in at the moment of death --
 *           fire death_anim->change_anim(victim, WM_R1_ANIM_FALL_BACK, ...)
 *           (the real hrt_fall_back_anim, already wired for real as
 *           WM_BRET_ANIM_FALL_BACK by Bret's own I_WILL_DIE self-death
 *           case) and apply LIFEBAR.ASM:1739-1753's own knockback: unless
 *           victim->x_vel is already > 2.0 px/tick, force it to +/-2.0 away
 *           from damage_source->x_int (matching the source's own default-
 *           then-override-by-side construction exactly, including its real
 *           quirk of only checking rightward speed -- a wrestler already
 *           flying left fast gets its velocity reduced to a flat -2.0, not
 *           left alone). Any other player_mode (LIFEBAR.ASM's own
 *           unmatched-mode catch-all, WM_PMODE_ATTACHED included) zeroes
 *           all three velocities and fires no anim, matching the source
 *           exactly for that catch-all case.
 *         - death_anim may be NULL (no wired backend for this victim --
 *           true for every wrestler but Bret in this port); the anim/
 *           knockback step is then simply skipped, same as if the pre-
 *           checks above had matched.
 *   - LIFEBAR.ASM:1593-1595 "update LAST_DAMAGE": unconditionally, on every
 *     call (fudged, attract-saved, genuinely dying, or a plain clamp
 *     alike), victim->last_damage is stamped with pcnt. This is what makes
 *     wm_arcade_wrestler_hit's own reduced_damage window real: it already
 *     read last_damage (REACT1.ASM's "elapsed_word(pcnt, last_damage)<=50"
 *     rapid-hit check) but nothing ever wrote it before this, so repeated
 *     attacks always dealt full_damage regardless of timing.
 *
 * NOT translated: CHECK_COMBO_GO (LIFEBAR.ASM:718 -- now actually located,
 * see wm/arcade/wm_arcade_mode_dead.h's own full derivation for why it's
 * provably always "not lit" in this port: no per-player combo-meter-fill
 * tracking at all, and instant_combos_on's 0-threshold bypass is an
 * AWARD.ASM credit-screen powerup toggle this port's credit system never
 * sets either -- so this function always allowing both combo_count
 * branches above, rather than gating them on CHECK_COMBO_GO like the
 * source does, is stricter than the source in a way that provably never
 * matters here), the lifebar flash-warning process (needs `ck_any_
 * teammates` and a `flash_obj`/`FLASH_PID` rendering process this port
 * doesn't have -- see wm_arcade_calc_closest's own note on the fixed
 * 2-actor boundary), ACTUAL_PLYRNUM/royal-rumble teammate propagation,
 * 8-on-1 wrestler_count bookkeeping, death sound (LIFEBAR.ASM's own
 * `triple_sound 034h` -- real, but this port's Bret backend leaves every
 * other real sound call a no-op too, wm_arcade_bret_callbacks_t.sound is
 * never wired anywhere, so adding a bridge for only this one call would be
 * inconsistent with every other already-real Bret sound cue), the #grnd
 * convulse_t/hitonground dispatch (real, but only reachable when a
 * wrestler's own player_mode is already WM_PMODE_ONGROUND or WM_PMODE_DEAD
 * at the moment of death, which this port's Bret dispatcher never sets --
 * so not guessed at, since no hrt_hitonground_anim data has been extracted
 * either), the #will_die HEADHELD deferral (sets i_will_die=180 without
 * immediately dying, but needs an eventual consumer this port doesn't have
 * and WM_PMODE_HEADHELD is likewise never set on Bret -- folded into the
 * same immediate catch-all as every other unmatched mode instead of
 * guessing at that consumer), and flash_red (pure lifebar-rendering
 * feedback, no rendering system exists here).
 */
void wm_arcade_adjust_health(wm_arcade_actor_t *victim, int16_t delta,
                             wm_arcade_actor_t *damage_source,
                             bool attract_mode, uint32_t pcnt,
                             int32_t *dam_mult,
                             const wm_arcade_death_anim_callback_t *death_anim);

/*
 * LIFEBAR.ASM:5310 SUBR clear_lifebar -- zero this wrestler's PLT_LIFE and
 * refresh his meter. Only the life half is translated; update_meter is
 * pure rendering and there is no meter to draw.
 *
 * ANIM.ASM:2990 _ani_waitroll is what calls it, and it is also the
 * consumer the note above says i_will_die lacked: adjust_health's #will_die
 * path defers a death by setting i_will_die instead of killing outright,
 * and ANI_WAITROLL is where that debt comes due once IMMOBILIZE_TIME runs
 * out.
 */
void wm_arcade_clear_lifebar(wm_arcade_actor_t *a);

#ifdef __cplusplus
}
#endif

#endif
