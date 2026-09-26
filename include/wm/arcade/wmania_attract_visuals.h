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
    /*
     * ATTRACT.ASM's own .string for source_label, or NULL when this port has
     * not extracted that screen's text yet.
     *
     * It used to be only the label. wm_attract_copyright_page1_labels held
     * "copyright_ln1".."copyright_ln9", that string occurred in exactly one
     * place in the tree, and nothing resolved it -- so every one of these
     * placements gave a renderer an exact x, an exact y, and no words.
     *
     * NULL means "not extracted", and a renderer must skip the line rather
     * than substitute anything: invented copy on a copyright or an AAMA
     * advisory screen would be a false legal notice.
     */
    const char *text;
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
