#ifndef WM_ARCADE_WIMP_FRAME_H
#define WM_ARCADE_WIMP_FRAME_H

#include <stdbool.h>
#include "wm/bret_sprites.h"
#include "wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WIMP image-directory tail mapping used by the wrestler collision image.
 * Hardware work already established zero-based tail slots 3/4 as the
 * channel-2 attachment X/Y pair (+0x26/+0x28 from the directory entry).
 * The following four signed words (+0x2A..+0x30) are the source IANI3
 * collision tuple consumed by COLLIS.ASM: X, Y, Z, ID.
 */
#define WM_WIMP_IANI3_X_SLOT 5
#define WM_WIMP_IANI3_Y_SLOT 6
#define WM_WIMP_IANI3_Z_SLOT 7
#define WM_WIMP_IANI3_ID_SLOT 8

bool wm_arcade_wimp_frame_box_from_tail(const int16_t tail[WM_WIMP_TAIL_WORDS],
                                         wm_arcade_frame_box_t *out);
bool wm_arcade_wimp_frame_box_from_sprite(const wm_source_sprite *sprite,
                                           wm_arcade_frame_box_t *out);

#ifdef __cplusplus
}
#endif
#endif
