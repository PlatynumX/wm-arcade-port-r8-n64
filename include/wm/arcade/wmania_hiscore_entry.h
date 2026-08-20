#ifndef WMANIA_HISCORE_ENTRY_H
#define WMANIA_HISCORE_ENTRY_H

#include "wm/arcade/wmania_hiscore_core.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WM_HS_ENTRY_GRID_COUNT 30u
#define WM_HS_ENTRY_GRID_COLUMNS 5u
#define WM_HS_ENTRY_TIMER_TICKS 0x700u
#define WM_HS_ENTRY_COUNTDOWN_START 750u
#define WM_HS_ENTRY_COUNTDOWN_DIVISOR 150u
#define WM_HS_ENTRY_REPEAT_STATIC 30u
#define WM_HS_ENTRY_REPEAT_MOVING 10u
#define WM_HS_ENTRY_DEBOUNCE 8u

#define WM_HS_ENTRY_DELETE_CELL 28u
#define WM_HS_ENTRY_END_CELL 29u

/*
 * Stick bits are source RLDU order:
 * bit0=Up, bit1=Down, bit2=Left, bit3=Right.
 * The original joytab is indexed by all four bits simultaneously.
 */
enum {
    WM_HS_STICK_UP    = 1u << 0,
    WM_HS_STICK_DOWN  = 1u << 1,
    WM_HS_STICK_LEFT  = 1u << 2,
    WM_HS_STICK_RIGHT = 1u << 3
};

typedef enum {
    WM_HS_ENTRY_THREE_PLUS_WRESTLER = 3,
    WM_HS_ENTRY_FIVE = 5
} WmHsEntryLength;

typedef enum {
    WM_HS_ENTRY_EVENT_NONE       = 0,
    WM_HS_ENTRY_EVENT_MOVE       = 1u << 0,
    WM_HS_ENTRY_EVENT_ADD        = 1u << 1,
    WM_HS_ENTRY_EVENT_COUNTDOWN  = 1u << 2,
    WM_HS_ENTRY_EVENT_SELECT     = 1u << 3,
    WM_HS_ENTRY_EVENT_FINISHED   = 1u << 4,
    WM_HS_ENTRY_EVENT_REPLACED   = 1u << 5,
    WM_HS_ENTRY_EVENT_TJM_VOICE  = 1u << 6,
    WM_HS_ENTRY_EVENT_SMJ_VOICE  = 1u << 7
} WmHsEntryEvent;

typedef struct {
    uint8_t stick_current;
    uint8_t stick_down;
    bool accept_down;
} WmHsEntryInput;

typedef struct {
    int16_t grid_x;
    int16_t grid_y;
    int16_t initials_x;
    int16_t initials_y;
    int16_t prompt_x;
    int16_t prompt_y;
    int16_t mode_title_x;
    int16_t mode_title_y;
    int16_t streak_label_x;
    int16_t streak_label_y;
    const char *initials_palette_symbol;
    const char *prompt_palette_symbol;
} WmHsEntryLayout;

/* Exact source positions for player 1 / player 2. */
extern const WmHsEntryLayout wm_hs_entry_layout[2];

#define WM_HS_ENTRY_BLOCK_SIZE 18
#define WM_HS_ENTRY_BACKGROUND_SOURCE_SYMBOL "wwfselbkBMOD"
#define WM_HS_ENTRY_BACKGROUND_X_OFFSET (-44)
#define WM_HS_ENTRY_BACKGROUND_Y_OFFSET 0
#define WM_HS_ENTRY_INITIALS_FONT_SYMBOL "osgemd_ascii"
#define WM_HS_ENTRY_PROMPT_FONT_SYMBOL "font9"
#define WM_HS_ENTRY_INITIALS_SPACING 10
#define WM_HS_ENTRY_PROMPT_SPACING 8

typedef uint32_t (*WmHsRandomRangeFn)(
    void *user,
    uint32_t max_inclusive);

typedef struct {
    WmHsEntryLength length;
    uint8_t player_index;
    uint8_t wrestler_index;

    uint8_t initials[WM_HS_NUM_INITIALS];
    uint8_t cursor_index;
    uint8_t committed;
    uint16_t timer_ticks;

    int16_t repeat_ticks;
    int16_t debounce_ticks;
    int8_t countdown_digit;

    bool finished;
    bool finalized;
    bool replacement_used;

    WmHsRandomRangeFn random_range;
    void *random_user;
} WmHsEntryState;

extern const uint8_t wm_hs_entry_grid_display[WM_HS_ENTRY_GRID_COUNT];
extern const uint8_t wm_hs_entry_grid_values[WM_HS_ENTRY_GRID_COUNT];

void wm_hs_entry_begin(
    WmHsEntryState *state,
    WmHsEntryLength length,
    uint8_t player_index,
    uint8_t wrestler_index,
    WmHsRandomRangeFn random_range,
    void *random_user);

/*
 * One source tick. Call once per emulated arcade game tick, not once per
 * arbitrary N64 render frame unless those clocks are intentionally coupled.
 */
uint32_t wm_hs_entry_tick(
    WmHsEntryState *state,
    const WmHsEntryInput *input);

/* Final five bytes to pass into an insertion routine. */
void wm_hs_entry_get_initials(
    const WmHsEntryState *state,
    uint8_t out[WM_HS_NUM_INITIALS]);

/* Source dirty-word filter and empty-initials check. */
bool wm_hs_entry_is_dirty(const uint8_t initials[WM_HS_NUM_INITIALS]);
bool wm_hs_entry_is_empty(const uint8_t initials[WM_HS_NUM_INITIALS]);

/* Cursor grid coordinates, direct 5x6 source layout. */
uint8_t wm_hs_entry_cursor_col(const WmHsEntryState *state);
uint8_t wm_hs_entry_cursor_row(const WmHsEntryState *state);

#ifdef __cplusplus
}
#endif

#endif
