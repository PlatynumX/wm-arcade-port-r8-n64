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
    WM_BMOD_VFLIP = 1u << 1
};

bool wm_bmod_decode_block(const wm_bmod_module *module, size_t index,
                          wm_bmod_block *out);

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
