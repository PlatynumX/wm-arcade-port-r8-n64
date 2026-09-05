#ifndef WM_ARCADE_MODE_DEAD_H
#define WM_ARCADE_MODE_DEAD_H

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DOINK.ASM:2920 SUBR mode_dead ;9 -- the shared MODE_DEAD entry every
 * wrestler's own #mode_table[9] points at via .ref (confirmed identical in
 * BRET.ASM:1305, RAZOR.ASM:1153, etc: it is one real routine, not
 * per-wrestler code). wm_arcade_move_bret's own MODE_DEAD case already
 * calls a `mode_dead` callback for this; it was left unwired (NULL).
 *
 * The real routine's job is deciding whether a freshly-dead wrestler gets
 * a "buckoff" comeback-to-life chance (mashing buttons to revive as a
 * zombie), gated on three checks, in order:
 *   1. `@royal_rumble` nonzero -> bail immediately, no flag even set.
 *   2. `is_8_on_1()` (PROGRESS.ASM:1516): reads `@belt_type`/`@CURRENT_LADDER`
 *      against the ladder's own final-battle slot.
 *   3. Own-side round-loss count (`@p1rounds`/`@p2rounds`) is 0 -> skip
 *      straight to "no buckoff"; otherwise fall through to a combo-meter
 *      check, `CHECK_COMBO_GO` (LIFEBAR.ASM:718): compares the player's
 *      `life_data[...].PLT_COMBO_SIZE` against 16, or against 0 if
 *      `@instant_combos_on` is set (an AWARD.ASM credit-screen powerup
 *      toggle).
 *
 * In this port every one of those is provably always false/zero, so every
 * path through the real routine collapses to the same outcome:
 *   - `@royal_rumble`: this port has no royal rumble mode at all (no
 *     ladder/team system -- see the confine_wrestler and calc_closest
 *     boundaries), so it never leaves its zero default.
 *   - `is_8_on_1`: reads `@belt_type` (jrz -> "no" immediately) and
 *     `@CURRENT_LADDER`, both belonging to the same unimplemented ladder
 *     system, so it always reports "not 8-on-1".
 *   - `CHECK_COMBO_GO`: this port never tracks per-player combo-meter fill
 *     (`life_data[...].PLT_COMBO_SIZE`) at all -- only life itself is real,
 *     via `init_life_data` -- so it stays at its zero default, always below
 *     the 16 threshold; and `@instant_combos_on` is a BSS flag only ever
 *     set by AWARD.ASM's credit/powerup screens, entirely outside this
 *     port's scope (see the credit/buy-in boundary), so it also never
 *     leaves its zero default and the 0-threshold "auto combos" bypass is
 *     never taken either. So `CHECK_COMBO_GO` always reports "not lit",
 *     regardless of the round-loss count that gates whether it even runs.
 *
 * That means the real routine's own round-count branch doesn't change the
 * outcome (both sides of it end at "#nobuck"), so this translation doesn't
 * need round-score data at all: wm_arcade_mode_dead(actor) always takes
 * the real "#nobuck" path on a wrestler's first dead tick -- setting
 * WM_STATUS_NO_BUCKOFF, exactly like the source's own
 * `STATUS_FLAGS |= M_NO_BUCKOFF` -- and, like the source's own top-of-
 * function early-outs, is a genuine no-op on every tick after (checked via
 * WM_STATUS_ZOMBIE/DID_BUCKOFF/NO_BUCKOFF/DO_BUCKOFF exactly as the source
 * checks B_ZOMBIE/DID_BUCKOFF/NO_BUCKOFF/DO_BUCKOFF).
 *
 * NOT translated, both real and both permanently unreachable while the
 * above stays true: the `#zmb` zombie-transform tail (roll up, stand,
 * run to the side of the arena, `change_wrestler`) -- it only ever runs
 * once WM_STATUS_ZOMBIE is set, which requires reaching `#dobuck` first --
 * and the `#count_btns`/`#dobuck` button-mashing buckoff-revival system
 * itself (sets WM_STATUS_ZOMBIE, does the opponent's pin-broken/raise-arm
 * bookkeeping, `clear_combo_meter`) -- both gated on the same
 * `CHECK_COMBO_GO` check proven above to never pass in this port.
 */
void wm_arcade_mode_dead(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif

#endif
