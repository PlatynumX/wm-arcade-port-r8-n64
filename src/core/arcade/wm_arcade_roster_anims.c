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

const char *wm_roster_anim_col(const wm_roster_anim_table *table,
                               int wrestler, int column)
{
    if (!table || wrestler < 0 || wrestler >= table->slots) {
        return NULL;
    }
    if (column < 0) {
        column = 0;
    }
    if (column >= table->columns) {
        column = table->columns - 1;
    }
    return table->row[wrestler * table->columns + column];
}

const char *wm_roster_anim_for(const wm_roster_anim_table *table, int wrestler)
{
    return wm_roster_anim_col(table, wrestler, 0);
}

const char *wm_roster_anim_facing(const wm_roster_anim_table *table,
                                  int wrestler, int facing_up)
{
    /*
     * FACE24TBL (MACROS.H:65) adds a long when MOVE_UP_BIT is clear, so
     * facing up is column 0 and facing down column 1.
     */
    return wm_roster_anim_col(table, wrestler, facing_up ? 0 : 1);
}
