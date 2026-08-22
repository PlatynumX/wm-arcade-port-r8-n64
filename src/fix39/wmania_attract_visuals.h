#ifndef WMANIA_ATTRACT_VISUALS_H
#define WMANIA_ATTRACT_VISUALS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t y;
    uint16_t palette_index;
} WmAttractGradientRow;

/*
 * Source do_the_grad_thang renders 31 rising and 32 falling palette rows.
 * Output capacity must be at least 63.
 */
size_t wm_attract_aama_gradient(
    WmAttractGradientRow *out,
    size_t capacity);

typedef struct {
    int16_t y;
    int16_t x;
    const char *source_label;
} WmAttractTextPlacement;

/* Designer-hint title/body positions from SETUP_LINE_1/SETUP_LINE. */
size_t wm_attract_hint_placements(
    size_t hint_index,
    WmAttractTextPlacement *out,
    size_t capacity);

/* General-tip title plus 11 table rows. */
size_t wm_attract_general_tip_placements(
    WmAttractTextPlacement *out,
    size_t capacity);

/* Exact copyright line placements, page-local. */
size_t wm_attract_copyright_page1_placements(
    WmAttractTextPlacement *out,
    size_t capacity);
size_t wm_attract_copyright_page2_placements(
    WmAttractTextPlacement *out,
    size_t capacity);

/* AAMA placements: advisory, rating, severity, then three body lines. */
size_t wm_attract_aama_placements(
    WmAttractTextPlacement *out,
    size_t capacity);

/* show_time_date's six source-authored text anchors. Dynamic weekday/date/time
 * strings occupy the *_setup anchors; the platform supplies their formatted
 * text from wm_attract_format_clock(). */
size_t wm_attract_time_date_placements(
    WmAttractTextPlacement *out,
    size_t capacity);

/* show_operatormsg centers every CMOS line at x=200 starting at y=50 with a
 * 45-pixel source stride. source_label is NULL because the bytes are operator
 * data rather than ROM labels. */
size_t wm_attract_operator_placements(
    size_t line_count,
    WmAttractTextPlacement *out,
    size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
