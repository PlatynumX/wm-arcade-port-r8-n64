#include "wm/arcade/wm_arcade_jjxm.h"

#include <string.h>

const wm_jjxm_table_t *wm_jjxm_find(const char *wrestler,
                                    const char *section,
                                    const char *entry)
{
    size_t i, j;
    if (!wrestler || !section || !entry) return NULL;
    for (i = 0; i < wm_jjxm_table_count; ++i) {
        const wm_jjxm_table_t *t = &wm_jjxm_tables[i];
        if (strcmp(t->wrestler, wrestler) != 0) continue;
        if (!t->section || strcmp(t->section, section) != 0) continue;
        for (j = 0; j < t->entry_count; ++j)
            if (strcmp(t->entries[j], entry) == 0) return t;
    }
    return NULL;
}

const char *wm_jjxm_select(const wm_jjxm_table_t *table,
                           uint16_t opp_mode,
                           int32_t xdist,
                           int32_t zdist)
{
    size_t i;
    if (!table) return NULL;
    for (i = 0; i < table->row_count; ++i) {
        const wm_jjxm_row_t *row = &table->rows[i];
        if (row->opp_mode != opp_mode) continue;
        if (row->dx < 0) return table->targets[row->less];
        /* `cmpi DX,a1 / jrgt MORE` -- strictly greater takes the far
           arm, so the near arm is the closed interval. */
        if (xdist > row->dx) return table->targets[row->more];
        if (zdist > row->dz) return table->targets[row->more];
        return table->targets[row->less];
    }
    /* JJXM_END: LOCKUP / rets. */
    return NULL;
}

const char *wm_jjxm_pick(const char *wrestler,
                         const char *section,
                         const char *entry,
                         const wm_arcade_actor_t *attacker,
                         const wm_arcade_actor_t *opponent)
{
    if (!attacker) return NULL;
    return wm_jjxm_select(wm_jjxm_find(wrestler, section, entry),
                          opponent ? (uint16_t)opponent->player_mode
                                   : (uint16_t)WM_PMODE_NORMAL,
                          attacker->closest_xdist,
                          attacker->closest_zdist);
}

int wm_jjxm_is(const char *target, const char *name)
{
    return target && name && strcmp(target, name) == 0;
}
