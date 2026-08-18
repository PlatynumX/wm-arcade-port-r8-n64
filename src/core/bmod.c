#include "wm/bmod.h"

bool wm_bmod_decode_block(const wm_bmod_module *module, size_t index,
                          wm_bmod_block *out) {
    if (!module || !out || !module->packed_words || index >= module->block_count)
        return false;
    const uint16_t *w = module->packed_words + index * 4u;
    const uint16_t a = w[0];
    const uint16_t h = w[3];
    out->palette = (uint8_t)((a & 0x000fu) | ((h >> 8) & 0x00f0u));
    out->flags = (uint8_t)((a >> 4) & 0x000fu);
    out->z = (uint8_t)(a >> 8);
    out->x = (int16_t)w[1];
    out->y = (int16_t)w[2];
    out->header_index = (uint16_t)(h & 0x0fffu);
    return true;
}

bool wm_bmod_block_intersects(const wm_bmod_block *b,
                              uint16_t block_w, uint16_t block_h,
                              int world_x, int world_y,
                              int view_left, int view_top,
                              int view_right, int view_bottom,
                              int pad_x, int pad_y) {
    if (!b || block_w == 0 || block_h == 0) return false;
    const int l = world_x + b->x;
    const int t = world_y + b->y;
    const int r = l + (int)block_w - 1;
    const int bot = t + (int)block_h - 1;
    return r >= view_left - pad_x && l <= view_right + pad_x &&
           bot >= view_top - pad_y && t <= view_bottom + pad_y;
}
