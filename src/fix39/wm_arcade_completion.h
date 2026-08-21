#ifndef WM_ARCADE_COMPLETION_H
#define WM_ARCADE_COMPLETION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wmania_hiscore_system.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t (*WmCompletionRngFn)(void *user, uint32_t inclusive_max);
#define WM_COMPLETION_MAX_PHASES 16u
#ifndef WM_ATTR_MAX_PHASES
#define WM_ATTR_MAX_PHASES WM_COMPLETION_MAX_PHASES
#endif

typedef enum {
    WM_COMPLETION_HS_RANK_HIGH = 0,
    WM_COMPLETION_HS_RANK_LOW = 1
} WmCompletionHsRankMode;
typedef enum {
    WM_COMPLETION_HS_ENTRY_NORMAL = 0,
    WM_COMPLETION_HS_ENTRY_TIME = 1,
    WM_COMPLETION_HS_ENTRY_SPECIAL_BEATEN = 2
} WmCompletionHsEntryMode;
typedef struct {
    bool enabled;
    uint8_t player_side;
    WmHsTableId table;
    WmCompletionHsRankMode rank_mode;
    WmCompletionHsEntryMode entry_mode;
    uint32_t source_score;
    uint32_t beaten_wrestler_mask;
} WmCompletionHsRequest;

typedef enum wm_postmatch_route {
    WM_POSTMATCH_NEXT_PREGAME = 0,
    WM_POSTMATCH_BUYIN,
    WM_POSTMATCH_GAME_OVER,
    WM_POSTMATCH_FINALE,
    WM_POSTMATCH_RUMBLE_WIN
} wm_postmatch_route;

typedef struct wm_postmatch_input {
    uint8_t pstatus;
    uint8_t match_winner;
    bool royal_rumble;
    bool rumble_human_won;
    bool final_match;
    bool anyone_bought_in_after_rumble;
} wm_postmatch_input;

typedef struct wm_postmatch_result {
    wm_postmatch_route route;
    uint8_t next_pstatus;
    uint8_t old_pstatus;
    uint16_t selection_delay_frames;
    bool clear_done_howard;
    bool save_old_pstatus;
    bool decrement_ladder_before_buyin;
    bool clear_match_winner_before_buyin;
    bool run_pin_speed_check;
    bool run_loser_sound;
    bool show_most_damage;
    bool clear_all_rumble_winstreaks;
    bool clear_royal_rumble;
} wm_postmatch_result;

wm_postmatch_result wm_match_route_after_match(const wm_postmatch_input *in);
bool wm_match_pregame_enters_finale(uint8_t pstatus, bool final_match);

/* SELECT.ASM two-human pregame branch. RR_AWARD is a source compile-time
 * option, so expose it instead of guessing its production definition. */
typedef struct wm_two_player_pregame_result {
    bool applicable;
    bool ask_belt_question;
    bool clear_royal_rumble;
    uint8_t question_type;
} wm_two_player_pregame_result;

wm_two_player_pregame_result wm_select_two_player_pregame(
    uint8_t pstatus, bool royal_rumble, bool rr_award_enabled);

typedef enum wm_rumble_step {
    WM_RUMBLE_START_IMAGE_UPDATER = 0,
    WM_RUMBLE_SHOW_MOST_DAMAGE,
    WM_RUMBLE_SET_OLD_PSTATUS_ONE,
    WM_RUMBLE_CLEAR_ALL_WINSTREAKS,
    WM_RUMBLE_SET_INPARTY,
    WM_RUMBLE_FIREWORKS,
    WM_RUMBLE_STOP_IMAGE_UPDATER,
    WM_RUMBLE_AUDIT_HUMAN_WIN,
    WM_RUMBLE_GAME_BEATEN,
    WM_RUMBLE_SET_PSTATUS_ZERO,
    WM_RUMBLE_SET_OLD_PSTATUS_THREE,
    WM_RUMBLE_SET_RR_LOSS_THREE,
    WM_RUMBLE_BUYIN_SELECT,
    WM_RUMBLE_CLEAR_RR_LOSS_AND_MODE,
    WM_RUMBLE_NEXT_PREGAME,
    WM_RUMBLE_GAME_OVER
} wm_rumble_step;

