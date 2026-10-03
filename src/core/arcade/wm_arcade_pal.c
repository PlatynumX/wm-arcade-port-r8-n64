/*
 * PAL.ASM's palette allocator, colour maps and faders.
 * See include/wm/arcade/wm_arcade_pal.h for the constants and the two
 * routines the shipped build does not contain.
 */
#include "wm/arcade/wm_arcade_pal.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* palette data                                                        */

const uint16_t *wm_palette_find(const char *name)
{
    int lo = 0;
    int hi = wm_palette_count - 1;

    if (!name) {
        return NULL;
    }
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int cmp = strcmp(name, wm_palettes[mid].name);
        if (cmp == 0) {
            return wm_palettes[mid].words;
        }
        if (cmp < 0) {
            hi = mid - 1;
        } else {
            lo = mid + 1;
        }
    }
    return NULL;
}

uint16_t wm_pal_color_count(const uint16_t *palette)
{
    return palette ? (uint16_t)(palette[0] & WM_PAL_COUNT_MASK) : 0u;
}

uint16_t wm_pal_dma(int index)
{
    return (uint16_t)((index & 0xFF) | ((index & 0xFF) << 8));
}

/* ------------------------------------------------------------------ */
/* the allocator                                                       */

void wm_pal_init(wm_pal_state *st)
{
    if (!st) {
        return;
    }
    /* pal_init clears PALRAM and PALTRAM and nothing else -- FADERAM
     * keeps whatever was in it, which is fine because every writer
     * fills a block before reading it back. */
    memset(st->slot, 0, sizeof(st->slot));
    memset(st->xfer, 0, sizeof(st->xfer));
}

bool wm_pal_find(const wm_pal_state *st, const uint16_t *palette, int *index)
{
    int i;

    if (!st) {
        return false;
    }
    for (i = 0; i < WM_NUMPAL; ++i) {
        if (st->slot[i] == palette) {
            if (index) {
                *index = i;
            }
            return true;
        }
    }
    return false;
}

bool wm_pal_set(wm_pal_state *st, const uint16_t *src, uint16_t dest,
                uint16_t count)
{
    int i;

    if (!st) {
        return false;
    }
    for (i = 0; i < WM_NUMPALT; ++i) {
        if (st->xfer[i].count == 0u) {
            st->xfer[i].src = src;
            st->xfer[i].dest = dest;
            /* PLDCNT last: it is what makes the cell live. */
            st->xfer[i].count = count;
            return true;
        }
    }
    return false;
}

static bool getf_claim(wm_pal_state *st, const uint16_t *palette, int slot,
                       int *index)
{
    /* gbp30: the destination is the palette number in bits 8-15 with a
     * start colour of zero, and the count comes from the block's own
     * first word -- flags included, because pal_set copies it whole. */
    uint16_t dest = (uint16_t)((slot & 0xFF) << 8);
    if (!wm_pal_set(st, palette + 1, dest, palette[0])) {
        return false;           /* no transfer cell: the slot stays free */
    }
    st->slot[slot] = palette;
    if (index) {
        *index = slot;
    }
    return true;
}

bool wm_pal_getf(wm_pal_state *st, const uint16_t *palette, int *index)
{
    int i;

    if (!st || !palette) {
        return false;
    }
    /* Already in colour ram? */
    if (wm_pal_find(st, palette, index)) {
        return true;
    }
    for (i = 0; i < WM_NMFPAL; ++i) {
        if (!st->slot[i]) {
            return getf_claim(st, palette, i, index);
        }
    }
    /* Nothing free: throw away what nothing is drawing and look again. */
    wm_pal_clean(st);
    for (i = 0; i < WM_NMFPAL; ++i) {
        if (!st->slot[i]) {
            return getf_claim(st, palette, i, index);
        }
    }
    return false;
}

bool wm_pal_init_with_diagp(wm_pal_state *st)
{
    const uint16_t *diagp = wm_palette_find("DIAGP");
    int slot = -1;

    wm_pal_init(st);
    if (!diagp) {
        return false;
    }
    /* "always start with DIAGP as pal 0!" */
    return wm_pal_getf(st, diagp, &slot) && slot == 0;
}

