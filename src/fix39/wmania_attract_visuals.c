#include "wmania_attract_visuals.h"

#include "wmania_attract_data.h"

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
