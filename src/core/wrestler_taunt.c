/*
 * DOINK.ASM:1594 do_taunt -- the start-of-round taunt, shared by all
 * eight wrestlers. See wm/wrestler_anim_tables.h.
 */
#include "wm/wrestler_anim_tables.h"
#include "wm/arcade/wm_arcade_combat_defs.h"

/* UTIL.ASM:1734 RNDPER, the same reading src/core/anim_code.c uses:
   RNDRNG0(999) is the identical stir and mul_high, so it reproduces
   RNDPER exactly rather than approximating it. `jrhi` means the event
   happens when the probability EXCEEDS the draw. */
static bool rndper(WmRng *rng, uint32_t per_mille) {
    if (!rng) return false;
    return wm_rng_rndrng0(rng, 999u) < per_mille;
}

const char *wm_wrestler_do_taunt(const wm_arcade_actor_t *actor,
                                 bool is_drone, WmRng *rng) {
    int slot;

    if (!actor) return NULL;

    if (is_drone) {
        /* `#drone: movi 250,a0 / calla RNDPER / jrhi #do_taunt` */
        if (!rndper(rng, 250u)) return NULL;
    } else {
        /* "Human chooses to taunt?" -- UP on the stick AND the block
           button, both held. `jaz SUCIDE` on either kills the process,
           so neither is optional. Read from the CURRENT readings the
           routine's own `calla read_switches` had just refreshed. */
        if (!(actor->stick_val_cur & WM_MOVE_UP)) return NULL;
        if (!(actor->but_val_cur & WM_BTN_BLOCK)) return NULL;
    }

    slot = (int)actor->wrestler_num;
    if (slot < 0 || slot >= WM_WRESTLER_ANIM_SLOTS) return NULL;
    return wm_wrestler_taunt_anims[slot];
}
