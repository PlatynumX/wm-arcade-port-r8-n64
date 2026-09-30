#ifndef WM_N64_RD7TEXT_H
#define WM_N64_RD7TEXT_H

#include <libdragon.h>

#include "wm/arcade/wm_arcade_stringer.h"

/*
 * Draw a text draw list as DMACNZ does: each glyph's ink mask stencilled in
 * one constant colour. `items`/`count` come from wm_text_build_draw_list, in
 * arcade coordinates; the scales are the frontend's 400x256 -> 320x240
 * transform, the same pair every other frontend asset uses.
 */
void wm_rd7text_draw(const wm_text_draw_item *items, int count,
                     color_t colour, float scale_x, float scale_y);

#endif
