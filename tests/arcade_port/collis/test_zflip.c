/*
 * LIFEBAR.ASM:4039 SHIFT_BARS_IN_Z -- the life bars stepping forward
 * in Z when the camera rises, and back when it drops.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_lifebar.h"

/* The band is inclusive at both ends, and the way-back band is the
   forward band shifted by exactly the same amount. */
static void test_band(void) {
    assert(wm_zflip_in_band(WM_ZFLIP_BAK_Z, WM_ZFLIP_FORWARD));
    assert(wm_zflip_in_band(WM_ZFLIP_NAME_Z, WM_ZFLIP_FORWARD));
    assert(wm_zflip_in_band(201, WM_ZFLIP_FORWARD));   /* frame_z */
    assert(wm_zflip_in_band(202, WM_ZFLIP_FORWARD));   /* bar_z */
    assert(!wm_zflip_in_band(WM_ZFLIP_BAK_Z - 1, WM_ZFLIP_FORWARD));
    assert(!wm_zflip_in_band(WM_ZFLIP_NAME_Z + 1, WM_ZFLIP_FORWARD));

    assert(wm_zflip_in_band(WM_ZFLIP_BAK_Z + WM_ZFLIP_FORWARD,
                            -WM_ZFLIP_FORWARD));
    assert(wm_zflip_in_band(WM_ZFLIP_NAME_Z + WM_ZFLIP_FORWARD,
                            -WM_ZFLIP_FORWARD));
    /* A bar that has not been shifted is not in the way-back band. */
    assert(!wm_zflip_in_band(WM_ZFLIP_BAK_Z, -WM_ZFLIP_FORWARD));
}

/* Forward then back returns every object to where it started. */
static void test_shift_is_reversible(void) {
    int32_t z[6];
    int32_t original[6];
    size_t moved;
    int i;

    z[0] = 100;                 /* a wrestler, well behind */
    z[1] = WM_ZFLIP_BAK_Z;
    z[2] = 201;
    z[3] = 202;
    z[4] = WM_ZFLIP_NAME_Z;
    z[5] = 500;                 /* something in front */
    memcpy(original, z, sizeof z);

    moved = wm_zflip_apply(z, 6, WM_ZFLIP_FORWARD);
    assert(moved == 4);
    assert(z[0] == 100 && z[5] == 500);
    for (i = 1; i <= 4; ++i) assert(z[i] == original[i] + WM_ZFLIP_FORWARD);

    moved = wm_zflip_apply(z, 6, -WM_ZFLIP_FORWARD);
    assert(moved == 4);
    assert(memcmp(z, original, sizeof z) == 0);
}

/*
 * The band moves WITH the shift, so a repeated forward shift finds
 * nothing left in range and is a no-op. That is worth knowing: it
 * means LAST_FLIP is not what keeps the arithmetic correct -- the
 * band does -- and what the flag actually buys is not walking the
 * whole object list four times a second for nothing.
 */
static void test_a_repeated_shift_is_a_no_op(void) {
    int32_t z[1];
    z[0] = WM_ZFLIP_BAK_Z;
    assert(wm_zflip_apply(z, 1, WM_ZFLIP_FORWARD) == 1);
    assert(wm_zflip_apply(z, 1, WM_ZFLIP_FORWARD) == 0);
    assert(z[0] == WM_ZFLIP_BAK_Z + WM_ZFLIP_FORWARD);
    /* And a repeated way-back is a no-op for the same reason. */
    assert(wm_zflip_apply(z, 1, -WM_ZFLIP_FORWARD) == 1);
    assert(wm_zflip_apply(z, 1, -WM_ZFLIP_FORWARD) == 0);
    assert(z[0] == WM_ZFLIP_BAK_Z);
}

/* The poll is every fourth tick and the flag suppresses repeats. */
static void test_hysteresis(void) {
    wm_zflip_t s;
    int i;
    unsigned shifts = 0;
    const int32_t below = WM_ZFLIP_POS - 1;
    const int32_t above = WM_ZFLIP_POS;

    wm_zflip_begin(&s);
    assert(!s.flipped);

    /* Below the threshold and never flipped: nothing to do, ever. */
    for (i = 0; i < 40; ++i) {
        if (wm_zflip_tick(&s, below, WM_ZFLIP_POS)) shifts++;
    }
    assert(shifts == 0);

    /* At the threshold: one shift forward, then silence. */
    shifts = 0;
    for (i = 0; i < 40; ++i) {
        if (wm_zflip_tick(&s, above, WM_ZFLIP_POS)) {
            shifts++;
            assert(s.shift_by == WM_ZFLIP_FORWARD);
        }
    }
    assert(shifts == 1);
    assert(s.flipped);

    /* Drop back below: one shift back, then silence. */
    shifts = 0;
    for (i = 0; i < 40; ++i) {
        if (wm_zflip_tick(&s, below, WM_ZFLIP_POS)) {
            shifts++;
            assert(s.shift_by == -WM_ZFLIP_FORWARD);
        }
    }
    assert(shifts == 1);
    assert(!s.flipped);
}

/* The poll really is four ticks apart, not every tick. */
static void test_poll_rate(void) {
    wm_zflip_t s;
    int i;
    int first = -1, second = -1;

    wm_zflip_begin(&s);
    for (i = 0; i < 40; ++i) {
        /* Alternate sides fast enough that every poll shifts. */
        int32_t y = (i / WM_ZFLIP_POLL_TICKS) % 2 ? WM_ZFLIP_POS
                                                  : WM_ZFLIP_POS - 1;
        if (wm_zflip_tick(&s, y, WM_ZFLIP_POS)) {
            if (first < 0) first = i;
            else if (second < 0) { second = i; break; }
        }
    }
    assert(first >= 0 && second > first);
    assert((second - first) % WM_ZFLIP_POLL_TICKS == 0);
}

/*
 * ZFLIP_FOR_SURE is how the end of a round forces the bars forward:
 * a threshold no camera position can be below.
 */
static void test_for_sure(void) {
    wm_zflip_t s;
    int i;
    unsigned shifts = 0;

    wm_zflip_begin(&s);
    /* Even a camera far above the ring, which is a large negative
       world Y in this game, is not below it. */
    for (i = 0; i < 20; ++i) {
        if (wm_zflip_tick(&s, -100000, WM_ZFLIP_FOR_SURE)) {
            shifts++;
            assert(s.shift_by == WM_ZFLIP_FORWARD);
        }
    }
    assert(shifts == 1);
    assert(s.flipped);
    assert(WM_ZFLIP_FOR_SURE < 0);
    assert(WM_ZFLIP_FOR_SURE < -100000);
}

int main(void) {
    test_band();
    test_shift_is_reversible();
    test_a_repeated_shift_is_a_no_op();
    test_hysteresis();
    test_poll_rate();
    test_for_sure();
    printf("zflip ok\n");
    return 0;
}
