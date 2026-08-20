#ifndef WM_SELECT_CONTINUE_H
#define WM_SELECT_CONTINUE_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/audio.h"
#include "wm/award.h"

/* SELECT.ASM source constants. */
#define WM_SELECT_CONTINUE_INITIAL_DIGIT 9u
#define WM_SELECT_CONTINUE_RESETS_ALLOWED 25u
#define WM_SELECT_CONTINUE_TICKS_PER_DIGIT (53u * 2u)
#define WM_SELECT_CONTINUE_TIMER_YPOS 208
#define WM_SELECT_CONTINUE_P1_CENTER_X 81
#define WM_SELECT_CONTINUE_P2_CENTER_X 321
#define WM_SELECT_CONTINUE_PLEASE_YPOS 80
#define WM_SELECT_CONTINUE_TEXT_YPOS 95
#define WM_SELECT_CONTINUE_CREDITS_YPOS 110
#define WM_SELECT_CONTINUE_TO_YPOS 125
#define WM_SELECT_CONTINUE_FREEPLAY_YPOS 120

typedef enum {
    WM_SELECT_CONTINUE_IDLE = 0,
    WM_SELECT_CONTINUE_WAIT_INITIALS,
    WM_SELECT_CONTINUE_OFFER,
    WM_SELECT_CONTINUE_ACCEPTED,
    WM_SELECT_CONTINUE_TIMEOUT
} wm_select_continue_phase;

typedef enum {
    WM_SELECT_CONTINUE_NO_EVENT = 0,
    WM_SELECT_CONTINUE_ACCEPT_EVENT,
    WM_SELECT_CONTINUE_TIMEOUT_EVENT
} wm_select_continue_event;

typedef struct {
    wm_select_continue_phase phase;
    uint8_t player;
    uint8_t source_wrestler;
    uint8_t digit;
    uint8_t resets_remaining;
    uint8_t credits_needed;
    uint16_t subcounter;
    uint32_t credit_snapshot;
    unsigned blink_countdown;
    bool initials_active;
    bool free_play;
    bool can_continue;
    bool prompt_visible;
} wm_select_continue_state;

void wm_select_continue_init(wm_select_continue_state *state);

/*
 * Direct portable entry corresponding to the losing-player path reached by
 * SELECT.ASM::buyin_select/player_cursor after OLD_PSTATUS says this player
 * lost the previous match.
 *
 * HI_INPUT_PID is represented by initials_active.  Cabinet credits/PSTATUS
 * remain external inputs rather than being fabricated inside this module.
 */
void wm_select_continue_begin(wm_select_continue_state *state,
                              unsigned player,
                              uint8_t source_wrestler,
                              bool initials_active,
                              uint32_t credits,
                              uint8_t credits_needed,
                              bool free_play,
                              bool can_continue,
                              wm_award_state *awards);

void wm_select_continue_set_initials_active(wm_select_continue_state *state,
                                            bool active);
void wm_select_continue_set_can_continue(wm_select_continue_state *state,
                                         bool can_continue);

/*
 * One original 53 Hz source tick of SELECT.ASM::buyin_counter.
 *
 * player_active is the translated PSTATUS bit. either_start_down is the
 * source behavior where either player's Start resets this timer. The final
 * two button arguments are player-specific get_but_val_down/current.
 */
wm_select_continue_event wm_select_continue_tick(
    wm_select_continue_state *state,
    bool player_active,
    uint32_t credits,
    bool either_start_down,
    bool player_button_down,
    bool player_button_current,
    wm_audio_state *audio,
    wm_award_state *awards);

bool wm_select_continue_visible(const wm_select_continue_state *state);

#endif
