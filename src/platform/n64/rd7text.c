/*
 * The DMACNZ text blit: RD7FONT glyph masks stencilled in one colour.
 *
 * UTIL.ASM's stringr1_1 reaches the hardware two ways, and this is the
 * simpler one. STRCNRMO_2 sets `DMACNZ|M_NOCOLL`, and SYS.EQU:217 says
 * `DMACNZ .equ 8008h  ;WRITE CONSTANT ON NON-ZERO DATA` -- so the glyph is a
 * stencil and the colour is the constant its caller passed in a6, which
 * show_copyright spells `movi [>1111,0000],a6  ;pal 0, color 17`. No palette
 * allocation, no per-glyph TLUT, no object list.
 *
 * The core decides every position (wm/arcade/wm_arcade_stringer.h) and hands
 * over a list of masks with arcade coordinates. This file does the one thing
 * left: put ink on the screen.
 *
 * NOT VERIFIED ON HARDWARE. It builds and links for mips64 and fits the ROM
 * budget, and the geometry it draws is checked headlessly, but nothing in this
 * container can run an RDP. See R9_GAMEPLAY_WIRING_NOTES.md for what to look
 * at on a console.
 */
#include "rd7text.h"

#include <libdragon.h>

#include "wm/arcade/wm_arcade_stringer.h"

/*
 * A glyph mask becomes an I8 surface: 0 where the glyph does not draw and
 * 255 where it does. Combined as alpha against a flat primitive colour, that
 * is exactly "write constant on non-zero data".
 *
 * The widest RD7FONT glyph is 11 pixels and the tallest 10, so one small
 * scratch buffer serves every glyph and nothing is allocated per frame. The
 * 8-byte alignment and the row pitch rounded up to 8 are what the RDP wants
 * for a texture upload; the surface's stride carries the padding so the extra
 * columns are never sampled.
 */
#define RD7_MAX_W 16
#define RD7_MAX_H 16
#define RD7_PITCH ((RD7_MAX_W + 7) & ~7)

static uint8_t s_mask[RD7_PITCH * RD7_MAX_H] __attribute__((aligned(8)));

void wm_rd7text_draw(const wm_text_draw_item *items, int count,
                     color_t colour, float scale_x, float scale_y) {
    if (!items || count <= 0) return;

    rdpq_set_mode_standard();
    rdpq_mode_filter(FILTER_POINT);
    /*
     * Take the RGB from the primitive colour and the alpha from the texture,
     * then drop anything transparent. That is the stencil: the mask decides
     * WHERE, the constant decides WHAT COLOUR.
     */
    rdpq_mode_combiner(RDPQ_COMBINER1((PRIM, 0, 0, 0), (0, 0, 0, TEX0)));
    rdpq_mode_alphacompare(1);
    rdpq_set_prim_color(colour);

    for (int i = 0; i < count; ++i) {
        const wm_text_draw_item *g = &items[i];
        if (!g->ink || g->width <= 0 || g->height <= 0) continue;
        if (g->width > RD7_MAX_W || g->height > RD7_MAX_H) continue;

        for (int y = 0; y < g->height; ++y) {
            uint8_t *row = &s_mask[y * RD7_PITCH];
            for (int x = 0; x < g->width; ++x) {
                row[x] = g->ink[y * g->width + x] ? 0xFF : 0x00;
            }
            for (int x = g->width; x < RD7_PITCH; ++x) {
                row[x] = 0x00;
            }
        }

        surface_t tex = surface_make_linear(s_mask, FMT_I8, RD7_PITCH, g->height);
        rdpq_tex_blit(&tex,
                      (float)g->x * scale_x, (float)g->y * scale_y,
                      &(rdpq_blitparms_t){
                          .width = g->width,
                          .height = g->height,
                          .scale_x = scale_x,
                          .scale_y = scale_y,
                          .filtering = false,
                      });
    }
}
