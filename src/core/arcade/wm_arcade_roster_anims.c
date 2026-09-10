#include "wm/arcade/wm_arcade_roster_anims.h"

#include <string.h>

const wm_roster_anim_table *wm_roster_anim_find(const char *name)
{
    int i;

    if (!name) {
        return NULL;
    }
    for (i = 0; i < wm_roster_anim_table_count; ++i) {
        if (strcmp(wm_roster_anim_tables[i].name, name) == 0) {
            return &wm_roster_anim_tables[i];
        }
    }
    return NULL;
}

const char *wm_roster_anim_for(const wm_roster_anim_table *table, int wrestler)
{
    if (!table || wrestler < 0 || wrestler >= table->slots) {
        return NULL;
    }
    return table->row[wrestler];
}
