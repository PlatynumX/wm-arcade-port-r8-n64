#ifndef WM_FIX39_RUNTIME_H
#define WM_FIX39_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm_arcade_combat.h"
#include "wm_arcade_roster.h"
#include "wmania_attract_core.h"
#include "wmania_attract_live.h"
#include "wmania_ring_onscreen.h"
#include "wmania_rope_command.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fix39/V11 integration spine.
 *
 * V11 keeps the contract from "linked into the ROM" to "entered from the
 * live source tick wherever the supplied translation is complete".  Missing
 * shared arcade services remain explicit; this wrapper never fabricates them.
 */

typedef struct {
    bool initialized;
    bool match_started;
    bool hiscore_reset_value_bound;
    bool hiscore_tables_valid;
    bool drone_runtime_ready;

    /* Proven live-entry counters. */
    uint32_t wrestler_dispatch_ticks;
    uint32_t wrestler_dispatch_ticks_by_player[2];
    uint32_t rope_process_ticks;
    uint32_t ringout_process_ticks;
    uint32_t special_process_ticks;
    uint32_t combat_collision_ticks;
    uint32_t attachment_service_calls;
    uint32_t attract_cycles_built;
    uint32_t attract_live_ticks;
    uint32_t attract_pending_skips;
    uint32_t attract_external_waits;
    uint32_t attract_platform_capabilities;

    /* Explicit source seams which still block a complete match. */
    bool movement_integrator_ready;
    bool animation_backend_ready;
    bool collision_boxes_ready;
    bool camera_onscreen_inputs_ready;
    bool ring_line_services_ready;
    bool secret_input_scheduler_ready;
    bool health_service_ready;
    bool audio_label_service_ready;
    bool special_spawn_command_service_ready;
    bool ringout_operator_state_ready;
    bool rope_renderer_ready;
    bool hiscore_persistence_ready;
    bool attract_renderer_ready;

    uint32_t pcnt;
    uint16_t round_tickcount;
    size_t attract_step_count;
} WmFix39Status;

typedef struct {
    /* Exact source identity emitted by the dedicated wrestler module. */
    const char *animation_label;
    const char *torso_animation_label;
    const char *sound_label;
    const char *external_special_label;

    /* Bret/Razor use source enum tokens until their animation pointer tables
       are connected to the native arcade-frame interpreter. */
    int32_t animation_token;
    int32_t torso_animation_token;
    int32_t sound_token;

    wm_arcade_roster_step_result_t last_character_step;
    uint32_t animation_events;
    uint32_t sound_events;
    uint32_t external_special_events;
} WmFix39ActorTrace;

void wm_fix39_runtime_init(void);

/* Exact translated RAND/RNDRNG0 service; HCOUNT/SP are N64 adapter inputs. */
void wm_fix39_rng_set_entropy(uint32_t hcount, uint32_t sp_value);
uint32_t wm_fix39_mainloop_step(uint32_t hcount, uint32_t sp_value);
uint32_t wm_fix39_rndrng0(uint32_t maximum_inclusive);
uint32_t wm_fix39_rng_state(void);

/* Exact translated non-gameplay attract cycle. */
size_t wm_fix39_attract_cycle_begin(void);
const WmAttractStep *wm_fix39_attract_step(size_t index);

/*
 * V11 attract ownership.  A screen is runnable only when its source control
 * path exists and, for newly-live screens, the N64 presentation adapter has
 * explicitly opted in.  This prevents an exact source scheduler from parking
 * forever on an unrendered black screen.
 */
enum {
    WM_FIX39_ATTRACT_CAP_DESIGNER_HINT = 1u << 0,
    WM_FIX39_ATTRACT_CAP_GENERAL_TIPS  = 1u << 1,
    WM_FIX39_ATTRACT_CAP_COPYRIGHT       = 1u << 2,
    WM_FIX39_ATTRACT_CAP_AAMA            = 1u << 3,
    WM_FIX39_ATTRACT_CAP_OPERATOR_MESSAGE = 1u << 4,
    WM_FIX39_ATTRACT_CAP_TIME_DATE        = 1u << 5,
    WM_FIX39_ATTRACT_CAP_LIVE_ALL         =
        WM_FIX39_ATTRACT_CAP_DESIGNER_HINT |
        WM_FIX39_ATTRACT_CAP_GENERAL_TIPS |
        WM_FIX39_ATTRACT_CAP_COPYRIGHT |
        WM_FIX39_ATTRACT_CAP_AAMA |
        WM_FIX39_ATTRACT_CAP_OPERATOR_MESSAGE |
        WM_FIX39_ATTRACT_CAP_TIME_DATE
};

void wm_fix39_attract_set_platform_capabilities(uint32_t capabilities);
WmAttractOwner wm_fix39_attract_step_owner(size_t index);
bool wm_fix39_attract_step_runnable(size_t index);
void wm_fix39_attract_note_pending_skip(size_t index);
bool wm_fix39_attract_screen_begin(size_t index);
bool wm_fix39_attract_screen_signal_external_result(bool available);
bool wm_fix39_attract_screen_signal_external_complete(void);
bool wm_fix39_attract_screen_tick(bool any_button);
const WmAttractLive *wm_fix39_attract_live_state(void);

/* HSTD source tables/system. */
void wm_fix39_hiscore_bind_reset_value(uint32_t adjusted_reset_value);
bool wm_fix39_hiscore_player_start_or_continue(uint32_t *remaining_out);
/* Entry 1 of the source BEATEN/INTER table, used by PROGRESS.ASM recent champ. */
const char *wm_fix39_hiscore_recent_initials(bool world_championship);

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
const WmFix39ActorTrace *wm_fix39_actor_trace(size_t index);

/*
 * Exact keep_onscreen bridge for the source wrestler loop.  The caller must
 * pass the real translated DISPLAY.ASM WORLDTLX and OLD_PSTATUS plus the
 * meter/climbing state.  V9 still does not invent those values.
 *
 * Source order:
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

/* Rope processes use the complete direct ROPES.ASM program corpus. */
bool wm_fix39_rope_process_alive(unsigned bank);
bool wm_fix39_rope_apply_command(WmRopeBank bank,
                                 WmRopeAction action,
                                 uint8_t selector,
                                 int32_t wrestler_z_fp16);

const WmFix39Status *wm_fix39_status(void);

#ifdef __cplusplus
}
#endif

#endif
