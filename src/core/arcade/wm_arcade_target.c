/*
 * TABLES.ASM:43 set_target_offsets, :71 get_mode_height, and ANIM.ASM:3753
 * ANI_TARGET. The grid they read is generated (tools/wltarget.py); this is
 * the arithmetic over it. See wm/arcade/wm_arcade_target.h.
 */
#include "wm/arcade/wm_arcade_target.h"

#include "wm/arcade/wm_arcade_combat_defs.h"

void wm_arcade_set_target_offsets(wm_arcade_actor_t *actor,
                                  const wm_arcade_actor_t *target, int area) {
    wm_target_offset o;
    if (!actor || !target) return;
    if (!wm_target_offsets(target->wrestler_num, (int32_t)target->player_mode,
                           area, &o))
        return;
    actor->tgt_xoff = o.x;
    actor->tgt_yoff = o.y;
    actor->tgt_zoff = o.z;
}

int32_t wm_arcade_get_mode_height(const wm_arcade_actor_t *actor) {
    wm_target_offset o;
    if (!actor) return 0;
    /*
     * `15n + 1` words into the block, and 15 words is one wrestler's five
     * areas of three, so word 1 of area 0 -- the HEAD's Y. Written as the
     * lookup it is rather than as the offset arithmetic, which is the same
     * number and says why.
     */
    if (!wm_target_offsets(actor->wrestler_num, (int32_t)actor->player_mode,
                           WM_TGT_HEAD, &o))
        return 0;
    return o.y;
}

void wm_arcade_anim_target(wm_arcade_actor_t *actor,
                           const wm_arcade_actor_t *target,
                           int target1, int target2, int mode) {
    int facing_feet, want_highest, area;
    if (!actor || !target) return;

    /* `xor a1,a0 / btst B_FLIPH,a0` -- Z set means the flips MATCH. */
    facing_feet = ((actor->obj_control ^ target->obj_control) &
                   WM_OBJ_FLIPH) == 0;
    /*
     * Facing his feet, the higher area index is the closer one; facing his
     * head, the lower one is. ATM_FARTHEST inverts each.
     */
    want_highest = facing_feet ? (mode == WM_ATM_CLOSEST)
                               : (mode != WM_ATM_CLOSEST);

    if (want_highest)                       /* `cmp a1,a0 / jrge #set` */
        area = target1 >= target2 ? target1 : target2;
    else                                    /* `jrle #set` */
        area = target1 <= target2 ? target1 : target2;

    wm_arcade_set_target_offsets(actor, target, area);
    /* `calla tgt_ground` -- WRESTLE2.ASM:1612, "Zero yer TGT_YOFF. Do this
       anytime you target an opponent who's on the ground." */
    actor->tgt_yoff = 0;
}

/* ------------------------------------------------------------------ *
 * LIFEBAR.ASM:3444 MOVE_NAME_ANNC's decision half.
 * ------------------------------------------------------------------ */

void wm_move_name_init(wm_move_name_state *st) {
    if (!st) return;
    st->shown[0] = 0u;
    st->shown[1] = 0u;
    st->showing[0] = 0u;
    st->showing[1] = 0u;
}

int wm_move_name_should_draw(wm_move_name_state *st, int side, int index,
                            int reduce_bog) {
    uint32_t *word;
    int bit;
    if (!st || index < 0 || index >= WM_MOVE_NAME_COUNT) return 0;
    if (side < 0 || side > 1) return 0;

    /* `cmpi 41,a10 / jrz #skip` -- 41 jumps PAST the reduce_bog test. */
    if (index != WM_MOVE_NAME_ALWAYS && reduce_bog) return 0;

    /* is_there_one_already reads the OTHER side's flag: `MOVE A9,A1 / NOT
       A1 / SLL 31 / SRL 31-4` is "invert the side and index by it". */
    if (st->showing[1 - side]) return 0;

    /*
     * "Messages display the first time only" -- one bit per index, in two
     * 32-bit words. Index 41 is 32+9, and `cmpi 9,a3 / jrz #cont` skips
     * the bit test for exactly that one, so the second wind shows every
     * time it happens.
     */
    if (index != WM_MOVE_NAME_ALWAYS) {
        word = &st->shown[index < 32 ? 0 : 1];
        bit = index < 32 ? index : index - 32;
        if (*word & (1u << bit)) return 0;
        *word |= (1u << bit);
    }
    return 1;
}

/* ------------------------------------------------------------------ *
 * SQUARE.ASM:40 square_root.
 * ------------------------------------------------------------------ */

int32_t wm_arcade_square_root(uint32_t v) {
    uint32_t a0 = v >> 5;               /* `srl 5,a0` */
    int bits, shift = 0;                /* a1, a14 */

    /*
     * `lmo a0,a1 / neg a1 / addk 32,a1` -- LMO gives the count of leading
     * zeros (and 0 for an input of 0, which this matches), so a1 is how
     * many bits of data are left. The source's own comment: "(0-27)".
     */
    bits = 32;
    if (a0) {
        uint32_t t = a0;
        int clz = 0;
        while (!(t & 0x80000000u)) { t <<= 1; ++clz; }
        bits = 32 - clz;
    }

    bits -= 10;
    if (bits > 0) {
        /*
         * "even after discarding lower 5, we've more than ten bits left.
         * divide by 2, rounding up, and that's half the number we shift
         * right right now, and THE number we shift left later."
         */
        shift = (bits + 1) >> 1;
        a0 >>= (unsigned)(shift * 2);
    }

    if (a0 >= WM_SQROOT_ENTRIES) a0 = WM_SQROOT_ENTRIES - 1;
    return (int32_t)((uint32_t)wm_sqroot_tab[a0] << shift);
}
