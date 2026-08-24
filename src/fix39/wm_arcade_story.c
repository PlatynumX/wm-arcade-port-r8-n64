#include "wm_arcade_completion.h"

#include <string.h>

static void story_append(wm_story_plan *p, wm_story_step step) {
    if (p->step_count < WM_STORY_MAX_STEPS)
        p->steps[p->step_count++] = step;
}

bool wm_story_wrestler_valid(uint8_t wrestler_index) {
    return wrestler_index <= 8u && wrestler_index != 7u;
}

wm_story_asset_key wm_story_asset_for_wrestler(uint8_t wrestler_index) {
    static const wm_story_asset_key table[9] = {
        WM_STORY_BRET, WM_STORY_RAZOR, WM_STORY_TAKER,
        WM_STORY_YOKO, WM_STORY_SHAWN, WM_STORY_BAM,
        WM_STORY_DOINK, WM_STORY_INVALID, WM_STORY_LEX
    };
    if (wrestler_index >= 9u) return WM_STORY_INVALID;
    return table[wrestler_index];
}

uint8_t wm_story_winner_wrestler(uint8_t pstatus,
                                 uint8_t player1_wrestler,
                                 uint8_t player2_wrestler) {
    /* STORIES.ASM indexes which_player with (PSTATUS >> 1). Finale never
     * reaches the two-human PSTATUS=3 case, so source-side 0 selects P1 and
     * source-side 1 selects P2. Preserve that selection directly. */
    return (pstatus >> 1u) != 0u ? player2_wrestler : player1_wrestler;
}

size_t wm_story_page_count(size_t line_count) {
    if (line_count == 0u) return 0u;
    return (line_count + WM_STORY_LINES_PER_PAGE - 1u) /
           WM_STORY_LINES_PER_PAGE;
}

size_t wm_story_page_line_count(size_t line_count, size_t page_index) {
    size_t first = page_index * WM_STORY_LINES_PER_PAGE;
    size_t remain;
    if (first >= line_count) return 0u;
    remain = line_count - first;
    return remain < WM_STORY_LINES_PER_PAGE ? remain : WM_STORY_LINES_PER_PAGE;
}

int16_t wm_story_line_y(size_t line_on_page) {
    if (line_on_page >= WM_STORY_LINES_PER_PAGE) return -1;
    return (int16_t)(WM_STORY_LINE_START_Y +
                     (int16_t)(line_on_page * WM_STORY_LINE_DELTA_Y));
}

void wm_story_build_plan(uint8_t wrestler_index, wm_story_plan *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));

    out->wrestler_index = wrestler_index;
    out->asset_key = wm_story_asset_for_wrestler(wrestler_index);
    out->valid = wm_story_wrestler_valid(wrestler_index);
    if (!out->valid) return;

    /* Exact source-space placement from show_wrestler_end_story. */
    out->mug_x = 0x17a;
    out->mug_y = 0x0af;
    out->logo_center_x = 120;
    out->logo_center_y = 50;
    out->belt_center_x = 316;
    out->belt_center_y = 198;
    out->text_x = 15;
    out->text_start_y = WM_STORY_LINE_START_Y;
    out->text_delta_y = WM_STORY_LINE_DELTA_Y;
    out->text_cutoff_y = WM_STORY_LINE_CUTOFF_Y;

    out->minimum_page_ticks = WM_STORY_MIN_PAGE_TICKS;
    out->page_input_window_ticks = WM_STORY_INPUT_WINDOW_TICKS;
    out->post_print_delay_ticks = WM_STORY_POST_PRINT_DELAY_TICKS;
    out->final_input_window_ticks = WM_STORY_INPUT_WINDOW_TICKS;
    out->fade_steps = WM_STORY_FADE_STEPS;
    out->fade_wait_ticks = WM_STORY_FADE_WAIT_TICKS;
    out->mug_flip_horizontal = true;

    story_append(out, WM_STORY_PAL_CLEAN);
    story_append(out, WM_STORY_FADE_DOWN_32);
    story_append(out, WM_STORY_WAIT_30);
    story_append(out, WM_STORY_WIPEOUT);
    story_append(out, WM_STORY_PAGE_FLIP_ON);
    story_append(out, WM_STORY_PAL_CLEAN_AGAIN);
    story_append(out, WM_STORY_SET_BACKGROUND);
    story_append(out, WM_STORY_CREATE_MUG);
    story_append(out, WM_STORY_CREATE_LOGO);
    story_append(out, WM_STORY_CREATE_BELT);
    story_append(out, WM_STORY_PRINT_PAGES);
    story_append(out, WM_STORY_FINAL_INPUT_WINDOW);
    story_append(out, WM_STORY_DELETE_ART_OBJECTS);
    story_append(out, WM_STORY_DELETE_TEXT_OBJECTS);
}
