#ifndef WM_FIX39_RUNTIME_H
#define WM_FIX39_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm_arcade_combat.h"
#include "wm_arcade_drone.h"
#include "wm_arcade_completion.h"
#include "wm_arcade_roster.h"
#include "wmania_attract_core.h"
#include "wmania_attract_live.h"
#include "wmania_hiscore_system.h"
#include "wmania_hiscore_persist.h"
#include "wmania_attract_secret.h"
#include "wm_arcade_special.h"
#include "wmania_ring_onscreen.h"
#include "wmania_rope_command.h"
#include "wmania_rope_spawn.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fix39/V13 integration spine.
 *
 * V13 keeps the contract from "linked into the ROM" to "entered from the
 * live source tick wherever the supplied translation is complete".  Missing
 * shared arcade services remain explicit; this wrapper never fabricates them.
 */

typedef struct {
    bool initialized;
    bool match_started;
    bool hiscore_reset_value_bound;
    bool hiscore_tables_valid;
    bool drone_runtime_ready;
    /* V13e chunked DRONE binding state.  C3 has source scalar/range/script
       payloads; plain RND + seek/code-call services remain explicit gates. */
    bool drone_scalar_tables_ready;
    bool drone_rndrng0_ready;
    bool drone_plain_rnd_ready;
    bool drone_range_tables_ready;
    bool drone_scripts_ready;
    bool attract_demo_setup_ready;
    bool postmatch_router_ready;
    bool story_plan_ready;
    bool fireworks_plan_ready;

    /* Proven live-entry counters. */
    uint32_t wrestler_dispatch_ticks;
    uint32_t wrestler_dispatch_ticks_by_player[2];
    uint32_t drone_ticks;
    uint32_t drone_input_ticks;
    /* Combat2DT: per-actor liveness distinguishes a global DRONE stall from
       one actor parking on a source service/script seam. */
    uint32_t drone_ticks_by_player[2];
    uint32_t drone_input_ticks_by_player[2];
    uint32_t actor_position_changes[2];
    uint32_t rope_process_ticks;
    uint32_t ringout_process_ticks;
    uint32_t special_process_ticks;
    uint32_t combat_collision_ticks;
    uint32_t combat_checkhit_ticks;
    uint32_t combat_attack_boxes_built;
    uint32_t combat_x_overlap_ticks;
    uint32_t combat_y_overlap_ticks;
    uint32_t combat_z_overlap_ticks;
    uint32_t combat_full_overlap_ticks;
    uint32_t combat_full_overlap_rejected;
    uint32_t combat_accepted_hits;
    uint32_t reaction_dispatches;
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

/* Combat2AR: live ROPES.ASM image-object bridge. The rope interpreter owns
 * these symbols; platform renderers only consume them. */
const char *wm_fix39_rope_image_symbol(WmRopeBank bank, WmRopeChannel channel, WmRopeHalf half);

/* Exact translated RAND/RNDRNG0 service; HCOUNT/SP are N64 adapter inputs. */
void wm_fix39_rng_set_entropy(uint32_t hcount, uint32_t sp_value);
uint32_t wm_fix39_mainloop_step(uint32_t hcount, uint32_t sp_value);
uint32_t wm_fix39_rndrng0(uint32_t maximum_inclusive);
uint32_t wm_fix39_rng_state(void);

/* Source-backed DRONE callback bundle under incremental binding.  V13e-c3
 * supplies RNDRNG0, exact scalar/range tables, decoded script bodies and
 * command-2 skill tables.  Plain RND, drone_seek and code-call seams are C4. */
const wm_arcade_drone_callbacks_t *wm_fix39_drone_callbacks(void);
const wm_arcade_drone_state_t *wm_fix39_drone_state(size_t index);

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
    WM_FIX39_ATTRACT_CAP_HISCORES         = 1u << 6,
    WM_FIX39_ATTRACT_CAP_LIVE_ALL         =
        WM_FIX39_ATTRACT_CAP_DESIGNER_HINT |
        WM_FIX39_ATTRACT_CAP_GENERAL_TIPS |
        WM_FIX39_ATTRACT_CAP_COPYRIGHT |
        WM_FIX39_ATTRACT_CAP_AAMA |
        WM_FIX39_ATTRACT_CAP_OPERATOR_MESSAGE |
        WM_FIX39_ATTRACT_CAP_TIME_DATE |
        WM_FIX39_ATTRACT_CAP_HISCORES
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

/* HSTD source tables/system. Renderer receives read-only live/factory data. */
const WmHsSystem *wm_fix39_hiscore_system(void);
void wm_fix39_hiscore_bind_reset_value(uint32_t adjusted_reset_value);
bool wm_fix39_hiscore_player_start_or_continue(uint32_t *remaining_out);
/* Entry 1 of the source BEATEN/INTER table, used by PROGRESS.ASM recent champ. */
const char *wm_fix39_hiscore_recent_initials(bool world_championship);

/* WRESTLE.ASM init_scroller + WRESTLE2.ASM scroll_world.
 * Values remain source 16.16. Normal attract (PSTATUS==0) tracks the first
 * live drone and its closest opponent; the current two-actor runtime is that
 * exact case. */
typedef struct {
    int32_t worldtlx_fp16;
    int32_t worldtly_fp16;
} WmFix39CameraState;

const WmFix39CameraState *wm_fix39_camera_state(void);
int16_t wm_fix39_camera_worldtlx_int(void);
int16_t wm_fix39_camera_worldtly_int(void);

/* Source-exact first 1v1 reset_start seeds from WRESTLE.ASM. */
int wm_fix39_frontend_to_arcade_roster(unsigned frontend_id);
void wm_fix39_match_begin(unsigned frontend_p1, unsigned frontend_p2);
bool wm_fix39_match_started(void);
void wm_fix39_match_set_cpu_vs_cpu(bool enabled);
void wm_fix39_match_tick(int8_t stick_x, int8_t stick_y,
                         bool run,
                         bool light_punch,
                         bool power_punch,
                         bool light_kick,
                         bool power_kick,
                         bool block);

const wm_arcade_actor_t *wm_fix39_actor(size_t index);
const WmFix39ActorTrace *wm_fix39_actor_trace(size_t index);
const char *wm_fix39_actor_source_frame(size_t index);
const char *wm_fix39_actor_source_torso_frame(size_t index);
const char *wm_fix39_actor_source_anim(size_t index);

/* Live animation/presentation bridge for COLLIS.ASM.  IANI3 fields come from
 * the exact current arcade image header; callers must not pass inferred sprite
 * bounds.  Collision stays disabled until both actors have valid frame boxes. */
bool wm_fix39_match_set_frame_box(size_t index,
                                  const wm_arcade_frame_box_t *frame);
void wm_fix39_match_clear_frame_box(size_t index);
int32_t wm_fix39_match_life(size_t index);

/* Already-ported shared combat services now reachable from the live spine. */
bool wm_fix39_match_spawn_special(size_t owner_index,
                                  wm_arcade_special_kind_t kind);
void wm_fix39_match_set_ringout_enabled(bool enabled);

/* Ported ATTRACT secret scheduler and HSTD persistence codec are wired through
 * explicit platform backends rather than left as isolated library code. */
void wm_fix39_secret_begin(uint32_t tsec_ticks);
bool wm_fix39_secret_tick(bool has_button, WmAttractSecretButton button);
void wm_fix39_hiscore_bind_persistence(const WmHsSaveBackend *backend);
WmHsLoadResult wm_fix39_hiscore_load_bound(uint32_t adjusted_reset_value);
bool wm_fix39_hiscore_save_bound(void);

/* Demo/presentation bridge only: keep the visible fighter pose in the same
 * coordinate/facing space as COLLIS while the full source animation renderer
 * is still being brought online. */
/* Diagnostic-only presenter snapshot. Strict source builds leave this API
 * disabled so screen/presenter coordinates cannot feed back into WRESTLE state. */
#if defined(WM_FIX39_DIAGNOSTIC_PRESENTER_POSE)
void wm_fix39_match_sync_presenter_pose(size_t index, int32_t x, int32_t z,
                                        bool flip_x, bool blocking);
#endif

/* Bret source-backed attack-window bridge for the temporary attract presenter.
 * action uses wm_demo_action numeric values. Only source-verified punch windows
 * are enabled; unsupported actions fail closed. */
void wm_fix39_match_bind_source_frame_attack(size_t index, uint8_t roster_id,
                                             const char *source_frame);
void wm_fix39_match_bind_bret_source_frame_attack(size_t index, const char *source_frame);

/*
 * Exact keep_onscreen bridge for the source wrestler loop.  The caller must
 * pass the real translated DISPLAY.ASM WORLDTLX and OLD_PSTATUS plus the
 * meter/climbing state.  V13 still does not invent those values.
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


/* V13 source-completion plans. These are direct source orchestration results;
 * platform render/audio/match-state adapters execute the returned steps. */
bool wm_fix39_attract_demo_plan(uint16_t amode_loops,
                                bool music_adjustment_nonzero,
                                WmAttractDemoPlan *out);
wm_postmatch_result wm_fix39_postmatch_route(const wm_postmatch_input *in);
void wm_fix39_rumble_plan(bool human_won, bool anyone_bought_in,
                          wm_rumble_plan *out);
void wm_fix39_finale_plan(bool eight_on_one, wm_finale_plan *out);
void wm_fix39_story_plan(uint8_t wrestler_index, wm_story_plan *out);
void wm_fix39_fireworks_plan(wm_fireworks_plan *out);
void wm_fix39_game_beaten_plan(const wm_game_beaten_input *in,
                               wm_game_beaten_plan *out);

/* Existing direct HSTD implementation remains the only table engine. These
 * adapters expose the source result hooks to the completion/match layer; they
 * do not fabricate initials or a save medium. */
bool wm_fix39_hiscore_begin_winstreak(uint8_t human_player_index,
                                      uint8_t wrestler_index,
                                      uint32_t old_winstreak_binary,
                                      WmHsPendingEntry *pending);
bool wm_fix39_hiscore_begin_pin_speed(uint8_t actor_index,
                                      uint8_t wrestler_index,
                                      uint8_t current_round,
                                      uint32_t match_timer_bcd,
                                      WmHsPendingEntry *pending);
bool wm_fix39_hiscore_begin_beaten(uint8_t human_player_index,
                                   uint8_t wrestler_index,
                                   bool world_belt,
                                   WmHsPendingEntry *pending);
bool wm_fix39_hiscore_begin_tag_time(uint8_t human_player_index,
                                     uint32_t match_timer_bcd,
                                     WmHsPendingEntry *pending);
uint16_t wm_fix39_hiscore_commit_pending(
    const WmHsPendingEntry *pending,
    const uint8_t initials[WM_HS_NUM_INITIALS]);

const WmFix39Status *wm_fix39_status(void);

#ifdef __cplusplus
}
#endif

#endif
