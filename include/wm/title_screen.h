#ifndef WM_TITLE_SCREEN_H
#define WM_TITLE_SCREEN_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t source_id;
    uint16_t width;
    uint16_t height;
    uint8_t source_flag;
    const uint8_t *pixels_ci8;
} wm_title_background_image;

typedef struct {
    const char *name;
    uint16_t *rgba5551_opaque;
    uint16_t *rgba5551_keyed;
    uint16_t color_count;
} wm_title_background_palette;

size_t wm_title_background_image_count(void);
const wm_title_background_image *wm_title_background_image_at(size_t index);
size_t wm_title_background_palette_count(void);
const wm_title_background_palette *wm_title_background_palette_at(size_t index);
const char *wm_title_background_source_name(void);
uint16_t wm_title_background_source_origin_x(void);
uint16_t wm_title_background_source_origin_y(void);

#endif
