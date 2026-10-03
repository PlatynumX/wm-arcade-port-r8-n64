/* SOUND.H's WRSNDX over DCSSOUND.ASM's tables -- see
   wm/wrestler_sound_tables.h. */
#include "wm/wrestler_sound_tables.h"

uint16_t wm_wrsnd_lookup(int wrestler_num, int move_index) {
    uint16_t entry;

    if (wrestler_num < 0 || wrestler_num >= WM_WRSND_WRESTLERS) return 0u;
    if (move_index < 0 || move_index >= WM_WRSND_MOVES) return 0u;

    entry = wm_wrsnd_master[wrestler_num][move_index];
    /* `jrz DONE?` comes first: a zero in the wrestler's own row is "no
       sound", and does NOT fall through to the default. */
    if (entry == 0u) return 0u;
    /* `jrp OKAY?` -- a positive entry is the wrestler's own. Anything
       else is DEFLT, and the default table answers instead (where a zero
       really does mean nothing). */
    if ((entry & 0x8000u) == 0u) return entry;
    return wm_wrsnd_default[move_index];
}

uint16_t wm_wrsnd_table_sound(uint16_t call, WmRng *rng) {
    const wm_wrsnd_random *t;
    size_t which;
    uint32_t pick;

    if (call == 0u) return 0u;
    if ((call & WM_WRSND_RANDOM_BIT) == 0u) return call;   /* `jreq triple_sound` */

    /* `xori 1000h,a0 / X32 a0 / addi #random_sound_tables,a0` */
    which = (size_t)(call ^ WM_WRSND_RANDOM_BIT);
    if (which >= wm_wrsnd_random_count) return 0u;
    t = &wm_wrsnd_random_tables[which];

    /* `move *a2+,a0,W / calla RNDRNG0` -- the first word is the inclusive
       maximum, so the sub-table has last_index+1 rows. With no RNG there
       is no draw to make, and saying nothing beats inventing one. */
    if (!rng) return 0u;
    pick = wm_rng_rndrng0(rng, t->last_index);
    if ((size_t)pick >= t->count) return 0u;
    return t->entries[pick];
}

static int play_one(int wrestler_num, int move_index, WmRng *rng, void *user,
                    void (*sound)(void *user, uint16_t call)) {
    uint16_t call = wm_wrsnd_lookup(wrestler_num, move_index);
    uint16_t out;
    if (call == 0u) return 0;
    out = wm_wrsnd_table_sound(call, rng);
    if (out == 0u) return 0;
    if (sound) sound(user, out);
    return 1;
}

int wm_wrsndx(int wrestler_num, int move1, int move2, WmRng *rng, void *user,
              void (*sound)(void *user, uint16_t call)) {
    int n = play_one(wrestler_num, move1, rng, user, sound);
    /* `.if $isname(SOUND2)` -- the second lookup is only assembled when
       the caller named one. */
    if (move2 >= 0) n += play_one(wrestler_num, move2, rng, user, sound);
    return n;
}