int32_t wm_pal_getf_by_name(wm_pal_state *st, const char *name)
{
    const uint16_t *pal = wm_palette_find(name);
    int slot = 0;

    if (!pal || !wm_pal_getf(st, pal, &slot)) {
        return 0;
    }
    return (int32_t)wm_pal_dma(slot);
}

void wm_pal_transfer(wm_pal_state *st)
{
    int cell;

    if (!st) {
        return;
    }
    for (cell = 0; cell < WM_NUMPALT; ++cell) {
        uint16_t count = st->xfer[cell].count;
        const uint16_t *src = st->xfer[cell].src;
        uint32_t at;
        uint32_t n;
        uint32_t i;

        if (count == 0u) {
            break;              /* `jrz #x` -- the first gap ends the drain */
        }
        st->xfer[cell].count = 0u;

        /* `zext a2 / sll 4 / add COLRAM` -- the whole destination word
         * scales, so palette n starts 256 colours in whatever its
         * length is. */
        at = st->xfer[cell].dest;
        n = (uint32_t)(count & WM_PAL_COUNT_MASK);
        for (i = 0; i < n; ++i) {
            if (at + i >= WM_COLRAM_WORDS || !src) {
                break;
            }
            st->colram[at + i] = src[i];
        }
    }
}

bool wm_pal_blacken(wm_pal_state *st, const uint16_t *palette)
{
    int slot = 0;
    int i;

    if (!st || !wm_pal_find(st, palette, &slot)) {
        return false;
    }
    /* `movk 32,b0` longs -- sixty-four words of black -- then a transfer
     * of exactly 64 colours, whatever the palette's own length is. */
    for (i = 0; i < 64; ++i) {
        st->fade_ram[0][i] = 0u;
    }
    return wm_pal_set(st, st->fade_ram[0], (uint16_t)((slot & 0xFF) << 8), 64u);
}

void wm_pal_clean(wm_pal_state *st)
{
    int i;

    if (!st) {
        return;
    }
    /* `movi PALRAM+PALRSIZ,a2 / movi NUMPAL-1,a3` -- slot 0 is never
     * freed, which is what makes pal_init's DIAGP permanent. */
    for (i = 1; i < WM_NUMPAL; ++i) {
        if (!st->slot[i]) {
            continue;
        }
        if (st->in_use && st->in_use(st->user, wm_pal_dma(i))) {
            continue;
        }
        if (st->ignore_special && st->ignore_this_pal &&
            st->ignore_this_pal(st->user, st->slot[i])) {
            continue;           /* `JRC cp80` -- carry means keep it */
        }
        st->slot[i] = NULL;
    }
}

/* ------------------------------------------------------------------ */
/* colour arithmetic                                                   */

static void unpack(uint16_t color, uint32_t *r, uint32_t *g, uint32_t *b)
{
    *b = (uint32_t)(color & WM_PAL_CHANNEL_MAX);
    *g = (uint32_t)((color >> 5) & WM_PAL_CHANNEL_MAX);
    *r = (uint32_t)((color >> 10) & WM_PAL_CHANNEL_MAX);
}

static uint16_t pack(uint32_t r, uint32_t g, uint32_t b)
{
    return (uint16_t)((r << 10) | (g << 5) | b);
}

/* `cmp a8,a3 / jrls ok / move a8,a3` -- an UNSIGNED compare against 31,
 * so anything that wrapped below zero comes out as 31, not as 0. */
static uint32_t clamp_channel(uint32_t v)
{
    return v <= (uint32_t)WM_PAL_CHANNEL_MAX ? v : (uint32_t)WM_PAL_CHANNEL_MAX;
}

void wm_pal_fade(uint16_t *dst, const uint16_t *src, int brightness)
{
    uint32_t n;
    uint32_t i;

    if (!dst || !src) {
        return;
    }
    dst[0] = src[0];            /* the count word, flags and all */
    n = (uint32_t)(src[0] & WM_PAL_COUNT_MASK);
    for (i = 0; i < n; ++i) {
        uint32_t r, g, b;
        unpack(src[1 + i], &r, &g, &b);
        r = clamp_channel((r * (uint32_t)brightness) >> 7);
        g = clamp_channel((g * (uint32_t)brightness) >> 7);
        b = clamp_channel((b * (uint32_t)brightness) >> 7);
        dst[1 + i] = pack(r, g, b);
    }
}

