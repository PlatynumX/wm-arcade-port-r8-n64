#include "wm_arcade_source_attack_frames.h"
#include <string.h>
typedef struct {
    const char *frame;
    unsigned char uses_z;
    uint16_t mode;
    int16_t x, y, z, w, h, d;
} wm_arcade_source_attack_frame_t;
#include "wm_arcade_bret_attack_frames_generated.h"

bool wm_arcade_bret_attack_for_source_frame(const char *source_frame,
                                             bool *uses_z,
                                             wm_arcade_attack_on_z_args_t *zargs,
                                             wm_arcade_attack_on_args_t *args)
{
    if (!source_frame || !uses_z || !zargs || !args) return false;
    for (unsigned i = 0; i < WM_FIX39_BRET_ATTACK_FRAME_COUNT; ++i) {
        const wm_arcade_source_attack_frame_t *r = &wm_bret_source_attack_frames[i];
        if (strcmp(source_frame, r->frame) != 0) continue;
        *uses_z = r->uses_z != 0;
        if (*uses_z) {
            zargs->attack_mode = r->mode; zargs->xoff = r->x; zargs->yoff = r->y;
            zargs->zoff = r->z; zargs->width = r->w; zargs->height = r->h; zargs->depth = r->d;
        } else {
            args->attack_mode = r->mode; args->xoff = r->x; args->yoff = r->y;
            args->width = r->w; args->height = r->h;
        }
        return true;
    }
    return false;
}
