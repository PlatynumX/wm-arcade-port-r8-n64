#include "wm/render_equivalence.h"

static const WmRenderLayerRule k_rules[] = {
    { WM_RENDER_LAYER_CROWD_BACK, "generated crowd/background assets", "background never covers ring actors" },
    { WM_RENDER_LAYER_RING_FLOOR, "ring arena assets", "ring mat below wrestlers and specials" },
    { WM_RENDER_LAYER_WRESTLER_SHADOW, "wrestler presentation", "source actor shadow under body" },
    { WM_RENDER_LAYER_ROPES_BACK, "ROPES.ASM back half", "back rope may pass behind actors" },
    { WM_RENDER_LAYER_WRESTLER_BODY, "WIMP/IANI3 wrestler image", "actor body between rope halves" },
    { WM_RENDER_LAYER_SPECIAL_OBJECT, "SPECIAL.ASM objects", "projectiles/objects over body before HUD" },
    { WM_RENDER_LAYER_ROPES_FRONT, "ROPES.ASM front half", "front rope may cover actors" },
    { WM_RENDER_LAYER_HUD_TEXT, "HSTD/SELECT/text", "screen text over gameplay sprites" },
    { WM_RENDER_LAYER_ATTRACT_OVERLAY, "ATTRACT presentation", "frontend overlays above background art" },
};

const WmRenderLayerRule *wm_render_layer_rules(size_t *count_out)
{
    if (count_out) *count_out = sizeof(k_rules) / sizeof(k_rules[0]);
    return k_rules;
}

bool wm_render_layer_order_is_source_safe(void)
{
    size_t i;
    for (i = 1u; i < sizeof(k_rules) / sizeof(k_rules[0]); ++i) {
        if ((int)k_rules[i - 1u].layer >= (int)k_rules[i].layer) return false;
    }
    return true;
}

bool wm_render_ci8_index_is_transparent(uint8_t index)
{
    return index == WM_RENDER_TRANSPARENT_CI8_INDEX;
}

bool wm_render_source_coord_is_valid(int32_t coord)
{
    return coord >= WM_RENDER_SOURCE_COORD_MIN && coord <= WM_RENDER_SOURCE_COORD_MAX;
}

bool wm_render_source_to_screen_i16(int32_t source_coord,
                                    int16_t world_origin,
                                    int16_t *out_screen_coord)
{
    int32_t v;
    if (!out_screen_coord || !wm_render_source_coord_is_valid(source_coord)) return false;
    v = source_coord - (int32_t)world_origin;
    if (v < -32768 || v > 32767) return false;
    *out_screen_coord = (int16_t)v;
    return true;
}
