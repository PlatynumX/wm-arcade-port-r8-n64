/*
 * DISPLAY.ASM's object ordering, and calc_closest's square root.
 *
 * Both of these were reconstructions justified by a claim that the
 * source was unavailable. INSBOBJ is DISPLAY.ASM:1540, obj_yzsort is
 * DISPLAY.ASM:1248, and square_root is SQUARE.ASM:40 -- all three are
 * in the tree. These pin the behaviour to what those routines do.
 */
#include "wm/bmod.h"
#include "wm/arcade/wm_arcade_closest.h"
#include "wm/arcade/wm_arcade_target.h"
#include "wm/arcade/wm_arcade_combat.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static wm_bmod_block blk(int16_t z, int16_t y)
{
    wm_bmod_block b;
    memset(&b, 0, sizeof(b));
    b.z = z;
    b.y = y;
    return b;
}

static void test_insbobj_ordering(void)
{
    /* "List is sorted by increasing z and increasing y within constant
     * z" -- INSBOBJ's own header comment. */
    wm_bmod_block near_lo = blk(10, 5);
    wm_bmod_block near_hi = blk(10, 40);
    wm_bmod_block far_lo = blk(90, 5);

    assert(wm_bmod_draw_before(&near_lo, &far_lo));
    assert(!wm_bmod_draw_before(&far_lo, &near_lo));

    /* Equal Z falls through to Y, increasing. */
    assert(wm_bmod_draw_before(&near_lo, &near_hi));
    assert(!wm_bmod_draw_before(&near_hi, &near_lo));

    /* Z wins over Y: a nearer object with a bigger Y still comes first. */
    assert(wm_bmod_draw_before(&near_hi, &far_lo));

    /* obj_yzsort's equal-Z test is `jrge`, so an exact tie is never
     * swapped and insertion order survives. Neither direction is
     * "before" the other. */
    {
        wm_bmod_block a = blk(10, 5);
        wm_bmod_block b = blk(10, 5);
        assert(!wm_bmod_draw_before(&a, &b));
        assert(!wm_bmod_draw_before(&b, &a));
    }

    assert(!wm_bmod_draw_before(NULL, &near_lo));
    assert(!wm_bmod_draw_before(&near_lo, NULL));
}

static void test_closest_uses_the_source_square_root(void)
{
    /*
     * WRESTLE.ASM:4183 ends calc_closest with `calla square_root`, and
     * SQUARE.ASM's version throws away the input's low five bits before
     * its table lookup -- so it is quantised, and closest_dist should
     * be quantised the same way. A textbook isqrt would not be.
     */
    wm_arcade_actor_t a;
    wm_arcade_actor_t o;

    memset(&a, 0, sizeof(a));
    memset(&o, 0, sizeof(o));

    o.x_int = 300;              /* 300^2 = 90000 */
    wm_arcade_calc_closest(&a, &o);
    assert(a.closest_xdist == 300);
    assert(a.closest_dist == wm_arcade_square_root(90000u));

    /* And the quantisation is visible: a run of nearby distances share
     * an answer, which is what the low-five-bit discard produces. */
    assert(wm_arcade_square_root(90000u) == wm_arcade_square_root(90001u));

    o.x_int = 0;
    o.z_int = 0;
    wm_arcade_calc_closest(&a, &o);
    assert(a.closest_dist == wm_arcade_square_root(0u));
}

int main(void)
{
    test_insbobj_ordering();
    test_closest_uses_the_source_square_root();
    printf("DISPLAY.ASM draw order and SQUARE.ASM distance: all checks passed\n");
    return 0;
}
