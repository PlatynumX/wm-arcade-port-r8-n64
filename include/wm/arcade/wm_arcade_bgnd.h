#ifndef WM_ARCADE_BGND_H
#define WM_ARCADE_BGND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BAKGND.ASM's background scroller: BGND_UD1 (:358), bgnd_scanmod
 * (:397), bgnd_addmod (:457), bgnd_get1stx (:182) and bgnd_delnonvis
 * (:794).
 *
 * scroll_world moves the camera and has been translated for a while.
 * This is what the camera moves OVER. A background is a MODULE of
 * blocks placed at a world position, and the arena is one of them:
 * WRESTLE.ASM:1615 `movi ring_mod,a0 / move a0,@BAKMODS,L` hands a
 * match a single-entry list naming ringBMOD at (105,-450), 211
 * blocks of it. Every frame BGND_UD1 deletes the blocks that have
 * scrolled off and adds the ones that have scrolled on.
 *
 * This is not the drawing. What comes out is the SET of blocks that
 * should be on the display list and the two edges of it -- which
 * became resident this frame and which stopped being -- for a
 * renderer to act on.
 *
 * ------------------------------------------------------------------
 * Three things in here are easy to lose, and all three are load
 * bearing:
 *
 * 1. THE BLOCKS ARE SORTED BY X, and the scan exploits it twice. A
 *    binary search finds where to start, and once a block's left
 *    edge is past the display's left edge the scan drops into a
 *    cheaper test and stops dead at the first block past the right
 *    edge. Walking all 211 every frame would give the same answer
 *    and miss the point of the routine.
 *
 * 2. A BLOCK CARRIES NO SIZE. Its record has a palette, flags, Z, X,
 *    Y and an index into the module's HEADER table, and the width
 *    and height live there. That indirection is why the two scan
 *    phases look different: the cheap one is arranged to avoid
 *    touching the header at all where it can.
 *
 * 3. ADDING AND DELETING DO NOT USE THE SAME TEST. Adding needs
 *    `X < BR.x` and `Y < BR.y`; deleting keeps on `X <= BR.x` and
 *    `Y <= BR.y`. A block exactly on the right or bottom edge is
 *    therefore not added, but is not deleted either once it is
 *    there. That asymmetry is in the shipped code and is reproduced
 *    rather than tidied -- flattening it would make blocks on the
 *    edge flicker in and out as the camera creeps.
 */

/* One packed 64-bit block record, as BGNDTBL.ASM writes it. */
typedef struct {
    /* MAP_PAL in bits 0-3, MAP_FLAGS in 4-7, MAP_Z in 8-15. */
    uint16_t packed;
    int16_t x;
    int16_t y;
    /* MAP_HDR: bits 0-11 index the header table, bits 12-15 are bits
       4-7 of the palette. 0xFFFF means the block is not allocated. */
    uint16_t hdr;
} wm_bgnd_block_t;

/* MAP_W and MAP_H out of the module's header table. */
typedef struct {
    uint16_t w;
    uint16_t h;
} wm_bgnd_header_t;

typedef struct {
    const char *name;
    int32_t width;
    int32_t height;
    const wm_bgnd_block_t *blocks;
    size_t block_count;
    const wm_bgnd_header_t *headers;
    size_t header_count;
} wm_bgnd_module_t;

/* A module and where the module list puts it. */
typedef struct {
    const wm_bgnd_module_t *module;
    int32_t x;
    int32_t y;
} wm_bgnd_placed_t;

extern const wm_bgnd_module_t wm_bgnd_modules[];
extern const size_t wm_bgnd_module_count;
extern const wm_bgnd_placed_t wm_bgnd_ring_mod[];
extern const size_t wm_bgnd_ring_mod_count;

/* BAKGND.ASM:44 `DISP_PAD .SET [20h,40h] ;Y:X` -- the border BGND_UD1
   adds around the screen so a block is resident before it is seen. */
