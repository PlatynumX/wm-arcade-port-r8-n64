/*
 * The shared object and arithmetic utilities -- see
 * wm/arcade/wm_arcade_objutil.h.
 */
#include "wm/arcade/wm_arcade_objutil.h"

int32_t wm_xy_add(int32_t a, int32_t b) {
    /* `addxy`: the halves add independently, with no carry across
       bit 15, and each half wraps within its own sixteen bits. */
    uint16_t x = (uint16_t)((uint32_t)a + (uint32_t)b);
    uint16_t y = (uint16_t)(((uint32_t)a >> 16) + ((uint32_t)b >> 16));
    return (int32_t)(((uint32_t)y << 16) | x);
}

int32_t wm_obj_getcpnt(const wm_obj_geom_t *o) {
    uint32_t pos, size;

    if (!o) return 0;
    /* `move *a8(OYVAL),a1,L / move *a8(OXPOS),a0 / movx a0,a1`. */
    pos = ((uint32_t)(uint16_t)o->ypos << 16) | (uint16_t)o->xpos;
    /* `move *a8(OSIZE),a0,L` -- OSIZEX at 130h, OSIZEY at 140h, so
       X is the low word. */
    size = ((uint32_t)o->sizey << 16) | o->sizex;
    /* `srl 1,a0` on the WHOLE register, then `andi >7fff7fff` to
       clear the bit the Y size dropped into the top of the X size,
       and bit 31. */
    size = (size >> 1) & 0x7fff7fffu;
    /* `addxy a1,a0`. */
    return wm_xy_add((int32_t)pos, (int32_t)size);
}

int wm_obj_find(const wm_obj_node_t *objs, size_t count,
                uint16_t want, uint16_t ignore_mask) {
    size_t i;

    if (!objs) return -1;
    /* `andn a2,a1` -- a2 is the bits to REMOVE, not the bits to keep. */
    want = (uint16_t)(want & (uint16_t)~ignore_mask);
    for (i = 0; i < count; ++i) {
        uint16_t got = (uint16_t)(objs[i].oid & (uint16_t)~ignore_mask);
        if (got == want) return (int)i;
    }
    /* `move a0,a0 / rets` with a0 == 0. */
    return -1;
}

void wm_obj_addworldxy(int32_t *oxval, int32_t *oyval,
                       int32_t worldtlx, int32_t worldtly) {
    /* Both 16.16, both in place; the adds are ordinary 32-bit adds,
       not addxy -- these are two separate longs, not a packed pair. */
    if (oxval) *oxval = (int32_t)((uint32_t)*oxval + (uint32_t)worldtlx);
    if (oyval) *oyval = (int32_t)((uint32_t)*oyval + (uint32_t)worldtly);
}

void wm_anipt_getxy(int32_t aniofx, int32_t aniofy, int32_t isizex,
                    bool fliph, int32_t *out_x, int32_t *out_y) {
    /* `move *a2(IANIOFFX),a0 / sll 16,a0` and the same for Y. */
    int32_t x = (int32_t)((uint32_t)aniofx << 16);
    int32_t y = (int32_t)((uint32_t)aniofy << 16);

    if (fliph) {
        /* `move *a2,a2 / subk 1,a2 / sll 16,a2 / neg a0 / add a2,a0`
           -- the decrement happens in whole pixels, before the shift. */
        int32_t span = (int32_t)((uint32_t)(isizex - 1) << 16);
        x = (int32_t)((uint32_t)span - (uint32_t)x);
    }
    if (out_x) *out_x = x;
    /* The source's own comment: Y is not flipped here. */
    if (out_y) *out_y = y;
}

uint32_t wm_vol_to_ht(uint32_t volume) {
    /* `mpyu 169 / divu 255 / inc` -- unsigned, and the increment is
       why silence still draws one pixel. */
    return (volume * 169u) / 255u + 1u;
}

uint32_t wm_form_crc32(const uint8_t *bytes, size_t count) {
    uint32_t acc = 0;
    size_t i;

    if (!bytes) return 0;
    for (i = 0; i < count; ++i) {
        /* `xor a0,a1 / rl 1,a1` -- xor the byte in, then rotate the
           whole 32-bit accumulator left one. */
        acc ^= bytes[i];
        acc = (acc << 1) | (acc >> 31);
    }
    return acc;
}

/* ---- obj_on / obj_off -------------------------------------------- */

int16_t wm_obj_on(int16_t oypos) {
    /* `andni 400h,a0`. */
    return (int16_t)((uint16_t)oypos & (uint16_t)~WM_OBJ_OFFSCREEN_BIT);
}

int16_t wm_obj_off(int16_t oypos) {
    /* `ori 400h,a0`. */
    return (int16_t)((uint16_t)oypos | (uint16_t)WM_OBJ_OFFSCREEN_BIT);
}

bool wm_obj_is_off(int16_t oypos) {
    return ((uint16_t)oypos & (uint16_t)WM_OBJ_OFFSCREEN_BIT) != 0u;
}

/* ---- scrn_rel_off ------------------------------------------------ */

size_t wm_scrn_rel_off(wm_obj_flags_t *objs, size_t count,
                       uint16_t spare_oid) {
    size_t i, n = 0;

    if (!objs) return 0;
    for (i = 0; i < count; ++i) {
        /* `cmpi CREDITID|CLSDEAD,a3 / jrz #sro_loop` -- an exact
           compare against the whole OID, not a masked one. */
        if (objs[i].oid == spare_oid) continue;
        if ((objs[i].oflags & WM_OFLAGS_M_SCRNREL) == 0u) continue;
        objs[i].oflags = (uint16_t)(objs[i].oflags &
                                    (uint16_t)~WM_OFLAGS_M_SCRNREL);
        n++;
    }
    return n;
}
