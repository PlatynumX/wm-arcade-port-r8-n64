#include <libdragon.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "wm/app.h"
#include "audio_backend.h"
#include "wm/bmod.h"
#include "wm/bret_sprites.h"
#include "wm/composite.h"
#include "wm/demo.h"
#include "wm/dcs_logo.h"
#include "dcs_effect.h"
#include "wm/roster.h"
#include "wm/pregame.h"
#include "wm/select_background.h"
#include "wm/select_sprites.h"
#include "wm/progress_wrestlers.h"
#include "wm/select.h"
#include "wm/sports_logo.h"
#include "wm/sports_background.h"
#include "wm/sports_motto.h"
#include "wm/title_screen.h"
#include "wm/title_sparkle.h"
#include "wm/visual.h"

#define STICK_DEADZONE 12

/* Original arcade frontend coordinates are 400x256 (SCRNEND [256,405]).
   Keep the original object coordinate system and transform only at draw time. */
#define WM_ARCADE_SCREEN_W 400.0f
#define WM_ARCADE_SCREEN_H 256.0f
#define WM_N64_SCREEN_W 320.0f
#define WM_N64_SCREEN_H 240.0f
#define WM_FRONTEND_SCALE_X (WM_N64_SCREEN_W / WM_ARCADE_SCREEN_W)
#define WM_FRONTEND_SCALE_Y (WM_N64_SCREEN_H / WM_ARCADE_SCREEN_H)

/* r6h4 hardware scan: animation channel 2 attachment lives in WIMP tail
   words 3/4 (raw directory offsets +38/+40), positive delta. */
#define WM_ATTACH_X_SLOT 3
#define WM_ATTACH_Y_SLOT 4

static wm_input_state read_input(joypad_port_t port, bool *connected) {
    wm_input_state out = {0};
    const bool present = joypad_is_connected(port);
    if (connected)
        *connected = present;
    if (!present)
        return out;

    const joypad_inputs_t in = joypad_get_inputs(port);
    const joypad_buttons_t now = joypad_get_buttons(port);
    const joypad_buttons_t pressed = joypad_get_buttons_pressed(port);
    out.stick_x = in.stick_x;
    out.stick_y = in.stick_y;
    if (out.stick_x >= -STICK_DEADZONE && out.stick_x <= STICK_DEADZONE &&
        out.stick_y >= -STICK_DEADZONE && out.stick_y <= STICK_DEADZONE) {
        if (now.d_left)  out.stick_x = -90;
        if (now.d_right) out.stick_x =  90;
        if (now.d_down)  out.stick_y = -90;
        if (now.d_up)    out.stick_y =  90;
    }

    /* Same arcade-action mapping on each physical N64 controller. */
    out.run         = now.a;
    out.light_punch = pressed.c_left;
    out.power_punch = pressed.c_up;
    out.light_kick  = pressed.c_right;
    out.power_kick  = pressed.c_down;
    out.block       = now.r;
    out.start       = pressed.start;
    out.l           = pressed.l;
    out.z           = pressed.z;
    out.b           = pressed.b;
    return out;
}

static void fill_rect(int x0, int y0, int x1, int y1, color_t color) {
    rdpq_set_mode_fill(color);
    rdpq_fill_rectangle(x0, y0, x1, y1);
}

static void text_line(int x, int y, const char *text) {
    rdpq_set_mode_standard();
    rdpq_text_print(NULL, 1, x, y, text);
}

static void draw_background(void) {
    fill_rect(0, 0, 320, 240, RGBA32(8, 10, 18, 255));
}

static void draw_ring_back(void) {
    draw_background();
    fill_rect(0, 82, 320, 240, RGBA32(20, 22, 28, 255));

    fill_rect(26, 96, 294, 214, RGBA32(92, 92, 102, 255));
    fill_rect(33, 102, 287, 205, RGBA32(194, 194, 198, 255));
    fill_rect(39, 108, 281, 199, RGBA32(216, 216, 216, 255));

    fill_rect(25, 83, 31, 205, RGBA32(38, 38, 44, 255));
    fill_rect(289, 83, 295, 205, RGBA32(38, 38, 44, 255));
    fill_rect(29, 105, 291, 108, RGBA32(188, 26, 40, 255));
    fill_rect(29, 114, 291, 117, RGBA32(188, 26, 40, 255));
    fill_rect(29, 123, 291, 126, RGBA32(188, 26, 40, 255));
}

static void draw_ring_front(void) {
    fill_rect(29, 183, 291, 186, RGBA32(202, 28, 44, 255));
    fill_rect(29, 192, 291, 195, RGBA32(202, 28, 44, 255));
    fill_rect(29, 201, 291, 204, RGBA32(202, 28, 44, 255));
    fill_rect(25, 180, 31, 209, RGBA32(38, 38, 44, 255));
    fill_rect(289, 180, 295, 209, RGBA32(38, 38, 44, 255));
}

static const wm_source_sprite *bret_object_palette(const wm_source_sprite *fallback) {
    /* The arcade renderer does not leave each image piece on its source palette.
       set_image() writes the wrestler's OBJ_PAL back to OPAL after plotting every
       primary and secondary piece. BRETIMG.ASM exports H4ST4A02 as Bret's base
       image, so use its palette as the portable equivalent of that shared object
       palette. Fall back only if the generated source data is incomplete. */
    const wm_source_sprite *base = wm_bret_sprite_find("H4ST4A02");
    if (!base || !base->palette_rgba5551 || !base->palette_colors)
        return fallback;
    if (fallback && base->palette_colors < fallback->palette_colors)
        return fallback;
    return base;
}

static void draw_source_sprite_scaled(const wm_source_sprite *spr, float anchor_x, float anchor_y,
                                      int display_off_x, int display_off_y,
                                      bool flip_x, const wm_source_sprite *palette_source,
                                      float scale_x, float scale_y) {
    if (!spr || !spr->pixels_ci8)
        return;

    const wm_source_sprite *pal = palette_source ? palette_source : spr;
    if (!pal->palette_rgba5551 || !pal->palette_colors)
        return;

    surface_t tex = surface_make_linear((void *)spr->pixels_ci8, FMT_CI8,
                                        spr->width, spr->height);

    /* Match the arcade OBJ_PAL behavior: every image piece belonging to Bret
       uses one wrestler palette. This prevents palette discontinuities at the
       channel-1/channel-2 waist overlap and removes one-frame TLUT color jumps. */
    rdpq_set_mode_standard();
    rdpq_mode_tlut(TLUT_RGBA16);
    rdpq_mode_filter(FILTER_POINT);
    rdpq_mode_alphacompare(1);
    rdpq_tex_upload_tlut(pal->palette_rgba5551, 0, pal->palette_colors);

    /* CI8 + TLUT leaves 2 KiB of texture TMEM. Explicitly split large source
       sprites into strips; this is the hardware-tested path that eliminated
       the r6 RSP/display_get backlog. */
    int pitch = (spr->width + 7) & ~7;
    int strip_h = pitch > 0 ? 2048 / pitch : 0;
    if (strip_h < 1) strip_h = 1;
    if (strip_h > 2) strip_h &= ~1;

    for (int t = 0; t < spr->height; t += strip_h) {
        int h = spr->height - t;
        if (h > strip_h) h = strip_h;
        rdpq_tex_blit(&tex, (float)anchor_x, (float)anchor_y,
                      &(rdpq_blitparms_t){
                          .t0 = t,
                          .height = h,
                          /* Use the arcade object's effective display offsets,
                             not each piece's private hotspot.  For channel 2
                             these already include primary IANIOFF - IANI2 +
                             secondary IANIOFF, exactly like set_image(). */
                          .cx = display_off_x,
                          .cy = display_off_y - t,
                          .flip_x = flip_x,
                          .scale_x = scale_x,
                          .scale_y = scale_y,
                          .filtering = false,
                      });
    }
}

static void draw_source_sprite(const wm_source_sprite *spr, int anchor_x, int anchor_y,
                               int display_off_x, int display_off_y,
                               bool flip_x, const wm_source_sprite *palette_source) {
    draw_source_sprite_scaled(spr, (float)anchor_x, (float)anchor_y,
                              display_off_x, display_off_y, flip_x, palette_source,
                              1.0f, 1.0f);
}

/* Draw one original BLIMP/BMOD background block. Unlike WIMP character
   sprites, background CI8 pixels choose their palette from the packed BMOD
   record. BAKGND.ASM MAP_FLAGS bit 2 controls whether index zero writes. */
static void draw_title_background_block(const wm_title_background_image *img,
                                        const wm_title_background_palette *pal,
                                        const wm_bmod_block *block) {
    if (!img || !pal || !block || !img->pixels_ci8 || !pal->color_count)
        return;

    const bool transparent = (block->flags & WM_BMOD_TRANSPARENT) != 0;
    uint16_t *tlut = transparent ? pal->rgba5551_keyed : pal->rgba5551_opaque;
    if (!tlut) return;

    surface_t tex = surface_make_linear((void *)img->pixels_ci8, FMT_CI8,
                                        img->width, img->height);
    rdpq_set_mode_standard();
    rdpq_mode_tlut(TLUT_RGBA16);
    rdpq_mode_filter(FILTER_POINT);
    rdpq_mode_alphacompare(1);
    rdpq_tex_upload_tlut(tlut, 0, pal->color_count);

    /* Keep the already hardware-tested CI8/TLUT strip size. For VFLIP, put
       each flipped strip at its mirrored destination so the full block (not
       each strip independently) is vertically flipped. */
    int pitch = (img->width + 7) & ~7;
    int strip_h = pitch > 0 ? 2048 / pitch : 0;
    if (strip_h < 1) strip_h = 1;
    if (strip_h > 2) strip_h &= ~1;

    const bool flip_x = (block->flags & WM_BMOD_HFLIP) != 0;
    const bool flip_y = (block->flags & WM_BMOD_VFLIP) != 0;
    const float x = (float)block->x * WM_FRONTEND_SCALE_X;
    for (int t = 0; t < img->height; t += strip_h) {
        int h = img->height - t;
        if (h > strip_h) h = strip_h;
        const int dest_row = flip_y ? (int)img->height - t - h : t;
        const float y = (float)(block->y + dest_row) * WM_FRONTEND_SCALE_Y;
        rdpq_tex_blit(&tex, x, y,
                      &(rdpq_blitparms_t){
                          .t0 = t,
                          .height = h,
                          .flip_x = flip_x,
                          .flip_y = flip_y,
                          .scale_x = WM_FRONTEND_SCALE_X,
                          .scale_y = WM_FRONTEND_SCALE_Y,
                          .filtering = false,
                      });
    }
}

static void draw_shadow(int x, int y) {
    fill_rect(x - 13, y - 3, x + 14, y + 2, RGBA32(86, 86, 90, 255));
}

static const wm_source_sprite *fighter_sprite(const wm_demo_fighter *f) {
    const wm_visual_frame *frame = wm_visual_current(&f->visual);
    return frame ? wm_bret_sprite_find(frame->source_frame) : NULL;
}

static bool fighter_uses_torso_layer(const wm_demo_fighter *f) {
    return f->action == WM_DEMO_IDLE || f->action == WM_DEMO_WALK ||
           f->action == WM_DEMO_BLOCK;
}

