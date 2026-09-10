/*
 * PAL.ASM's allocator and faders.
 *
 * The state is about 80 KB (FADERAM and colour RAM), so it is static
 * here -- as the header says it must be anywhere else too.
 */
#include "wm/arcade/wm_arcade_pal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static wm_pal_state g_st;

/* Two throwaway palettes with a count word and known colours. */
static const uint16_t pal_a[] = { 3, 0x7C00, 0x03E0, 0x001F };  /* R, G, B */
static const uint16_t pal_b[] = { 2, 0x7FFF, 0x0000 };
static const uint16_t pal_c[] = { 1, 0x4210 };

static void reset(void)
{
    memset(&g_st, 0, sizeof(g_st));
    wm_pal_init(&g_st);
}

static void test_palette_data(void)
{
    const uint16_t *diagp = wm_palette_find("DIAGP");
    const uint16_t *bam = wm_palette_find("BAMBLU_P");
    int i;

    assert(wm_palette_count == 337);
    assert(diagp && wm_pal_color_count(diagp) == 29);
    /* IMGPAL.ASM:739 -- the first three colours, read straight out. */
    assert(diagp[1] == 0x0000 && diagp[2] == 0x56B5 && diagp[3] == 0x7BDE);
    /* The palette anim_code's #set_pal asks for by name really exists. */
    assert(bam && wm_pal_color_count(bam) == 64);
    assert(wm_palette_find("NO_SUCH_P") == NULL);

    /* Every block's count word agrees with its length, and no shipped
     * palette sets any of the flag bits above the low nine. */
    for (i = 0; i < wm_palette_count; ++i) {
        const uint16_t *w = wm_palettes[i].words;
        assert(w != NULL);
        assert((w[0] & ~(uint16_t)WM_PAL_COUNT_MASK) == 0);
        assert(wm_pal_color_count(w) >= 1);
        assert(wm_pal_color_count(w) <= WM_FPALSZ);
    }
    /* Sorted, so the bisect in wm_palette_find is valid. */
    for (i = 1; i < wm_palette_count; ++i) {
        assert(strcmp(wm_palettes[i - 1].name, wm_palettes[i].name) < 0);
    }
}

static void test_allocation(void)
{
    int slot = -1;
    int again = -1;

    reset();
    assert(!wm_pal_find(&g_st, pal_a, &slot));

    assert(wm_pal_getf(&g_st, pal_a, &slot) && slot == 0);
    assert(wm_pal_getf(&g_st, pal_b, &slot) && slot == 1);
    /* Asking twice returns the slot it already has -- and does not
     * queue a second transfer. */
    assert(wm_pal_getf(&g_st, pal_a, &again) && again == 0);
    assert(g_st.xfer[2].count == 0);

    assert(wm_pal_find(&g_st, pal_a, &slot) && slot == 0);
    assert(wm_pal_find(&g_st, pal_c, &slot) == false);

    /* The DMA form the source returns doubles the index into both bytes. */
    assert(wm_pal_dma(0) == 0x0000);
    assert(wm_pal_dma(1) == 0x0101);
    assert(wm_pal_dma(15) == 0x0F0F);
}

static bool everything_is_drawn(void *user, uint16_t dma_pal);

static void test_a_full_pool_cleans_then_fails(void)
{
    /* 80 distinct palettes fill PALRAM exactly. PALTRAM only holds 60
     * transfers, though, and pal_getf refuses a slot it cannot queue --
     * so filling the pool needs the vblank drain in between, which is
     * exactly the arcade's arrangement. */
    static uint16_t junk[WM_NUMPAL + 2][2];
    int i;
    int slot = -1;

    reset();
    for (i = 0; i < WM_NUMPAL + 2; ++i) {
        junk[i][0] = 1;
        junk[i][1] = (uint16_t)i;
    }
    for (i = 0; i < WM_NUMPAL; ++i) {
        assert(wm_pal_getf(&g_st, junk[i], &slot) && slot == i);
        wm_pal_transfer(&g_st);
    }
    /* The pool is full and pal_clean is the only way out. */
    assert(!wm_pal_find(&g_st, junk[WM_NUMPAL], &slot));

    /* With no in_use hook pal_clean frees slots 1..79 but never 0. */
    wm_pal_clean(&g_st);
    assert(g_st.slot[0] == junk[0]);
    assert(g_st.slot[1] == NULL);
    assert(g_st.slot[WM_NUMPAL - 1] == NULL);

    assert(wm_pal_getf(&g_st, junk[WM_NUMPAL], &slot) && slot == 1);
}

