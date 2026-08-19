#ifndef WM_SELECT_BACKGROUND_H
#define WM_SELECT_BACKGROUND_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t source_id;
    uint16_t width;
    uint16_t height;
    uint8_t source_flag;
    const uint8_t *pixels_ci8;
} wm_select_background_image;

typedef struct {
    const char *name;
    uint16_t *rgba5551_opaque;
    uint16_t *rgba5551_keyed;
    uint16_t color_count;
} wm_select_background_palette;

size_t wm_select_main_image_count(void);
const wm_select_background_image *wm_select_main_image_at(size_t index);
size_t wm_select_main_palette_count(void);
const wm_select_background_palette *wm_select_main_palette_at(size_t index);
const char *wm_select_main_source_name(void);
uint16_t wm_select_main_source_origin_x(void);
uint16_t wm_select_main_source_origin_y(void);

size_t wm_select_choice_image_count(void);
const wm_select_background_image *wm_select_choice_image_at(size_t index);
size_t wm_select_choice_palette_count(void);
const wm_select_background_palette *wm_select_choice_palette_at(size_t index);
const char *wm_select_choice_source_name(void);
uint16_t wm_select_choice_source_origin_x(void);
uint16_t wm_select_choice_source_origin_y(void);

size_t wm_progress_image_count(void);
const wm_select_background_image *wm_progress_image_at(size_t index);
size_t wm_progress_palette_count(void);
const wm_select_background_palette *wm_progress_palette_at(size_t index);
const char *wm_progress_source_name(void);
uint16_t wm_progress_source_origin_x(void);
uint16_t wm_progress_source_origin_y(void);

#endif