static void draw_fighter(const wm_demo_fighter *f) {
    const wm_source_sprite *spr = fighter_sprite(f);
    draw_shadow(f->screen_x, f->screen_y);
    if (!spr) {
        fill_rect(f->screen_x - 10, f->screen_y - 48,
                  f->screen_x + 10, f->screen_y, RGBA32(230, 30, 150, 255));
        return;
    }

    const wm_source_sprite *object_pal = bret_object_palette(spr);
    draw_source_sprite(spr, f->screen_x, f->screen_y,
                       spr->xani, spr->yani, f->flip_x, object_pal);

    if (fighter_uses_torso_layer(f)) {
        const wm_visual_frame *torso_frame = wm_visual_current(&f->torso_visual);
        const wm_source_sprite *torso = torso_frame
            ? wm_bret_sprite_find(torso_frame->source_frame) : NULL;
        if (torso) {
            int a2x = spr->wimp_tail[WM_ATTACH_X_SLOT];
            int a2y = spr->wimp_tail[WM_ATTACH_Y_SLOT];
            if (!(a2x == -1 && a2y == -1) &&
                a2x >= -512 && a2x <= 512 && a2y >= -512 && a2y <= 512) {
                int secondary_xoff, secondary_yoff;
                wm_secondary_display_offsets(spr->xani, spr->yani, a2x, a2y,
                                             torso->xani, torso->yani,
                                             &secondary_xoff, &secondary_yoff);
                /* Do not negate a hand-built delta when flipped.  The arcade
                   leaves ODXOFF alone and flips the object around that offset;
                   rdpq_tex_blit does the same when flip_x is applied before
                   transformations.  This removes the small waist shear seen
                   in the r8 hardware video. */
                draw_source_sprite(torso, f->screen_x, f->screen_y,
                                   secondary_xoff, secondary_yoff,
                                   f->flip_x, object_pal);
            }
        }
    }
}

static void draw_health_bar(int x, int y, int width, int health, bool right_align) {
    int inner = (width - 4) * health / 100;
    fill_rect(x, y, x + width, y + 8, RGBA32(24, 24, 30, 255));
    fill_rect(x + 2, y + 2, x + width - 2, y + 6, RGBA32(70, 18, 22, 255));
    if (inner <= 0) return;
    if (right_align)
        fill_rect(x + width - 2 - inner, y + 2, x + width - 2, y + 6,
                  RGBA32(228, 210, 72, 255));
    else
        fill_rect(x + 2, y + 2, x + 2 + inner, y + 6,
                  RGBA32(228, 210, 72, 255));
}

static void draw_match_hud(const wm_app *app) {
    const wm_demo *demo = &app->demo;
    const wm_wrestler_def *p1 = wm_roster_get(app->p1_choice);
    const wm_wrestler_def *p2 = wm_roster_get(app->p2_choice);
    char line[128];

    draw_health_bar(8, 68, 132, demo->p1.health, false);
    draw_health_bar(180, 68, 132, demo->p2.health, true);
    text_line(8, 66, p1 ? p1->name : "P1");
    text_line(230, 66, p2 ? p2->name : "CPU");

    snprintf(line, sizeof(line), "%s/%s hp:%d   %s/%s hp:%d",
             wm_demo_action_name(demo->p1.action), wm_demo_facing_name(demo->p1.facing),
             demo->p1.health,
             wm_demo_action_name(demo->p2.action), wm_demo_facing_name(demo->p2.facing),
             demo->p2.health);
    text_line(8, 14, line);

    snprintf(line, sizeof(line), "hits:%u blocks:%u  P1:%u/%u CPU:%u/%u AI:%s",
             demo->total_hits, demo->total_blocks,
             demo->p1.hit_count, demo->p1.action_count,
             demo->p2.hit_count, demo->p2.action_count,
             demo->ai_enabled ? "ON" : "OFF");
    text_line(8, 29, line);

    snprintf(line, sizeof(line), "p1:%d,%d stun:%u  p2:%d,%d stun:%u",
             demo->p1.screen_x, demo->p1.screen_y, demo->p1.stun_ticks,
             demo->p2.screen_x, demo->p2.screen_y, demo->p2.stun_ticks);
    text_line(8, 44, line);

    if ((p1 && !p1->visual_backend_ready) || (p2 && !p2->visual_backend_ready))
        text_line(8, 214, "SOURCE SLOT SELECTED - BRET ART BACKEND USED AS PLACEHOLDER");
}

static void render_dcs_logo(const wm_app *app) {
    fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));
    const wm_source_sprite *spr = wm_dcs_logo_sprite();
    if (!spr) return;

    const wm_dcs_phase phase = app->attract.dcs_phase;
    switch (phase) {
        case WM_DCS_ROT_UNSKIPPABLE:
        case WM_DCS_ROT_SKIPPABLE:
        case WM_DCS_BURST_FLASH:
        case WM_DCS_BURST_WAIT:
        case WM_DCS_BURST_SKIPPABLE:
            wm_n64_render_dcs_effect(app, spr);
            return;
        default:
            break;
    }

    if (phase != WM_DCS_STATIC && phase != WM_DCS_ROT_SETUP &&
        phase != WM_DCS_STATIC_RETURN)
        return;

    draw_source_sprite_scaled(spr,
                              200.0f * WM_FRONTEND_SCALE_X,
                              120.0f * WM_FRONTEND_SCALE_Y,
                              spr->xani, spr->yani, false, spr,
                              WM_FRONTEND_SCALE_X, WM_FRONTEND_SCALE_Y);
}

/* SPORTBKBMOD source-faithful cached renderer.
   The previous N64 path decoded/sorted/drew all 71 BMOD records every video
   frame and repeatedly switched their TLUTs. That changed no source state, but
   made the visible -2/+2 movement much more expensive than the arcade process.

   Compose the exact 1100x400 source module once from the same records, pixels,
   flags, Y/Z order and BKPALS colors. Per frame, reproduce BAKGND by copying
   only the visible 400x256 source window for the six original logo_mod starts.
   Source coordinates and movement timing are untouched. */
#define WM_SPORTS_MODULE_W 1100
#define WM_SPORTS_MODULE_H 400
#define WM_SPORTS_VIEW_W 400
#define WM_SPORTS_VIEW_H 256
#define WM_SPORTS_MAX_MERGED_COLORS 256

static uint8_t sports_module_ci8[WM_SPORTS_MODULE_W * WM_SPORTS_MODULE_H]
    __attribute__((aligned(8)));
static uint8_t sports_view_ci8[WM_SPORTS_VIEW_W * WM_SPORTS_VIEW_H]
    __attribute__((aligned(8)));
static uint16_t sports_merged_tlut[WM_SPORTS_MAX_MERGED_COLORS]
    __attribute__((aligned(8)));
static unsigned sports_merged_colors;
static bool sports_background_cache_ready;

static uint8_t sports_merge_color(uint16_t rgba5551) {
    for (unsigned i = 1; i < sports_merged_colors; ++i) {
        if (sports_merged_tlut[i] == rgba5551)
            return (uint8_t)i;
    }
    if (sports_merged_colors >= WM_SPORTS_MAX_MERGED_COLORS)
        return 0;
    sports_merged_tlut[sports_merged_colors] = rgba5551;
    return (uint8_t)sports_merged_colors++;
}

static void sports_compose_block(const wm_bmod_block *b) {
    const wm_sports_background_image *img =
        wm_sports_background_image_at(b->header_index);
    const wm_sports_background_palette *pal =
        wm_sports_background_palette_at(b->palette);
    if (!img || !pal || !img->pixels_ci8 || !pal->rgba5551_opaque)
        return;

    const bool transparent = (b->flags & WM_BMOD_TRANSPARENT) != 0;
    const bool flip_x = (b->flags & WM_BMOD_HFLIP) != 0;
    const bool flip_y = (b->flags & WM_BMOD_VFLIP) != 0;

    for (int dy = 0; dy < (int)img->height; ++dy) {
        const int out_y = (int)b->y + dy;
        if ((unsigned)out_y >= WM_SPORTS_MODULE_H)
            continue;
        const int sy = flip_y ? (int)img->height - 1 - dy : dy;

        for (int dx = 0; dx < (int)img->width; ++dx) {
            const int out_x = (int)b->x + dx;
            if ((unsigned)out_x >= WM_SPORTS_MODULE_W)
                continue;

            const int sx = flip_x ? (int)img->width - 1 - dx : dx;
            const uint8_t src =
                img->pixels_ci8[(size_t)sy * img->width + (size_t)sx];

            if (transparent && src == 0)
                continue;
            if (src >= pal->color_count)
                continue;

            const uint8_t merged =
                sports_merge_color(pal->rgba5551_opaque[src]);
            if (merged == 0)
                continue;

            sports_module_ci8[(size_t)out_y * WM_SPORTS_MODULE_W +
                              (size_t)out_x] = merged;
        }
    }
}

static void sports_background_cache_init(void) {
    if (sports_background_cache_ready)
        return;

    const wm_named_bmod *named = wm_source_bmod_find("SPORTBKBMOD");
    if (!named || named->module.block_count != 71)
        return;

    memset(sports_module_ci8, 0, sizeof(sports_module_ci8));
    memset(sports_merged_tlut, 0, sizeof(sports_merged_tlut));
    sports_merged_tlut[0] = 0;
    sports_merged_colors = 1;

    struct sports_cache_ref { wm_bmod_block block; };
    struct sports_cache_ref refs[71];
    size_t count = 0;

    for (size_t i = 0; i < named->module.block_count; ++i) {
        if (!wm_bmod_decode_block(&named->module, i, &refs[count].block))
            continue;
        ++count;
    }

    for (size_t i = 1; i < count; ++i) {
        struct sports_cache_ref key = refs[i];
        size_t j = i;
        while (j > 0 && wm_bmod_draw_before(&key.block, &refs[j - 1].block)) {
            refs[j] = refs[j - 1];
            --j;
        }
        refs[j] = key;
    }

    for (size_t i = 0; i < count; ++i)
        sports_compose_block(&refs[i].block);

    sports_background_cache_ready = true;
    debugf("sports: cached exact SPORTBKBMOD 1100x400 (%lu blocks, %u colors)\n",
           (unsigned long)count, sports_merged_colors);
}

static void sports_copy_visible_module(int module_x, int module_y) {
    const int left = module_x < 0 ? 0 : module_x;
    const int top = module_y < 0 ? 0 : module_y;
    const int right =
        module_x + WM_SPORTS_MODULE_W > WM_SPORTS_VIEW_W
            ? WM_SPORTS_VIEW_W : module_x + WM_SPORTS_MODULE_W;
    const int bottom =
        module_y + WM_SPORTS_MODULE_H > WM_SPORTS_VIEW_H
            ? WM_SPORTS_VIEW_H : module_y + WM_SPORTS_MODULE_H;

    if (left >= right || top >= bottom)
        return;

    const int src_x = left - module_x;
    const int src_y = top - module_y;
    const size_t width = (size_t)(right - left);

    for (int y = top; y < bottom; ++y) {
        const uint8_t *src =
            sports_module_ci8 +
            (size_t)(src_y + y - top) * WM_SPORTS_MODULE_W +
            (size_t)src_x;
        uint8_t *dst =
            sports_view_ci8 +
            (size_t)y * WM_SPORTS_VIEW_W +
            (size_t)left;

        for (size_t x = 0; x < width; ++x) {
            if (src[x] != 0)
                dst[x] = src[x];
        }
    }
}

static void render_sports_background(const wm_app *app) {
    sports_background_cache_init();
    if (!sports_background_cache_ready)
        return;

    /* ATTR.ASM::logo_mod universe starts, verbatim. */
    static const int16_t module_starts[6][2] = {
        {-400,    0},
        {-800,  400},
        {-1200, 800},
        {-1600,1200},
        {-2000,1600},
        {-2400,2000},
    };

    memset(sports_view_ci8, 0, sizeof(sports_view_ci8));

    for (size_t i = 0; i < 6; ++i) {
        /* BAKGND universe -> display conversion subtracts WORLDTL. */
        const int x = module_starts[i][0] - app->attract.sports_world_x;
        const int y = module_starts[i][1] - app->attract.sports_world_y;
        sports_copy_visible_module(x, y);
    }

    surface_t tex =
        surface_make_linear((void *)sports_view_ci8, FMT_CI8,
                            WM_SPORTS_VIEW_W, WM_SPORTS_VIEW_H);

    rdpq_set_mode_standard();
    rdpq_mode_tlut(TLUT_RGBA16);
    rdpq_mode_filter(FILTER_POINT);
    rdpq_mode_alphacompare(1);
    rdpq_tex_upload_tlut(sports_merged_tlut, 0, sports_merged_colors);

    /* 400-byte CI8 rows: four source rows per TMEM-safe strip. */
    for (int t = 0; t < WM_SPORTS_VIEW_H; t += 4) {
        int h = WM_SPORTS_VIEW_H - t;
        if (h > 4) h = 4;
        rdpq_tex_blit(&tex, 0.0f, (float)t * WM_FRONTEND_SCALE_Y,
                      &(rdpq_blitparms_t){
                          .t0 = t,
                          .height = h,
                          .scale_x = WM_FRONTEND_SCALE_X,
                          .scale_y = WM_FRONTEND_SCALE_Y,
                          .filtering = false,
                      });
    }
}

