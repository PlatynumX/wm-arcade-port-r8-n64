#include "wm/render_equivalence.h"
#include "wm_arcade_wimp_frame.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    size_t count = 0u;
    const WmRenderLayerRule *rules = wm_render_layer_rules(&count);
    int16_t out = 0;
    wm_source_sprite sprite = {0};
    wm_arcade_frame_box_t box;

    assert(rules != NULL);
    assert(count >= 8u);
    assert(wm_render_layer_order_is_source_safe());
    assert(rules[0].layer == WM_RENDER_LAYER_CROWD_BACK);
    assert(rules[count - 1u].layer == WM_RENDER_LAYER_ATTRACT_OVERLAY);

    assert(wm_render_ci8_index_is_transparent(0u));
    assert(!wm_render_ci8_index_is_transparent(1u));
    assert(wm_render_source_coord_is_valid(0));
    assert(wm_render_source_coord_is_valid(4096));
    assert(!wm_render_source_coord_is_valid(4097));
    assert(wm_render_source_to_screen_i16(100, 25, &out));
    assert(out == 75);

    /* Renderer/collision frame metadata must fail closed on sentinel WIMP tails. */
    sprite.wimp_tail[WM_WIMP_IANI3_X_SLOT] = -1;
    sprite.wimp_tail[WM_WIMP_IANI3_Y_SLOT] = -1;
    sprite.wimp_tail[WM_WIMP_IANI3_Z_SLOT] = -1;
    sprite.wimp_tail[WM_WIMP_IANI3_ID_SLOT] = -1;
    assert(!wm_arcade_wimp_frame_box_from_sprite(&sprite, &box));

    sprite.wimp_tail[WM_WIMP_IANI3_X_SLOT] = 10;
    sprite.wimp_tail[WM_WIMP_IANI3_Y_SLOT] = 20;
    sprite.wimp_tail[WM_WIMP_IANI3_Z_SLOT] = 30;
    sprite.wimp_tail[WM_WIMP_IANI3_ID_SLOT] = 40;
    assert(wm_arcade_wimp_frame_box_from_sprite(&sprite, &box));
    assert(box.iani3x == 10);
    assert(box.iani3y == 20);
    assert(box.iani3z == 30);
    assert(box.iani3id == 40);

    printf("Renderer equivalence regression: PASS\n");
    printf("layers=%zu transparent_ci8=%u\n", count, WM_RENDER_TRANSPARENT_CI8_INDEX);
    return 0;
}
