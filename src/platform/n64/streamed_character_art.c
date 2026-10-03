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
/*
 * A frame file is `WM` + a big-endian u16 palette index + the CI8
 * pixels. The TLUT is NOT in it: the whole roster is 5,126 frames
 * sharing 23 palettes, so storing one per frame cost 2.5 MiB of
 * cartridge writing those 23 out 5,126 times. They live beside the
 * frames as pN.pal and are cached separately below, because a
 * palette has to outlive the frame that asked for it.
 */
#define WM_STREAM_HEADER 4
/*
 * 32, which is more than the 23 the whole roster has -- so nothing is
 * ever evicted. That is not generosity, it is a correctness
 * requirement: a cached frame's sprite points AT a palette slot, and
 * evicting one out from under a frame still in the frame cache would
 * leave it pointing at another wrestler's colours. 32 slots is 16 KiB.
 */
#define WM_STREAM_PAL_SLOTS 32
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

typedef struct {
    bool valid;
    int32_t wrestler_num;
    uint16_t index;
    uint32_t stamp;
    uint16_t colors[WM_STREAM_TLUT_COLORS];
} wm_stream_pal_entry;

static wm_stream_pal_entry pal_cache[WM_STREAM_PAL_SLOTS];
static uint32_t pal_stamp;

/*
 * One wrestler's palette N, loaded once and kept. The returned
 * pointer has to stay valid for as long as any frame using it is in
 * the frame cache, which is why these are a fixed array rather than
 * malloc'd alongside the pixels.
 */
static const uint16_t *palette_for(int32_t wrestler_num, uint16_t index) {
    wm_stream_pal_entry *slot = NULL;
    char path[96];
    FILE *fp;
    unsigned i;

    for (i = 0; i < WM_STREAM_PAL_SLOTS; ++i) {
        if (pal_cache[i].valid && pal_cache[i].wrestler_num == wrestler_num &&
            pal_cache[i].index == index) {
            pal_cache[i].stamp = ++pal_stamp;
            return pal_cache[i].colors;
        }
    }
    for (i = 0; i < WM_STREAM_PAL_SLOTS; ++i) {
        if (!pal_cache[i].valid) { slot = &pal_cache[i]; break; }
        if (!slot || pal_cache[i].stamp < slot->stamp) slot = &pal_cache[i];
    }
    if (!slot) return NULL;

    snprintf(path, sizeof(path), "rom:/wrestlers/%ld/p%u.pal",
             (long)wrestler_num, (unsigned)index);
    fp = fopen(path, "rb");
    if (!fp) return NULL;
    slot->valid = false;
    if (fread(slot->colors, 1, WM_STREAM_TLUT_BYTES, fp)
            != WM_STREAM_TLUT_BYTES) {
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    data_cache_hit_writeback(slot->colors, sizeof(slot->colors));
    slot->wrestler_num = wrestler_num;
    slot->index = index;
    slot->stamp = ++pal_stamp;
    slot->valid = true;
    return slot->colors;
}

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
    size_t pixel_bytes;
    uint8_t header[WM_STREAM_HEADER];
    uint16_t palette_index;
    const uint16_t *colors;

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

    snprintf(path, sizeof(path), "rom:/wrestlers/%ld/%s.bin",
             (long)wrestler_num, source_frame);
    fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    /* `WM` then the big-endian palette index. A file that does not
       start with the magic is not one of ours and is refused rather
       than read as pixels. */
    if (fread(header, 1, WM_STREAM_HEADER, fp) != WM_STREAM_HEADER ||
        header[0] != 'W' || header[1] != 'M') {
        fclose(fp);
        return NULL;
    }
    palette_index = (uint16_t)((header[2] << 8) | header[3]);

    colors = palette_for(wrestler_num, palette_index);
    if (!colors) {
        fclose(fp);
        return NULL;
    }

    slot = pick_slot();
    if (slot->memory) {
        free(slot->memory);
        slot->memory = NULL;
    }
    memset(&slot->sprite, 0, sizeof(slot->sprite));
    slot->valid = false;

    slot->memory = malloc(pixel_bytes);
    if (!slot->memory) {
        fclose(fp);
        return NULL;
    }

    if (fread(slot->memory, 1, pixel_bytes, fp) != pixel_bytes) {
        fclose(fp);
        free(slot->memory);
        slot->memory = NULL;
        return NULL;
    }
    fclose(fp);

    data_cache_hit_writeback(slot->memory, pixel_bytes);

    slot->wrestler_num = wrestler_num;
    snprintf(slot->frame, sizeof(slot->frame), "%s", source_frame);
    slot->bytes = pixel_bytes;
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
    /* Owned by the palette cache above, which outlives this entry. */
    slot->sprite.palette_rgba5551 = (uint16_t *)(void *)(uintptr_t)colors;
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
    memset(pal_cache, 0, sizeof(pal_cache));
    pal_stamp = 0;
}

size_t wm_n64_streamed_character_cached(void) {
    size_t n = 0;
    for (unsigned i = 0; i < WM_STREAM_CACHE_SLOTS; ++i)
        if (cache[i].valid) ++n;
    return n;
}