typedef struct wm_rumble_plan {
    wm_rumble_step steps[WM_ATTR_MAX_PHASES];
    size_t step_count;
} wm_rumble_plan;

void wm_rumble_build_plan(bool human_won, bool anyone_bought_in,
                          wm_rumble_plan *out);

typedef enum wm_finale_step {
    WM_FINALE_SET_INPARTY = 0,
    WM_FINALE_AUDIT_CREDIT_LENGTH,
    WM_FINALE_CLEAR_GAME_TIME_AND_STARTS,
    WM_FINALE_START_IMAGE_UPDATER,
    WM_FINALE_FIREWORKS,
    WM_FINALE_STOP_IMAGE_UPDATER,
    WM_FINALE_WRESTLER_STORY,
    WM_FINALE_GAME_BEATEN,
    WM_FINALE_CREATE_TEXT_LINE,
    WM_FINALE_TERMINAL_GAME_BEATEN
} wm_finale_step;

typedef struct wm_finale_plan {
    wm_finale_step steps[WM_ATTR_MAX_PHASES];
    size_t step_count;
} wm_finale_plan;

void wm_finale_build_plan(bool eight_on_one, wm_finale_plan *out);

/* SELECT.ASM GAME_BEATEN orchestration.  Rendering/process creation stays in
 * the target adapter; this preserves the source order and conditional paths. */
#define WM_GAME_BEATEN_MAX_STEPS 24u

typedef enum wm_game_beaten_step {
    WM_GB_INIT_LADDER = 0,
    WM_GB_SET_INPARTY,
    WM_GB_SOUND_998,
    WM_GB_RANDOM_VICTORY_SOUND,
    WM_GB_FADE_MASTER_5_32,
    WM_GB_WIPEOUT,
    WM_GB_PAGE_FLIP_ON,
    WM_GB_START_CREDIT_BOX,
    WM_GB_PLAYER_SELECT_BACKGROUND,
    WM_GB_CLEAR_TIMEOUT,
    WM_GB_DISPLAY_CROUTONS,
    WM_GB_CLEAR_PLAYER_INITIALS,
    WM_GB_START_PLAYER_CURSOR,
    WM_GB_DO_BEATEN_GAME,
    WM_GB_CLEAR_ALL_INITIALS,
    WM_GB_DO_TAG_GAME_P1,
    WM_GB_DO_TAG_GAME_P2,
    WM_GB_WAIT_FOR_INITIAL_INPUT,
    WM_GB_WIPEOUT_FOR_TABLE,
    WM_GB_HSTD_BACKGROUND,
    WM_GB_PRINT_INTER,
    WM_GB_PRINT_BEATEN,
    WM_GB_PRINT_TAG,
    WM_GB_HOLD_SEVEN_SECONDS
} wm_game_beaten_step;

typedef struct wm_game_beaten_input {
    uint8_t pstatus;
    bool royal_rumble;
    bool rr_award_enabled; /* source compile-time RR_AWARD seam */
    bool belt_type;
    uint8_t selected_wrestler_index;
    int32_t match_timer_p1;
    int32_t match_timer_p2;
    int32_t display_audit_value;
} wm_game_beaten_input;

typedef struct wm_game_beaten_plan {
    wm_game_beaten_step steps[WM_GAME_BEATEN_MAX_STEPS];
    size_t step_count;
    uint8_t selected_player_side;
    uint8_t cursor_side;
    uint16_t fixed_sound_command;
    uint16_t random_sound_command;
    uint8_t fade_speed;
    uint8_t fade_frames;
    uint8_t display_level;
    uint8_t display_seconds;
    bool suppress_tag_table_display;
    uint32_t tag_match_time_bcd;
    WmCompletionHsRequest score_request_p1;
    WmCompletionHsRequest score_request_p2;
} wm_game_beaten_plan;

