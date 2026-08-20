#ifndef WM_PREGAME_H
#define WM_PREGAME_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/audio.h"
#include "wm/input.h"
#include "wm/roster.h"
#include "wm/progress_wrestlers.h"

/*
 * Source-level port of SELECT.ASM::pregame_show and the one-player portions
 * of PROGRESS.ASM::ask_belt_question / PUT_UP_PROGRESS.
 *
 * All phase timing is in the arcade's 53 Hz source ticks.  Rendering is kept
 * platform-side, exactly like the character-select port.
 */

typedef enum {
    WM_PREGAME_BELT_INTERCONTINENTAL = 0,
    WM_PREGAME_BELT_WWF = 1
} wm_pregame_belt;

typedef enum {
    WM_PREGAME_BELT_SETUP = 0,
    WM_PREGAME_BELT_SLIDE,
    WM_PREGAME_BELT_SELECT,
    WM_PREGAME_BELT_FLASH,
    WM_PREGAME_PROGRESS_SETUP,
    WM_PREGAME_PROGRESS_SCROLL,
    WM_PREGAME_PROGRESS_HOLD,
    WM_PREGAME_PROGRESS_CLOSE,
    WM_PREGAME_READY_FOR_MATCH
} wm_pregame_phase;

#define WM_PREGAME_LADDER_ENTRIES 15u
#define WM_PREGAME_PLAYABLE_LADDER_ENTRIES 7u
#define WM_PREGAME_MAX_OPPONENTS 3u

typedef struct {
    uint32_t packed; /* source long: count|op3|op2|op1 */
} wm_pregame_ladder_entry;

#define WM_PREGAME_PROGRESS_BITS 40u

typedef struct {
    int32_t x_fp;
    int32_t y_fp;
    int32_t xvel_fp;
    int32_t yvel_fp;
    uint16_t delay;
    uint8_t kind;
    uint8_t anim_index;
    bool anim_started;
    bool active;
} wm_progress_bit;

typedef struct {
    wm_pregame_phase phase;
    wm_pregame_belt belt_type;

    /* SELECT/PROGRESS source wrestler number: Bret=0 ... Lex=8, slot 7 spare. */
    uint8_t player_source_wrestler;
    wm_wrestler_id player_roster_wrestler;

    /* INIT_LADDER_TABLE / CURRENT_LADDER state. */
    wm_pregame_ladder_entry ladder[WM_PREGAME_LADDER_ENTRIES];
    int current_ladder_index;
    uint8_t opponent_count;
    uint8_t opponents[WM_PREGAME_MAX_OPPONENTS];
    uint32_t rng_state; /* isolated portable bridge for missing RNDRNG0 primitive */

    /* PROGRESS.ASM world scroll registers represented as source pixels/fixed 16.16. */
    int belt_world_y;
    int32_t progress_world_x_fp;
    /* PROGRESS.ASM::CREATE_TEMP_WRESTLER / TEMP_SPEED actor state. */
    int32_t progress_player_x_fp;
    int32_t progress_temp_speed_fp;
    wm_progress_action progress_player_action;
    wm_progress_action progress_opponent_action;
    unsigned progress_player_anim_ticks;
    unsigned progress_opponent_anim_ticks;

    unsigned phase_ticks;
    unsigned belt_anim_ticks; /* palette_cycle: advances throughout title question */
    unsigned belt_wait_ticks;
    unsigned flash_ticks;
    unsigned progress_counter;
    unsigned flash_frame;
    /* PROGRESS.ASM CLOSE_PROGRESS_SCREEN state. */
    unsigned progress_close_delay;
    unsigned progress_close_speed;
    unsigned progress_close_move_ticks;
    unsigned progress_close_post_ticks;
    int8_t progress_shake_x;
    int8_t progress_shake_y;
    wm_progress_bit progress_bits[WM_PREGAME_PROGRESS_BITS];
    bool progress_bits_created;
    unsigned match_count;
    unsigned win_streak; /* PROGRESS.ASM p1winstreak/p2winstreak display value. */

    bool finished;
    bool ready_for_match;
} wm_pregame_state;

void wm_pregame_init(wm_pregame_state *state,
                     uint8_t selected_source_wrestler,
                     wm_wrestler_id selected_roster_wrestler);

void wm_pregame_tick(wm_pregame_state *state,
                     const wm_input_state *input,
                     wm_audio_state *audio);

uint8_t wm_pregame_opponent_at(const wm_pregame_state *state, unsigned index);
const char *wm_pregame_phase_name(wm_pregame_phase phase);

#endif
