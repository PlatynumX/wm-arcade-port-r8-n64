#ifndef WMANIA_HISCORE_PRESENT_H
#define WMANIA_HISCORE_PRESENT_H

#include "wmania_hiscore_system.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Renderer-neutral port of the arcade high-score presentation contract.
 * Coordinates/assets remain adapter-side, while row selection, pairing,
 * titles, highlight indices, score interpretation and source timing units
 * live here.
 */

typedef struct {
    uint16_t rank;
    uint16_t physical_index;
    uint16_t partner_physical_index; /* tag only, otherwise 0 */
    uint8_t initials[WM_HS_NUM_INITIALS + 1u];
    uint8_t partner_initials[WM_HS_NUM_INITIALS + 1u];
    uint32_t score_bcd;
    uint32_t score_binary;
    uint8_t defeated_count;
    uint8_t defeated_wrestler_bits;
    int8_t wrestler_index; /* hidden 4th initial - 'A' for streak/pin */
    bool highlighted;
} WmHsDisplayRow;

typedef enum {
    WM_HS_PRESENT_INTER = 0,
    WM_HS_PRESENT_BEATEN = 1,
    WM_HS_PRESENT_TAG = 2,
    WM_HS_PRESENT_PIN = 3,
    WM_HS_PRESENT_STREAK = 4,
    WM_HS_PRESENT_COUNT = 5
} WmHsPresentScreen;

typedef struct {
    /* Source comments describe packed positions as [Y,X]. */
    int16_t first_initials_y;
    int16_t first_initials_x;
    int16_t first_score_y;
    int16_t first_score_x;
    int16_t row_y_step;

    /* Streak screen's second column; zero for single-column screens. */
    int16_t second_initials_y;
    int16_t second_initials_x;
    int16_t second_score_y;
    int16_t second_score_x;

    /* Shared title JAM_STR values from HSTD.ASM. */
    int16_t title_x;
    int16_t title_y;
    uint8_t title_space_width;
    const char *title_font_symbol;
    const char *title_palette_symbol;
} WmHsPresentLayout;

typedef struct {
    WmHsPresentScreen screen;
    WmHsTableId table_id;
    const char *title;
    uint8_t rows_at_once;
    uint8_t total_rows;
    uint8_t table_stride;
    bool scrolling;
    const char *source_background_symbol;
    WmHsPresentLayout layout;
} WmHsPresentDescriptor;

extern const WmHsPresentDescriptor wm_hs_present_sequence[WM_HS_PRESENT_COUNT];

/* Source timing is expressed in TSEC units; adapter supplies actual TSEC. */
#define WM_HS_PRESENT_TRANSITION_TSEC_NUM 1u
#define WM_HS_PRESENT_TRANSITION_TSEC_DEN 2u
#define WM_HS_PRESENT_FINAL_HOLD_TSEC 5u
#define WM_HS_PRESENT_SCROLL_GROUP_HOLD_TICKS 85u
#define WM_HS_PRESENT_SCROLL_STEP_TICKS_A 36u
#define WM_HS_PRESENT_SCROLL_STEP_TICKS_B 34u
#define WM_HS_PRESENT_SCROLL_LAST_OFF_TICKS 0x15u

/* Shared backdrop objects created by every table printer. */
#define WM_HS_PRESENT_BAR_X 10
#define WM_HS_PRESENT_BAR_Y 21
#define WM_HS_PRESENT_SHADOW_X 13
#define WM_HS_PRESENT_SHADOW_Y 30
#define WM_HS_PRESENT_BAR_SOURCE_SYMBOL "MVEBAR_R"
#define WM_HS_PRESENT_SHADOW_SOURCE_SYMBOL "SHADOW01"

/*
 * Fills renderer rows.
 * start_rank is 1-based for world/inter scrolling screens.
 * Other screens ignore it and emit their fixed source-visible rows.
 * Returns number of rows written.
 */
size_t wm_hs_present_rows(
    const WmHsSystem *system,
    WmHsPresentScreen screen,
    uint16_t start_rank,
    WmHsDisplayRow *out_rows,
    size_t capacity);

/* Arcade time display helper: BCDBIN then val_to_dec_tenths_asc. */
bool wm_hs_format_time(
    uint32_t time_bcd,
    char *buffer,
    size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif
