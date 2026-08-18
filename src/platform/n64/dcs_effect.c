#include <libdragon.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dcs_effect.h"

/* ATTR.ASM walks dcslogo as a 186-pixel-wide 5-bit WIMP image.
   The two setup passes prove an 88-row source extent:
     ROT:   31 columns * 6 px = 186, 44 rows * 2 px = 88
     BURST: 46 columns * 4 px + 2 = 186, 21 base rows * 4 with y+6 sampling.
   Keep all particle math in the original 400x256 arcade coordinate space and
   apply the N64 320x240 transform only when the completed CI8 canvas is blit. */
#define DCS_ARCADE_W 400
#define DCS_ARCADE_H 256
#define DCS_SRC_W 186
#define DCS_SRC_H 88

#define DCS_SCALE_X (320.0f / 400.0f)
#define DCS_SCALE_Y (240.0f / 256.0f)

#define DCS_ROT_COLS 31
#define DCS_ROT_ROWS 44
#define DCS_ROT_COUNT (DCS_ROT_COLS * DCS_ROT_ROWS)
#define DCS_ROT_DST_X 107
#define DCS_ROT_DST_Y 76
#define DCS_ROT_X_STEP 6
#define DCS_ROT_Y_STEP 2
#define DCS_ROT_BLOCK_W 4
#define DCS_ROT_BLOCK_H 2
#define DCS_ROT_RNG_MAX 0x32000

#define DCS_BURST_COLS 46
#define DCS_BURST_ROWS 21
#define DCS_BURST_COUNT (DCS_BURST_COLS * DCS_BURST_ROWS)
#define DCS_BURST_DST_X_BITS 0x0358
#define DCS_BURST_DST_Y_BITS 0x4C000
#define DCS_BURST_X_STEP_BITS 0x20
#define DCS_BURST_Y_STEP_BITS 0x4000
#define DCS_BURST_X_RNG_MAX 0x0F
#define DCS_BURST_Y_RNG_MAX 0x2100
#define DCS_BURST_GRAVITY 0x01F0
#define DCS_BURST_FLOOR_BITS 0xFA000
#define DCS_BURST_BLOCK_W 4
#define DCS_BURST_BLOCK_H 3

static uint8_t dcs_fx_canvas[DCS_ARCADE_W * DCS_ARCADE_H] __attribute__((aligned(8)));

typedef struct {
    int32_t x_bits;
    int32_t y_bits;
    int32_t x_vel;
    int32_t y_vel;
    bool visible;
} dcs_burst_particle;

static dcs_burst_particle dcs_burst[DCS_BURST_COUNT];
static uint32_t dcs_burst_seed;
static unsigned dcs_burst_tick;
static bool dcs_burst_ready;

static uint32_t dcs_mix32(uint32_t x) {
    /* Portable entropy source only. RNDRNGS on Wolf Unit mixes live TMS state,
       so exact velocities are intentionally non-deterministic on the arcade.
       This hash supplies stable per-particle entropy while preserving the
       recovered RNDRNGS signed range and all source motion equations. */
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return x;
}

static int32_t dcs_signed_random(uint32_t seed, uint32_t key, uint32_t maximum) {
    const uint32_t r = dcs_mix32(seed ^ (key * 0x9E3779B9u));
    const uint64_t span = (uint64_t)maximum * 2u + 1u;
    const uint32_t scaled = (uint32_t)(((uint64_t)r * span) >> 32);
    return (int32_t)scaled - (int32_t)maximum;
}

static int32_t dcs_sra7(int32_t v) {
    /* TMS34010 SRA is an arithmetic shift. Spell out the negative case rather
       than depending on implementation-defined signed right-shift behavior. */
    if (v >= 0)
        return v >> 7;
    return -(int32_t)(((uint32_t)(-((int64_t)v)) + 127u) >> 7);
}

static int dcs_floor_div_65536(int64_t v) {
    if (v >= 0)
        return (int)(v >> 16);
    return -(int)(((-v) + 0xFFFF) >> 16);
}

static int dcs_floor_div_bits(int32_t v, int divisor) {
    if (v >= 0)
        return v / divisor;
    return -(int)(((-(int64_t)v) + divisor - 1) / divisor);
}

static uint32_t dcs_effect_seed(const wm_app *app) {
    /* The original RNDRNGS input changes with live machine/process state.
       amode_loops is stable for one DCS invocation but changes on later passes,
       giving each attract loop a different deterministic N64 realization. */
    return 0x44435331u ^ ((uint32_t)app->attract.amode_loops * 0xA511E9B3u);
}

static bool dcs_source_valid(const wm_source_sprite *spr) {
    return spr && spr->pixels_ci8 && spr->palette_rgba5551 &&
           spr->palette_colors && spr->width >= DCS_SRC_W &&
           spr->height >= DCS_SRC_H;
}

static uint8_t dcs_src(const wm_source_sprite *spr, int x, int y) {
    if (!spr || x < 0 || y < 0 || x >= spr->width || y >= spr->height)
        return 0;
    return spr->pixels_ci8[(size_t)y * spr->width + (size_t)x];
}