static void render_sports_motto(void) {
    /* ATTR.ASM::rule_str:
       JAM_STR osgmd8_ascii,6,0,200,225,SGMD8WHT,print_string_C2 */
    const char *text = wm_sports_motto_text();
    if (!text) return;

    int width = 0;
    for (const char *p = text; *p; ++p) {
        if (*p == ' ') {
            width += 6;
            continue;
        }
        const wm_source_sprite *g = wm_sports_motto_glyph(*p);
        if (g) width += g->width;
    }

    float pen_x = (200.0f - (float)width * 0.5f) * WM_FRONTEND_SCALE_X;
    const float y = 225.0f * WM_FRONTEND_SCALE_Y;

    for (const char *p = text; *p; ++p) {
        if (*p == ' ') {
            pen_x += 6.0f * WM_FRONTEND_SCALE_X;
            continue;
        }
        const wm_source_sprite *g = wm_sports_motto_glyph(*p);
        if (!g) continue;
        draw_source_sprite_scaled(g, pen_x, y, g->xani, g->yani, false, g,
                                  WM_FRONTEND_SCALE_X, WM_FRONTEND_SCALE_Y);
        /* JAM_STR cspace=0; glyph image width supplies the character advance. */
        pen_x += (float)g->width * WM_FRONTEND_SCALE_X;
    }
}

static void render_midway_sports(const wm_app *app) {
    fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));
    /* show_sports_logo stays blank through SLEEPK 2 + object creation + SLEEPK 1. */
    if (app->attract.call_ticks < WM_SPORTS_LOGO_VISIBLE_TICK)
        return;

    render_sports_background(app);

    /* ATTR.ASM LOGO_LIST: SPRTLG01..SPRTLG17 share one object anchor. */
    const float anchor_x = 200.0f * WM_FRONTEND_SCALE_X;
    const float anchor_y = 118.0f * WM_FRONTEND_SCALE_Y;
    const size_t count = wm_sports_logo_sprite_count();
    for (size_t i = 0; i < count; ++i) {
        const wm_source_sprite *spr = wm_sports_logo_sprite_at(i);
        if (!spr) continue;
        draw_source_sprite_scaled(spr, anchor_x, anchor_y,
                                  spr->xani, spr->yani, false, spr,
                                  WM_FRONTEND_SCALE_X, WM_FRONTEND_SCALE_Y);
    }

    render_sports_motto();
}


static const wm_source_sprite *title_sparkle_frame(const wm_title_sparkle *sp) {
    if (!sp || !sp->active) return NULL;

    size_t base = 0;
    unsigned frames = 0;
    switch (sp->family) {
        /* Rev 1.30 ROM animation-pointer order, selected by RNDRNG0(4). */
        case 0: base = WM_TITLE_SPARKLE_SMALL_A_BASE; frames = WM_TITLE_SMALL_SPARKLE_FRAMES; break;
        case 1: base = WM_TITLE_SPARKLE_SMALL_B_BASE; frames = WM_TITLE_SMALL_SPARKLE_FRAMES; break;
        case 2: base = WM_TITLE_SPARKLE_SMALL_C_BASE; frames = WM_TITLE_SMALL_SPARKLE_FRAMES; break;
        case 3: base = WM_TITLE_SPARKLE_BIG_A_BASE; frames = WM_TITLE_BIG_SPARKLE_FRAMES; break;
        case 4: base = WM_TITLE_SPARKLE_BIG_B_BASE; frames = WM_TITLE_BIG_SPARKLE_FRAMES; break;
        default: return NULL;
    }
    if ((unsigned)sp->frame >= frames) return NULL;
    return wm_title_sparkle_sprite_at(base + sp->frame);
}

static void draw_title_sparkle(const wm_title_sparkle *sp) {
    const wm_source_sprite *spr = title_sparkle_frame(sp);
    if (!spr) return;
    draw_source_sprite_scaled(spr,
                              (float)sp->x * WM_FRONTEND_SCALE_X,
                              (float)sp->y * WM_FRONTEND_SCALE_Y,
                              spr->xani, spr->yani, false, spr,
                              WM_FRONTEND_SCALE_X, WM_FRONTEND_SCALE_Y);
}

static void render_title_screen(const wm_app *app) {
    fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));
    if (app->attract.call_ticks < WM_TITLE_SETUP_TICKS)
        return;

    const wm_named_bmod *named = wm_source_bmod_find("NTITLESCBMOD");
    if (!named || named->module.block_count > 64)
        return;

    /* BAKGND.ASM creates these as background objects through INSBOBJ and the
       display dispatcher runs YZSORT.  Reconstruct that Z/Y ordering here;
       source-order-only equal-Z ties incorrectly cover half of MIDWAY.  The
       packed BMOD records themselves remain verbatim. */
    struct title_draw_ref {
        wm_bmod_block block;
    } refs[64];
    size_t count = 0;
    for (size_t i = 0; i < named->module.block_count; ++i) {
        if (!wm_bmod_decode_block(&named->module, i, &refs[count].block))
            continue;
        ++count;
    }
    for (size_t i = 1; i < count; ++i) {
        struct title_draw_ref key = refs[i];
        size_t j = i;
        while (j > 0 && wm_bmod_draw_before(&key.block, &refs[j - 1].block)) {
            refs[j] = refs[j - 1];
            --j;
        }
        refs[j] = key;
    }

    for (size_t i = 0; i < count; ++i) {
        const wm_bmod_block *block = &refs[i].block;
        const wm_title_background_image *img =
            wm_title_background_image_at(block->header_index);
        const wm_title_background_palette *pal =
            wm_title_background_palette_at(block->palette);
        draw_title_background_block(img, pal, block);
    }

    /* SPARKLE.IMG is the original WIMP artwork. The core models the
       recovered FLASH_PID child overlap directly: sequential 40-site sweep first,
       then RANDOM_SPARKLE reuses those same sites. */
    for (size_t i = 0; i < WM_TITLE_SPARKLE_SLOT_COUNT; ++i)
        draw_title_sparkle(&app->attract.title_glints[i]);

    /* The module is 403x256 against the arcade's 400x256 viewport, so the
       normal arcade->N64 transform naturally clips the final three source
       columns. */
}









/* BEGIN SOURCE SELECT RENDERER */
static void draw_select_background_block(const wm_select_background_image *img,
                                         const wm_select_background_palette *pal,
                                         const wm_bmod_block *block,
                                         int module_x, int module_y) {
    if (!img || !pal || !block || !img->pixels_ci8 || !pal->color_count)
        return;

    const int bx = module_x + (int)block->x;
    const int by = module_y + (int)block->y;
    if (bx >= 400 || by >= 256 ||
        bx + (int)img->width <= 0 || by + (int)img->height <= 0)
        return;

    const bool transparent = (block->flags & WM_BMOD_TRANSPARENT) != 0;
    uint16_t *tlut = transparent ? pal->rgba5551_keyed : pal->rgba5551_opaque;
    if (!tlut) return;

    surface_t tex = surface_make_linear((void *)img->pixels_ci8, FMT_CI8,
                                        img->width, img->height);
    rdpq_set_mode_standard();
    rdpq_mode_tlut(TLUT_RGBA16);
    rdpq_mode_filter(FILTER_POINT);
    rdpq_mode_alphacompare(1);
    rdpq_tex_upload_tlut(tlut, 0, pal->color_count);

    int pitch = (img->width + 7) & ~7;
    int strip_h = pitch > 0 ? 2048 / pitch : 0;
    if (strip_h < 1) strip_h = 1;
    if (strip_h > 2) strip_h &= ~1;

    const bool flip_x = (block->flags & WM_BMOD_HFLIP) != 0;
    const bool flip_y = (block->flags & WM_BMOD_VFLIP) != 0;
    const float x = (float)bx * WM_FRONTEND_SCALE_X;

    for (int t = 0; t < img->height; t += strip_h) {
        int h = img->height - t;
        if (h > strip_h) h = strip_h;
        const int dest_row = flip_y ? (int)img->height - t - h : t;
        const float y = (float)(by + dest_row) * WM_FRONTEND_SCALE_Y;
        rdpq_tex_blit(&tex, x, y,
                      &(rdpq_blitparms_t){
                          .t0 = t,
                          .height = h,
                          .flip_x = flip_x,
                          .flip_y = flip_y,
                          .scale_x = WM_FRONTEND_SCALE_X,
                          .scale_y = WM_FRONTEND_SCALE_Y,
                          .filtering = false,
                      });
    }
}

static void render_select_module(const char *name, bool choice,
                                 int module_x, int module_y) {
    const wm_named_bmod *named = wm_source_bmod_find(name);
    if (!named || named->module.block_count > 64)
        return;

    struct select_draw_ref { wm_bmod_block block; } refs[64];
    size_t count = 0;
    for (size_t i = 0; i < named->module.block_count; ++i) {
        if (wm_bmod_decode_block(&named->module, i, &refs[count].block))
            ++count;
    }
    for (size_t i = 1; i < count; ++i) {
        struct select_draw_ref key = refs[i];
        size_t j = i;
        while (j > 0 && wm_bmod_draw_before(&key.block, &refs[j - 1].block)) {
            refs[j] = refs[j - 1];
            --j;
        }
        refs[j] = key;
    }

    for (size_t i = 0; i < count; ++i) {
        const wm_bmod_block *b = &refs[i].block;
        const wm_select_background_image *img =
            choice ? wm_select_choice_image_at(b->header_index)
                   : wm_select_main_image_at(b->header_index);
        const wm_select_background_palette *pal =
            choice ? wm_select_choice_palette_at(b->palette)
                   : wm_select_main_palette_at(b->palette);
        draw_select_background_block(img, pal, b, module_x, module_y);
    }
}

static void draw_select_sprite_named(const char *name, int x, int y, bool flip_x) {
    const wm_source_sprite *spr = wm_select_sprite_find(name);
    if (!spr) return;
    draw_source_sprite_scaled(spr,
                              (float)x * WM_FRONTEND_SCALE_X,
                              (float)y * WM_FRONTEND_SCALE_Y,
                              spr->xani, spr->yani, flip_x, spr,
                              WM_FRONTEND_SCALE_X, WM_FRONTEND_SCALE_Y);
}


/* Wolf-unit BEGINOBJP/OPAL equivalent for source WIMP pixels.  Keep the
   original image CI8 indices, but feed the palette selected by PROGRESS.ASM. */
static void draw_select_sprite_named_palette(const char *name, int x, int y,
                                             bool flip_x, const char *palette_name) {
    const wm_source_sprite *spr = wm_select_sprite_find(name);
    const wm_select_palette *pal = wm_select_palette_find(palette_name);
    if (!spr || !pal || !pal->rgba5551 || !pal->color_count) return;

    wm_source_sprite palette_proxy = *spr;
    palette_proxy.palette_rgba5551 = pal->rgba5551;
    palette_proxy.palette_colors = pal->color_count;
    draw_source_sprite_scaled(spr,
                              (float)x * WM_FRONTEND_SCALE_X,
                              (float)y * WM_FRONTEND_SCALE_Y,
                              spr->xani, spr->yani, flip_x, &palette_proxy,
                              WM_FRONTEND_SCALE_X, WM_FRONTEND_SCALE_Y);
}

/* belt_prompt_setup is JAM_STR(osgemd_ascii,8,0,200,18+256,BLUE,0).
   The exact OSGEMD WIMP glyphs and source coordinates are ported here.
   BLUE is a live text-engine color selector rather than an IMGPAL symbol;
   until that text-color path is decoded, retain each glyph's source WIMP
   palette rather than inventing a replacement turquoise palette. */
static const wm_source_sprite *belt_osgemd_glyph(char c) {
    char name[16];
    if (c >= 'A' && c <= 'Z') {
        snprintf(name, sizeof(name), "OSGEMD_%c", c);
        return wm_select_sprite_find(name);
    }
    if (c == ':') return wm_select_sprite_find("OSGEMD_COL");
    if (c == ' ') return NULL;
    return NULL;
}