#define WM_BGND_PAD_X 0x40
#define WM_BGND_PAD_Y 0x20
/* BAKGND.ASM:46 WIDEST_BLOCK, how far left of the display edge the
   search has to begin so a wide block overlapping it is not missed. */
#define WM_BGND_WIDEST_BLOCK 250

typedef struct {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} wm_bgnd_rect_t;

/*
 * BGND_UD1's own framing: WORLDTL plus SCRNTL and SCRNLR, then
 * padded outward by DISP_PAD on both corners.
 */
wm_bgnd_rect_t wm_bgnd_display_rect(int32_t worldtl_x, int32_t worldtl_y,
                                    int32_t scrntl_x, int32_t scrntl_y,
                                    int32_t scrnlr_x, int32_t scrnlr_y);

/*
 * BAKBITS: one bit per block across the whole module list, saying
 * whether it is on the display list. The ring alone is 211, and the
 * source sizes the real table at 10,000 bits.
 */
#define WM_BGND_MAX_BLOCKS 1024

typedef struct {
    uint8_t resident[WM_BGND_MAX_BLOCKS / 8];
    size_t resident_count;
} wm_bgnd_state_t;

void wm_bgnd_state_init(wm_bgnd_state_t *s);
bool wm_bgnd_is_resident(const wm_bgnd_state_t *s, size_t block);

/* One block changing residency. */
typedef struct {
    /* Its index across the whole list, which is its BAKBIT. */
    size_t index;
    const wm_bgnd_module_t *module;
    const wm_bgnd_block_t *block;
    /* Where it sits in the world: the module's origin plus its own
       offset, and the size out of its header. */
    int32_t world_x;
    int32_t world_y;
    uint16_t w;
    uint16_t h;
} wm_bgnd_event_t;

/*
 * BAKGND.ASM:182 bgnd_get1stx. The first block in `module` whose X is
 * at least `x`, by binary search down to a span of five and then a
 * linear walk -- the source's own threshold. Returns the block index,
 * or the block count when there is none.
 *
 * Exposed because the search is the reason the scan is cheap, and a
 * search that is subtly wrong still returns plausible answers.
 */
size_t wm_bgnd_get1stx(const wm_bgnd_module_t *module, int32_t x);

/*
 * BAKGND.ASM:794 bgnd_delnonvis, then :397 bgnd_scanmod -- in that
 * order, which is BGND_UD1's order and matters: deleting first frees
 * the objects the adds then take.
 *
 * `added` and `removed` receive the two edges. Either may be NULL to
 * be counted without being recorded.
 *
 * The COUNTS are always the true ones, even when an array was too
 * small: `*overflow` then says the DETAILS of some events are
 * missing. The residency bits are correct either way, so asking
 * again with a bigger array reports nothing -- the state has already
 * moved. An overflow means a renderer has lost work to do, not that
 * the scroller is confused.
 */
void wm_bgnd_update(wm_bgnd_state_t *s,
                    const wm_bgnd_placed_t *list, size_t list_count,
                    wm_bgnd_rect_t view,
                    wm_bgnd_event_t *added, size_t added_cap,
                    size_t *added_count,
                    wm_bgnd_event_t *removed, size_t removed_cap,
                    size_t *removed_count,
                    bool *overflow);

/* The two halves on their own, for callers that want one. */
size_t wm_bgnd_delnonvis(wm_bgnd_state_t *s,
                         const wm_bgnd_placed_t *list, size_t list_count,
                         wm_bgnd_rect_t view,
                         wm_bgnd_event_t *out, size_t cap, bool *overflow);
size_t wm_bgnd_scanmod(wm_bgnd_state_t *s,
                       const wm_bgnd_placed_t *list, size_t list_count,
                       wm_bgnd_rect_t view,
                       wm_bgnd_event_t *out, size_t cap, bool *overflow);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_BGND_H */