uint8_t wm_game_beaten_display_level(int32_t audit_value);
uint32_t wm_game_beaten_tag_time(int32_t p1_time_bcd, int32_t p2_time_bcd);
void wm_game_beaten_build_plan(const wm_game_beaten_input *in,
                               WmCompletionRngFn rng, void *rng_user,
                               wm_game_beaten_plan *out);

/* SELECT.ASM pin_speed_in_case + HSTD.ASM pin_speed_check gates. */
typedef enum wm_pin_speed_after_input {
    WM_PIN_SPEED_INPUT_EXIT = 0,
    WM_PIN_SPEED_INPUT_RESTART_SELECT_WAIT
} wm_pin_speed_after_input;

bool wm_pin_speed_prompt_context_allowed(uint16_t total_matches,
                                         uint8_t current_round,
                                         bool final_match);
wm_pin_speed_after_input wm_pin_speed_after_input_action(uint8_t saved_pstatus,
                                                          uint8_t live_pstatus);

uint8_t wm_match_music_command(bool royal_rumble, bool eight_on_one,
                               bool selector_bit);
bool wm_match_should_play_vince_intro(uint16_t total_matches, uint8_t pstatus);
bool wm_match_should_play_marble_voice(uint32_t rng_0_to_5);

/* -------------------------------------------------------------------------
 * STORIES.ASM: wrestler ending-story orchestration
 * ------------------------------------------------------------------------- */

#define WM_ARCADE_TICKS_PER_SECOND 60u
#define WM_STORY_LINES_PER_PAGE 12u
#define WM_STORY_LINE_START_Y 90
#define WM_STORY_LINE_DELTA_Y 12
#define WM_STORY_LINE_CUTOFF_Y 230
#define WM_STORY_MIN_PAGE_TICKS (5u * WM_ARCADE_TICKS_PER_SECOND)
#define WM_STORY_INPUT_WINDOW_TICKS (15u * WM_ARCADE_TICKS_PER_SECOND)
#define WM_STORY_POST_PRINT_DELAY_TICKS (WM_ARCADE_TICKS_PER_SECOND / 2u)
#define WM_STORY_FADE_WAIT_TICKS 30u
#define WM_STORY_FADE_STEPS 32u

/* These keys preserve the source table order without embedding target asset
 * addresses. Wrestler number 7 is explicitly invalid in STORIES.ASM. */
typedef enum wm_story_asset_key {
    WM_STORY_BRET = 0,
    WM_STORY_RAZOR,
    WM_STORY_TAKER,
    WM_STORY_YOKO,
    WM_STORY_SHAWN,
    WM_STORY_BAM,
    WM_STORY_DOINK,
    WM_STORY_INVALID,
    WM_STORY_LEX
} wm_story_asset_key;

typedef enum wm_story_step {
    WM_STORY_PAL_CLEAN = 0,
    WM_STORY_FADE_DOWN_32,
    WM_STORY_WAIT_30,
    WM_STORY_WIPEOUT,
    WM_STORY_PAGE_FLIP_ON,
    WM_STORY_PAL_CLEAN_AGAIN,
    WM_STORY_SET_BACKGROUND,
    WM_STORY_CREATE_MUG,
    WM_STORY_CREATE_LOGO,
    WM_STORY_CREATE_BELT,
    WM_STORY_PRINT_PAGES,
    WM_STORY_FINAL_INPUT_WINDOW,
    WM_STORY_DELETE_ART_OBJECTS,
    WM_STORY_DELETE_TEXT_OBJECTS
} wm_story_step;

#define WM_STORY_MAX_STEPS 16u

