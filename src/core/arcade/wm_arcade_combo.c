/* LIFEBAR.ASM:750 ADD_TO_COMBO_COUNT -- see wm/arcade/wm_arcade_combo.h. */
#include "wm/arcade/wm_arcade_combo.h"

void wm_arcade_add_to_combo_count(wm_arcade_actor_t *a, int32_t move_bits) {
    if (!a) return;

    /* `AND a1,a2 / JRNZ ALREADY_ADDED_ONCE` -- the branch is real, but it
       changes only whether COMBO_START is re-OR'd; see the header. */
    if ((a->combo_start & move_bits) == 0)
        a->combo_start |= move_bits;

    ++a->combo_size;
    if (a->combo_size >= WM_COMBO_SUPER_SIZE) a->combo_flash = 1;
}