static void test_the_pool_is_full_and_nothing_is_free(void)
{
    /* When every slot is genuinely drawn, pal_clean frees nothing and
     * pal_getf fails -- the source's `#err` exit. */
    static uint16_t junk[WM_NUMPAL + 1][2];
    int i;
    int slot = -1;

    reset();
    for (i = 0; i < WM_NUMPAL + 1; ++i) {
        junk[i][0] = 1;
        junk[i][1] = (uint16_t)i;
    }
    for (i = 0; i < WM_NUMPAL; ++i) {
        assert(wm_pal_getf(&g_st, junk[i], &slot));
        wm_pal_transfer(&g_st);
    }
    g_st.in_use = everything_is_drawn;
    assert(!wm_pal_getf(&g_st, junk[WM_NUMPAL], &slot));
    assert(g_st.slot[WM_NUMPAL - 1] == junk[WM_NUMPAL - 1]);
}

static bool everything_is_drawn(void *user, uint16_t dma_pal)
{
    (void)user;
    (void)dma_pal;
    return true;
}

static bool keep_pal_b(void *user, const uint16_t *palette)
{
    (void)user;
    return palette == pal_b;
}

static void test_clean_asks_before_freeing(void)
{
    int slot = -1;

    reset();
    assert(wm_pal_getf(&g_st, pal_a, &slot) && slot == 0);
    assert(wm_pal_getf(&g_st, pal_b, &slot) && slot == 1);
    assert(wm_pal_getf(&g_st, pal_c, &slot) && slot == 2);

    g_st.in_use = everything_is_drawn;
    wm_pal_clean(&g_st);
    assert(g_st.slot[1] == pal_b && g_st.slot[2] == pal_c);

    /* IGNORE_THIS_PAL only gets asked while IGNORE_SPECIAL is set. */
    g_st.in_use = NULL;
    g_st.ignore_this_pal = keep_pal_b;
    g_st.ignore_special = false;
    wm_pal_clean(&g_st);
    assert(g_st.slot[1] == NULL && g_st.slot[2] == NULL);

    reset();
    assert(wm_pal_getf(&g_st, pal_a, &slot));
    assert(wm_pal_getf(&g_st, pal_b, &slot));
    assert(wm_pal_getf(&g_st, pal_c, &slot));
    g_st.ignore_this_pal = keep_pal_b;
    g_st.ignore_special = true;
    wm_pal_clean(&g_st);
    assert(g_st.slot[1] == pal_b);      /* vetoed */
    assert(g_st.slot[2] == NULL);       /* not */
}

static void test_a_full_transfer_queue_refuses_the_slot(void)
{
    /* pal_getf only claims a slot if the upload could be queued. If
     * PALTRAM is full the palette is left unassigned, not half-assigned
     * with nothing on its way to the hardware. */
    static uint16_t junk[WM_NUMPALT + 2][2];
    int i;
    int slot = -1;

    reset();
    for (i = 0; i < WM_NUMPALT + 2; ++i) {
        junk[i][0] = 1;
        junk[i][1] = (uint16_t)(i + 1);
    }
    for (i = 0; i < WM_NUMPALT; ++i) {
        assert(wm_pal_getf(&g_st, junk[i], &slot));
    }
    assert(!wm_pal_set(&g_st, pal_a + 1, 0, 1));    /* queue is full */
    assert(!wm_pal_getf(&g_st, junk[WM_NUMPALT], &slot));
    assert(!wm_pal_find(&g_st, junk[WM_NUMPALT], &slot));
    assert(g_st.slot[WM_NUMPALT] == NULL);
}

static void test_transfer(void)
{
    int slot = -1;

    reset();
    assert(wm_pal_getf(&g_st, pal_a, &slot) && slot == 0);
    assert(wm_pal_getf(&g_st, pal_b, &slot) && slot == 1);
    assert(g_st.xfer[0].count == 3 && g_st.xfer[0].dest == 0x0000);
    assert(g_st.xfer[1].count == 2 && g_st.xfer[1].dest == 0x0100);

    wm_pal_transfer(&g_st);
    assert(g_st.colram[0] == 0x7C00);
    assert(g_st.colram[1] == 0x03E0);
    assert(g_st.colram[2] == 0x001F);
    /* Palette 1 lands 256 colours along, not three -- the destination
     * word scales whole, so the stride is fixed. */
    assert(g_st.colram[WM_PAL_MAP_STRIDE + 0] == 0x7FFF);
    assert(g_st.colram[WM_PAL_MAP_STRIDE + 1] == 0x0000);

    /* Every cell it drained is free again. */
    assert(g_st.xfer[0].count == 0 && g_st.xfer[1].count == 0);
    wm_pal_transfer(&g_st);             /* idempotent with nothing queued */

    /* A start colour rides in the low byte of the destination. */
    assert(wm_pal_set(&g_st, pal_c + 1, (uint16_t)((1 << 8) | 5), 1));
    wm_pal_transfer(&g_st);
    assert(g_st.colram[WM_PAL_MAP_STRIDE + 5] == 0x4210);
}

