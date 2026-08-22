#include "wm_arcade_drone_source_ranges.h"

#include <stddef.h>

typedef struct WmFix39DroneModeRecord {
    int32_t my_mode;
    int32_t opp_mode;
    const wm_arcade_drone_script_list_t *script_list;
} WmFix39DroneModeRecord;

typedef struct WmFix39DroneModeList {
    const WmFix39DroneModeRecord *records;
    size_t count;
} WmFix39DroneModeList;

#include "wm_arcade_drone_source_ranges_generated.h"

bool wm_arcade_drone_source_ranges_ready(void)
{
    return WM_FIX39_DRONE_RANGES_GENERATED != 0;
}

const wm_arcade_drone_script_list_t *wm_arcade_drone_source_range_script_list(
    const wm_arcade_actor_t *self,
    const wm_arcade_actor_t *opp,
    int range_band,
    int my_mode,
    int opp_mode,
    void *user)
{
    const WmFix39DroneModeList *list;
    size_t i;
    (void)opp;
    (void)user;

    if (!wm_arcade_drone_source_ranges_ready() || !self)
        return NULL;
    if (range_band < 0 || range_band >= WM_FIX39_DRONE_RANGE_BAND_COUNT)
        return NULL;
    if (self->wrestler_num < 0 || self->wrestler_num >= WM_FIX39_DRONE_RANGE_WRESTLER_COUNT)
        return NULL;

    list = &wm_fix39_drone_range_table[range_band][self->wrestler_num];
    if (!list->records || list->count == 0u)
        return NULL;

    /* DRONE.ASM #mdlp checks signed my-mode and opponent-mode bytes in source
     * order.  A negative byte skips that side of the comparison. */
    for (i = 0u; i < list->count; ++i) {
        const WmFix39DroneModeRecord *r = &list->records[i];
        if (r->my_mode >= 0 && r->my_mode != my_mode)
            continue;
        if (r->opp_mode >= 0 && r->opp_mode != opp_mode)
            continue;
        return r->script_list;
    }
    return NULL;
}
