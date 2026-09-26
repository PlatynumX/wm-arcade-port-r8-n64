/*
 * The small shared routines UTIL.ASM, DISPLAY.ASM, MENU.ASM and
 * AUDIT.ASM export: object geometry, a masked list search, a bar
 * height, and the checksum that is not a CRC32.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_objutil.h"

static void test_packed_xy(void) {
    int32_t v = wm_xy_pack(300, -40);
    assert(wm_xy_x(v) == 300);
    assert(wm_xy_y(v) == -40);

    /* addxy adds the halves independently: no carry across bit 15. */
    {
        int32_t a = wm_xy_pack((int16_t)0xffff, 1);   /* x = -1 */
        int32_t b = wm_xy_pack(1, 1);
        int32_t s = wm_xy_add(a, b);
        assert(wm_xy_x(s) == 0);
        /* X wrapping did NOT bump Y to 3. */
        assert(wm_xy_y(s) == 2);
    }
}

/* GETCPNT: centre = position + size/2, halved as one 32-bit shift. */
static void test_getcpnt(void) {
    wm_obj_geom_t o;
    int32_t c;

    o.xpos = 100;
    o.ypos = 50;
    o.sizex = 40;
    o.sizey = 20;
    c = wm_obj_getcpnt(&o);
    assert(wm_xy_x(c) == 120);
    assert(wm_xy_y(c) == 60);

    /*
     * An ODD height. The single `srl 1` drops the Y size's bottom
     * bit into the top bit of the X size, and `andi >7fff7fff`
     * clears it -- so the X centre is unaffected, which is the
     * whole point of the mask.
     */
    o.sizex = 40;
    o.sizey = 21;
    c = wm_obj_getcpnt(&o);
    assert(wm_xy_x(c) == 120);          /* not 120 + 32768 */
    assert(wm_xy_y(c) == 50 + 10);      /* 21/2 truncates to 10 */

    /*
     * The other half of the mask, `7fff` in the HIGH word, clears
     * bit 31 -- which a logical shift right has already cleared, so
     * it never does anything. A huge height halves normally.
     */
    o.sizex = 4;
    o.sizey = 0x8000;
    c = wm_obj_getcpnt(&o);
    assert(wm_xy_y(c) == (int16_t)(50 + 0x4000));
    assert(wm_xy_x(c) == 102);

    /* Negative positions work, because only the size is masked. */
    o.xpos = -200;
    o.ypos = -30;
    o.sizex = 10;
    o.sizey = 6;
    c = wm_obj_getcpnt(&o);
    assert(wm_xy_x(c) == -195);
    assert(wm_xy_y(c) == -27);
}

/* obj_find: the mask names the bits to IGNORE, not the bits to keep. */
static void test_obj_find(void) {
    wm_obj_node_t objs[4];
    memset(objs, 0, sizeof objs);
    objs[0].oid = 0x1234;
    objs[1].oid = 0x6001;
    objs[2].oid = 0x6002;
    objs[3].oid = 0x6001;

    /* No mask: an exact match, first one wins. */
    assert(wm_obj_find(objs, 4, 0x6001, 0x0000) == 1);
    assert(wm_obj_find(objs, 4, 0x9999, 0x0000) == -1);

    /* Ignore the low byte and 0x6000-anything matches the first. */
    assert(wm_obj_find(objs, 4, 0x6000, 0x00ff) == 1);
    assert(wm_obj_find(objs, 4, 0x6099, 0x00ff) == 1);

    /* Ignore everything and the first object matches whatever you
       ask for -- `andn` with an all-ones mask leaves zero on both
       sides. */
    assert(wm_obj_find(objs, 4, 0xabcd, 0xffff) == 0);

    /* Empty list: the source's `move a0,a0` Z. */
    assert(wm_obj_find(objs, 0, 0x1234, 0) == -1);
    assert(wm_obj_find(NULL, 4, 0x1234, 0) == -1);
}

static void test_addworldxy(void) {
    int32_t x = 100 << 8, y = -(200 << 8);
    wm_obj_addworldxy(&x, &y, 50 << 8, 25 << 8);
    assert(x == (150 << 8));
    assert(y == -(175 << 8));

    /* These are two separate longs, so a big X really does carry
       through bit 16 rather than wrapping in a 16-bit field. */
    x = 0xffff;
    y = 0;
    wm_obj_addworldxy(&x, &y, 1, 0);
    assert(x == 0x10000);
}

static void test_anipt_getxy(void) {
    int32_t x = 0, y = 0;

    wm_anipt_getxy(12, 34, 64, false, &x, &y);
    assert(x == (12 << 16));
    assert(y == (34 << 16));

    /* Flipped: (ISIZEX - 1) - offset, decremented in whole pixels
       BEFORE the shift. */
    wm_anipt_getxy(12, 34, 64, true, &x, &y);
    assert(x == ((64 - 1) << 16) - (12 << 16));
    assert(x == (51 << 16));
    /* Y is untouched either way -- the source says so. */
    assert(y == (34 << 16));

    /* An attach point at the right edge flips to -1 pixels, not 0:
       the source does not clamp. */
    wm_anipt_getxy(64, 0, 64, true, &x, &y);
    assert(x == -(1 << 16));

    assert(WM_OCTRL_B_FLIPH == 4);
}

