#ifndef WM_SELECT_SCREEN_H
#define WM_SELECT_SCREEN_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/audio.h"
#include "wm/roster.h"
#include "wm/select.h"

/*
 * Portable SELECT.ASM state for the one-player product path.
 * Rendering stays platform-side; all timing here is in original 53 Hz source ticks.
 */
typedef struct {
    wm_select_cursor p1;
    wm_select_clock clock; /* retained for ABI while SELECT.ASM timer is ported below */
    unsigned select_ticks_remaining;

    bool active;
    bool finished;
    bool howard_queued;
    bool name_pending;

    unsigned setup_ticks;
    unsigned manual_debounce;
    unsigned name_wait;
    unsigned final_wait;
    unsigned buyin_blink_countdown;
    bool buyin_name_visible;
    bool cursor_z_flip;

    bool prev_up;
    bool prev_down;
    bool prev_left;
    bool prev_right;

    uint8_t last_clock_digit;
    uint8_t player_pal_pref;
    uint8_t selected_source_wrestler;
    uint32_t rng_state;
} wm_select_screen_state;

void wm_select_screen_init(wm_select_screen_state *state);

void wm_select_screen_tick(wm_select_screen_state *state,
                           int stick_x, int stick_y,
                           bool start_pressed,
                           bool light_punch_pressed,
                           bool power_punch_pressed,
                           bool light_kick_pressed,
                           bool power_kick_pressed,
                           wm_audio_state *audio,
                           wm_wrestler_id *p1_choice);

uint8_t wm_select_screen_current_source(const wm_select_screen_state *state);
int wm_select_screen_clock_digit(const wm_select_screen_state *state);
bool wm_select_screen_highlight_visible(const wm_select_screen_state *state);

bool wm_select_screen_cursor_z_flipped(const wm_select_screen_state *state);
uint8_t wm_select_screen_player_palette_preference(const wm_select_screen_state *state);
#endif