typedef struct wm_story_plan {
    bool valid;
    uint8_t wrestler_index;
    wm_story_asset_key asset_key;
    wm_story_step steps[WM_STORY_MAX_STEPS];
    size_t step_count;

    /* Source-space object/text placement. */
    int16_t mug_x;
    int16_t mug_y;
    int16_t logo_center_x;
    int16_t logo_center_y;
    int16_t belt_center_x;
    int16_t belt_center_y;
    int16_t text_x;
    int16_t text_start_y;
    int16_t text_delta_y;
    int16_t text_cutoff_y;

    uint16_t minimum_page_ticks;
    uint16_t page_input_window_ticks;
    uint16_t post_print_delay_ticks;
    uint16_t final_input_window_ticks;
    uint8_t fade_steps;
    uint8_t fade_wait_ticks;
    bool mug_flip_horizontal;
} wm_story_plan;

bool wm_story_wrestler_valid(uint8_t wrestler_index);
wm_story_asset_key wm_story_asset_for_wrestler(uint8_t wrestler_index);
uint8_t wm_story_winner_wrestler(uint8_t pstatus,
                                 uint8_t player1_wrestler,
                                 uint8_t player2_wrestler);
size_t wm_story_page_count(size_t line_count);
size_t wm_story_page_line_count(size_t line_count, size_t page_index);
int16_t wm_story_line_y(size_t line_on_page);
void wm_story_build_plan(uint8_t wrestler_index, wm_story_plan *out);

/* -------------------------------------------------------------------------
 * FIREWORK.ASM: finale/Royal-Rumble fireworks + figure-eight camera
 * ------------------------------------------------------------------------- */

#define WM_FW_FLARE_SOUND 1244u
#define WM_FW_EXPLOSION_SOUND 1252u
#define WM_FW_EXPLOSION_BASE_Y (-260)
#define WM_FW_EXPLOSION_WINDOW_TICKS (6u * WM_ARCADE_TICKS_PER_SECOND)
#define WM_FW_CHEER_WINDOW_TICKS (2u * WM_ARCADE_TICKS_PER_SECOND)
#define WM_FW_CHEER_INTERVAL_TICKS 15u
#define WM_FW_FIZZLE_WAIT_TICKS WM_ARCADE_TICKS_PER_SECOND
#define WM_FW_PAN_INITIAL_DELAY_TICKS (WM_ARCADE_TICKS_PER_SECOND / 2u)
#define WM_FW_CAMERA_TOLERANCE 3

typedef struct wm_fw_flare_position {
    int16_t x;
    int16_t y;
} wm_fw_flare_position;

typedef struct wm_fw_explosion {
    int16_t x;
    int16_t y;
    uint16_t z;
    uint8_t animation_variant;
    uint8_t palette_index;
    uint16_t sound_command;
} wm_fw_explosion;

typedef struct wm_fw_pan_point {
    uint16_t ticks;
    int16_t x;
    int16_t y;
} wm_fw_pan_point;

typedef struct wm_fw_camera_segment {
    int32_t target_x_q16;
    int32_t target_y_q16;
    int32_t dx_q16;
    int32_t dy_q16;
    int16_t target_x;
    int16_t target_y;
} wm_fw_camera_segment;

typedef enum wm_fw_congrats_mode {
    WM_FW_CONGRATS_1V3 = 0,
    WM_FW_CONGRATS_1V8 = 1,
    WM_FW_CONGRATS_2V8 = 2
} wm_fw_congrats_mode;

typedef enum wm_fw_cleanup_process {
    WM_FW_PROC_ANNC = 0,
    WM_FW_PROC_METER,
    WM_FW_PROC_TIMER,
    WM_FW_PROC_FLASH,
    WM_FW_PROC_ICON,
    WM_FW_PROC_SPECIAL_MOVE,
    WM_FW_PROC_PINHIM_ANIM,
    WM_FW_PROC_REWIRE,
    WM_FW_PROC_ZSHIFT,
    WM_FW_PROC_GETUP,
    WM_FW_PROC_FLASH_COMBO_P1,
    WM_FW_PROC_FLASH_COMBO_P2,
    WM_FW_PROC_CYCLER,
    WM_FW_PROC_FX,
    WM_FW_PROC_ADD_INITIALS,
    WM_FW_PROC_OVERHEAD
} wm_fw_cleanup_process;

