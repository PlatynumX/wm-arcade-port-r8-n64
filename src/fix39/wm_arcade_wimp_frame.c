#include "wm_arcade_wimp_frame.h"

static bool plausible_renderer_frame(const wm_arcade_frame_box_t *f)
{
    /* Renderer-facing WIMP asset guard. Sentinel -1 tuples are not wrestler
       collision images, and absurd dimensions indicate the wrong WIMP object.
       COLLIS.ASM source collision does NOT use this filter; it uses the raw
       CUR_FRAME tail adapter below. */
    if (!f) return false;
    if (f->iani3x == -1 && f->iani3y == -1 &&
        f->iani3z == -1 && f->iani3id == -1)
        return false;
    if (f->iani3z <= 0 || f->iani3id <= 0)
        return false;
    if (f->iani3x < -512 || f->iani3x > 512) return false;
    if (f->iani3y < -512 || f->iani3y > 512) return false;
    if (f->iani3z > 512 || f->iani3id > 512) return false;
    return true;
}

bool wm_arcade_wimp_frame_box_from_tail(
    const int16_t tail[WM_WIMP_TAIL_WORDS],
    wm_arcade_frame_box_t *out)
{
    if (!tail || !out) return false;

    /* COLLIS.ASM::set_collision_boxes reads IANI3X/Y/Z/ID directly from
       CUR_FRAME. Preserve those signed source words exactly: no range,
       sentinel, width, or height filter belongs on this collision path. */
    out->iani3x = tail[WM_WIMP_IANI3_X_SLOT];
    out->iani3y = tail[WM_WIMP_IANI3_Y_SLOT];
    out->iani3z = tail[WM_WIMP_IANI3_Z_SLOT];
    out->iani3id = tail[WM_WIMP_IANI3_ID_SLOT];
    return true;
}

bool wm_arcade_wimp_frame_box_from_sprite(
    const wm_source_sprite *sprite,
    wm_arcade_frame_box_t *out)
{
    wm_arcade_frame_box_t f;

    if (!sprite || !out) return false;

    f.iani3x = sprite->wimp_tail[WM_WIMP_IANI3_X_SLOT];
    f.iani3y = sprite->wimp_tail[WM_WIMP_IANI3_Y_SLOT];
    f.iani3z = sprite->wimp_tail[WM_WIMP_IANI3_Z_SLOT];
    f.iani3id = sprite->wimp_tail[WM_WIMP_IANI3_ID_SLOT];

    if (!plausible_renderer_frame(&f))
        return false;

    *out = f;
    return true;
}