static void dcs_plot(int x, int y, uint8_t pixel) {
    if ((unsigned)x >= DCS_ARCADE_W || (unsigned)y >= DCS_ARCADE_H)
        return;
    dcs_fx_canvas[(size_t)y * DCS_ARCADE_W + (size_t)x] = pixel;
}

static void dcs_copy_rot_block(const wm_source_sprite *spr,
                               int sx, int sy, int dx, int dy) {
    for (int y = 0; y < DCS_ROT_BLOCK_H; ++y)
        for (int x = 0; x < DCS_ROT_BLOCK_W; ++x)
            dcs_plot(dx + x, dy + y, dcs_src(spr, sx + x, sy + y));
}

static void dcs_copy_burst_block(const wm_source_sprite *spr,
                                 int sx, int sy, int dx, int dy) {
    /* SETUP_ALL_PIX1 samples rows base and base+3; SETUP_ALL_PIX2 samples
       base+6. The three packed 32-bit values are written to adjacent output
       scanlines by ADD_PIXEL_VEL. */
    static const int source_row_delta[DCS_BURST_BLOCK_H] = {0, 3, 6};
    for (int y = 0; y < DCS_BURST_BLOCK_H; ++y)
        for (int x = 0; x < DCS_BURST_BLOCK_W; ++x)
            dcs_plot(dx + x, dy + y,
                     dcs_src(spr, sx + x, sy + source_row_delta[y]));
}

static void dcs_draw_canvas(const wm_source_sprite *spr) {
    surface_t tex = surface_make_linear((void *)dcs_fx_canvas, FMT_CI8,
                                        DCS_ARCADE_W, DCS_ARCADE_H);
    rdpq_set_mode_standard();
    rdpq_mode_tlut(TLUT_RGBA16);
    rdpq_mode_filter(FILTER_POINT);
    rdpq_mode_alphacompare(1);
    rdpq_tex_upload_tlut(spr->palette_rgba5551, 0, spr->palette_colors);

    /* CI8 + TLUT leaves 2 KiB texture TMEM, matching the existing hardware-
       tested frontend path. 400-byte rows therefore use four-row strips. */
    const int pitch = (DCS_ARCADE_W + 7) & ~7;
    int strip_h = pitch > 0 ? 2048 / pitch : 1;
    if (strip_h < 1) strip_h = 1;
    if (strip_h > 2) strip_h &= ~1;

    for (int t = 0; t < DCS_ARCADE_H; t += strip_h) {
        int h = DCS_ARCADE_H - t;
        if (h > strip_h) h = strip_h;
        rdpq_tex_blit(&tex, 0.0f, (float)t * DCS_SCALE_Y,
                      &(rdpq_blitparms_t){
                          .t0 = t,
                          .height = h,
                          .scale_x = DCS_SCALE_X,
                          .scale_y = DCS_SCALE_Y,
                          .filtering = false,
                      });
    }
}

static unsigned dcs_rot_tick(const wm_app *app) {
    switch (app->attract.dcs_phase) {
        case WM_DCS_ROT_UNSKIPPABLE:
            return app->attract.phase_ticks + 1u;
        case WM_DCS_ROT_SKIPPABLE:
            return WM_DCS_ROT_UNSKIPPABLE_TICKS +
                   app->attract.phase_ticks + 1u;
        default:
            return 0u;
    }
}

static void dcs_render_rot(const wm_app *app, const wm_source_sprite *spr) {
    const unsigned tick = dcs_rot_tick(app);
    const uint32_t seed = dcs_effect_seed(app);
    memset(dcs_fx_canvas, 0, sizeof(dcs_fx_canvas));

    /* ADD_PIXEL_ROT:
         xvel += xacc
         xpos += xvel
       where xacc = -(RNDRNGS(0x32000) SRA 7).
       The closed form below is exactly those two updates repeated 'tick' times. */
    for (int row = 0; row < DCS_ROT_ROWS; ++row) {
        for (int col = 0; col < DCS_ROT_COLS; ++col) {
            const unsigned index = (unsigned)(row * DCS_ROT_COLS + col);
            const int32_t initial_vel =
                dcs_signed_random(seed, 0x10000u + index, DCS_ROT_RNG_MAX);
            const int32_t accel = -dcs_sra7(initial_vel);
            const int64_t initial_x =
                (int64_t)(DCS_ROT_DST_X + col * DCS_ROT_X_STEP) << 16;
            const int64_t n = tick;
            const int64_t xpos = initial_x +
                n * (int64_t)initial_vel +
                (int64_t)accel * n * (n + 1) / 2;
            const int dx = dcs_floor_div_65536(xpos);
            const int dy = DCS_ROT_DST_Y + row * DCS_ROT_Y_STEP;
            const int sx = col * DCS_ROT_X_STEP;
            const int sy = row * DCS_ROT_Y_STEP;
            dcs_copy_rot_block(spr, sx, sy, dx, dy);
        }
    }
    dcs_draw_canvas(spr);
}

