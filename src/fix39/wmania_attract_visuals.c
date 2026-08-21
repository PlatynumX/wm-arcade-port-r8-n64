#include "wmania_attract_visuals.h"

#include "wmania_attract_data.h"
#include "wmania_attract_time.h"

size_t wm_attract_aama_gradient(
    WmAttractGradientRow *out,
    size_t capacity)
{
    size_t n = 0u;
    int p;

    if (out == 0 || capacity < 63u) {
        return 0u;
    }

    /*
     * do_the_grad_thang starts palette 31 and advances destination by two
     * pixels per row for 31 rows, then reverses for 32 rows.
     * Portable output records the palette staircase; renderer chooses the
     * translated framebuffer/RDP primitive.
     */
    for (p = 31; p >= 1; --p) {
        out[n].y = (int16_t)(n * 2u);
        out[n].palette_index = (uint16_t)p;
        ++n;
    }

    for (p = 32; p >= 1; --p) {
        out[n].y = (int16_t)(n * 2u + 0x84);
        out[n].palette_index = (uint16_t)(32 - p);
        ++n;
    }

    return n;
}


size_t wm_attract_hint_placements(
    size_t hint_index,
    WmAttractTextPlacement *out,
    size_t capacity)
{
    const WmAttractHint *hint;
    size_t i;
    size_t need;

    if (hint_index >= WM_ATTRACT_ACTIVE_HINTS) return 0u;
    hint = &wm_attract_hints[hint_index];
    need = 1u + hint->body_line_count;
    if (out == 0 || capacity < need) return 0u;

    out[0].x = WM_ATTRACT_HINT_TITLE_X;
    out[0].y = WM_ATTRACT_HINT_TITLE_Y;
    out[0].source_label = hint->title_label;
    for (i = 0u; i < hint->body_line_count; ++i) {
        out[i + 1u].x = WM_ATTRACT_HINT_BODY_X;
        out[i + 1u].y = (int16_t)(WM_ATTRACT_HINT_BODY_Y +
                                  (int)i * WM_ATTRACT_HINT_BODY_LINE_STEP);
        out[i + 1u].source_label = hint->body_line_labels[i];
    }
    return need;
}

size_t wm_attract_general_tip_placements(
    WmAttractTextPlacement *out,
    size_t capacity)
{
    size_t i;
    const size_t need = 1u + WM_FIX39_ATTRACT_GENERAL_TIP_ROWS;
    if (out == 0 || capacity < need) return 0u;
    out[0].x = WM_FIX39_ATTRACT_GENERAL_TIPS_TITLE_X;
    out[0].y = WM_FIX39_ATTRACT_GENERAL_TIPS_TITLE_Y;
    out[0].source_label = WM_FIX39_ATTRACT_GENERAL_TIPS_TITLE_LABEL;
    for (i = 0u; i < WM_FIX39_ATTRACT_GENERAL_TIP_ROWS; ++i) {
        out[i + 1u].x = 200;
        out[i + 1u].y = (int16_t)(WM_FIX39_ATTRACT_GENERAL_TIPS_FIRST_Y +
                                  (int)i * WM_FIX39_ATTRACT_GENERAL_TIPS_LINE_STEP);
        out[i + 1u].source_label = wm_attract_general_tip_labels[i];
    }
    return need;
}

size_t wm_attract_copyright_page1_placements(
    WmAttractTextPlacement *out,
    size_t capacity)
{
    size_t i;

    if (out == 0 || capacity < WM_FIX39_ATTRACT_COPYRIGHT_PAGE1_LINES) {
        return 0u;
    }

    for (i = 0; i < WM_FIX39_ATTRACT_COPYRIGHT_PAGE1_LINES; ++i) {
        out[i].y = (int16_t)(
            WM_FIX39_ATTRACT_COPYRIGHT_FIRST_Y +
            (int)i * WM_FIX39_ATTRACT_COPYRIGHT_LINE_STEP);
        out[i].x = WM_FIX39_ATTRACT_COPYRIGHT_X;
        out[i].source_label = wm_attract_copyright_page1_labels[i];
    }

    return WM_FIX39_ATTRACT_COPYRIGHT_PAGE1_LINES;
}

size_t wm_attract_copyright_page2_placements(
    WmAttractTextPlacement *out,
    size_t capacity)
{
    size_t i;

    if (out == 0 || capacity < WM_FIX39_ATTRACT_COPYRIGHT_PAGE2_LINES) {
        return 0u;
    }

    for (i = 0; i < WM_FIX39_ATTRACT_COPYRIGHT_PAGE2_LINES; ++i) {
        out[i].y = (int16_t)(
            WM_FIX39_ATTRACT_COPYRIGHT_FIRST_Y +
            (int)i * WM_FIX39_ATTRACT_COPYRIGHT_LINE_STEP);
        out[i].x = WM_FIX39_ATTRACT_COPYRIGHT_X;
        out[i].source_label = wm_attract_copyright_page2_labels[i];
    }

    return WM_FIX39_ATTRACT_COPYRIGHT_PAGE2_LINES;
}

size_t wm_attract_aama_placements(
    WmAttractTextPlacement *out,
    size_t capacity)
{
    static const int16_t y[6] = {
        94, 114, 114, 125, 136, 147
    };
    static const int16_t x[6] = {
        200, 0x00b1, 0x0104, 200, 200, 200
    };
    size_t i;

    if (out == 0 || capacity < 6u) {
        return 0u;
    }

    for (i = 0; i < 6u; ++i) {
        out[i].y = y[i];
        out[i].x = x[i];
        out[i].source_label = wm_attract_aama_labels[i];
    }

    return 6u;
}


size_t wm_attract_time_date_placements(
    WmAttractTextPlacement *out,
    size_t capacity)
{
    static const char *const labels[6] = {
        "date_time_prompt",
        "day_of_week_setup",
        "date_setup",
        "time_setup",
        "its_time_to_message",
        "wrestlemania_message"
    };
    static const int16_t y[6] = {
        WM_FIX39_ATTRACT_TIME_DATE_PROMPT_Y,
        WM_FIX39_ATTRACT_TIME_DATE_WEEKDAY_Y,
        WM_FIX39_ATTRACT_TIME_DATE_DATE_Y,
        WM_FIX39_ATTRACT_TIME_DATE_TIME_Y,
        WM_FIX39_ATTRACT_TIME_DATE_PLAY_PROMPT_Y,
        WM_FIX39_ATTRACT_TIME_DATE_WRESTLEMANIA_Y
    };
    size_t i;
    if (!out || capacity < 6u) return 0u;
    for (i = 0u; i < 6u; ++i) {
        out[i].x = 200;
        out[i].y = y[i];
        out[i].source_label = labels[i];
    }
    return 6u;
}

size_t wm_attract_operator_placements(
    size_t line_count,
    WmAttractTextPlacement *out,
    size_t capacity)
{
    size_t i;
    if (!out || capacity < line_count) return 0u;
    for (i = 0u; i < line_count; ++i) {
        out[i].x = WM_ATTRACT_OPERATOR_X;
        out[i].y = (int16_t)(WM_ATTRACT_OPERATOR_FIRST_Y +
                             (int)i * WM_ATTRACT_OPERATOR_LINE_STEP);
        out[i].source_label = 0;
    }
    return line_count;
}