typedef enum wm_fw_cleanup_object {
    WM_FW_OBJ_ANNOUNCER_TEXT = 0,
    WM_FW_OBJ_METER_FRAME,
    WM_FW_OBJ_METER_BAR,
    WM_FW_OBJ_TIMER_DIGIT,
    WM_FW_OBJ_WWF_ICON,
    WM_FW_OBJ_WINSTREAK,
    WM_FW_OBJ_PIN_HIM,
    WM_FW_OBJ_PERFECT
} wm_fw_cleanup_object;

typedef enum wm_fireworks_step {
    WM_FW_PAL_CLEAN = 0,
    WM_FW_FADE_DOWN_32,
    WM_FW_WAIT_30,
    WM_FW_KILL_MATCH_UI_PROCESSES,
    WM_FW_KNOCKOUT_DRONES,
    WM_FW_DELETE_MATCH_UI_OBJECTS,
    WM_FW_DISABLE_BOG_REDUCTION,
    WM_FW_WAKE_CROWD,
    WM_FW_PAL_CLEAN_AGAIN,
    WM_FW_FLASH_WHITE,
    WM_FW_PLAY_FLARE_SOUND,
    WM_FW_SPAWN_22_FLARES,
    WM_FW_FADE_UP_64,
    WM_FW_WAIT_16,
    WM_FW_FLASH_WHITE_AGAIN,
    WM_FW_PLAY_FLARE_SOUND_AGAIN,
    WM_FW_WAIT_60,
    WM_FW_START_PAN_PROCESS,
    WM_FW_EXPLOSIONS_FOR_360_TICKS,
    WM_FW_SET_PAN_DOWN,
    WM_FW_CHEER_FOR_120_TICKS,
    WM_FW_SET_FLARES_TO_FIZZLE,
    WM_FW_WAIT_FINAL_60
} wm_fireworks_step;

#define WM_FIREWORKS_MAX_STEPS 32u

typedef struct wm_fireworks_plan {
    wm_fireworks_step steps[WM_FIREWORKS_MAX_STEPS];
    size_t step_count;
    uint8_t fade_down_steps;
    uint8_t fade_up_steps;
    uint8_t fade_down_wait_ticks;
    uint8_t pre_pan_wait_ticks;
    uint16_t explosion_window_ticks;
    uint16_t cheer_window_ticks;
    uint8_t cheer_interval_ticks;
    uint8_t final_fizzle_wait_ticks;
    uint16_t flare_sound_command;
    uint16_t explosion_sound_command;
} wm_fireworks_plan;

size_t wm_firework_cleanup_process_count(void);
bool wm_firework_cleanup_process_get(size_t index, wm_fw_cleanup_process *out);
size_t wm_firework_cleanup_object_count(void);
bool wm_firework_cleanup_object_get(size_t index, wm_fw_cleanup_object *out);
size_t wm_firework_flare_count(void);
bool wm_firework_flare_get(size_t index, wm_fw_flare_position *out);
size_t wm_firework_pan_point_count(void);
bool wm_firework_pan_point_get(size_t index, wm_fw_pan_point *out);
uint16_t wm_firework_next_explosion_delay(WmCompletionRngFn rng, void *rng_user);
void wm_firework_make_explosion(WmCompletionRngFn rng, void *rng_user,
                                wm_fw_explosion *out);
wm_fw_congrats_mode wm_firework_congrats_mode(bool royal_rumble,
                                               bool eight_on_one);
void wm_firework_camera_segment_init(int32_t start_x_q16,
                                     int32_t start_y_q16,
                                     const wm_fw_pan_point *point,
                                     wm_fw_camera_segment *out);
bool wm_firework_camera_step(wm_fw_camera_segment *segment,
                             int32_t *world_x_q16,
                             int32_t *world_y_q16);
void wm_fireworks_build_plan(wm_fireworks_plan *out);


#ifdef __cplusplus
}
#endif
#endif