static void test_transfer_stops_at_the_first_gap(void)
{
    /* `move *a0,a4 / jrz #x` -- the drain ends at the first empty cell
     * rather than scanning all sixty, so anything queued behind a hole
     * waits for the next pass. pal_set always fills the lowest free
     * cell, so this only bites when something else frees one. */
    reset();
    assert(wm_pal_set(&g_st, pal_a + 1, 0x0000, 3));
    assert(wm_pal_set(&g_st, pal_b + 1, 0x0100, 2));
    g_st.xfer[0].count = 0;             /* punch a hole */

    wm_pal_transfer(&g_st);
    assert(g_st.colram[WM_PAL_MAP_STRIDE] == 0);    /* never reached */
    assert(g_st.xfer[1].count == 2);                /* still queued */
}

static void test_blacken(void)
{
    int slot = -1;

    reset();
    assert(!wm_pal_blacken(&g_st, pal_a));  /* not assigned yet */
    assert(wm_pal_getf(&g_st, pal_a, &slot) && slot == 0);
    wm_pal_transfer(&g_st);
    assert(g_st.colram[0] == 0x7C00);

    assert(wm_pal_blacken(&g_st, pal_a));
    /* 64 colours of black, whatever the palette's own length is. */
    assert(g_st.xfer[0].count == 64);
    wm_pal_transfer(&g_st);
    assert(g_st.colram[0] == 0 && g_st.colram[63] == 0);
}

static void test_fade_maths(void)
{
    uint16_t dst[8];
    static const uint16_t src[] = { 3, 0x7C00, 0x03E0, 0x001F };

    /* Full brightness is 128 and reproduces the palette exactly. */
    wm_pal_fade(dst, src, WM_PAL_FADE_FULL);
    assert(dst[0] == 3);
    assert(dst[1] == 0x7C00 && dst[2] == 0x03E0 && dst[3] == 0x001F);

    /* Half brightness halves each channel: 31 * 64 >> 7 == 15. */
    wm_pal_fade(dst, src, 64);
    assert(((dst[1] >> 10) & 0x1F) == 15);
    assert(((dst[2] >> 5) & 0x1F) == 15);
    assert((dst[3] & 0x1F) == 15);

    wm_pal_fade(dst, src, 0);
    assert(dst[1] == 0 && dst[2] == 0 && dst[3] == 0);

    /* pal_addb adds instead, and clamps at 31. */
    wm_pal_addb(dst, src, 0);
    assert(dst[1] == 0x7C00);
    wm_pal_addb(dst, src, 31);
    assert(dst[1] == 0x7FFF && dst[2] == 0x7FFF && dst[3] == 0x7FFF);

    /* The clamp is an UNSIGNED compare, so a channel driven below zero
     * saturates to white rather than to black. No shipped caller ever
     * passes a negative brightness, so this path is unreachable in the
     * game -- it is reproduced, not corrected. */
    wm_pal_addb(dst, src, -1);
    assert((dst[1] & 0x1F) == 0x1F);    /* blue was 0, now full */

    /* do_fade's scale is /256, not /128 -- a different ramp. */
    assert(wm_pal_scale_color(0x7C00, WM_PAL_LEVEL_FULL) == 0x7C00);
    assert(((wm_pal_scale_color(0x7C00, 128) >> 10) & 0x1F) == 15);
    assert(wm_pal_scale_color(0x7FFF, 0) == 0);
}

static void test_fade_keeps_the_count_word(void)
{
    /* pal_fade copies the count word through untouched -- flags and
     * all -- and only masks it for its own loop. */
    uint16_t dst[4];
    static const uint16_t flagged[] = { 0xFE00 | 2, 0x7C00, 0x03E0, 0x1234 };

    dst[3] = 0xDEAD;
    wm_pal_fade(dst, flagged, WM_PAL_FADE_FULL);
    assert(dst[0] == (0xFE00 | 2));
    assert(dst[1] == 0x7C00 && dst[2] == 0x03E0);
    /* Only the nine masked bits of the count word drive the loop, so
     * the flags do not lengthen it: the fourth word is not touched. */
    assert(dst[3] == 0xDEAD);
}