/* vol_to_ht: 0-255 in, 1-170 out, and never zero. */
static void test_vol_to_ht(void) {
    uint32_t v;
    assert(wm_vol_to_ht(0) == 1);
    assert(wm_vol_to_ht(255) == 170);
    /* Monotonic, and inside the range the comment claims. */
    for (v = 0; v <= 255; ++v) {
        uint32_t h = wm_vol_to_ht(v);
        assert(h >= 1 && h <= 170);
        if (v) assert(h >= wm_vol_to_ht(v - 1));
    }
    /* Unsigned division truncates: half volume is not half height
       plus one, it is 84. */
    assert(wm_vol_to_ht(128) == (128u * 169u) / 255u + 1u);
    assert(wm_vol_to_ht(128) == 85);
}

/*
 * form_crc32 is an xor-and-rotate accumulator, not a CRC32. Any
 * real CRC32 of "123456789" is 0xCBF43926; this is not that, and
 * the test says so rather than leaving the name to mislead.
 */
static void test_form_crc32(void) {
    static const uint8_t msg[] = "123456789";
    uint32_t acc;

    assert(wm_form_crc32(NULL, 4) == 0);
    assert(wm_form_crc32(msg, 0) == 0);

    /* One byte: xor in, then rotate left one. */
    {
        static const uint8_t one[] = { 0x01 };
        assert(wm_form_crc32(one, 1) == 0x00000002u);
        {
            static const uint8_t hi[] = { 0x80 };
            assert(wm_form_crc32(hi, 1) == 0x00000100u);
        }
    }

    /* The rotate really is a rotate: a bit that reaches bit 31 comes
       back round to bit 0 rather than falling off. */
    {
        uint8_t bits[32];
        memset(bits, 0, sizeof bits);
        bits[0] = 0x01;
        /* 31 more zero bytes rotate that single bit all the way. */
        assert(wm_form_crc32(bits, 32) == 0x00000001u);
    }

    acc = wm_form_crc32(msg, sizeof msg - 1);
    assert(acc != 0xCBF43926u);        /* not a CRC32 */
    /* Recomputed here the way the loop is written, as a check that
       the implementation is the loop and not something else. */
    {
        uint32_t ref = 0;
        size_t i;
        for (i = 0; i + 1 < sizeof msg; ++i) {
            ref ^= msg[i];
            ref = (ref << 1) | (ref >> 31);
        }
        assert(acc == ref);
    }
    /* Order matters -- it is not a plain xor. */
    {
        static const uint8_t ab[] = { 0x12, 0x34 };
        static const uint8_t ba[] = { 0x34, 0x12 };
        assert(wm_form_crc32(ab, 2) != wm_form_crc32(ba, 2));
    }
}

/* obj_on / obj_off hide by setting bit 10 of OYPOS, not by moving. */
static void test_obj_on_off(void) {
    int16_t y = 120;

    assert(!wm_obj_is_off(y));
    y = wm_obj_off(y);
    assert(wm_obj_is_off(y));
    assert(y == 120 + 0x400);
    /* Idempotent, which a plain add would not be. */
    assert(wm_obj_off(y) == y);
    y = wm_obj_on(y);
    assert(y == 120);
    assert(wm_obj_on(y) == y);

    /* It is a BIT, so an object whose real Y already had bit 10 set
       would be SHOWN by obj_off. Nothing on this screen sits below
       1024, but it is why this is not an add. */
    {
        int16_t low = (int16_t)(0x400 + 5);
        assert(wm_obj_is_off(low));
        assert(wm_obj_on(low) == 5);
    }
    assert(WM_OBJ_OFFSCREEN_BIT == 0x400);
}

/* scrn_rel_off clears M_SCRNREL on everything but the credit box. */
static void test_scrn_rel_off(void) {
    const uint16_t credit = 0x1234;      /* CREDITID|CLSDEAD */
    wm_obj_flags_t objs[5];
    size_t n;
    int i;

    for (i = 0; i < 5; ++i) {
        objs[i].oid = (uint16_t)(0x2000 + i);
        objs[i].oflags = WM_OFLAGS_M_SCRNREL | 0x0001;
    }
    objs[2].oid = credit;
    objs[4].oflags = 0x0001;             /* already clear */

    n = wm_scrn_rel_off(objs, 5, credit);
    assert(n == 3);
    for (i = 0; i < 5; ++i) {
        if (i == 2) {
            assert(objs[i].oflags & WM_OFLAGS_M_SCRNREL);
        } else {
            assert(!(objs[i].oflags & WM_OFLAGS_M_SCRNREL));
        }
        /* Nothing else in the flags word is touched. */
        assert(objs[i].oflags & 0x0001);
    }

    /* The spare test is an exact compare on the whole OID, so an
       object merely sharing the class is not spared. */
    objs[0].oid = (uint16_t)(credit ^ 0x0001);
    objs[0].oflags = WM_OFLAGS_M_SCRNREL;
    assert(wm_scrn_rel_off(objs, 1, credit) == 1);

    assert(WM_OFLAGS_M_SCRNREL == 0x2000);
}

int main(void) {
    test_packed_xy();
    test_getcpnt();
    test_obj_find();
    test_addworldxy();
    test_anipt_getxy();
    test_vol_to_ht();
    test_form_crc32();
    test_obj_on_off();
    test_scrn_rel_off();
    printf("objutil ok\n");
    return 0;
}
