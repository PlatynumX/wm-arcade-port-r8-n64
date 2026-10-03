#ifndef WM_ARCADE_JOYSTAT_H
#define WM_ARCADE_JOYSTAT_H

#include <stdbool.h>
#include <stdint.h>
#include "wm/arcade/wm_arcade_bret.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE.ASM's shared per-wrestler input-history ring buffer
 * (`wrest_joystat`, WRESTLE.ASM:206 `BSSX wrest_joystat,32*16*NUM_WRES`)
 * and the two real routines built on it: `update_joystat` (WRESTLE.ASM:
 * 4569) records it every tick, `check_secret_moves` (WRESTLE.ASM:4851)
 * matches a wrestler's own secret-move table against it. This is what
 * `wm_arcade_bret_callbacks_t.check_secret_moves` (wm/arcade/
 * wm_arcade_bret.h) needed to become real: `wm_arcade_bret_fire_secret`
 * (the per-move condition/dispatch logic, e.g. face rake, jump kick,
 * super cut) and `wm_arcade_bret_secret_patterns` (the 8 real value/mask
 * sequences, already transcribed from BRET.ASM's own tables) existed, but
 * nothing ever detected the actual joystick sequence to call them with.
 *
 * `update_joystat` translated exactly: reads `STICK_VAL_CUR` masked to
 * just LEFT/RIGHT and shifted to bits 10/11 (WM_J_LEFT/WM_J_RIGHT, i.e.
 * "absolute" left/right, ignoring facing) OR'd with a facing-relative
 * direction value at bits 0-3 (WM_J_UP/DOWN/TOWARD/AWAY and their
 * diagonals) -- the raw stick value unchanged if facing right, or run
 * through the source's own 16-entry xflip_table (WRESTLE.ASM:4650, which
 * swaps only the LEFT/RIGHT bits, preserving UP/DOWN) if facing left, so
 * "toward"/"away" always mean toward/away from whichever way Bret is
 * currently facing. This combined value gets inserted into the ring
 * buffer's head (oldest entry dropped) whenever either: a stick UP/DOWN
 * edge occurred this tick and the combined value is nonzero, or a button
 * was newly pressed this tick (BUT_VAL_DOWN), tagged with that one button
 * bit at bits 4-8 (WM_B_PUNCH/BLOCK/SPUNCH/KICK/SKICK) OR'd with the same
 * directional value -- one queue entry per button if several are pressed
 * on the same tick, matching the source's own per-bit insert loop.
 *
 * `check_secret_moves` translated exactly (wm_arcade_joystat_matches):
 * one pattern's steps are stored newest-first (step[0] is the final
 * trigger, matching wm_arcade_bret_secret_patterns' existing layout).
 * Step 0 must match the queue's current head exactly, with no tolerance
 * -- the source's own "special check on the first entry", which (unlike
 * every later step) is never allowed to skip back further in history
 * looking for a match. Steps 1+ walk backward through the queue, and a
 * step's masked value only needs to appear within a *shared* budget of up
 * to 8 skipped "irrelevant" (masked-to-zero) older entries in between
 * (WRESTLE.ASM's own "only skip 8 masked entries", movk 8,a3) -- that
 * budget is spent across the whole pattern, not per step, exactly
 * matching the source's single a3 counter initialized once per table
 * attempt. Once every step matches, the whole sequence must fit within
 * the pattern's own max_ticks window, measured from the oldest matched
 * step to now (WRESTLE.ASM's own 16-bit-wraparound-safe elapsed check,
 * same elapsed_word truncation already used by wm_arcade_react.c's
 * reduced-damage window).
 *
 * One real, deliberate exclusion from wm_arcade_joystat_matches itself:
 * WRESTLE.ASM's own `bret_secret_moves` table (BRET.ASM:148) has a 9th,
 * *first* entry, `#charge_ddt`, that is executable "hold test" code
 * rather than a value/mask table -- checked once, before the table scan,
 * skipping the scan entirely if it fires (holding SPUNCH for 100+ ticks
 * then releasing it triggers a running/standing DDT directly, no
 * directional gesture needed). `wm_arcade_bret_secret_patterns`' own 8
 * entries exclude it for exactly this reason, so `wm_arcade_joystat_matches`
 * never sees it; `wm_bret_backend_check_secret_moves` (src/core/
 * bret_backend.c) checks it directly instead, every tick, ahead of the
 * table scan -- along with `hrt_charge_flying_kick`/`hrt_charge_face_rake`
 * (BRET.ASM:543/614), two more real "hold, then release" watchers with the
 * identical shape, using `wm_arcade_update_joy_dtime`'s real per-button
 * hold-duration counters (`wm/arcade/wm_arcade_combat.h`, WRESTLE.ASM's
 * own `update_joy_dtime`) for all three.
 */

#define WM_JOYSTAT_DEPTH 16

typedef struct wm_arcade_joystat_entry {
    uint16_t value;
    uint16_t tickcount;
} wm_arcade_joystat_entry_t;

typedef struct wm_arcade_joystat {
    /* [0] is the most recently inserted entry. */
    wm_arcade_joystat_entry_t entries[WM_JOYSTAT_DEPTH];
} wm_arcade_joystat_t;

void wm_arcade_joystat_init(wm_arcade_joystat_t *js);

void wm_arcade_joystat_update(wm_arcade_joystat_t *js,
                              const wm_arcade_actor_t *actor,
                              uint16_t round_tickcount);

bool wm_arcade_joystat_matches(const wm_arcade_joystat_t *js,
                               uint16_t round_tickcount,
                               const wm_arcade_bret_sequence_step_t *steps,
                               uint16_t step_count,
                               uint16_t max_ticks);

#ifdef __cplusplus
}
#endif

#endif
