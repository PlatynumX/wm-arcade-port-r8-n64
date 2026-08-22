#include "wm_arcade_wimp_frame.h"

static bool plausible(const wm_arcade_frame_box_t *f)
{
    /* Source WIMP coordinates are small signed pixel-space values.  Sentinel
       -1 tuples are not collision images.  Keep this adapter fail-closed if a
       non-wrestler WIMP object reaches it. */
    if (!f) return false;
    if (f->iani3x == -1 && f->iani3y == -1 && f->iani3z == -1 && f->iani3id == -1)
        return false;
    if (f->iani3z <= 0 || f->iani3id <= 0)
        return false;
    if (f->iani3x < -512 || f->iani3x > 512) return false;
    if (f->iani3y < -512 || f->iani3y > 512) return false;
    if (f->iani3z > 512 || f->iani3id > 512) return false;
    return true;
}

bool wm_arcade_wimp_frame_box_from_sprite(const wm_source_sprite *sprite,
                                           wm_arcade_frame_box_t *out)
{
    wm_arcade_frame_box_t f;
    if (!sprite || !out) return false;
    f.iani3x = sprite->wimp_tail[WM_WIMP_IANI3_X_SLOT];
    f.iani3y = sprite->wimp_tail[WM_WIMP_IANI3_Y_SLOT];
    f.iani3z = sprite->wimp_tail[WM_WIMP_IANI3_Z_SLOT];
    f.iani3id = sprite->wimp_tail[WM_WIMP_IANI3_ID_SLOT];
    if (!plausible(&f)) return false;
    *out = f;
    return true;
}
