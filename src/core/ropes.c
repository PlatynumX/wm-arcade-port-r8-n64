#include "wm/ropes.h"
#include <string.h>

void wm_ropes_init(wm_rope_system *r) {
    memset(r, 0, sizeof(*r));
    for (unsigned i = 0; i < WM_ROPE_GROUP_COUNT; ++i)
        r->group[i].present = true;
}

bool wm_rope_command(wm_rope_system *r, wm_rope_group group,
                     uint8_t action, uint8_t position_or_magnitude,
                     wm_fix16 wrestler_z) {
    if (!r || group >= WM_ROPE_GROUP_COUNT || action >= WM_ROPE_COMMAND_COUNT)
        return false;

    wm_rope_state *s = &r->group[group];
    if (!s->present)
        return false; /* Mirrors original: ignore if rope process does not exist. */

    s->action = action;
    s->position_or_magnitude = position_or_magnitude;
    s->wrestler_z = wrestler_z;
    ++s->generation;

    /* TODO later: port command_table/sspring/dspring script selection exactly
       after the original table labels are converted into portable data IDs. */
    return true;
}
