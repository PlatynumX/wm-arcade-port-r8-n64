#ifndef WM_FRAME_GEOMETRY_H
#define WM_FRAME_GEOMETRY_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Per-frame WIMP image geometry (width, height, animation origin and
 * the channel-2 attachment pair) for every source frame the port can
 * show, across the whole roster, with no pixel or palette data
 * attached.
 *
 * This exists as its own small generated table (tools/bret_geometry_bundle.py,
 * src/generated/frame_geometry.c) distinct from wm/bret_sprites.h's
 * wm_source_sprite (which also carries CI8 pixels + an RGB555 palette, and
 * is only compiled into the N64 asset build -- see the Makefile's ASSET_C
 * list). Hit-box math is engine logic that has to run identically on host
 * ctest and on-console; it must not depend on which pixel data happens to
 * be linked in, especially once stock artwork is replaced. See
 * wm_hurt_box_for_frame (wm/bret_backend.h) for the one thing this
 * table currently feeds.
 */
typedef struct {
    const char *source_frame;
    uint16_t width;
    uint16_t height;
    int16_t xani;
    int16_t yani;
    /*
     * The channel-2 attachment pair -- WIMP tail words 3 and 4, which
     * hardware scanning established as where the SECOND animation
     * channel hangs off this frame. (-1, -1) means the frame carries
     * no attachment and the torso layer is not drawn over it.
     *
     * Geometry, not pixels, for the same reason the sizes above are
     * here: the composite offset has to come out the same on host
     * ctest and on console, and must not depend on which wrestler's
     * artwork happens to be linked in.
     */
    int16_t attach_x;
    int16_t attach_y;
} wm_frame_geometry_t;

const wm_frame_geometry_t *wm_frame_geometry_find(const char *source_frame);
size_t wm_frame_geometry_count(void);

#ifdef __cplusplus
}
#endif

#endif