static int belt_osgemd_width(const char *text) {
    int width = 0;
    if (!text) return 0;
    for (; *text; ++text) {
        const wm_source_sprite *spr = belt_osgemd_glyph(*text);
        if (spr) width += (int)spr->width;
        else if (*text == ' ') width += 6;
    }
    return width;
}

static void draw_belt_osgemd_centered(const char *text, int center_x, int y) {
    if (!text) return;

    /*
     * PROGRESS.ASM::belt_prompt_setup:
     *   JAM_STR osgemd_ascii,8,0,200,18+256,BLUE,0
     *
     * OSGEMD.IMG contains the exact source `blue` palette.  The extracted
     * OSGEMD_C record points at that palette, so use it as the text-engine
     * palette override for every glyph instead of leaving each letter on its
     * unrelated embedded authoring palette.
     */
    const wm_source_sprite *blue_palette_source =
        wm_select_sprite_find("OSGEMD_C");

    int x = center_x - belt_osgemd_width(text) / 2;
    for (; *text; ++text) {
        const wm_source_sprite *spr = belt_osgemd_glyph(*text);
        if (spr) {
            wm_source_sprite palette_proxy = *spr;
            if (blue_palette_source &&
                blue_palette_source->palette_rgba5551 &&
                blue_palette_source->palette_colors) {
                palette_proxy.palette_rgba5551 =
                    blue_palette_source->palette_rgba5551;
                palette_proxy.palette_colors =
                    blue_palette_source->palette_colors;
            }
            draw_source_sprite_scaled(spr,
                                      (float)x * WM_FRONTEND_SCALE_X,
                                      (float)y * WM_FRONTEND_SCALE_Y,
                                      spr->xani, spr->yani, false,
                                      &palette_proxy,
                                      WM_FRONTEND_SCALE_X,
                                      WM_FRONTEND_SCALE_Y);
            x += (int)spr->width;
        } else if (*text == ' ') {
            x += 6;
        }
    }
}
static int select_font9_advance(char c) {
    if (c == ' ') return 5;
    char name[8];
    if (c >= 'A' && c <= 'Z') snprintf(name, sizeof(name), "FNT9_%c", c);
    else if (c >= '0' && c <= '9') snprintf(name, sizeof(name), "FNT9_%c", c);
    else return 0;
    const wm_source_sprite *spr = wm_select_sprite_find(name);
    return spr ? (int)spr->width + 1 : 0;
}

static int select_font9_width(const char *text) {
    int w = 0;
    if (!text) return 0;
    for (; *text; ++text) w += (*text == '!') ? 4 : select_font9_advance(*text);
    return w;
}

static void draw_select_font9(const char *text, int x, int y, bool centered, bool yellow) {
    if (!text) return;
    if (centered) x -= select_font9_width(text) / 2;
    for (; *text; ++text) {
        char c = *text;
        if (c == ' ') { x += 5; continue; }
        if (c == '!') { x += 4; continue; }
        char name[9];
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            snprintf(name, sizeof(name), yellow ? "YFNT9_%c" : "FNT9_%c", c);
            const wm_source_sprite *spr = wm_select_sprite_find(name);
            if (spr) {
                draw_select_sprite_named(name, x, y, false);
                x += (int)spr->width + 1;
            }
        }
    }
}

static void render_select_buyin_overlay(const wm_app *app) {
    if (!app)
        return;

    const wm_select_continue_state *cont = &app->continue_select;
    if (wm_select_continue_visible(cont)) {
        const int center = cont->player
            ? WM_SELECT_CONTINUE_P2_CENTER_X
            : WM_SELECT_CONTINUE_P1_CENTER_X;

        /*
         * SELECT.ASM::buyin_text (.if 1):
         * PLEASE      y=80, white
         * CONTINUE!   y=95, white
         * x CREDIT(S) y=110, yellow
         * TO CONTINUE y=125, yellow
         * FREE PLAY   y=120, yellow in free-play mode.
         */
        if (cont->free_play) {
            draw_select_font9("FREE PLAY", center,
                              WM_SELECT_CONTINUE_FREEPLAY_YPOS,
                              true, true);
        }

        draw_select_font9("PLEASE", center,
                          WM_SELECT_CONTINUE_PLEASE_YPOS,
                          true, false);
        draw_select_font9("CONTINUE", center,
                          WM_SELECT_CONTINUE_TEXT_YPOS,
                          true, false);

        if (!cont->free_play) {
            const unsigned shown =
                cont->credits_needed < 10u ? cont->credits_needed : 9u;
            char digit_name[8];
            snprintf(digit_name, sizeof(digit_name), "YFNT9_%u", shown);

            /* buyin_text source digit X: P1 50 (54 for one), P2 0x122
               (0x126 for one). */
            int digit_x;
            if (cont->player)
                digit_x = cont->credits_needed == 1u ? 0x126 : 0x122;
            else
                digit_x = cont->credits_needed == 1u ? 54 : 50;

            draw_select_sprite_named(
                digit_name, digit_x, WM_SELECT_CONTINUE_CREDITS_YPOS, false);
            draw_select_font9(
                cont->credits_needed == 1u ? "  CREDIT" : "  CREDITS",
                center, WM_SELECT_CONTINUE_CREDITS_YPOS, true, true);
            draw_select_font9("TO CONTINUE", center,
                              WM_SELECT_CONTINUE_TO_YPOS, true, true);
        }

        /* buyin_counter source object: x=80/320, y=TIMER_YPOS=208. */
        char timer_name[9];
        snprintf(timer_name, sizeof(timer_name), "FNT9_%u", cont->digit);
        draw_select_sprite_named(
            timer_name,
            cont->player ? 320 : 80,
            WM_SELECT_CONTINUE_TIMER_YPOS,
            false);

        /*
         * CR_CONTP selects original WF_INSERT/WF_START artwork. can_continue
         * is an explicit cabinet-credit bridge; no N64 credit rule is invented.
         */
        if (cont->prompt_visible) {
            draw_select_sprite_named(
                cont->can_continue ? "WF_START" : "WF_INSERT",
                cont->player ? 0x142 : 0x51,
                184,
                false);
        }

        draw_select_font9("CREDIT 01", 200, 12, true, true);
        return;
    }

    if (!app->select.p2_joined) {
        /* SELECT.ASM #norm inactive-P2 branch retained from Fix36. */
        draw_select_font9("CHALLENGER", 321, 60, true, true);
        draw_select_font9("NEEDED!",    321, 75, true, true);
        draw_select_font9("2 CREDITS",  321, 110, true, true);
        draw_select_font9("TO START",   321, 125, true, true);
        if (app->select.buyin_name_visible)
            draw_select_sprite_named("WF_INSERT", 0x142, 184, false);
    }

    draw_select_font9("CREDIT 01", 200, 12, true, true);
}

static void render_select_random_message(const wm_app *app) {
    if (!app)
        return;

    /* SELECT.ASM message_setup: P1 x=79, P2 x=321, y=15. */
    if (app->select.p1.random_dest >= 0)
        draw_select_font9("CALLA RNDPER", 79, 15, true, false);
    if (app->select.p2_joined && app->select.p2.random_dest >= 0)
        draw_select_font9("CALLA RNDPER", 321, 15, true, false);
}

static const char *const select_croutons[8] = {
    "CRUT_DK","CRUT_RR","CRUT_UN","CRUT_YK",
    "CRUT_SM","CRUT_BM","CRUT_BH","CRUT_LX"
};

static const int16_t select_crouton_pos[8][2] = {
    {164,45},{204,45},{164,90},{204,90},
    {164,135},{204,135},{164,180},{204,180}
};

static const char *select_name_image(uint8_t source_id) {
    static const char *const names[9] = {
        "NAM_BRT","NAM_RZR","NAM_UND","NAM_YOK","NAM_SHN2",
        "NAM_BAM2","NAM_DNK",NULL,"NAM_LEX"
    };
    return source_id < 9u ? names[source_id] : NULL;
}

static void draw_select_mug_at(uint8_t source_id,
                               int anchor_x, int anchor_y,
                               bool flip_x) {
    static const char *const mug[9][8] = {
        {"BHMUG_A","BHMUG_B","BHMUG_C","BHMUG_D","BHMUG_E","BHMUG_F","BHMUG_G","BHMUG_H"},
        {"RRMUG_A","RRMUG_B","RRMUG_C","RRMUG_D","RRMUG_E","RRMUG_F","RRMUG_G","RRMUG_H"},
        {"UNMUG_A","UNMUG_B","UNMUG_C","UNMUG_D","UNMUG_E","UNMUG_F","UNMUG_G","UNMUG_H"},
        {"YKMUG_A","YKMUG_B","YKMUG_C","YKMUG_D","YKMUG_E","YKMUG_F","YKMUG_G","YKMUG_H"},
        {"SMMUG_A","SMMUG_B","SMMUG_C","SMMUG_D","SMMUG_E","SMMUG_F","SMMUG_G","SMMUG_H"},
        {"BMMUG_A","BMMUG_B","BMMUG_C","BMMUG_D","BMMUG_E","BMMUG_F","BMMUG_G","BMMUG_H"},
        {"DKMUG_A","DKMUG_B","DKMUG_C","DKMUG_D","DKMUG_E","DKMUG_F","DKMUG_G","DKMUG_H"},
        {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
        {"LXMUG_A","LXMUG_B","LXMUG_C","LXMUG_D","LXMUG_E","LXMUG_F","LXMUG_G","LXMUG_H"},
    };
    if (source_id >= 9u)
        return;

    for (int i = 0; i < 8; ++i)
        if (mug[source_id][i])
            draw_select_sprite_named(mug[source_id][i],
                                     anchor_x, anchor_y, flip_x);
}


/* AWARD.ASM::get_bonus_icons stores half the summed WIMP widths in
   icon_string_width.  Reproduce that centering directly from the exact
   extracted WIMP widths. */
static int bonus_icon_string_width(const wm_bonus_icon_list *icons) {
    if (!icons) return 0;
    int width = 0;
    for (size_t i = 0; i < icons->count; ++i) {
        const char *name = wm_award_bonus_icon_source_name(icons->icon[i]);
        const wm_source_sprite *spr = wm_select_sprite_find(name);
        if (spr) width += (int)spr->width;
    }
    return width;
}

static void draw_bonus_icon_string(uint32_t total, int center_x, int y) {
    const wm_bonus_icon_list icons = wm_award_get_bonus_icons(total);
    int x = center_x - bonus_icon_string_width(&icons) / 2;
    for (size_t i = 0; i < icons.count; ++i) {
        const char *name = wm_award_bonus_icon_source_name(icons.icon[i]);
        const wm_source_sprite *spr = wm_select_sprite_find(name);
        if (!spr) continue;
        draw_select_sprite_named(name, x, y, false);
        x += (int)spr->width;
    }
}

/* AWARD.ASM::show_bonus_icons. */
static void render_select_bonus_icons(const wm_app *app,
                                      unsigned player) {
    if (!app || player > 1u)
        return;

    const uint32_t total = wm_award_select_total(&app->awards, player);
    if (!total)
        return;

    const int msg_x = player ? WM_BONUS_MSG_XPOS2 : WM_BONUS_MSG_XPOS1;
    const int icon_x = player ? WM_BONUS_ICON_XPOS2 : WM_BONUS_ICON_XPOS1;
    draw_select_sprite_named("SKILBON", msg_x, WM_BONUS_MSG_YPOS, false);
    draw_bonus_icon_string(total, icon_x, WM_BONUS_ICON_YPOS);
}

/* AWARD.ASM::show_progress_bicons. */
static void render_progress_bonus_icons(const wm_app *app) {
    if (!app) return;
    const uint32_t total = wm_award_progress_total(&app->awards);
    if (!total) return;

    draw_bonus_icon_string(total,
                           WM_PROGRESS_BONUS_ICON_XPOS,
                           WM_PROGRESS_BONUS_ICON_YPOS);
}

static void render_character_select(const wm_app *app) {
    fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));
    if (!app || app->select.setup_ticks == 0u)
        return;

    render_select_module("wwfselbkBMOD", false, -40, 0);
    render_select_module("choiceBMOD", true, 3, 256);

    const uint8_t p1_source = wm_select_screen_current_source(&app->select);
    draw_select_mug_at(p1_source, 20, 175, false);
    render_select_bonus_icons(app, 0u);

    uint8_t p2_source = 0xffu;
    if (app->select.p2_joined) {
        p2_source = wm_select_screen_p2_current_source(&app->select);
        /* p2info: mug x=400-18, y=175, M_FLIPH. */
        draw_select_mug_at(p2_source, 382, 175, true);
        render_select_bonus_icons(app, 1u);
    }

    const uint8_t p1_slot =
        app->select.p1.index < 8u ? app->select.p1.index : 0u;
    const uint8_t p2_slot =
        app->select.p2.index < 8u ? app->select.p2.index : 1u;
    const int p1x = select_crouton_pos[p1_slot][0];
    const int p1y = select_crouton_pos[p1_slot][1];
    const int p2x = select_crouton_pos[p2_slot][0];
    const int p2y = select_crouton_pos[p2_slot][1];

    /*
     * SELECT.ASM object Z:
     * P1 plate 2<->3, highlight 5<->4.
     * P2 plate 3<->2, highlight 6<->7.
     */
    const bool p1_flip =
        wm_select_screen_cursor_z_flipped(&app->select);
    const bool p2_flip =
        wm_select_screen_p2_cursor_z_flipped(&app->select);
    const int p1_plate_z = p1_flip ? 3 : 2;
    const int p2_plate_z = p2_flip ? 2 : 3;

    if (app->select.p2_joined && p2_plate_z < p1_plate_z) {
        draw_select_sprite_named("CRUTPLT_R", p2x, p2y, false);
        draw_select_sprite_named("CRUTPLT_B", p1x, p1y, false);
    } else {
        draw_select_sprite_named("CRUTPLT_B", p1x, p1y, false);
        if (app->select.p2_joined)
            draw_select_sprite_named("CRUTPLT_R", p2x, p2y, false);
    }

    const bool p1_hilite =
        wm_select_screen_highlight_visible(&app->select);
    if (p1_flip && p1_hilite)
        draw_select_sprite_named("CRUTHI_B", p1x, p1y, false);

    for (int i = 0; i < 8; ++i)
        draw_select_sprite_named(select_croutons[i],
                                 select_crouton_pos[i][0],
                                 select_crouton_pos[i][1], false);

    if (!p1_flip && p1_hilite)
        draw_select_sprite_named("CRUTHI_B", p1x, p1y, false);

    if (app->select.p2_joined &&
        wm_select_screen_p2_highlight_visible(&app->select)) {
        /*
         * SELECT.ASM #place_cursor: when both cursors occupy one crouton,
         * only P2 HILITE is nudged +2,+2; the red plate is not moved.
         */
        const int overlap = p1_slot == p2_slot ? 2 : 0;
        draw_select_sprite_named("CRUTHI_R",
                                 p2x + overlap, p2y + overlap, false);
    }

    /* p1info/p2info PI_NAME_X are hexadecimal source constants. */
    const char *p1_name = select_name_image(p1_source);
    if (p1_name)
        draw_select_sprite_named(p1_name, 0x51, 184, false);

    if (app->select.p2_joined) {
        const char *p2_name = select_name_image(p2_source);
        if (p2_name)
            draw_select_sprite_named(p2_name, 0x142, 184, false);
    }

    render_select_buyin_overlay(app);
    render_select_random_message(app);

    const int digit = wm_select_screen_clock_digit(&app->select);
    if (digit >= 0 && digit <= 9) {
        char fnt[8];
        snprintf(fnt, sizeof(fnt), "FNT9_%d", digit);
        draw_select_sprite_named(fnt, 0xCB, 232, false);
    }
}


