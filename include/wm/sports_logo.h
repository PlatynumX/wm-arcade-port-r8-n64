#ifndef WM_SPORTS_LOGO_H
#define WM_SPORTS_LOGO_H

#include <stddef.h>
#include "wm/bret_sprites.h"

#define WM_SPORTS_LOGO_PIECES 17

const wm_source_sprite *wm_sports_logo_sprite_find(const char *source_frame);
const wm_source_sprite *wm_sports_logo_sprite_at(size_t index);
size_t wm_sports_logo_sprite_count(void);

#endif
