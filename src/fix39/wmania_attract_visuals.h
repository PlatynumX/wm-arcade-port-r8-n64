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

#ifdef __cplusplus
}
#endif

#endif