static void test_global_fade(void)
{
    wm_pal_global_fade job;
    int slot = -1;
    int steps = 0;

    reset();
    assert(wm_pal_getf(&g_st, pal_a, &slot) && slot == 0);
    wm_pal_transfer(&g_st);
    g_st.irqskye = 0x7FFF;

    /* fade_down: 256 down to 0 in the caller's increment. */
    wm_pal_fade_down(&g_st, &job, NULL, 32);
    assert(job.level == WM_PAL_LEVEL_FULL);
    assert(g_st.irqskyeo == 0x7FFF);    /* snapshotted at the start */

    while (wm_pal_global_fade_step(&g_st, &job)) {
        wm_pal_transfer(&g_st);
        ++steps;
        assert(steps < 64);
    }
    wm_pal_transfer(&g_st);
    /* Nine passes -- 256, 224, ... 32, 0 -- and the loop test is at the
     * bottom, so it exits with the level already past the end. */
    assert(steps == 8);
    assert(job.level == -32);
    assert(g_st.colram[0] == 0);        /* red faded to black */
    assert(g_st.irqskye == 0);

    /* fade_up runs the other way and its last pass is at full level. */
    wm_pal_fade_up(&g_st, &job, NULL, 32);
    while (wm_pal_global_fade_step(&g_st, &job)) {
        wm_pal_transfer(&g_st);
    }
    wm_pal_transfer(&g_st);
    assert(g_st.colram[0] == 0x7C00);

    /* The _half pair run between 128 and 256 with a fixed step of 16. */
    wm_pal_fade_down_half(&g_st, &job, NULL);
    assert(job.level == 256 && job.end == 128 && job.inc == 16);
    wm_pal_fade_up_half(&g_st, &job, NULL);
    assert(job.level == 128 && job.end == 256 && job.inc == 16);
}

static void test_global_fade_skip_list(void)
{
    wm_pal_global_fade job;
    const uint16_t *skip[2];
    int slot = -1;

    reset();
    assert(wm_pal_getf(&g_st, pal_a, &slot) && slot == 0);
    assert(wm_pal_getf(&g_st, pal_b, &slot) && slot == 1);
    wm_pal_transfer(&g_st);

    skip[0] = pal_b;
    skip[1] = NULL;
    /* Two passes: one at full level, one at zero. */
    wm_pal_fade_down(&g_st, &job, skip, 256);
    while (wm_pal_global_fade_step(&g_st, &job)) {
        wm_pal_transfer(&g_st);
    }
    wm_pal_transfer(&g_st);

    assert(g_st.colram[0] == 0);                        /* faded */
    assert(g_st.colram[WM_PAL_MAP_STRIDE] == 0x7FFF);   /* skipped */
}

static void test_init_parks_diagp_at_zero(void)
{
    const uint16_t *diagp = wm_palette_find("DIAGP");
    int slot = -1;

    memset(&g_st, 0, sizeof(g_st));
    assert(wm_pal_init_with_diagp(&g_st));
    assert(g_st.slot[0] == diagp);
    assert(wm_pal_find(&g_st, diagp, &slot) && slot == 0);

    /* pal_clean never touches slot 0, which is the whole point of
     * taking it first. */
    wm_pal_clean(&g_st);
    assert(g_st.slot[0] == diagp);

    /* The by-name bridge for the animation VM's pal_getf seam. */
    assert(wm_pal_getf_by_name(&g_st, "BAMBLU_P") == wm_pal_dma(1));
    assert(wm_pal_getf_by_name(&g_st, "BAMBLU_P") == wm_pal_dma(1));
    assert(wm_pal_getf_by_name(&g_st, "NO_SUCH_P") == 0);
}

int main(void)
{
    test_palette_data();
    test_allocation();
    test_a_full_pool_cleans_then_fails();
    test_the_pool_is_full_and_nothing_is_free();
    test_clean_asks_before_freeing();
    test_a_full_transfer_queue_refuses_the_slot();
    test_transfer();
    test_transfer_stops_at_the_first_gap();
    test_blacken();
    test_fade_maths();
    test_fade_keeps_the_count_word();
    test_global_fade();
    test_global_fade_skip_list();
    test_init_parks_diagp_at_zero();
    printf("PAL.ASM allocator and faders: all checks passed\n");
    return 0;
}