/* PROGRESS.ASM uses LADDERBMOD as a horizontally-scrolled packed Wolf Unit
   background.  Keep this separate from the select modules because it is
   decoded from its own ERHDRS/ERPALS source tables. */
static void render_progress_module(const char *name, int module_x, int module_y) {
    const wm_named_bmod *named = wm_source_bmod_find(name);
    if (!named || named->module.block_count > 64)
        return;

    struct progress_draw_ref { wm_bmod_block block; } refs[64];
    size_t count = 0;
    for (size_t i = 0; i < named->module.block_count; ++i) {
        if (wm_bmod_decode_block(&named->module, i, &refs[count].block))
            ++count;
    }
    for (size_t i = 1; i < count; ++i) {
        struct progress_draw_ref key = refs[i];
        size_t j = i;
        while (j > 0 && wm_bmod_draw_before(&key.block, &refs[j - 1].block)) {
            refs[j] = refs[j - 1];
            --j;
        }
        refs[j] = key;
    }

    for (size_t i = 0; i < count; ++i) {
        const wm_bmod_block *b = &refs[i].block;
        const wm_select_background_image *img = wm_progress_image_at(b->header_index);
        const wm_select_background_palette *pal = wm_progress_palette_at(b->palette);
        draw_select_background_block(img, pal, b, module_x, module_y);
    }
}

static void render_belt_choice(const wm_app *app) {
    fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));
    if (!app) return;

    /* PROGRESS.ASM::ask_belt_question authors the whole composition one
       Wolf-unit screen below the viewport, then moves WORLDTLY +0x18/tick
       until 255.  Preserve those source positions instead of hand-layout. */
    const int wy = app->pregame.belt_world_y;
    /*
     * SELECT.ASM::plyrsel_mod installs both modules before belt selection:
     *   wwfselbkBMOD  at (-40,0)
     *   choiceBMOD    at (3,256)
     * PROGRESS.ASM::ask_belt_question advances WORLDTLY and calls BGND_UD1;
     * screen-space Y is therefore module_y - belt_world_y.
     */
    render_select_module("wwfselbkBMOD", false, -40, -wy);
    render_select_module("choiceBMOD", true, 3, 256 - wy);
    const int top_y = 93 + 256 - wy;
    const int bottom_y = 184 + 256 - wy;
    const bool inter_selected =
        app->pregame.belt_type == WM_PREGAME_BELT_INTERCONTINENTAL;

    /* PROGRESS.ASM requests BEGINOBJP(MVEBAR_R,DPLT_P_P), but DPLT_P_P is
       not defined in this source drop's IMGPAL.ASM.  Preserve MVEBAR_R's
       embedded WIMP palette until that shipped/shared palette is recovered;
       do not alias it to a different known palette.  SHADOW01 is likewise
       original source artwork; JUDDER_SHADOW remains a later fidelity item. */
    draw_select_sprite_named_palette("MVEBAR_R", 10, 30 + 256 - wy, false, "DPLT_P_P");
    draw_select_sprite_named("SHADOW01", 13, 39 + 256 - wy, false);

    draw_belt_osgemd_centered("SELECT YOUR TITLE:", 200, 18 + 256 - wy);

    /* Two independent PCYC_PID processes walk ten source palettes, one frame
       every four source ticks, and apply each palette to the A/B glow pair. */
    static const char *const top_glow_pal[10] = {
        "CHGLWT_P", "CHGLWT1P", "CHGLWT2P", "CHGLWT3P", "CHGLWT4P",
        "CHGLWT5P", "CHGLWT6P", "CHGLWT7P", "CHGLWT8P", "CHGLWT9P"
    };
    static const char *const bottom_glow_pal[10] = {
        "CHGLWB_P", "CHGLWB1P", "CHGLWB2P", "CHGLWB3P", "CHGLWB4P",
        "CHGLWB5P", "CHGLWB6P", "CHGLWB7P", "CHGLWB8P", "CHGLWB9P"
    };
    const unsigned pal_frame = (app->pregame.belt_anim_ticks / 4u) % 10u;

    /* hilight toggles OCTRL bit 2 between each option's glow pair and shadow
       pair.  Fix18 drew all eight simultaneously; that is not the arcade
       object state.  After flash_it, the source deletes both glow pairs. */
    const bool post_flash_hold =
        app->pregame.phase == WM_PREGAME_BELT_FLASH &&
        app->pregame.phase_ticks >= 18u;

    if (!post_flash_hold) {
        if (inter_selected) {
            draw_select_sprite_named_palette("CHOGLOT_A", 200, top_y, false,
                                             top_glow_pal[pal_frame]);
            draw_select_sprite_named_palette("CHOGLOT_B", 200, top_y, false,
                                             top_glow_pal[pal_frame]);
            draw_select_sprite_named("CHSHDB_A", 201, bottom_y, false);
            draw_select_sprite_named("CHSHDB_B", 201, bottom_y, false);
        } else {
            draw_select_sprite_named("CHSHDT_A", 200, top_y, false);
            draw_select_sprite_named("CHSHDT_B", 200, top_y, false);
            draw_select_sprite_named_palette("CHOGLOB_A", 201, bottom_y, false,
                                             bottom_glow_pal[pal_frame]);
            draw_select_sprite_named_palette("CHOGLOB_B", 201, bottom_y, false,
                                             bottom_glow_pal[pal_frame]);
        }
    } else {
        /* DELOBJ removes top glow A/B and bottom glow A/B after flash_it. */
        if (inter_selected) {
            draw_select_sprite_named("CHSHDB_A", 201, bottom_y, false);
            draw_select_sprite_named("CHSHDB_B", 201, bottom_y, false);
        } else {
            draw_select_sprite_named("CHSHDT_A", 200, top_y, false);
            draw_select_sprite_named("CHSHDT_B", 200, top_y, false);
        }
    }

    /* hilight also swaps the option plate/text OPALs. */
    const char *top_plate_pal = inter_selected ? "DPLT_P2P" : "DPLT_W_P";
    const char *top_text_pal = inter_selected ? "WSF_Y_P" : "WSF_W_P";
    const char *bottom_plate_pal = inter_selected ? "DPLT_W_P" : "DPLT_P2P";
    const char *bottom_text_pal = inter_selected ? "WSF_W_P" : "WSF_Y_P";

    /* flash_it touches only the selected CHOICBK object: three loops of
       3 ticks off / 3 ticks on.  The INTER/WORLD label itself stays present. */
    bool selected_plate_visible = true;
    if (app->pregame.phase == WM_PREGAME_BELT_FLASH &&
        app->pregame.phase_ticks < 18u) {
        selected_plate_visible = ((app->pregame.phase_ticks / 3u) & 1u) != 0u;
    }

    if (!inter_selected || selected_plate_visible)
        draw_select_sprite_named_palette("CHOICBK", 200, top_y, false, top_plate_pal);
    draw_select_sprite_named_palette("INTER", 200, top_y, false, top_text_pal);

    if (inter_selected || selected_plate_visible)
        draw_select_sprite_named_palette("CHOICBK", 201, bottom_y, false, bottom_plate_pal);
    draw_select_sprite_named_palette("WORLD", 201, bottom_y, false, bottom_text_pal);
}

static void draw_progress_piece(const wm_source_sprite *spr,
                                const wm_progress_palette *pal,
                                float anchor_x, float anchor_y,
                                int display_off_x, int display_off_y,
                                bool flip_x) {
    if (!spr || !pal || !pal->rgba5551 || !pal->color_count) return;
    wm_source_sprite palette_proxy = *spr;
    palette_proxy.palette_rgba5551 = pal->rgba5551;
    palette_proxy.palette_colors = pal->color_count;
    draw_source_sprite_scaled(spr,
                              anchor_x * WM_FRONTEND_SCALE_X,
                              anchor_y * WM_FRONTEND_SCALE_Y,
                              display_off_x, display_off_y, flip_x,
                              &palette_proxy,
                              WM_FRONTEND_SCALE_X, WM_FRONTEND_SCALE_Y);
}

/* PROGRESS.ASM::CREATE_TEMP_WRESTLER uses the same two-channel wrestler
   image/attachment machinery as the match renderer.  The only substitutions
   here are the platform draw calls: animation labels, delays, WIMP pixels,
   x/y offsets, channel-2 attachment metadata and wrestler_pal all come from
   the historical source and original WIMP containers. */
