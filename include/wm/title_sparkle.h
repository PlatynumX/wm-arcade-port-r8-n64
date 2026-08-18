#ifndef WM_TITLE_SPARKLE_H
#define WM_TITLE_SPARKLE_H

#include <stddef.h>
#include "wm/bret_sprites.h"

/* Exact frame inventory from IMG/MISC.LOD -> sparkle.img. */
#define WM_TITLE_BIG_SPARKLE_FRAMES 15u
#define WM_TITLE_SMALL_SPARKLE_FRAMES 13u
#define WM_TITLE_SPARKLE_BIG_A_BASE 0u
#define WM_TITLE_SPARKLE_BIG_B_BASE 15u
#define WM_TITLE_SPARKLE_SMALL_A_BASE 30u
#define WM_TITLE_SPARKLE_SMALL_B_BASE 43u
#define WM_TITLE_SPARKLE_SMALL_C_BASE 56u
#define WM_TITLE_SPARKLE_SOURCE_FRAMES 69u

const wm_source_sprite *wm_title_sparkle_sprite_find(const char *source_frame);
const wm_source_sprite *wm_title_sparkle_sprite_at(size_t index);
size_t wm_title_sparkle_sprite_count(void);

#endif