static void dcs_burst_reset(const wm_app *app) {
    const uint32_t seed = dcs_effect_seed(app);
    for (int row = 0; row < DCS_BURST_ROWS; ++row) {
        for (int col = 0; col < DCS_BURST_COLS; ++col) {
            const unsigned index = (unsigned)(row * DCS_BURST_COLS + col);
            dcs_burst_particle *p = &dcs_burst[index];
            p->x_bits = DCS_BURST_DST_X_BITS + col * DCS_BURST_X_STEP_BITS;
            p->y_bits = DCS_BURST_DST_Y_BITS + row * DCS_BURST_Y_STEP_BITS;
            p->x_vel = dcs_signed_random(seed, 0x20000u + index,
                                         DCS_BURST_X_RNG_MAX);
            p->y_vel = dcs_signed_random(seed, 0x30000u + index,
                                         DCS_BURST_Y_RNG_MAX) * 2;
            p->visible = false;
        }
    }
    dcs_burst_seed = seed;
    dcs_burst_tick = 0;
    dcs_burst_ready = true;
}

static void dcs_burst_step(void) {
    for (unsigned i = 0; i < DCS_BURST_COUNT; ++i) {
        dcs_burst_particle *p = &dcs_burst[i];
        const int32_t new_y_vel = p->y_vel + DCS_BURST_GRAVITY;

        /* Source JRZ NO_PLOT_ANYTHING occurs before the new velocity is written
           back, so the record remains unchanged on this rare exact-zero case. */
        if (new_y_vel == 0) {
            p->visible = false;
            continue;
        }

        p->y_vel = new_y_vel;
        p->y_bits += p->y_vel;

        if (p->y_bits >= DCS_BURST_FLOOR_BITS) {
            /* NO_WORRY_ABOUT_Y_BOUNCE stores -gravity and skips the plot for
               this tick; it does not reflect the incoming velocity. */
            p->y_vel = -DCS_BURST_GRAVITY;
            p->visible = false;
            continue;
        }

        p->x_bits += p->x_vel;
        p->visible = true;
    }
}

static unsigned dcs_burst_target_tick(const wm_app *app) {
    switch (app->attract.dcs_phase) {
        case WM_DCS_BURST_FLASH:
            return app->attract.phase_ticks + 1u;
        case WM_DCS_BURST_WAIT:
            return WM_DCS_BURST_FLASH_TICKS +
                   app->attract.phase_ticks + 1u;
        case WM_DCS_BURST_SKIPPABLE:
            return WM_DCS_BURST_FLASH_TICKS +
                   WM_DCS_BURST_WAIT_TICKS +
                   app->attract.phase_ticks + 1u;
        default:
            return 0u;
    }
}

static void dcs_render_burst(const wm_app *app, const wm_source_sprite *spr) {
    const uint32_t seed = dcs_effect_seed(app);
    const unsigned target = dcs_burst_target_tick(app);

    if (!dcs_burst_ready || dcs_burst_seed != seed || target < dcs_burst_tick)
        dcs_burst_reset(app);
    while (dcs_burst_tick < target) {
        dcs_burst_step();
        ++dcs_burst_tick;
    }

    memset(dcs_fx_canvas, 0, sizeof(dcs_fx_canvas));
    for (int row = 0; row < DCS_BURST_ROWS; ++row) {
        for (int col = 0; col < DCS_BURST_COLS; ++col) {
            const unsigned index = (unsigned)(row * DCS_BURST_COLS + col);
            const dcs_burst_particle *p = &dcs_burst[index];
            if (!p->visible)
                continue;

            const int dx = dcs_floor_div_bits(p->x_bits, 8);
            const int dy = dcs_floor_div_bits(p->y_bits, 0x1000);
            const int sx = col * 4;
            const int sy = row * 4;
            dcs_copy_burst_block(spr, sx, sy, dx, dy);
        }
    }
    dcs_draw_canvas(spr);

    /* ATTR.ASM sets IRQSKYE white BEFORE each of the three one-tick sleeps:
       white, normal, white, normal, white, normal. With phase tick zero being
       the first rendered flash frame, white therefore occurs on 0/2/4. */
    if (app->attract.dcs_phase == WM_DCS_BURST_FLASH &&
        (app->attract.phase_ticks & 1u) == 0u) {
        rdpq_set_mode_fill(RGBA32(255, 255, 255, 255));
        rdpq_fill_rectangle(0, 0, 320, 240);
    }
}

void wm_n64_render_dcs_effect(const wm_app *app, const wm_source_sprite *spr) {
    if (!app || !dcs_source_valid(spr))
        return;

    switch (app->attract.dcs_phase) {
        case WM_DCS_ROT_UNSKIPPABLE:
        case WM_DCS_ROT_SKIPPABLE:
            dcs_render_rot(app, spr);
            break;
        case WM_DCS_BURST_FLASH:
        case WM_DCS_BURST_WAIT:
        case WM_DCS_BURST_SKIPPABLE:
            dcs_render_burst(app, spr);
            break;
        default:
            break;
    }
}
