#include <libdragon.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wm/frame_geometry.h"
#include "wm/streamed_character_art.h"

#define WM_STREAM_CACHE_SLOTS 8
#define WM_STREAM_FRAME_NAME 32
#define WM_STREAM_TLUT_COLORS 256
#define WM_STREAM_TLUT_BYTES (WM_STREAM_TLUT_COLORS * 2u)
#define WM_ATTACH_X_SLOT 3
#define WM_ATTACH_Y_SLOT 4

typedef struct {
    bool valid;
    int32_t wrestler_num;
    char frame[WM_STREAM_FRAME_NAME];
    uint8_t *memory;
    size_t bytes;
    uint32_t stamp;
    wm_source_sprite sprite;
} wm_stream_cache_entry;

static wm_stream_cache_entry cache[WM_STREAM_CACHE_SLOTS];
static uint32_t cache_stamp;

static bool source_roster_id(int32_t id) {
    return (id >= 0 && id <= 6) || id == 8;
}

static wm_stream_cache_entry *find_hit(int32_t wrestler_num,
                                       const char *frame) {
    for (unsigned i = 0; i < WM_STREAM_CACHE_SLOTS; ++i) {
        if (cache[i].valid && cache[i].wrestler_num == wrestler_num &&
            strcmp(cache[i].frame, frame) == 0)
            return &cache[i];
    }
    return NULL;
}

static wm_stream_cache_entry *pick_slot(void) {
    wm_stream_cache_entry *oldest = &cache[0];
    for (unsigned i = 0; i < WM_STREAM_CACHE_SLOTS; ++i) {
        if (!cache[i].valid)
            return &cache[i];
        if (cache[i].stamp < oldest->stamp)
            oldest = &cache[i];
    }
    return oldest;
}

const wm_source_sprite *wm_n64_streamed_character_find(
    int32_t wrestler_num, const char *source_frame) {
    const wm_frame_geometry_t *geo;
    wm_stream_cache_entry *slot;
    char path[96];
    FILE *fp;
    size_t pixel_bytes, palette_offset, alloc_bytes;

    if (!source_frame || !source_roster_id(wrestler_num))
        return NULL;

    slot = find_hit(wrestler_num, source_frame);
    if (slot) {
        slot->stamp = ++cache_stamp;
        return &slot->sprite;
    }

    geo = wm_frame_geometry_find(source_frame);
    if (!geo || geo->width == 0 || geo->height == 0)
        return NULL;

    pixel_bytes = (size_t)geo->width * (size_t)geo->height;
    palette_offset = (pixel_bytes + 7u) & ~(size_t)7u;
    alloc_bytes = palette_offset + WM_STREAM_TLUT_BYTES;

    snprintf(path, sizeof(path), "rom:/wrestlers/%ld/%s.bin",
             (long)wrestler_num, source_frame);
    fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    slot = pick_slot();
    if (slot->memory) {
        free(slot->memory);
        slot->memory = NULL;
    }
    memset(&slot->sprite, 0, sizeof(slot->sprite));
    slot->valid = false;

    slot->memory = malloc(alloc_bytes);
    if (!slot->memory) {
        fclose(fp);
        return NULL;
    }
    memset(slot->memory, 0, alloc_bytes);

    if (fread(slot->memory, 1, pixel_bytes, fp) != pixel_bytes ||
        fread(slot->memory + palette_offset, 1,
              WM_STREAM_TLUT_BYTES, fp) != WM_STREAM_TLUT_BYTES) {
        fclose(fp);
        free(slot->memory);
        slot->memory = NULL;
        return NULL;
    }
    fclose(fp);

    data_cache_hit_writeback(slot->memory, alloc_bytes);

    slot->wrestler_num = wrestler_num;
    snprintf(slot->frame, sizeof(slot->frame), "%s", source_frame);
    slot->bytes = alloc_bytes;
    slot->stamp = ++cache_stamp;

    slot->sprite.source_frame = slot->frame;
    slot->sprite.source_container = "DragonFS wrestler frame";
    slot->sprite.width = geo->width;
    slot->sprite.height = geo->height;
    slot->sprite.xani = geo->xani;
    slot->sprite.yani = geo->yani;
    for (unsigned i = 0; i < WM_WIMP_TAIL_WORDS; ++i)
        slot->sprite.wimp_tail[i] = -1;
    slot->sprite.wimp_tail[WM_ATTACH_X_SLOT] = geo->attach_x;
    slot->sprite.wimp_tail[WM_ATTACH_Y_SLOT] = geo->attach_y;
    slot->sprite.pixels_ci8 = slot->memory;
    slot->sprite.palette_rgba5551 =
        (uint16_t *)(void *)(slot->memory + palette_offset);
    slot->sprite.palette_colors = WM_STREAM_TLUT_COLORS;
    slot->valid = true;
    return &slot->sprite;
}

void wm_n64_streamed_character_reset(void) {
    for (unsigned i = 0; i < WM_STREAM_CACHE_SLOTS; ++i) {
        free(cache[i].memory);
        memset(&cache[i], 0, sizeof(cache[i]));
    }
    cache_stamp = 0;
}

size_t wm_n64_streamed_character_cached(void) {
    size_t n = 0;
    for (unsigned i = 0; i < WM_STREAM_CACHE_SLOTS; ++i)
        if (cache[i].valid) ++n;
    return n;
}