void wm_pal_addb(uint16_t *dst, const uint16_t *src, int brightness)
{
    uint32_t n;
    uint32_t i;

    if (!dst || !src) {
        return;
    }
    dst[0] = src[0];
    n = (uint32_t)(src[0] & WM_PAL_COUNT_MASK);
    for (i = 0; i < n; ++i) {
        uint32_t r, g, b;
        unpack(src[1 + i], &r, &g, &b);
        r = clamp_channel(r + (uint32_t)brightness);
        g = clamp_channel(g + (uint32_t)brightness);
        b = clamp_channel(b + (uint32_t)brightness);
        dst[1 + i] = pack(r, g, b);
    }
}

uint16_t wm_pal_scale_color(uint16_t color, int level)
{
    uint32_t r, g, b;
    unpack(color, &r, &g, &b);
    /* `mpyu a10,a7 / srl 8,a7` and no clamp -- 31 * 256 >> 8 is 31, so
     * the channel cannot leave five bits. */
    r = (r * (uint32_t)level) >> 8;
    g = (g * (uint32_t)level) >> 8;
    b = (b * (uint32_t)level) >> 8;
    return pack(r, g, b);
}

/* ------------------------------------------------------------------ */
/* do_fade                                                             */

static void global_fade_start(wm_pal_state *st, wm_pal_global_fade *job,
                              const uint16_t *const *skip,
                              int start, int end, int inc)
{
    memset(job, 0, sizeof(*job));
    job->skip = skip;
    job->start = start;
    job->end = end;
    job->inc = inc;
    job->level = start;
    /* do_fade's first act is to snapshot the autoerase colour. */
    st->irqskyeo = st->irqskye;
}

void wm_pal_fade_up(wm_pal_state *st, wm_pal_global_fade *job,
                    const uint16_t *const *skip, int inc)
{
    if (st && job) {
        global_fade_start(st, job, skip, 0, WM_PAL_LEVEL_FULL, inc);
    }
}

void wm_pal_fade_down(wm_pal_state *st, wm_pal_global_fade *job,
                      const uint16_t *const *skip, int inc)
{
    if (st && job) {
        global_fade_start(st, job, skip, WM_PAL_LEVEL_FULL, 0, inc);
    }
}

void wm_pal_fade_up_half(wm_pal_state *st, wm_pal_global_fade *job,
                         const uint16_t *const *skip)
{
    if (st && job) {
        global_fade_start(st, job, skip, 128, WM_PAL_LEVEL_FULL, 16);
    }
}

void wm_pal_fade_down_half(wm_pal_state *st, wm_pal_global_fade *job,
                           const uint16_t *const *skip)
{
    if (st && job) {
        global_fade_start(st, job, skip, WM_PAL_LEVEL_FULL, 128, 16);
    }
}

static bool in_skip_list(const uint16_t *const *skip, const uint16_t *pal)
{
    int i;
    if (!skip) {
        return false;
    }
    for (i = 0; skip[i]; ++i) {
        if (skip[i] == pal) {
            return true;
        }
    }
    return false;
}

bool wm_pal_global_fade_step(wm_pal_state *st, wm_pal_global_fade *job)
{
    int i;

    if (!st || !job) {
        return false;
    }
    /* The autoerase colour scales with everything else. */
    st->irqskye = wm_pal_scale_color(st->irqskyeo, job->level);

    for (i = 0; i < WM_NUMPAL; ++i) {
        const uint16_t *pal = st->slot[i];
        uint16_t *scratch = st->fade_ram[i];
        uint32_t n;
        uint32_t c;

        /* FADERAM advances one block per SLOT, empty ones included, so
         * every palette keeps its own scratch. */
        if (!pal || in_skip_list(job->skip, pal)) {
            continue;
        }
        n = (uint32_t)(pal[0] & WM_PAL_COUNT_MASK);
        for (c = 0; c < n && c < WM_FPALSZ; ++c) {
            scratch[c] = wm_pal_scale_color(pal[1 + c], job->level);
        }
        /* do_fade passes the palette's colour count -- read before the
         * copy, and not re-read out of the scratch as pal_fadein does. */
        wm_pal_set(st, scratch, (uint16_t)((i & 0xFF) << 8), pal[0]);
    }

    if (job->end > job->start) {
        job->level += job->inc;
        return job->level <= job->end;
    }
    job->level -= job->inc;
    return job->level >= job->end;
}
