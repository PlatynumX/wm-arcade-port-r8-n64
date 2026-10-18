/*
 * SELECT.ASM:533 work_out_match_time -- the pin-speed clock, and
 * ADJUST.ASM:1624 BCDBIN, which it needs.
 */
#include <assert.h>
#include <stdio.h>

#include "wm_arcade_string.h"

static void test_bcdbin(void) {
    assert(wm_bcd_to_bin(0) == 0);
    assert(wm_bcd_to_bin(0x1234) == 1234);
    assert(wm_bcd_to_bin(0x99999999u) == 99999999u);

    /* It is the inverse of BINBCD for anything BINBCD did not
       clamp. */
    {
        uint32_t v;
        for (v = 0; v < 2000; ++v) {
            assert(wm_bcd_to_bin(wm_bin_to_bcd(v)) == v);
        }
        assert(wm_bcd_to_bin(wm_bin_to_bcd(99999999u)) == 99999999u);
    }
    /* Above the clamp it is not, because BINBCD returned a
       constant rather than the number. */
    assert(wm_bin_to_bcd(100000000u) == 0x99999999u);
    assert(wm_bcd_to_bin(0x99999999u) != 100000000u);

    /* A nibble past 9 is not rejected, just multiplied by its
       place: 0x1A is 10*1 + 1*10. */
    assert(wm_bcd_to_bin(0x1A) == 20);
    assert(wm_bcd_to_bin(0x0F) == 15);
}

/*
 * The conversion runs on 55 ticks to the second while the rest of
 * the game runs on 53, so a pin time comes out short. This pins the
 * arcade's answer rather than the correct one.
 */
static void test_ticks_to_hundredths(void) {
    assert(WM_MATCH_TIME_TICK_DIVISOR == 55);
    /* (100 << 8) / 55 truncated by the assembler. */
    assert(WM_MATCH_TIME_SCALE == 465);
    assert(WM_MATCH_TIME_SCALE == (100 << 8) / 55);

    assert(wm_match_time_ticks_to_hundredths(0) == 0);
    assert(wm_match_time_ticks_to_hundredths(55) == (55 * 465) >> 8);

    /*
     * One real second is 53 ticks, and the routine calls that 96
     * hundredths -- nearly four per cent short.
     */
    {
        int32_t one_second = wm_match_time_ticks_to_hundredths(53);
        assert(one_second == (53 * 465) >> 8);
        assert(one_second == 96);
        assert(one_second < 100);
    }
    /* Ten seconds: 530 ticks reported as 9.62 seconds. */
    assert(wm_match_time_ticks_to_hundredths(530) == 962);

    /* Monotonic, and a negative span stays negative rather than
       becoming enormous. */
    {
        int32_t t;
        for (t = 1; t < 5000; t += 37) {
            assert(wm_match_time_ticks_to_hundredths(t) >=
                   wm_match_time_ticks_to_hundredths(t - 1));
        }
    }
    assert(wm_match_time_ticks_to_hundredths(-53) < 0);
}

/* The 0xIIFF shuffle the source's comment describes. */
static void test_match_time_value(void) {
    /* No fraction: the low byte moves to bits 12-19 and everything
       above bit 8 moves down eight. */
    {
        uint32_t v = wm_match_time_value(0x00001234u, 0);
        uint32_t whole = (0x1234u >> 8) | ((0x1234u & 0xffu) << 12);
        assert(v == wm_bcd_to_bin(whole));
    }
    /* A half fraction is 50 hundredths. */
    {
        uint32_t hundredths = ((uint32_t)0x8000u * 100u) >> 16;
        assert(hundredths == 50);
        /* With a zero whole part, the answer is just the fraction
           round-tripped through BCD -- which is 50. */
        assert(wm_match_time_value(0, 0x8000u) == 50);
    }
    /* A full 16-bit fraction saturates at 99, not 100: the shift
       truncates. */
    assert(((0xffffu * 100u) >> 16) == 99);
    assert(wm_match_time_value(0, 0xffffu) == 99);
    assert(wm_match_time_value(0, 0) == 0);
}

/* What actually reaches the high score table. */
static void test_store(void) {
    /* An ordinary time is stored as BCD. */
    assert(wm_match_time_store(1234) == (int32_t)wm_bin_to_bcd(1234));
    assert(wm_match_time_store(0) == 0);

    /* The two rejections, both spelled -1. */
    assert(wm_match_time_store(WM_MATCH_TIME_LIMIT) == WM_MATCH_TIME_NONE);
    assert(wm_match_time_store(WM_MATCH_TIME_LIMIT + 1) ==
           WM_MATCH_TIME_NONE);
    assert(wm_match_time_store(-1) == WM_MATCH_TIME_NONE);
    /* `jrge`, so the limit itself is rejected and one below is not. */
    assert(wm_match_time_store(WM_MATCH_TIME_LIMIT - 1) !=
           WM_MATCH_TIME_NONE);

    /* 50000 hundredths is 500 seconds by the routine's own reckoning,
       so the cap is generous rather than tight. */
    assert(WM_MATCH_TIME_LIMIT == 50000);
}

int main(void) {
    test_bcdbin();
    test_ticks_to_hundredths();
    test_match_time_value();
    test_store();
    printf("match time ok\n");
    return 0;
}
