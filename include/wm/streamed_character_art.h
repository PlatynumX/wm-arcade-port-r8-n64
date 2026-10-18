#ifndef WM_STREAMED_CHARACTER_ART_H
#define WM_STREAMED_CHARACTER_ART_H

#include <stddef.h>
#include <stdint.h>
#include "wm/bret_sprites.h"

#ifdef __cplusplus
extern "C" {
#endif

/* N64 DragonFS frame cache. `wrestler_num` is the original source roster id
   (0..6,8), not a display/select slot. The payload itself stays in ROM and
   only the requested frame plus its TLUT is resident in RDRAM. */
const wm_source_sprite *wm_n64_streamed_character_find(
    int32_t wrestler_num, const char *source_frame);
void wm_n64_streamed_character_reset(void);
size_t wm_n64_streamed_character_cached(void);

#ifdef __cplusplus
}
#endif
#endif