/* ANIM.ASM::set_images anchors wrestler OYVAL at Z*Y_SCALE_MULTIPLIER and
   puts OBJ_YPOSINT into ODYOFF.  PROGRESS.ASM writes MOVI 100 to the 32-bit
   OBJ_YPOS field, which leaves OBJ_YPOSINT (the high/int word) at zero.
   Therefore the progression actors start on the projected ground line; the
   literal 100 is fractional low-word state, not a 100-pixel vertical lift. */
#define WM_PROGRESS_Y_SCALE_MULTIPLIER 0x3566

static int wm_progress_ground_y(int source_z) {
    return (int)(((int32_t)source_z * (int32_t)WM_PROGRESS_Y_SCALE_MULTIPLIER) >> 16);
}

static int wm_progress_actor_y(int source_y, int source_z) {
    return wm_progress_ground_y(source_z) - source_y;
}

static void draw_progress_actor(uint8_t source_wrestler,
                                wm_progress_action action,
                                unsigned anim_ticks,
                                int world_x, int source_y, int source_z,
                                int world_scroll_x,
                                bool facing_left) {
    const wm_progress_anim *primary_anim =
        wm_progress_anim_get(source_wrestler, action, false);
    const wm_progress_anim *torso_anim =
        wm_progress_anim_get(source_wrestler, action, true);
    const wm_progress_anim_frame *pf =
        wm_progress_anim_frame_at(primary_anim, anim_ticks);
    const wm_progress_anim_frame *tf =
        wm_progress_anim_frame_at(torso_anim, anim_ticks);
    const wm_progress_palette *pal =
        wm_progress_palette_for_wrestler(source_wrestler);
    if (!pf || !pal) return;

    const wm_source_sprite *primary = wm_progress_sprite_find(pf->source_frame);
    if (!primary) return;

    /* ANI_OFFSET mutates the wrestler process position.  Bam's wait animation
       moves him -800 before clever_addr adds +800, so clever inherits the
       wait-side displacement rather than starting from a fresh zero origin. */
    int inherited_x = 0;
    if (source_wrestler == 5u && action == WM_PROGRESS_ACT_CLEVER)
        inherited_x = -800;

    const int actor_x = world_x + inherited_x + pf->offset_x;
    const int actor_y = source_y + pf->offset_y;
    const int actor_z = source_z + pf->offset_z;
    const float sx = (float)(actor_x - world_scroll_x);
    const float sy = (float)wm_progress_actor_y(actor_y, actor_z);
    const bool primary_flip = facing_left ^ pf->xflip;
    draw_progress_piece(primary, pal, sx, sy,
                        primary->xani, primary->yani, primary_flip);

    if (!tf) return;
    const wm_source_sprite *torso = wm_progress_sprite_find(tf->source_frame);
    if (!torso) return;

    const int a2x = primary->wimp_tail[WM_ATTACH_X_SLOT];
    const int a2y = primary->wimp_tail[WM_ATTACH_Y_SLOT];
    if (a2x == -1 && a2y == -1) return;
    if (a2x < -512 || a2x > 512 || a2y < -512 || a2y > 512) return;

    int secondary_xoff = 0, secondary_yoff = 0;
    wm_secondary_display_offsets(primary->xani, primary->yani, a2x, a2y,
                                 torso->xani, torso->yani,
                                 &secondary_xoff, &secondary_yoff);
    const bool torso_flip = facing_left ^ tf->xflip;
    draw_progress_piece(torso, pal, sx, sy,
                        secondary_xoff, secondary_yoff, torso_flip);
}

typedef struct {
    uint8_t wrestler;
    wm_progress_action action;
    unsigned anim_ticks;
    int world_x;
    int source_y;
    int source_z;
    bool facing_left;
    bool opponent;
} wm_progress_draw_actor;

static void wm_progress_sort_actors(wm_progress_draw_actor *a, unsigned count) {
    /* The arcade object renderer updates OYVAL from Z projection to preserve
       priority.  For these temporary wrestlers, ascending source Z is the
       equivalent painter order: farther actors first, nearer actors last. */
    for (unsigned i = 1; i < count; ++i) {
        wm_progress_draw_actor key = a[i];
        unsigned j = i;
        while (j > 0 && a[j - 1].source_z > key.source_z) {
            a[j] = a[j - 1];
            --j;
        }
        a[j] = key;
    }
}

static const char *wm_progress_fuji_frame(unsigned t) {
    /* PROGRESS.ASM::FUJI_ANIM: 7,7,7,60,7,7,7. */
    if (t < 7u) return "FUJI01";
    t -= 7u;
    if (t < 7u) return "FUJI02";
    t -= 7u;
    if (t < 7u) return "FUJI03";
    t -= 7u;
    if (t < 60u) return "FUJI04";
    t -= 60u;
    if (t < 7u) return "FUJI03";
    t -= 7u;
    if (t < 7u) return "FUJI02";
    return "FUJI01";
}

static void draw_progress_fuji(const wm_progress_draw_actor *a,
                               unsigned opponent_count,
                               int world_scroll_x) {
    if (!a || !a->opponent || a->wrestler != 3u || opponent_count != 1u)
        return;

    /* CREATE_FUJI first moves Yokozuna +30 in world X, then creates Fuji at
       that shifted X-80. Fuji is a separate source object and keeps its own
       authored screen-space Y path. */
    const int fuji_x = a->world_x - 80 - world_scroll_x;
    const char *frame = "FUJI01";
    if (a->action == WM_PROGRESS_ACT_CLEVER)
        frame = wm_progress_fuji_frame(a->anim_ticks);
    draw_select_sprite_named(frame, fuji_x, 240, false);
}

static void draw_progress_urn(const wm_progress_draw_actor *a,
                              int world_scroll_x) {
    if (!a || !a->opponent || a->wrestler != 2u ||
        a->action != WM_PROGRESS_ACT_CLEVER || a->anim_ticks < 48u)
        return;

    /* und_clever_anim runs eight 6-tick frames before CREATE_URN. URN_ANIM is
       five BLUURN frames at six ticks each, then the object dies. */
    unsigned t = a->anim_ticks - 48u;
    if (t >= 30u) return;
    char frame[16];
    snprintf(frame, sizeof(frame), "BLUURN%02u", (t / 6u) + 1u);
    const int x = a->world_x - world_scroll_x;
    const int y = wm_progress_ground_y(a->source_z) - 0x5A;
    draw_select_sprite_named(frame, x, y, false);
}

static void draw_progress_water(const wm_progress_draw_actor *a,
                                int world_scroll_x) {
    if (!a || !a->opponent || a->wrestler != 6u ||
        a->action != WM_PROGRESS_ACT_CLEVER || a->anim_ticks < 18u)
        return;

    /* dnk_clever_anim invokes CREATE_WATER after three 6-tick frames. */
    unsigned t = a->anim_ticks - 18u;
    if (t >= 39u) return;

    const char *frame;
    if (t < 4u) frame = "WATER01";
    else if (t < 8u) frame = "WATER02";
    else if (t < 23u) frame = "WATER03";
    else if (t < 27u) frame = "WATER04";
    else if (t < 31u) frame = "WATER05";
    else if (t < 35u) frame = "WATER06";
    else frame = "WATER07";

    int water_world_x = a->world_x - 10;
    if (t >= 8u && t < 23u) {
        /* START_WATER stores (start-0x240)/15, then MOVE_WATER subtracts that
           once per tick.  Keep the arithmetic in source world coordinates. */
        const int delta = (water_world_x - 0x240) / 15;
        unsigned steps = t - 8u;
        if (steps > 15u) steps = 15u;
        water_world_x -= delta * (int)steps;
    } else if (t >= 23u) {
        water_world_x = 0x240;
    }
    const int y = wm_progress_ground_y(a->source_z) - 0x58;
    draw_select_sprite_named(frame, water_world_x - world_scroll_x, y, false);
}

static unsigned wm_progress_build_actors(const wm_app *app,
                                         wm_progress_draw_actor out[4]) {
    if (!app || !out) return 0u;
    unsigned n = 0u;
    out[n++] = (wm_progress_draw_actor){
        app->pregame.player_source_wrestler,
        app->pregame.progress_player_action,
        app->pregame.progress_player_anim_ticks,
        (int)(app->pregame.progress_player_x_fp >> 16),
        0, 0x470, false, false
    };

    static const int16_t one_x[1]   = {675};
    static const int16_t one_z[1]   = {0x470};
    static const int16_t two_x[2]   = {705, 655};
    static const int16_t two_z[2]   = {0x490, 0x450};
    static const int16_t three_x[3] = {720, 675, 630};
    static const int16_t three_z[3] = {0x4A0, 0x470, 0x440};
    const int16_t *xs = one_x, *zs = one_z;
    unsigned count = app->pregame.opponent_count;
    if (count == 2u) { xs = two_x; zs = two_z; }
    else if (count >= 3u) { xs = three_x; zs = three_z; count = 3u; }

    for (unsigned i = 0; i < count && n < 4u; ++i) {
        uint8_t w = wm_pregame_opponent_at(&app->pregame, i);
        if (w == 0xffu) continue;
        int x = xs[i];
        /* CREATE_FUJI mutates Yoko's OBJ_XPOS by +30 when he is the only
           opponent.  Preserve that mutation before both wrestler and Fuji draw. */
        if (count == 1u && w == 3u) x += 30;
        out[n++] = (wm_progress_draw_actor){
            w, app->pregame.progress_opponent_action,
            app->pregame.progress_opponent_anim_ticks,
            x, 0, zs[i], true, true
        };
    }
    return n;
}

static void draw_progress_actors_and_effects(const wm_app *app,
                                             int world_scroll_x) {
    wm_progress_draw_actor actors[4];
    unsigned count = wm_progress_build_actors(app, actors);
    wm_progress_sort_actors(actors, count);
    for (unsigned i = 0; i < count; ++i) {
        wm_progress_draw_actor *a = &actors[i];
        draw_progress_actor(a->wrestler, a->action, a->anim_ticks,
                            a->world_x, a->source_y, a->source_z,
                            world_scroll_x, a->facing_left);
        draw_progress_fuji(a, app->pregame.opponent_count, world_scroll_x);
        draw_progress_urn(a, world_scroll_x);
        draw_progress_water(a, world_scroll_x);
    }
}

/* Fix29: source-authentic dynamic progression HUD.
   PROGRESS.ASM JAM_STR(font9_ascii,8,0,...) uses an 8-pixel space and zero
   inter-glyph spacing.  Keep this separate from SELECT.ASM's font9 metrics. */
static const wm_source_sprite *progress_font9_glyph(char c, char name[12]) {
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
        snprintf(name, 12, "YFNT9_%c", c);
    else if (c == '\'')
        snprintf(name, 12, "YFNT9_APO");
    else {
        name[0] = '\0';
        return NULL;
    }
    return wm_select_sprite_find(name);
}
static int progress_font9_width(const char *text) {
    int width = 0;
    if (!text) return 0;
    for (; *text; ++text) {
        if (*text == ' ') {
            width += 8;
            continue;
        }
        char name[12];
        const wm_source_sprite *spr = progress_font9_glyph(*text, name);
        if (spr) width += (int)spr->width;
    }
    return width;
}
static void draw_progress_font9(const char *text, int x, int y, bool centered) {
    if (!text) return;
    if (centered) x -= progress_font9_width(text) / 2;
    for (; *text; ++text) {
        if (*text == ' ') {
            x += 8;
            continue;
        }
        char name[12];
        const wm_source_sprite *spr = progress_font9_glyph(*text, name);
        if (!spr) continue;
        draw_select_sprite_named(name, x, y, false);
        x += (int)spr->width;
    }
}
/* PROGRESS.ASM::DO_LADDER_BITS uses win_ascii, mess_spacing=2 and WINFONT. */
static void draw_progress_win_number(unsigned value, unsigned maximum,
                                     int x, int y) {
    if (value > maximum) value = maximum;
    char digits[16];
    snprintf(digits, sizeof(digits), "%u", value);
    for (const char *c = digits; *c; ++c) {
        char name[12];
        snprintf(name, sizeof(name), "WFONT_%c", *c);
        const wm_source_sprite *spr = wm_select_sprite_find(name);
        if (!spr) continue;
        draw_select_sprite_named_palette(name, x, y, false, "WINFONT");
        x += (int)spr->width + 2;
    }
}
/* Recent-champion initials switch to wsf14_ascii while retaining the source
   mess_spacing=2 / mess_space_width=3 values set immediately beforehand. */
