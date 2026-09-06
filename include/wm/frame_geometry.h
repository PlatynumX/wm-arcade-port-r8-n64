#ifndef WM_FRAME_GEOMETRY_H
#define WM_FRAME_GEOMETRY_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Per-frame WIMP image geometry (width/height/xani/yani) for every Bret
 * source frame wm/bret_visuals.h and wm/bret_attacks.h reference, with no
 * pixel or palette data attached.
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
} wm_frame_geometry_t;

const wm_frame_geometry_t *wm_frame_geometry_find(const char *source_frame);
size_t wm_frame_geometry_count(void);

#ifdef __cplusplus
}
#endif

#endif
