#include "wm/anim_puppet.h"

const wm_anim_puppet_row *wm_anim_puppet_row_at(size_t id,
                                                int32_t wrestler_num,
                                                size_t index) {
    const wm_anim_puppet_table *t = wm_anim_puppet_table_at(id);
    const wm_anim_puppet_slot *slot;
    if (!t || wrestler_num < 0 || wrestler_num >= 9) return 0;
    slot = &t->slots[wrestler_num];
    if (index >= slot->count) return 0;
    return &slot->rows[index];
}
