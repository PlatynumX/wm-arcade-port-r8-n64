#ifndef WM_BMOD_H
#define WM_BMOD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Source BAKGND.ASM packed 64-bit block record:
   word0: palette low nibble, flags nibble, Z byte
   word1: X
   word2: Y
   word3: header index in bits 0-11, palette high nibble in bits 12-15. */
typedef struct {
    uint8_t palette;
    uint8_t flags;
    uint8_t z;
    int16_t x;
    int16_t y;
    uint16_t header_index;
} wm_bmod_block;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t block_count;
    const uint16_t *packed_words; /* 4 words per block */
} wm_bmod_module;

typedef struct {
    const char *name;
    wm_bmod_module module;
    const char *headers_label;
    const char *palettes_label;
} wm_named_bmod;

enum {
    WM_BMOD_HFLIP = 1u << 0,
    WM_BMOD_VFLIP = 1u << 1,
    /* BAKGND.ASM bgnd_addblk tests MAP_FLAGS bit 2 before deciding whether
       palette index zero writes to the framebuffer. */
    WM_BMOD_TRANSPARENT = 1u << 2
};

bool wm_bmod_decode_block(const wm_bmod_module *module, size_t index,
                          wm_bmod_block *out);

/* The Midway background-object ordering, from the source rather than from
   observation.  BAKGND.ASM:721 sends normal background blocks through
   INSBOBJ, which is DISPLAY.ASM:1540 -- an earlier comment here said that
   body was "not present in this game's source drop", which was wrong; so is
   obj_yzsort, at DISPLAY.ASM:1248, and the two agree.

   INSBOBJ's own header says it: "List is sorted by increasing z and
   increasing y within constant z".  Its walk steps past every node whose Z
   is smaller and, at equal Z, every node whose Y is smaller, inserting
   before the first that is not.  obj_yzsort maintains the same order with a
   bubble pass, and its equal-Z test is `jrge` -- so two objects at the same
   Z and the same Y are never swapped and keep the order they were inserted
   in.  Returning false for an exact tie is that stability. */
bool wm_bmod_draw_before(const wm_bmod_block *a, const wm_bmod_block *b);

/* Original background visibility test used by BGND_UD1/bgnd_addmod. The N64
   backend can use this before issuing RDP draws; pad is in source pixels. */
bool wm_bmod_block_intersects(const wm_bmod_block *b,
                              uint16_t block_w, uint16_t block_h,
                              int world_x, int world_y,
                              int view_left, int view_top,
                              int view_right, int view_bottom,
                              int pad_x, int pad_y);

/* Implemented by generated BGNDTBL source data in N64 builds. */
size_t wm_source_bmod_count(void);
const wm_named_bmod *wm_source_bmod_at(size_t i);
const wm_named_bmod *wm_source_bmod_find(const char *name);

#endif
