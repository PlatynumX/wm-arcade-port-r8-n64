#ifndef WM_SPORTS_BACKGROUND_H
#define WM_SPORTS_BACKGROUND_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t width;
    uint16_t height;
    const uint8_t *pixels_ci8;
} wm_sports_background_image;

typedef struct {
    const char *name;
    uint16_t *rgba5551_opaque;
    uint16_t *rgba5551_keyed;
    uint16_t color_count;
} wm_sports_background_palette;

size_t wm_sports_background_image_count(void);
const wm_sports_background_image *wm_sports_background_image_at(size_t index);
size_t wm_sports_background_palette_count(void);
const wm_sports_background_palette *wm_sports_background_palette_at(size_t index);
#endif