static void draw_progress_wsf14(const char *text, int x, int y) {
    if (!text) return;
    for (; *text; ++text) {
        if (*text == ' ') {
            x += 3;
            continue;
        }
        if (*text < 'A' || *text > 'Z') continue;
        char name[12];
        snprintf(name, sizeof(name), "WSF14_%c", *text);
        const wm_source_sprite *spr = wm_select_sprite_find(name);
        if (!spr) continue;
        draw_select_sprite_named_palette(name, x, y, false, "WSF_Y_P");
        x += (int)spr->width + 2;
    }
}
static const char *progress_factory_recent_champ(const wm_app *app) {
    /* HSTD.ASM factory tables: BEATEN_ROM_TABLE + HS_SIZE begins "MIKE ";
       INTER_ROM_TABLE + HS_SIZE begins "MARK ".  Persistence can replace this
       with live CMOS/high-score state when that subsystem is ported. */
    return app->pregame.belt_type == WM_PREGAME_BELT_WWF ? "MIKE " : "MARK ";
}
static void draw_progress_ladder_bits(const wm_app *app) {
    if (!app) return;
    enum { STATX = 5, STATY = 30 };
    const bool champ = app->pregame.belt_type == WM_PREGAME_BELT_WWF;
    const char *plate = champ ? "DPLT_P_P" : "DPLT_R_P";
    const char *belt_pal = champ ? "SWWFBB_P" : "SWWFBW_P";
    const char *flash_pal = champ ? "FLASHP_P" : "FLASHR_P";

    /* PROGRESS.ASM::INTER_BITS_LIST / CHAMP_BITS_LIST, in source order. */
    draw_select_sprite_named_palette("STATBAR",  STATX,      STATY,      false, plate);
    draw_select_sprite_named_palette("BLUESH",   STATX+50,   STATY+19,   false, "BARB_P");
    draw_select_sprite_named_palette("TXTBAR1",  STATX+60,   STATY+26,   false, plate);
    draw_select_sprite_named_palette("WINS_IMG", STATX+20,   STATY+38,   false, "WINFONT");
    draw_select_sprite_named_palette("BLUESH",   STATX+36,   STATY+33,   false, "BARB_P");
    draw_select_sprite_named_palette("TXTBAR1",  STATX+46,   STATY+40,   false, plate);
    draw_select_sprite_named_palette("MATCH_IMG",STATX+29,   STATY+24,   false, "WINFONT");
    draw_select_sprite_named_palette("BLUESH",   STATX+23,   STATY+47,   false, "BARB_P");
    draw_select_sprite_named_palette("TXTBAR1",  STATX+230,  STATY+40,   false, plate);
    draw_select_sprite_named_palette("TXTBAR1",  STATX+297,  STATY+40,   false, plate);
    draw_select_sprite_named_palette("TXTBAR1",  STATX+360,  STATY+40,   false, plate);
    draw_select_sprite_named_palette("TXTPCE",   STATX+260,  STATY+40,   false, plate);
    draw_select_sprite_named_palette("TXTPCE",   STATX+335,  STATY+40,   false, plate);
    draw_select_sprite_named_palette("RCHAMP",   STATX+199,  STATY+38,   false, "WINFONT");
    draw_select_sprite_named_palette("SWWFBLT",  STATX+345,  STATY+9,    false, belt_pal);
    draw_select_sprite_named_palette("LBAR_GENB",STATX+111,  STATY,      true,  "BARB_P");
    draw_select_sprite_named_palette("LBAR_GENB",STATX+64,   STATY+51,   true,  "BARB_P");

    /* FLASH_BIT_ANIM: 01,02,03,04,05,04,03,02, two source ticks each.
       `flash_frame` is maintained in source ticks by the portable state. */
    static const char *const flash_frames[8] = {
        "FLASH01","FLASH02","FLASH03","FLASH04",
        "FLASH05","FLASH04","FLASH03","FLASH02"
    };
    static const int16_t flash_x[7] = {31,47,63,79,114,130,165};
    unsigned marker_count = app->pregame.current_ladder_index >= 0
        ? (unsigned)app->pregame.current_ladder_index + 1u : 1u;
    if (marker_count > 7u) marker_count = 7u;
    const char *ff = flash_frames[(app->pregame.flash_frame / 2u) & 7u];
    for (unsigned i = 0; i < marker_count; ++i)
        draw_select_sprite_named_palette(ff, flash_x[i], STATY+9, false, flash_pal);
    /* Fix29 exact DO_LADDER_BITS dynamic fields. */
    draw_progress_win_number(app->pregame.match_count, 999u, STATX+76, STATY+19+3+2);
    draw_progress_win_number(app->pregame.win_streak, 9999999u, STATX+7+50, STATY+19+4+12+3);
    draw_progress_wsf14(progress_factory_recent_champ(app), 0x15F, STATY+19+12+3);
}


/* PROGRESS.ASM::FIND_LOGO_IMAGE / LOGO_IMAGE_TABLE. */
static const char *progress_logo_image(uint8_t source_id) {
    static const char *const logos[10] = {
        "HRT3", "RZR3", "UND3", "YOK3", "SHN3",
        "BAM3", "DNK3", "LEX3", "LEX3", "WWFCHAL"
    };
    return source_id < 10u ? logos[source_id] : NULL;
}

/* PROGRESS.ASM::MOVE_BLOC: left DAGs travel +speed, right DAGs -speed. */
static int progress_close_block_x(int start_x, unsigned travel) {
    return start_x < 200 ? start_x + (int)travel
                         : start_x - (int)travel;
}

/* INIT_BLOC_STUFF uses the image SAG and a separately supplied B_DAG. */
static void draw_progress_bloc(const wm_app *app, const char *name,
                               int start_x, int y, unsigned travel) {
    if (!app || !name) return;
    const wm_source_sprite *spr = wm_select_sprite_find(name);
    if (!spr) return;
    const int x = progress_close_block_x(start_x, travel)
                + app->pregame.progress_shake_x;
    const int sy = y + app->pregame.progress_shake_y;
    draw_source_sprite_scaled(spr,
                              (float)x * WM_FRONTEND_SCALE_X,
                              (float)sy * WM_FRONTEND_SCALE_Y,
                              0, 0, false, spr,
                              WM_FRONTEND_SCALE_X, WM_FRONTEND_SCALE_Y);
}

/* INIT_BLOC + SETUP_LOGOS + SETUP_THIS_LOGO_DAG, one-player close path. */
static void draw_progress_close_transition(const wm_app *app) {
    if (!app || app->pregame.phase != WM_PREGAME_PROGRESS_CLOSE)
        return;

    unsigned travel = app->pregame.progress_close_move_ticks *
                      app->pregame.progress_close_speed;
    if (travel > 200u) travel = 200u;

    static const struct {
        const char *name;
        int16_t y, x;
    } init_bloc_tab[10] = {
        {"CSELBK_A",   0,-204},
        {"CSELBK_C", 133,-204},
        {"CSELBK_E", 252,-204},
        {"CSELBK_B",   0, 404},
        {"CSELBK_D", 133, 404},
        {"CSELBK_F", 252, 404},
        {"CSELBV_A",   0,  -4},
        {"CSELBV_C", 133,  -4},
        {"CSELBV_B",   0, 400},
        {"CSELBV_D", 133, 400},
    };

    for (unsigned i=0; i<10u; ++i)
        draw_progress_bloc(app, init_bloc_tab[i].name,
                           init_bloc_tab[i].x, init_bloc_tab[i].y, travel);

    const char *human_name =
        progress_logo_image(app->pregame.player_source_wrestler);
    const wm_source_sprite *human =
        human_name ? wm_select_sprite_find(human_name) : NULL;
    if (human) {
        const int x = -200 + (200 - (int)human->width) / 2;
        const int y = (255 - (int)human->height) / 2;
        draw_progress_bloc(app, human_name, x, y, travel);
    }

    unsigned count = app->pregame.opponent_count;
    if (count > WM_PREGAME_MAX_OPPONENTS)
        count = WM_PREGAME_MAX_OPPONENTS;

    int stack_h = -15;
    for (unsigned i=0; i<count; ++i) {
        const uint8_t id = wm_pregame_opponent_at(&app->pregame,i);
        const char *name = progress_logo_image(id);
        const wm_source_sprite *spr =
            name ? wm_select_sprite_find(name) : NULL;
        if (spr) stack_h += (int)spr->height + 15;
    }
    if (stack_h < 0) stack_h = 0;

    int y = (255 - stack_h) / 2;
    for (unsigned i=0; i<count; ++i) {
        const uint8_t id = wm_pregame_opponent_at(&app->pregame,i);
        const char *name = progress_logo_image(id);
        const wm_source_sprite *spr =
            name ? wm_select_sprite_find(name) : NULL;
        if (!spr) continue;
        const int x = 400 + (200 - (int)spr->width) / 2;
        draw_progress_bloc(app,name,x,y,travel);
        y += (int)spr->height + 15;
    }
}


static const char *progress_bit_frame_name(const wm_progress_bit *b) {
    static const char *const chip1_f[] = {
        "CHIP1_01","CHIP1_03","CHIP1_05","CHIP1_07","CHIP1_09",
        "CHIP1_11","CHIP1_13","CHIP1_15","CHIP1_17","CHIP1_19"
    };
    static const char *const chip1_b[] = {
        "CHIP1_19","CHIP1_17","CHIP1_15","CHIP1_13","CHIP1_11",
        "CHIP1_09","CHIP1_07","CHIP1_05","CHIP1_03","CHIP1_01"
    };
    static const char *const chip2_f[] = {
        "CHIP2_01","CHIP2_03","CHIP2_05","CHIP2_07","CHIP2_09"
    };
    static const char *const chip2_b[] = {
        "CHIP2_09","CHIP2_07","CHIP2_05","CHIP2_03","CHIP2_01"
    };
    static const char *const chip3_f[] = {
        "CHIP3_01","CHIP3_03","CHIP3_05","CHIP3_07","CHIP3_09",
        "CHIP3_11","CHIP3_13","CHIP3_15","CHIP3_17","CHIP3_19"
    };
    static const char *const chip3_b[] = {
        "CHIP3_19","CHIP3_17","CHIP3_15","CHIP3_13","CHIP3_11",
        "CHIP3_09","CHIP3_07","CHIP3_05","CHIP3_03","CHIP3_01"
    };
    static const char *const chip4_f[] = {
        "CHIP4_01","CHIP4_03","CHIP4_05","CHIP4_07","CHIP4_09"
    };
    static const char *const chip4_b[] = {
        "CHIP4_09","CHIP4_07","CHIP4_05","CHIP4_03","CHIP4_01"
    };
    static const char *const chip5_f[] = {
        "CHIP5_01","CHIP5_03","CHIP5_05","CHIP5_07","CHIP5_09",
        "CHIP5_11","CHIP5_13","CHIP5_15","CHIP5_17","CHIP5_19"
    };
    static const char *const chip5_b[] = {
        "CHIP5_19","CHIP5_17","CHIP5_15","CHIP5_13","CHIP5_11",
        "CHIP5_09","CHIP5_07","CHIP5_05","CHIP5_03","CHIP5_01"
    };
    static const char *const initial[10] = {
        "CHIP1_01","CHIP1_01","CHIP2_01","CHIP2_01","CHIP3_01",
        "CHIP3_01","CHIP4_01","CHIP4_01","CHIP5_01","CHIP5_01"
    };
    static const char *const sparks[4] = {
        "SPKD1_09","SPKD2_09","SPKD4_09","SPKR1_09"
    };

    if (!b || !b->active) return NULL;
    if (b->kind >= 10u) return sparks[b->kind - 10u];
    if (!b->anim_started) return initial[b->kind];

    switch (b->kind) {
        case 0: return chip1_f[b->anim_index % 10u];
        case 1: return chip1_b[b->anim_index % 10u];
        case 2: return chip2_f[b->anim_index % 5u];
        case 3: return chip2_b[b->anim_index % 5u];
        case 4: return chip3_f[b->anim_index % 10u];
        case 5: return chip3_b[b->anim_index % 10u];
        case 6: return chip4_f[b->anim_index % 5u];
        case 7: return chip4_b[b->anim_index % 5u];
        case 8: return chip5_f[b->anim_index % 10u];
        case 9: return chip5_b[b->anim_index % 10u];
        default: return NULL;
    }
}

