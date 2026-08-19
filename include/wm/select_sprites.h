#ifndef WM_SELECT_SPRITES_H
#define WM_SELECT_SPRITES_H

#include <stddef.h>
#include <stdint.h>
#include "wm/bret_sprites.h"

typedef struct {
    const char *source_name;
    uint16_t *rgba5551;
    uint16_t color_count;
} wm_select_palette;

const wm_source_sprite *wm_select_sprite_find(const char *source_frame);
const wm_source_sprite *wm_select_sprite_at(size_t index);
size_t wm_select_sprite_count(void);

/* Named palette lookup for live PROGRESS.ASM OPAL overrides.  These are
   generated from IMGPAL.ASM rather than inferred from a screenshot. */
const wm_select_palette *wm_select_palette_find(const char *source_name);

#endif
