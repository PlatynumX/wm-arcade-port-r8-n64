#ifndef WM_BRET_SPRITES_H
#define WM_BRET_SPRITES_H
#include <stddef.h>
#include <stdint.h>

#define WM_WIMP_TAIL_WORDS 9

typedef struct {
    const char *source_frame;
    const char *source_container;
    uint16_t width;
    uint16_t height;
    int16_t xani;
    int16_t yani;
    /* Raw 18-byte metadata tail from the 0x32-byte WIMP image directory entry.
       r8 preserves this verbatim; hardware scanning established tail words 3/4 as the channel-2 attachment pair.  LOAD2's PWRD
       mapping is being identified on hardware instead of guessed again. */
    int16_t wimp_tail[WM_WIMP_TAIL_WORDS];
    const uint8_t *pixels_ci8;
    uint16_t *palette_rgba5551;
    uint16_t palette_colors;
} wm_source_sprite;

const wm_source_sprite *wm_bret_sprite_find(const char *source_frame);
size_t wm_bret_sprite_count(void);

#endif