static void draw_progress_bits(const wm_app *app) {
    if (!app || app->pregame.phase != WM_PREGAME_PROGRESS_CLOSE ||
        !app->pregame.progress_bits_created)
        return;

    for (unsigned i = 0; i < WM_PREGAME_PROGRESS_BITS; ++i) {
        const wm_progress_bit *b = &app->pregame.progress_bits[i];
        const char *name = progress_bit_frame_name(b);
        if (!name) continue;

        const wm_source_sprite *spr = wm_select_sprite_find(name);
        const wm_select_palette *pal = wm_select_palette_find(
            b->kind < 10u ? "CHIP_B_P" : "SPKPRP_P");
        if (!spr || !pal || !pal->rgba5551 || !pal->color_count) continue;

        wm_source_sprite palette_proxy = *spr;
        palette_proxy.palette_rgba5551 = pal->rgba5551;
        palette_proxy.palette_colors = pal->color_count;

        const int x = (int)(b->x_fp >> 16) + app->pregame.progress_shake_x;
        const int y = (int)(b->y_fp >> 16) + app->pregame.progress_shake_y;
        draw_source_sprite_scaled(spr,
                                  (float)x * WM_FRONTEND_SCALE_X,
                                  (float)y * WM_FRONTEND_SCALE_Y,
                                  0, 0, false, &palette_proxy,
                                  WM_FRONTEND_SCALE_X, WM_FRONTEND_SCALE_Y);
    }
}

static void render_progress_screen(const wm_app *app) {
    fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));
    if (!app) return;

    /* PROGRESS_BMOD = LADDERBMOD at (-60,-45), with WORLDTLX advancing in
       16.16 source coordinates during MOVE_PROGRESS. */
    const int world_x = (int)(app->pregame.progress_world_x_fp >> 16);
    render_progress_module("LADDERBMOD", -60 - world_x, -45);
    render_progress_bonus_icons(app);

    /* Screen-relative ladder furniture is drawn before the temporary
       wrestlers in the same source composition. */
    draw_progress_ladder_bits(app);

    /* CREATE_TEMP_WRESTLER: player starts at world X=140/Y=100.  During
       MOVE_PROGRESS both player X and WORLDTLX advance by RUN_SPEED, keeping
       the selected wrestler running in place while the arena scrolls beneath
       him. Opponents sit farther down the same world and scroll into view. */
    draw_progress_actors_and_effects(app, world_x);

    /* PROGRESS.ASM tonites_matchup/type_setup/belt_setup verbatim:
       JAM_STR font9_ascii,8,0 at source coordinates 22,13 / 241,28 / 241,41. */
    draw_progress_font9("TONIGHT'S PROGRAM", 22, 13, false);
    if (app->pregame.belt_type == WM_PREGAME_BELT_WWF)
        draw_progress_font9("WWF CHAMPIONSHIP", 241, 28, true);
    else
        draw_progress_font9("INTERCONTINENTAL", 241, 28, true);
    draw_progress_font9("TITLE", 241, 41, true);
    /* start_credbox is external/shared; no invented cabinet text. */
    if (app->pregame.phase == WM_PREGAME_PROGRESS_CLOSE)
        draw_progress_close_transition(app);
    draw_progress_bits(app);
}

static void render_pregame(const wm_app *app) {
    if (!app) {
        fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));
        return;
    }

    switch (app->pregame.phase) {
        case WM_PREGAME_BELT_SETUP:
        case WM_PREGAME_BELT_SLIDE:
        case WM_PREGAME_BELT_SELECT:
        case WM_PREGAME_BELT_FLASH:
            render_belt_choice(app);
            break;

        case WM_PREGAME_PROGRESS_SETUP:
        case WM_PREGAME_PROGRESS_SCROLL:
        case WM_PREGAME_PROGRESS_HOLD:
        case WM_PREGAME_PROGRESS_CLOSE:
            render_progress_screen(app);
            break;

        case WM_PREGAME_READY_FOR_MATCH:
        default:
            /* Deliberate architecture boundary.  The old render_match() is a
               development harness, not the arcade start_match path. */
            fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));
            break;
    }
}

/* END SOURCE SELECT RENDERER */








static __attribute__((unused)) void render_match(const wm_app *app) {
    const wm_demo *demo = &app->demo;
    draw_ring_back();

    if (demo->p1.screen_y <= demo->p2.screen_y) {
        draw_fighter(&demo->p1);
        draw_fighter(&demo->p2);
    } else {
        draw_fighter(&demo->p2);
        draw_fighter(&demo->p1);
    }
    draw_ring_front();

    /* This is the currently translated start_match/gameplay subset.  It is
       driven by ATTRACT.ASM::show_gameplay timing now; no frontend menu owns it. */
    if (app->show_debug)
        draw_match_hud(app);

}

static void render_app(const wm_app *app) {
    surface_t *disp = display_get();
    rdpq_attach(disp, NULL);

    if (app->mode == WM_APP_MODE_SELECT) {
        render_character_select(app);
    } else if (app->mode == WM_APP_MODE_PREGAME) {
        render_pregame(app);
    } else if (app->mode == WM_APP_MODE_MATCH_INIT) {
        /* Honest handoff boundary: render_match() is still a dev harness. */
        fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));
    } else switch (app->attract.call) {
        case WM_ATTRACT_DCS_LOGO:
            render_dcs_logo(app);
            break;
        case WM_ATTRACT_SHOW_SPORTS_LOGO:
            render_midway_sports(app);
            break;
        case WM_ATTRACT_SHOW_TITLE:
            render_title_screen(app);
            break;
        case WM_ATTRACT_SHOW_GAMEPLAY:
            /* The existing combat renderer is a development harness only.
               Normal product rendering can never present it as start_match. */
            fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));
            break;
        default:
            /* Untranslated source routines are skipped by the portable core;
               never display a replacement title/select/credits screen here. */
            fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));
            break;
    }

    /* Stay synchronous on hardware: this is the path that stopped the two-CI8
       wrestler RSP queue timeout in r6h1. */
    rdpq_detach_wait();
    display_show(disp);
}


/*
 * WM_N64_WALLCLOCK_SOURCE_SYNC
 *
 * The Wolf Unit simulation clock is 53 Hz. It must advance from elapsed
 * real time, not from the number of frames the N64 renderer manages to
 * complete. The old path called wm_app_video_frame() once per render loop;
 * when the source-exact DCS particle renderer dropped below 60 fps, that
 * silently slowed ATTRACT.ASM while DCS PCM continued at real audio speed.
 *
 * Use microsecond wall time and an integer phase accumulator. This has no
 * fractional drift: every elapsed second contributes exactly 53 source ticks.
 */
#define WM_N64_SOURCE_HZ 53ull
#define WM_N64_USEC_PER_SEC 1000000ull

typedef struct {
    uint64_t last_us;
    uint64_t phase;
    wm_input_state latched_input[2];
} wm_n64_source_timer;

static void wm_n64_latch_source_input(wm_input_state *dst,
                                      const wm_input_state *src) {
    if (!dst || !src) return;

    dst->stick_x = src->stick_x;
    dst->stick_y = src->stick_y;
    dst->start |= src->start;
    dst->run |= src->run;
    dst->light_punch |= src->light_punch;
    dst->power_punch |= src->power_punch;
    dst->light_kick |= src->light_kick;
    dst->power_kick |= src->power_kick;
    dst->block |= src->block;
    dst->l |= src->l;
    dst->z |= src->z;
    dst->b |= src->b;
}

static unsigned wm_n64_source_ticks_due(wm_n64_source_timer *timer,
                                        uint64_t now_us) {
    if (!timer) return 0;

    if (timer->last_us == 0) {
        timer->last_us = now_us;
        return 0;
    }

    const uint64_t elapsed_us = now_us - timer->last_us;
    timer->last_us = now_us;

    timer->phase += elapsed_us * WM_N64_SOURCE_HZ;
    const unsigned due = (unsigned)(timer->phase / WM_N64_USEC_PER_SEC);
    timer->phase %= WM_N64_USEC_PER_SEC;
    return due;
}

static void wm_n64_run_source_ticks_dual(
    wm_app *app,
    wm_n64_source_timer *timer,
    const wm_input_state *sampled_p1,
    const wm_input_state *sampled_p2) {
    if (!app || !timer || !sampled_p1 || !sampled_p2)
        return;

    wm_n64_latch_source_input(&timer->latched_input[0], sampled_p1);
    wm_n64_latch_source_input(&timer->latched_input[1], sampled_p2);

    const unsigned due = wm_n64_source_ticks_due(timer, get_ticks_us());
    if (!due)
        return;

    wm_input_state tick_input[2] = {
        timer->latched_input[0],
        timer->latched_input[1]
    };
    memset(timer->latched_input, 0, sizeof(timer->latched_input));

    for (unsigned i = 0; i < due; ++i) {
        wm_app_tick_dual(app, &tick_input[0], &tick_input[1]);

        for (unsigned p = 0; p < 2u; ++p) {
            tick_input[p].start = false;
            tick_input[p].light_punch = false;
            tick_input[p].power_punch = false;
            tick_input[p].light_kick = false;
            tick_input[p].power_kick = false;
            tick_input[p].l = false;
            tick_input[p].z = false;
            tick_input[p].b = false;
        }

        tick_input[0].run = sampled_p1->run;
        tick_input[0].block = sampled_p1->block;
        tick_input[0].stick_x = sampled_p1->stick_x;
        tick_input[0].stick_y = sampled_p1->stick_y;

        tick_input[1].run = sampled_p2->run;
        tick_input[1].block = sampled_p2->block;
        tick_input[1].stick_x = sampled_p2->stick_x;
        tick_input[1].stick_y = sampled_p2->stick_y;
    }
}

int main(void) {
    wm_app app;
    wm_n64_source_timer source_timer = {0};

    debug_init_emulog();
    debug_init_usblog();
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2,
                 GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    rdpq_debug_start();
    joypad_init();
    dfs_init(DFS_DEFAULT_LOCATION);
    wm_n64_audio_init();
    sports_background_cache_init();
    rdpq_text_register_font(1, rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_VAR));

    wm_app_init(&app);
    source_timer.last_us = get_ticks_us();
    debugf("wm_arcade_port r9: 53 Hz arcade wall clock, render-rate independent\n");
    debugf("embedded Bret source sprites: %u\n", (unsigned)wm_bret_sprite_count());
    debugf("embedded Midway Sports logo pieces: %u\n", (unsigned)wm_sports_logo_sprite_count());
    debugf("embedded source title background: %s (%lu blocks, %lu palettes)\n",
           wm_title_background_source_name(),
           (unsigned long)wm_title_background_image_count(),
           (unsigned long)wm_title_background_palette_count());
    debugf("embedded packed BMOD modules: %u\n", (unsigned)wm_source_bmod_count());
    debugf("ATTRACT.ASM preserves initial 8 source-tick blank; harness-only gameplay excluded\n");
    debugf("controls: A run, C-L LP, C-U PP, C-R LK, C-D PK, R block\n");
    debugf("Fix36: Controller 2 Start buys P2 into SELECT.ASM\n");

    while (1) {
        joypad_poll();

        bool connected[2] = {false, false};
        wm_input_state input[2] = {
            read_input(JOYPAD_PORT_1, &connected[0]),
            read_input(JOYPAD_PORT_2, &connected[1])
        };

        wm_n64_run_source_ticks_dual(&app, &source_timer,
                                     &input[0], &input[1]);
        wm_n64_audio_service(&app);
        (void)connected;
        render_app(&app);
    }
}
