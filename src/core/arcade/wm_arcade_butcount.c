#include "wm/arcade/wm_arcade_butcount.h"

#include <stddef.h>

/*
 * PLYR.EQU:152-156, in order. The source walks the same five fields with
 * pointer arithmetic (`addi PUNCHB_COUNT,a2` then `addi 16,a2` per button)
 * because they are adjacent WORDs; this walks the same five by index,
 * which is the same traversal without depending on struct layout.
 */
static int32_t *counter_at(wm_arcade_actor_t *actor, int index) {
    switch (index) {
        case 0: return &actor->punchb_count;
        case 1: return &actor->blockb_count;
        case 2: return &actor->spunchb_count;
        case 3: return &actor->kickb_count;
        case 4: return &actor->skickb_count;
        default: return NULL;
    }
}

void wm_arcade_count_button_presses(wm_arcade_actor_t *actor) {
    int i;
    uint16_t down;
    if (!actor) return;

    /* `calla wres_get_but_val_down / move a0,a0 / jrz #exit`: nothing newly
       pressed this tick, nothing to count. */
    down = actor->but_val_down;
    if (down == 0) return;

    /* `movk 5,a1` ... `srl 1,a0 / jrnc #no_but`: bit 0 upward, one counter
       per bit, in the PLYR.EQU order the fields are declared in. */
    for (i = 0; i < 5; ++i) {
        if (down & (uint16_t)(1u << i)) {
            int32_t *c = counter_at(actor, i);
            if (c) ++(*c);
        }
    }
}

void wm_arcade_clear_button_presses(wm_arcade_actor_t *actor) {
    int i;
    if (!actor) return;
    for (i = 0; i < 5; ++i) {
        int32_t *c = counter_at(actor, i);
        if (c) *c = 0;
    }
}

int32_t wm_arcade_button_count(const wm_arcade_actor_t *actor, int index) {
    int32_t *c;
    if (!actor) return 0;
    c = counter_at((wm_arcade_actor_t *)actor, index);
    return c ? *c : 0;
}
