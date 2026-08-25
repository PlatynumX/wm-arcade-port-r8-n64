#ifndef WM_RENDER_EQUIVALENCE_H
#define WM_RENDER_EQUIVALENCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WM_RENDER_TRANSPARENT_CI8_INDEX 0u
#define WM_RENDER_SOURCE_COORD_MIN (-4096)
#define WM_RENDER_SOURCE_COORD_MAX (4096)

typedef enum {
    WM_RENDER_LAYER_CROWD_BACK = 10,
    WM_RENDER_LAYER_RING_FLOOR = 20,
    WM_RENDER_LAYER_WRESTLER_SHADOW = 30,
    WM_RENDER_LAYER_ROPES_BACK = 40,
    WM_RENDER_LAYER_WRESTLER_BODY = 50,
    WM_RENDER_LAYER_SPECIAL_OBJECT = 60,
    WM_RENDER_LAYER_ROPES_FRONT = 70,
    WM_RENDER_LAYER_HUD_TEXT = 80,
    WM_RENDER_LAYER_ATTRACT_OVERLAY = 90
} WmRenderLayer;

typedef struct {
    WmRenderLayer layer;
    const char *source_owner;
    const char *reason;
} WmRenderLayerRule;

const WmRenderLayerRule *wm_render_layer_rules(size_t *count_out);
bool wm_render_layer_order_is_source_safe(void);
bool wm_render_ci8_index_is_transparent(uint8_t index);
bool wm_render_source_coord_is_valid(int32_t coord);
bool wm_render_source_to_screen_i16(int32_t source_coord,
                                    int16_t world_origin,
                                    int16_t *out_screen_coord);

#ifdef __cplusplus
}
#endif

#endif
