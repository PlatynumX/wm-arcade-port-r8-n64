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

static wm_input_state read_input(bool *connected) {
    wm_input_state out = {0};
    joypad_poll();
    *connected = joypad_is_connected(JOYPAD_PORT_1);
    if (!*connected)
        return out;

    const joypad_inputs_t in = joypad_get_inputs(JOYPAD_PORT_1);
    const joypad_buttons_t now = joypad_get_buttons(JOYPAD_PORT_1);
    const joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    out.stick_x = in.stick_x;
    out.stick_y = in.stick_y;
    if (out.stick_x >= -STICK_DEADZONE && out.stick_x <= STICK_DEADZONE &&
        out.stick_y >= -STICK_DEADZONE && out.stick_y <= STICK_DEADZONE) {
        if (now.d_left)  out.stick_x = -90;
        if (now.d_right) out.stick_x =  90;
        if (now.d_down)  out.stick_y = -90;
        if (now.d_up)    out.stick_y =  90;
    }

    /* User-selected N64 layout. Keep the portable core named after the
       arcade actions rather than after N64 physical buttons. */
    out.run         = now.a;
    out.light_punch = pressed.c_left;
    out.power_punch = pressed.c_up;
    out.light_kick  = pressed.c_right;
    out.power_kick  = pressed.c_down;
    out.block       = now.r;

    out.start = pressed.start;
    out.l = pressed.l;
    out.z = pressed.z;
    out.b = pressed.b; /* reserved; intentionally not required by gameplay */
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

    switch (app->attract.call) {
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
    wm_input_state latched_input;
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

static void wm_n64_run_source_ticks(wm_app *app,
                                    wm_n64_source_timer *timer,
                                    const wm_input_state *sampled_input) {
    if (!app || !timer || !sampled_input) return;

    wm_n64_latch_source_input(&timer->latched_input, sampled_input);

    const unsigned due = wm_n64_source_ticks_due(timer, get_ticks_us());
    if (!due) return;

    wm_input_state tick_input = timer->latched_input;
    memset(&timer->latched_input, 0, sizeof(timer->latched_input));

    for (unsigned i = 0; i < due; ++i) {
        wm_app_tick(app, &tick_input);

        tick_input.start = false;
        tick_input.light_punch = false;
        tick_input.power_punch = false;
        tick_input.light_kick = false;
        tick_input.power_kick = false;
        tick_input.l = false;
        tick_input.z = false;
        tick_input.b = false;

        tick_input.run = sampled_input->run;
        tick_input.block = sampled_input->block;
        tick_input.stick_x = sampled_input->stick_x;
        tick_input.stick_y = sampled_input->stick_y;
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

    while (1) {
        bool connected = false;
        wm_input_state input = read_input(&connected);
        wm_n64_run_source_ticks(&app, &source_timer, &input);
        wm_n64_audio_service(&app);
        (void)connected;
        render_app(&app);
    }
}
