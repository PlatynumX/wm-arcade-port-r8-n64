#ifndef WM_RING_ARENA_ASSETS_H
#define WM_RING_ARENA_ASSETS_H
#include <stddef.h>
#include <stdint.h>
typedef struct { uint16_t width,height,ctrl; const char *path; } wm_ring_arena_image;
typedef struct { const char *name; uint16_t color_count; uint16_t *rgba5551_opaque; uint16_t *rgba5551_keyed; } wm_ring_arena_palette;
typedef struct { uint8_t palette,flags,z; int16_t x,y; uint16_t header_index; } wm_ring_arena_block;
uint16_t wm_ring_arena_width(void);
uint16_t wm_ring_arena_height(void);
size_t wm_ring_arena_block_count(void);
const wm_ring_arena_block *wm_ring_arena_block_at(size_t i);
const wm_ring_arena_image *wm_ring_arena_image_at(size_t i);
const wm_ring_arena_palette *wm_ring_arena_palette_at(size_t i);
#endif
