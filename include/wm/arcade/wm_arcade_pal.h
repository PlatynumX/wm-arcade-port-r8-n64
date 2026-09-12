/*
 * PAL.ASM -- the palette allocator, the colour maps, and the faders.
 *
 * Translated from original/wwf-wrestlemania/PAL.ASM against the constants
 * in SYS.EQU. The board is a T-Unit (`TUNIT .equ 1`), so:
 *
 *     NUMPAL 80   NMFPAL 80   NMBPAL 0   NUMPALT 60   PAL64 0
 *
 * NMBPAL being zero matters: `pal_getb` sits inside `.if NMBPAL` and is
 * therefore *not assembled into the shipped game at all*. Its one caller
 * (BAKGND.ASM:632) is inside the same guard and falls through to
 * `pal_getf`, so with no background pool the foreground pool is the
 * whole of PALRAM and there is one allocator, not two. It is not
 * translated here, and neither is the `addk NMBPAL,a3` adjustment in
 * the same guard.
 *
 * Half of PAL.ASM is dead code in this game, and counting the call
 * sites is what says so. Live: pal_getf (76 callers), pal_clean (20),
 * fade_down (17), fade_up (13), pal_find (5), pal_set (5), pal_init (3),
 * pal_transfer (1) and fade_down_half (1). Never called from anywhere
 * outside PAL.ASM: pal_blacken, pal_fadein, pal_fadeout, pal_fadeout2,
 * pal_fadeinx, pal_fadeoutx, pal_fmwht, pal_towht, addbrt_ae and
 * fade_up_half. The process-driven faders in that dead half
 * (`fadein`, `fadeout`, `fadeonep`, `brightenonep`, `addbrt_ae`) are
 * not translated -- they are process wrappers around pal_fade and
 * pal_addb, which are here and are what actually does the work.
 *
 * A palette is one contiguous block: a count word, then that many RGB555
 * colours (blue 0-4, green 5-9, red 10-14). Only the count word's low
 * nine bits are the length; the rest are flags that every routine masks
 * off before looping (`sll 32-9 / srl 32-9`) but passes through intact
 * when it copies the word. IMGPAL.ASM's 337 palettes are extracted whole
 * by tools/wlpal.py.
 *
 * What is not here: `pal_transfer`'s destination is real hardware colour
 * RAM at COLRAM. This keeps an ordinary array indexed exactly the way the
 * source computes the address, so the arithmetic is testable and a
 * platform layer can upload from it.
 */
#ifndef WM_ARCADE_PAL_H
#define WM_ARCADE_PAL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SYS.EQU:283-289, the TUNIT branch. */
#define WM_NUMPAL 80
#define WM_NMFPAL 80
#define WM_NMBPAL 0
/* SYS.EQU:303 -- how many transfers may be queued between vblanks. */
#define WM_NUMPALT 60
/* PAL.ASM:47-48 */
#define WM_FPALNUM 8
#define WM_FPALSZ 256

/*
 * pal_transfer turns a destination word into a colour-RAM address with
 * `sll 4` on the whole `palette<<8 | start colour`, so consecutive
 * palettes are 256 colours apart whatever a given palette's length is.
 */
#define WM_PAL_MAP_STRIDE 256
#define WM_COLRAM_WORDS (WM_NUMPAL * WM_PAL_MAP_STRIDE)

/* The nine bits of the count word that are the colour count. */
#define WM_PAL_COUNT_MASK 0x1FF
/* A five-bit colour channel, which is what every clamp compares against. */
#define WM_PAL_CHANNEL_MAX 0x1F

/* pal_fade divides by 128 after multiplying, so full brightness is 128. */
#define WM_PAL_FADE_FULL 128
/* do_fade divides by 256 instead -- a different scale, deliberately. */
#define WM_PAL_LEVEL_FULL 256

/* One of IMGPAL.ASM's palettes: `words[0]` is the count word. */
typedef struct wm_palette {
    const char *name;
    const uint16_t *words;
} wm_palette;

extern const wm_palette wm_palettes[];
extern const int wm_palette_count;

/* Look one up by its IMGPAL.ASM label, or NULL. */
const uint16_t *wm_palette_find(const char *name);

/* The colour count a block declares, with the flag bits masked off. */
uint16_t wm_pal_color_count(const uint16_t *palette);

/* PALTRAM: one queued transfer. PLDCNT is written last in the source
 * ("Must set last") because the vblank half reads it to decide whether
 * the cell is live -- so a zero count means the cell is free. */
typedef struct wm_pal_transfer_cell {
    uint16_t count;             /* PLDCNT, count word including its flags */
    const uint16_t *src;        /* PALSRC */
    uint16_t dest;              /* PALDEST: palette<<8 | start colour */
} wm_pal_transfer_cell;

/*
 * pal_clean frees a slot no drawn object is pointing at, which means
 * walking OBJLST and BAKLST. Those lists belong to the display code, so
 * they arrive here as a question instead.
 */
typedef bool (*wm_pal_in_use_fn)(void *user, uint16_t dma_pal);
/* IGNORE_THIS_PAL: returns true (the source's carry) to keep a palette
 * that nothing is drawing. Consulted only while IGNORE_SPECIAL is set. */
typedef bool (*wm_pal_ignore_fn)(void *user, const uint16_t *palette);

/*
 * The allocator's whole state. This is large -- FADERAM alone is
 * 80 * 256 words -- so give it static or heap storage, never a stack
 * frame.
 */
