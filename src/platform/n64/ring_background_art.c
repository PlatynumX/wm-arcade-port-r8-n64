#include <libdragon.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wm/ring_background_art.h"

#define WM_RING_IMAGE_SLOTS 128u
#define WM_RING_PALETTE_SLOTS 48u
#define WM_RING_TLUT_COLORS 256u
#define WM_RING_TLUT_BYTES (WM_RING_TLUT_COLORS * 2u)
#define WM_RING_HEADER_BYTES 6u

typedef struct {
    bool valid;
    uint8_t *memory;
    size_t bytes;
    wm_ring_background_image image;
} ring_image_slot;

static ring_image_slot images[WM_RING_IMAGE_SLOTS];

typedef struct {
    bool valid;
    unsigned index;
    bool transparent;
    uint16_t colors[WM_RING_TLUT_COLORS];
} ring_palette_slot;

static ring_palette_slot palettes[WM_RING_PALETTE_SLOTS]
    __attribute__((aligned(8)));

const wm_ring_background_image *wm_n64_ring_background_image(
    size_t header_index, uint16_t expected_width, uint16_t expected_height)
{
    ring_image_slot *slot;
    char path[64];
    uint8_t header[WM_RING_HEADER_BYTES];
    FILE *fp;
    size_t bytes;
    uint16_t width, height;

    if (header_index >= WM_RING_IMAGE_SLOTS ||
        expected_width == 0 || expected_height == 0)
        return NULL;

    slot = &images[header_index];
    if (slot->valid) {
        if (slot->image.width == expected_width &&
            slot->image.height == expected_height)
            return &slot->image;
        free(slot->memory);
        memset(slot, 0, sizeof(*slot));
    }

    snprintf(path, sizeof(path), "rom:/ring/h%03lu.bin",
             (unsigned long)header_index);
    fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    if (fread(header, 1, sizeof(header), fp) != sizeof(header) ||
        header[0] != 'W' || header[1] != 'R') {
        fclose(fp);
        return NULL;
    }

    width = (uint16_t)((header[2] << 8) | header[3]);
    height = (uint16_t)((header[4] << 8) | header[5]);
    if (width != expected_width || height != expected_height) {
        fclose(fp);
        return NULL;
    }

    bytes = (size_t)width * (size_t)height;
    slot->memory = malloc(bytes);
    if (!slot->memory) {
        fclose(fp);
        return NULL;
    }

    if (fread(slot->memory, 1, bytes, fp) != bytes) {
        fclose(fp);
        free(slot->memory);
        memset(slot, 0, sizeof(*slot));
        return NULL;
    }
    fclose(fp);

    data_cache_hit_writeback(slot->memory, bytes);
    slot->bytes = bytes;
    slot->image.width = width;
    slot->image.height = height;
    slot->image.pixels_ci8 = slot->memory;
    slot->valid = true;
    return &slot->image;
}

const uint16_t *wm_n64_ring_background_palette(
    unsigned palette_index, bool transparent)
{
    ring_palette_slot *slot = NULL;
    char path[64];
    FILE *fp;

    for (unsigned i = 0; i < WM_RING_PALETTE_SLOTS; ++i) {
        if (palettes[i].valid &&
            palettes[i].index == palette_index &&
            palettes[i].transparent == transparent)
            return palettes[i].colors;
        if (!palettes[i].valid && !slot)
            slot = &palettes[i];
    }
    if (!slot)
        return NULL;

    snprintf(path, sizeof(path), "rom:/ring/p%u_%c.pal",
             palette_index, transparent ? 't' : 'o');
    fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    if (fread(slot->colors, 1, WM_RING_TLUT_BYTES, fp) !=
        WM_RING_TLUT_BYTES) {
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    data_cache_hit_writeback(slot->colors, sizeof(slot->colors));
    slot->index = palette_index;
    slot->transparent = transparent;
    slot->valid = true;
    return slot->colors;
}

void wm_n64_ring_background_sync(
    const wm_bgnd_state_t *state,
    const wm_bgnd_module_t *module)
{
    bool needed[WM_RING_IMAGE_SLOTS];

    memset(needed, 0, sizeof(needed));

    if (state && module) {
        for (size_t bi = 0; bi < module->block_count; ++bi) {
            size_t hi;
            if (!wm_bgnd_is_resident(state, bi))
                continue;
            hi = (size_t)(module->blocks[bi].hdr & 0x0fffu);
            if (hi < WM_RING_IMAGE_SLOTS)
                needed[hi] = true;
        }
    }

    for (size_t i = 0; i < WM_RING_IMAGE_SLOTS; ++i) {
        if (!images[i].valid || needed[i])
            continue;
        free(images[i].memory);
        memset(&images[i], 0, sizeof(images[i]));
    }
}

void wm_n64_ring_background_reset(void)
{
    for (size_t i = 0; i < WM_RING_IMAGE_SLOTS; ++i) {
        free(images[i].memory);
        memset(&images[i], 0, sizeof(images[i]));
    }
    memset(palettes, 0, sizeof(palettes));
}

size_t wm_n64_ring_background_cached_images(void)
{
    size_t count = 0;
    for (size_t i = 0; i < WM_RING_IMAGE_SLOTS; ++i)
        if (images[i].valid)
            ++count;
    return count;
}
