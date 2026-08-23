#ifndef WM_RING_ROPE_ASSETS_H
#define WM_RING_ROPE_ASSETS_H
#include <stddef.h>
#include <stdint.h>
typedef struct { const char *symbol,*container,*path; uint16_t width,height; int16_t xani,yani; uint32_t pixel_bytes; uint16_t palette_offset,palette_colors; } wm_ring_rope_asset;
const wm_ring_rope_asset *wm_ring_rope_asset_find(const char *symbol);
size_t wm_ring_rope_asset_count(void);
#endif