typedef struct wm_pal_state {
    const uint16_t *slot[WM_NUMPAL];                /* PALRAM */
    wm_pal_transfer_cell xfer[WM_NUMPALT];          /* PALTRAM */
    uint16_t fade_ram[WM_NMFPAL][WM_FPALSZ];        /* FADERAM */
    uint16_t colram[WM_COLRAM_WORDS];               /* COLRAM */

    uint16_t irqskye;           /* MAIN.ASM's autoerase colour */
    uint16_t irqskyeo;          /* PAL.ASM's saved copy of it */
    int16_t palfmin;            /* the floor pal_fadeout2 stops at */

    bool ignore_special;        /* the IGNORE_SPECIAL flag pal_clean reads */
    wm_pal_in_use_fn in_use;
    wm_pal_ignore_fn ignore_this_pal;
    void *user;
} wm_pal_state;

/* The DMA-format palette number the source returns: `n | n<<8`. */
uint16_t wm_pal_dma(int index);

/*
 * pal_init (PAL.ASM:101): clear PALRAM and PALTRAM. The source then
 * grabs DIAGP so it is always palette 0 -- do that with
 * wm_pal_getf(st, wm_palette_find("DIAGP")), which is what
 * wm_pal_init_with_diagp does. FADERAM is deliberately not cleared,
 * matching the source.
 */
void wm_pal_init(wm_pal_state *st);
bool wm_pal_init_with_diagp(wm_pal_state *st);

/*
 * pal_find (PAL.ASM:195): which colour map a palette is assigned to.
 * Returns false when it is not assigned. Note that the source returns 0
 * in A0 both for "slot 0" and for "not found" and separates them by the
 * Z flag alone; the bool is that flag.
 */
bool wm_pal_find(const wm_pal_state *st, const uint16_t *palette, int *index);

/*
 * pal_getf (PAL.ASM:236): the palette's existing slot if it already has
 * one, else the first free slot, else -- after one pal_clean -- fail.
 * Allocating also queues the transfer that uploads it, and the slot is
 * only claimed if that queue had room.
 */
bool wm_pal_getf(wm_pal_state *st, const uint16_t *palette, int *index);

/*
 * The bridge for wm_anim_env's `pal_getf` seam, which asks by name:
 * resolve an IMGPAL.ASM label and allocate it a colour map, returning
 * the DMA-format palette number or 0.
 *
 * Zero doubles as "no palette" there, and that is safe here: slot 0 is
 * DIAGP, which pal_init takes before anything else can and pal_clean
 * never frees, so no caller of this can legitimately be handed it.
 */
int32_t wm_pal_getf_by_name(wm_pal_state *st, const char *name);

/* pal_set (PAL.ASM:367): queue one transfer. False when PALTRAM is full. */
bool wm_pal_set(wm_pal_state *st, const uint16_t *src, uint16_t dest,
                uint16_t count);

/*
 * pal_transfer (PAL.ASM:400): drain PALTRAM into colour RAM. Called
 * during vblank in the arcade.
 *
 * Two details are the source's. The loop stops at the first cell with a
 * zero count rather than scanning all sixty, so a gap hides everything
 * behind it -- and pal_set always fills the lowest free cell, so gaps
 * only appear if something else frees one. And the count is masked to
 * nine bits here, not by pal_set, so a count of exactly 512 transfers
 * nothing.
 */
void wm_pal_transfer(wm_pal_state *st);

/* pal_blacken (PAL.ASM:470): zero 64 colours of an assigned palette. */
bool wm_pal_blacken(wm_pal_state *st, const uint16_t *palette);

/* pal_clean (PAL.ASM:126): free every slot from 1 up that nothing draws.
 * Slot 0 is skipped, which is why pal_init parks DIAGP there. */
void wm_pal_clean(wm_pal_state *st);

/*
 * pal_fade (PAL.ASM:760): copy `src` into `dst` with every channel
 * multiplied by `brightness` and divided by 128, clamped to 31. The
 * count word is copied through untouched, flags and all.
 */
void wm_pal_fade(uint16_t *dst, const uint16_t *src, int brightness);

/*
 * pal_addb (PAL.ASM:1048): the same shape, but `brightness` is *added*
 * to each channel. The clamp is an unsigned compare against 31, so a
 * negative sum saturates to full brightness rather than to black. The
 * shipped callers only ever pass 0..31, so that path is unreachable in
 * the game; it is reproduced rather than corrected.
 */
void wm_pal_addb(uint16_t *dst, const uint16_t *src, int brightness);

/* do_fade's per-colour scaling (PAL.ASM:1213): channel * level / 256.
 * No clamp, and none is needed -- 31 * 256 >> 8 is 31. */
uint16_t wm_pal_scale_color(uint16_t color, int level);

/*
 * do_fade (PAL.ASM:1180) and its four entry points. One pass over every
 * assigned palette at the current level, then the level moves by
 * `inc` toward `end`. fade_up is 0->256, fade_down 256->0, and the
 * _half pair run between 128 and 256 with an increment of 16.
 */
typedef struct wm_pal_global_fade {
    const uint16_t *const *skip; /* NULL-terminated, or NULL for "fade all" */
    int start;
    int end;
    int inc;                    /* always positive; direction comes from start/end */
    int level;                  /* A10 */
} wm_pal_global_fade;

void wm_pal_fade_up(wm_pal_state *st, wm_pal_global_fade *job,
                    const uint16_t *const *skip, int inc);
void wm_pal_fade_down(wm_pal_state *st, wm_pal_global_fade *job,
                      const uint16_t *const *skip, int inc);
void wm_pal_fade_up_half(wm_pal_state *st, wm_pal_global_fade *job,
                         const uint16_t *const *skip);
void wm_pal_fade_down_half(wm_pal_state *st, wm_pal_global_fade *job,
                           const uint16_t *const *skip);
bool wm_pal_global_fade_step(wm_pal_state *st, wm_pal_global_fade *job);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_PAL_H */
