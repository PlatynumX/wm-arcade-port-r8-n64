#ifndef WM_SELECT_SPRITES_H
#define WM_SELECT_SPRITES_H

#include <stddef.h>
#include "wm/bret_sprites.h"

const wm_source_sprite *wm_select_sprite_find(const char *source_frame);
const wm_source_sprite *wm_select_sprite_at(size_t index);
size_t wm_select_sprite_count(void);

#endif
