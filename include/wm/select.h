#ifndef WM_SELECT_H
#define WM_SELECT_H

#include <stdbool.h>
#include <stdint.h>
#include "wm/roster.h"
#include "wm/source_clock.h"

#define WM_SELECT_VISIBLE_SLOTS 8
#define WM_SELECT_SOURCE_WRESTLERS 9
#define WM_SELECT_TIME_TICKS (WM_SOURCE_TICKS_PER_SEC * 15)
#define WM_SELECT_FINAL_WAIT_TICKS 30
#define WM_SELECT_RND_MOVE_SPEED 5
#define WM_SELECT_RND_WANDER 14

typedef struct {
    int16_t x;
    int16_t y;
} wm_select_point;

typedef struct {
    const char *bmod_name;
    int16_t x;
    int16_t y;
} wm_select_bmod_entry;

typedef struct {
    uint8_t power;
    uint8_t speed;
    uint8_t agility;
    uint8_t recovery;
} wm_select_attributes;

typedef struct {
    uint8_t start_index;
    wm_select_point mug_position;
    bool mug_flip_x;
    uint16_t move_sound;
    uint16_t select_sound;
} wm_select_player_def;

typedef enum {
    WM_SOURCE_WRESTLER_BRET = 0,
    WM_SOURCE_WRESTLER_RAZOR = 1,
    WM_SOURCE_WRESTLER_UNDERTAKER = 2,
    WM_SOURCE_WRESTLER_YOKOZUNA = 3,
    WM_SOURCE_WRESTLER_SHAWN = 4,
    WM_SOURCE_WRESTLER_BAM_BAM = 5,
    WM_SOURCE_WRESTLER_DOINK = 6,
    WM_SOURCE_WRESTLER_ADAM_BOMB = 7,
    WM_SOURCE_WRESTLER_LEX = 8,
} wm_source_wrestler_id;

typedef enum {
    WM_SELECT_DIR_NONE = 0,
    WM_SELECT_DIR_DOWN,
    WM_SELECT_DIR_UP,
    WM_SELECT_DIR_LEFT,
    WM_SELECT_DIR_RIGHT,
} wm_select_direction;


typedef struct {
    uint8_t pstatus_snapshot;
    uint16_t ticks_remaining;
    bool time_out;
} wm_select_clock;

typedef struct {
    uint8_t index;
    uint8_t start_index;
    int8_t random_dest;       /* -1 when normal selection is active */
    uint8_t random_delay;
    uint8_t random_wander;
    bool selected;
    int8_t selected_source_wrestler;
} wm_select_cursor;

extern const wm_select_point wm_select_crouton_positions[WM_SELECT_VISIBLE_SLOTS];
extern const uint8_t wm_select_slot_source_wrestlers[WM_SELECT_VISIBLE_SLOTS];
extern const wm_select_attributes wm_select_source_attributes[WM_SELECT_SOURCE_WRESTLERS];
extern const wm_select_player_def wm_select_players[2];
extern const wm_select_bmod_entry wm_select_background_modules[2];

uint8_t wm_select_move(uint8_t index, wm_select_direction direction);
uint8_t wm_select_random_move(uint8_t index, uint8_t source_direction_roll,
                              bool source_fallback_roll);
uint8_t wm_select_home_move(uint8_t index, uint8_t destination,
                            uint8_t source_one_in_three_roll,
                            uint8_t source_direction_roll,
                            bool source_fallback_roll);
uint8_t wm_select_slot_to_source_wrestler(uint8_t slot);
bool wm_select_source_to_roster(uint8_t source_id, wm_wrestler_id *out);
void wm_select_cursor_init(wm_select_cursor *cursor, unsigned player);
bool wm_select_random_can_begin(const wm_select_cursor *cursor,
                                bool start_held, bool up_held);
bool wm_select_begin_random(wm_select_cursor *cursor, uint8_t source_destination);
/* One source random-selection movement event (the source schedules these every
   rnd_movespeed ticks). Returns true when the cursor has reached its final
   destination and the caller should take the same path as #but_hit. */
bool wm_select_random_event(wm_select_cursor *cursor,
                            uint8_t source_one_in_three_roll,
                            uint8_t source_direction_roll,
                            bool source_fallback_roll);
bool wm_select_choose(wm_select_cursor *cursor, uint8_t *source_wrestler_out);

void wm_select_clock_init(wm_select_clock *clock, uint8_t pstatus);
/* Execute one SELECT.ASM::select_clock source tick after its SLEEPK 1.
   A PSTATUS change resets the full 15-second window.  If OLD_PSTATUS is
   still nonzero at expiry, the source jumps back to #reset instead of
   asserting time_out. */
void wm_select_clock_tick(wm_select_clock *clock, uint8_t pstatus, uint8_t old_pstatus);

#endif
