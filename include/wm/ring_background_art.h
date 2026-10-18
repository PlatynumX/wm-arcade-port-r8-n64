#ifndef WM_RING_BACKGROUND_ART_H
#define WM_RING_BACKGROUND_ART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_bgnd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t width;
    uint16_t height;
    const uint8_t *pixels_ci8;
} wm_ring_background_image;

const wm_ring_background_image *wm_n64_ring_background_image(
    size_t header_index, uint16_t expected_width, uint16_t expected_height);

const uint16_t *wm_n64_ring_background_palette(
    unsigned palette_index, bool transparent);

void wm_n64_ring_background_sync(
    const wm_bgnd_state_t *state,
    const wm_bgnd_module_t *module);

void wm_n64_ring_background_reset(void);
size_t wm_n64_ring_background_cached_images(void);

#ifdef __cplusplus
}
#endif
#endif
