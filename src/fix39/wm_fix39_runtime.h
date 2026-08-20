#ifndef WM_FIX39_RUNTIME_H
#define WM_FIX39_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm_arcade_combat.h"
#include "wmania_attract_core.h"
#include "wmania_ring_onscreen.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fix39 integration spine.
 *
 * This file intentionally does not invent the still-missing arcade services.
 * It makes the translated bundles live in the N64 program while keeping hard
 * source boundaries explicit (operator HSR adjustment, DRONE data/rnd,
 * camera WORLDTLX, and wrestler_veladd/friction/animation).
 */

typedef struct {
    bool initialized;
    bool match_started;
    bool hiscore_reset_value_bound;
    bool hiscore_tables_valid;
    bool drone_runtime_ready;
    uint32_t pcnt;
    uint16_t round_tickcount;
    size_t attract_step_count;
} WmFix39Status;

void wm_fix39_runtime_init(void);

/* Exact translated RAND/RNDRNG0 service; HCOUNT/SP are N64 adapter inputs. */
void wm_fix39_rng_set_entropy(uint32_t hcount, uint32_t sp_value);
uint32_t wm_fix39_mainloop_step(uint32_t hcount, uint32_t sp_value);
uint32_t wm_fix39_rndrng0(uint32_t maximum_inclusive);
uint32_t wm_fix39_rng_state(void);

/* Build the exact non-gameplay attract cycle as a live source shadow. */
size_t wm_fix39_attract_cycle_begin(void);
const WmAttractStep *wm_fix39_attract_step(size_t index);

/*
 * HSTD's reset counter depends on an operator adjustment value.  The current
 * N64 frontend does not expose that adjustment, so Fix39 refuses to guess it.
 * Factory tables/validation are live immediately; start/continue decrement is
 * enabled only after the real translated adjustment is supplied here.
 */
void wm_fix39_hiscore_bind_reset_value(uint32_t adjusted_reset_value);
bool wm_fix39_hiscore_player_start_or_continue(uint32_t *remaining_out);

/* Source-exact first 1v1 reset_start seeds from WRESTLE.ASM. */
void wm_fix39_match_begin(unsigned frontend_p1, unsigned frontend_p2);
bool wm_fix39_match_started(void);
void wm_fix39_match_tick(int8_t stick_x, int8_t stick_y,
                         bool run,
                         bool light_punch,
                         bool power_punch,
                         bool light_kick,
                         bool power_kick,
                         bool block);

const wm_arcade_actor_t *wm_fix39_actor(size_t index);

/*
 * Exact keep_onscreen bridge for the source wrestler loop.  The caller must
 * pass the translated DISPLAY.ASM WORLDTLX integer and OLD_PSTATUS.  It is
 * deliberately separate from match_tick until the live camera + veladd loop
 * are ported, so Fix39 never fabricates a camera position.
 *
 * Source order is:
 * update_joystat -> count_button_presses -> keep_onscreen ->
 * wrestler_veladd -> wrestler_friction -> animate_wrestler -> collision.
 */
typedef struct {
    int16_t worldtlx_int;
    int16_t old_pstatus;
    uint16_t *allow_offscreen_io;
    int16_t p1_climbing_thru;
    int16_t p2_climbing_thru;
    uint32_t p1_meter_saved_a8;
    uint32_t p1_meter_saved_a9;
    uint32_t p1_meter_saved_a10;
    uint32_t p2_meter_saved_a8;
    uint32_t p2_meter_saved_a9;
    uint32_t p2_meter_saved_a10;
} WmFix39OnscreenInputs;

WmRingOnscreenEvents wm_fix39_keep_onscreen_before_velocity(
    const WmFix39OnscreenInputs *inputs);

/* Rope processes are instantiated from the direct ROPES.ASM translation. */
bool wm_fix39_rope_process_alive(unsigned bank);

const WmFix39Status *wm_fix39_status(void);

#ifdef __cplusplus
}
#endif

#endif
