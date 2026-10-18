/*
 * PROGRESS.ASM:4273 get_rnd_wrestler / WRESTLE2.ASM:4548
 * choose_buddies -- see wm/arcade/wm_arcade_buddies.h.
 */
#include "wm/arcade/wm_arcade_buddies.h"

unsigned wm_buddy_excluded_count(uint8_t excluded) {
    unsigned n = 0u, i;
    /* `movk 8,a0 / #lp1: srl 1,a14 / jrnc #nxt1 / inc a8 / dsj`. */
    for (i = 0; i < WM_BUDDY_WRESTLER_SLOTS; ++i)
        n += (unsigned)((excluded >> i) & 1u);
    return n;
}

unsigned wm_get_rnd_wrestler(uint8_t excluded, WmRng *rng) {
    unsigned taken = wm_buddy_excluded_count(excluded);
    unsigned nth;
    unsigned w;

    /*
     * `movk 7,a0 / sub a8,a0 / calla RNDRNG0 / inc a0`. With all
     * eight excluded the source computes RNDRNG0(-1) and the walk
     * below runs off the end of the mask; there is no free slot to
     * return, so this stops rather than inventing one.
     */
    if (taken >= WM_BUDDY_WRESTLER_SLOTS) return 0u;

    nth = (unsigned)(rng ? wm_rng_rndrng0(rng, 7u - taken) : 0u) + 1u;
    /* `movi -1,a14 / #lp2: inc a14 / btst a14,a7 / jrnz #lp2 /
       dsj a0,#lp2` -- the nth slot whose bit is clear. */
    for (w = 0; w < WM_BUDDY_WRESTLER_SLOTS; ++w) {
        if (excluded & (uint8_t)(1u << w)) continue;
        if (--nth == 0u) return w;
    }
    return 0u;
}

wm_buddies_t wm_choose_buddies(unsigned index1, unsigned index2,
                               WmRng *rng) {
    wm_buddies_t out;
    uint8_t mask = 0;

    /* `movk 1,a0 / move @index1,a14 / sll a14,a0 / or a0,a7`, twice.
       A shift by 8 or more is undefined on the host and simply lost
       on the 34010, so an out-of-range index contributes nothing. */
    if (index1 < WM_BUDDY_WRESTLER_SLOTS) mask |= (uint8_t)(1u << index1);
    if (index2 < WM_BUDDY_WRESTLER_SLOTS) mask |= (uint8_t)(1u << index2);

    /* `calla get_rnd_wrestler / PUSH a0`. */
    out.first = wm_get_rnd_wrestler(mask, rng);
    /* `inc a8 / movk 1,a14 / sll a0,a14 / or a14,a7` -- the first
       draw joins the mask, so the second can never repeat it. */
    if (out.first < WM_BUDDY_WRESTLER_SLOTS)
        mask |= (uint8_t)(1u << out.first);
    /* `calla get_rnd_wrestler / PULL a1`. */
    out.second = wm_get_rnd_wrestler(mask, rng);
    if (out.second < WM_BUDDY_WRESTLER_SLOTS)
        mask |= (uint8_t)(1u << out.second);

    out.excluded = mask;
    return out;
}

unsigned wm_buddy_for_player1(const wm_buddies_t *b) {
    /* `PUSH a0,a1` is `mmtm sp,a0,a1`, which pushes the HIGHER
       register first, so a0 -- the second draw -- is on top and the
       first `PULL a11` takes it. Slot 2, PSIDE_PLYR1. */
    return b ? b->second : 0u;
}

unsigned wm_buddy_for_player2(const wm_buddies_t *b) {
    /* The second `PULL a11` takes a1: the first draw. Slot 3,
       PSIDE_PLYR2. */
    return b ? b->first : 0u;
}
