#include <libdragon.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "wm/app.h"
#include "wm/bmod.h"
#include "wm/bret_sprites.h"
#include "wm/composite.h"
#include "wm/demo.h"
#include "wm/dcs_logo.h"
#include "wm/roster.h"
#include "wm/sports_logo.h"
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

    const wm_dcs_phase phase = app->attract.dcs_phase;
    if (phase == WM_DCS_BURST_FLASH) {
        /* DCS_LOGO toggles IRQSKYE white/normal for one tick each, three times.
           The ADD_PIXEL_VEL particles themselves are not substituted here; that
           hardware plotter is a separate source translation. */
        if (app->attract.phase_ticks & 1u)
            fill_rect(0, 0, 320, 240, RGBA32(255, 255, 255, 255));
        return;
    }

    /* The source object is visible before ADD_PIXEL_ROT and for the ten-tick
       redisplay between the two pixel effects.  During the source pixel effects
       the original object has M_NODISP/deletion set, so do not fake a logo. */
    if (phase != WM_DCS_STATIC && phase != WM_DCS_ROT_SETUP &&
        phase != WM_DCS_STATIC_RETURN)
        return;

    const wm_source_sprite *spr = wm_dcs_logo_sprite();
    if (!spr) return;
    draw_source_sprite_scaled(spr,
                              200.0f * WM_FRONTEND_SCALE_X,
                              120.0f * WM_FRONTEND_SCALE_Y,
                              spr->xani, spr->yani, false, spr,
                              WM_FRONTEND_SCALE_X, WM_FRONTEND_SCALE_Y);
}

static void render_midway_sports(const wm_app *app) {
    fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));

    /* show_sports_logo does not unblank until after SLEEPK 2, object creation,
       and SLEEPK 1. */
    if (app->attract.call_ticks < WM_SPORTS_LOGO_VISIBLE_TICK)
        return;

    /* show_sports_logo creates all 17 BEGINOBJ objects at one arcade anchor.
       Their individual WIMP hotspots position each piece; preserving those
       offsets keeps the segmented Midway Sports artwork assembled. */
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

    /* app->attract.sports_world_x/y already execute the original X-=2/Y+=2
       process timing. SPORTBKBMOD and osgmd8 are deliberately not replaced by
       made-up artwork; their exact source renderers remain to be translated. */
}


static const wm_source_sprite *title_sparkle_frame(const wm_title_sparkle *sp) {
    if (!sp || !sp->active) return NULL;

    size_t base = 0;
    unsigned frames = 0;
    switch (sp->family) {
        case 0: base = WM_TITLE_SPARKLE_BIG_A_BASE; frames = WM_TITLE_BIG_SPARKLE_FRAMES; break;
        case 1: base = WM_TITLE_SPARKLE_BIG_B_BASE; frames = WM_TITLE_BIG_SPARKLE_FRAMES; break;
        case 2: base = WM_TITLE_SPARKLE_SMALL_A_BASE; frames = WM_TITLE_SMALL_SPARKLE_FRAMES; break;
        case 3: base = WM_TITLE_SPARKLE_SMALL_B_BASE; frames = WM_TITLE_SMALL_SPARKLE_FRAMES; break;
        case 4: base = WM_TITLE_SPARKLE_SMALL_C_BASE; frames = WM_TITLE_SMALL_SPARKLE_FRAMES; break;
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

    /* BAKGND.ASM inserts blocks into the background object list by OZPOS.
       Execute that depth ordering here while keeping source order stable for
       equal-Z blocks. The packed records themselves remain verbatim. */
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
        while (j > 0 && refs[j - 1].block.z > key.block.z) {
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

    /* SPARKLE.IMG is the original WIMP artwork named by IMG/MISC.LOD.
       ATTRACT.ASM starts both FLASH_PID/SPRINKLE_GLINTS and
       ATTRACT_ANIMPID/RANDOM_SPARKLE at this same title boundary; the portable
       core owns their state and lifetime, while this backend only plots it. */
    for (size_t i = 0; i < WM_TITLE_GLINT_COUNT; ++i)
        draw_title_sparkle(&app->attract.title_glints[i]);
    draw_title_sparkle(&app->attract.title_random_sparkle);

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

int main(void) {
    wm_app app;

    debug_init_emulog();
    debug_init_usblog();
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2,
                 GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    rdpq_debug_start();
    joypad_init();
    rdpq_text_register_font(1, rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_VAR));

    wm_app_init(&app);
    debugf("wm_arcade_port r9: source-engine pass, 53 Hz arcade clock / 60 Hz video\n");
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
        (void)wm_app_video_frame(&app, &input);
        (void)connected;
        render_app(&app);
    }
}
