#include "wm_arcade_completion.h"

#include <limits.h>
#include <string.h>

#define Q16_ONE 65536L

static const wm_fw_cleanup_process cleanup_processes[] = {
    WM_FW_PROC_ANNC,
    WM_FW_PROC_METER,
    WM_FW_PROC_TIMER,
    WM_FW_PROC_FLASH,
    WM_FW_PROC_ICON,
    WM_FW_PROC_SPECIAL_MOVE,
    WM_FW_PROC_PINHIM_ANIM,
    WM_FW_PROC_REWIRE,
    WM_FW_PROC_ZSHIFT,
    WM_FW_PROC_GETUP,
    WM_FW_PROC_FLASH_COMBO_P1,
    WM_FW_PROC_FLASH_COMBO_P2,
    WM_FW_PROC_CYCLER,
    WM_FW_PROC_FX,
    WM_FW_PROC_ADD_INITIALS,
    WM_FW_PROC_OVERHEAD
};

static const wm_fw_cleanup_object cleanup_objects[] = {
    WM_FW_OBJ_ANNOUNCER_TEXT,
    WM_FW_OBJ_METER_FRAME,
    WM_FW_OBJ_METER_BAR,
    WM_FW_OBJ_TIMER_DIGIT,
    WM_FW_OBJ_WWF_ICON,
    WM_FW_OBJ_WINSTREAK,
    WM_FW_OBJ_PIN_HIM,
    WM_FW_OBJ_PERFECT
};

static const wm_fw_flare_position flare_positions[] = {
    { 798, 128 }, { 848, 128 }, { 898, 128 }, { 948, 128 },
    { 998, 128 }, {1048, 128 }, {1098, 128 }, {1148, 128 },
    {1198, 128 }, {1248, 128 }, {1298, 128 }, {1348, 128 },
    { 770, 148 }, { 753, 168 }, { 736, 188 }, { 719, 208 }, { 702, 228 },
    {1372, 148 }, {1389, 168 }, {1406, 188 }, {1423, 208 }, {1440, 228 }
};

/* FIREWORK.ASM panning_points: the exact source-space figure-eight path.
 * The first segment uses INIT_PAN_SPEED (TSEC=60); all remaining table
 * segments use 3 ticks. */
static const wm_fw_pan_point pan_points[] = {
    {60,  850, -260},
    { 3,  848, -270}, { 3,  844, -280}, { 3,  836, -288},
    { 3,  825, -296}, { 3,  812, -302}, { 3,  798, -306},
    { 3,  783, -308}, { 3,  767, -308}, { 3,  752, -306},
    { 3,  738, -302}, { 3,  725, -296}, { 3,  714, -288},
    { 3,  706, -280}, { 3,  702, -270}, { 3,  700, -260},
    { 3,  702, -250}, { 3,  706, -240}, { 3,  714, -232},
    { 3,  725, -224}, { 3,  737, -218}, { 3,  752, -214},
    { 3,  767, -212}, { 3,  783, -212}, { 3,  798, -214},
    { 3,  812, -218}, { 3,  825, -224}, { 3,  836, -232},
    { 3,  844, -240}, { 3,  848, -250}, { 3,  850, -260},
    { 3,  852, -270}, { 3,  856, -280}, { 3,  864, -288},
    { 3,  875, -296}, { 3,  888, -302}, { 3,  902, -306},
    { 3,  917, -308}, { 3,  933, -308}, { 3,  948, -306},
    { 3,  962, -302}, { 3,  975, -296}, { 3,  986, -288},
    { 3,  994, -280}, { 3,  998, -270}, { 3, 1000, -260},
    { 3,  998, -250}, { 3,  994, -240}, { 3,  986, -232},
    { 3,  975, -224}, { 3,  963, -218}, { 3,  948, -214},
    { 3,  933, -212}, { 3,  917, -212}, { 3,  902, -214},
    { 3,  888, -218}, { 3,  875, -224}, { 3,  864, -232},
    { 3,  856, -240}, { 3,  852, -250}, { 3,  850, -260}
};

static uint32_t rand_inclusive(WmCompletionRngFn rng, void *user, uint32_t maxv) {
    if (!rng) return 0u;
    return rng(user, maxv) % (maxv + 1u);
}

static int32_t q16_from_int(int16_t v) {
    return (int32_t)((int64_t)v * (int64_t)Q16_ONE);
}

/* TMS34010 SRA 16 semantics: signed arithmetic shift, i.e. floor for
 * negative fractional values rather than C99 division's truncation-to-zero. */
static int32_t q16_arshift16(int32_t v) {
    if (v >= 0) return v / (int32_t)Q16_ONE;
    return -(int32_t)(((int64_t)(-(int64_t)v) + (Q16_ONE - 1)) / Q16_ONE);
}

static int32_t abs32(int32_t v) {
    if (v == INT32_MIN) return INT32_MAX;
    return v < 0 ? -v : v;
}

size_t wm_firework_cleanup_process_count(void) {
    return sizeof(cleanup_processes) / sizeof(cleanup_processes[0]);
}

bool wm_firework_cleanup_process_get(size_t index, wm_fw_cleanup_process *out) {
    if (!out || index >= wm_firework_cleanup_process_count()) return false;
    *out = cleanup_processes[index];
    return true;
}

size_t wm_firework_cleanup_object_count(void) {
    return sizeof(cleanup_objects) / sizeof(cleanup_objects[0]);
}

bool wm_firework_cleanup_object_get(size_t index, wm_fw_cleanup_object *out) {
    if (!out || index >= wm_firework_cleanup_object_count()) return false;
    *out = cleanup_objects[index];
    return true;
}

size_t wm_firework_flare_count(void) {
    return sizeof(flare_positions) / sizeof(flare_positions[0]);
}

bool wm_firework_flare_get(size_t index, wm_fw_flare_position *out) {
    if (!out || index >= wm_firework_flare_count()) return false;
    *out = flare_positions[index];
    return true;
}

size_t wm_firework_pan_point_count(void) {
    return sizeof(pan_points) / sizeof(pan_points[0]);
}

bool wm_firework_pan_point_get(size_t index, wm_fw_pan_point *out) {
    if (!out || index >= wm_firework_pan_point_count()) return false;
    *out = pan_points[index];
    return true;
}

uint16_t wm_firework_next_explosion_delay(WmCompletionRngFn rng, void *rng_user) {
    /* do_exfw_loop: RNDRNG0(6)+1, then subtract that exact sleep from the
     * six-second countdown. */
    return (uint16_t)(rand_inclusive(rng, rng_user, 6u) + 1u);
}

void wm_firework_make_explosion(WmCompletionRngFn rng, void *rng_user,
                                wm_fw_explosion *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));

    /* animate_fwexp consumes RNG in this exact order. */
    out->y = (int16_t)((int32_t)rand_inclusive(rng, rng_user, 96u) +
                       WM_FW_EXPLOSION_BASE_Y - 48);
    out->x = (int16_t)(850u + rand_inclusive(rng, rng_user, 350u));
    out->z = (uint16_t)(0x700u + rand_inclusive(rng, rng_user, 0x200u));
    out->animation_variant = (uint8_t)rand_inclusive(rng, rng_user, 1u);
    out->palette_index = (uint8_t)rand_inclusive(rng, rng_user, 4u);
    out->sound_command = WM_FW_EXPLOSION_SOUND;
}

wm_fw_congrats_mode wm_firework_congrats_mode(bool royal_rumble,
                                               bool eight_on_one) {
    if (royal_rumble) return WM_FW_CONGRATS_2V8;
    if (eight_on_one) return WM_FW_CONGRATS_1V8;
    return WM_FW_CONGRATS_1V3;
}

void wm_firework_camera_segment_init(int32_t start_x_q16,
                                     int32_t start_y_q16,
                                     const wm_fw_pan_point *point,
                                     wm_fw_camera_segment *out) {
    int32_t ticks;
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!point || point->ticks == 0u) return;

    out->target_x = point->x;
    out->target_y = point->y;
    out->target_x_q16 = q16_from_int(point->x);
    out->target_y_q16 = q16_from_int(point->y);
    ticks = (int32_t)point->ticks;

    /* calc_dxdy uses signed division on the 16.16 differences. C99 signed
     * division truncates toward zero, matching the intended DIVS behavior. */
    out->dx_q16 = (out->target_x_q16 - start_x_q16) / ticks;
    out->dy_q16 = (out->target_y_q16 - start_y_q16) / ticks;
}

bool wm_firework_camera_step(wm_fw_camera_segment *segment,
                             int32_t *world_x_q16,
                             int32_t *world_y_q16) {
    int32_t xi, yi;
    if (!segment || !world_x_q16 || !world_y_q16) return true;

    *world_x_q16 += segment->dx_q16;
    *world_y_q16 += segment->dy_q16;

    xi = q16_arshift16(*world_x_q16);
    yi = q16_arshift16(*world_y_q16);

    if (abs32(xi - segment->target_x) <= WM_FW_CAMERA_TOLERANCE)
        segment->dx_q16 = 0;
    if (abs32(yi - segment->target_y) <= WM_FW_CAMERA_TOLERANCE)
        segment->dy_q16 = 0;

    return segment->dx_q16 == 0 && segment->dy_q16 == 0;
}

static void fw_append(wm_fireworks_plan *p, wm_fireworks_step step) {
    if (p->step_count < WM_FIREWORKS_MAX_STEPS)
        p->steps[p->step_count++] = step;
}

void wm_fireworks_build_plan(wm_fireworks_plan *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));

    out->fade_down_steps = 32u;
    out->fade_up_steps = 64u;
    out->fade_down_wait_ticks = 30u;
    out->pre_pan_wait_ticks = WM_ARCADE_TICKS_PER_SECOND;
    out->explosion_window_ticks = WM_FW_EXPLOSION_WINDOW_TICKS;
    out->cheer_window_ticks = WM_FW_CHEER_WINDOW_TICKS;
    out->cheer_interval_ticks = WM_FW_CHEER_INTERVAL_TICKS;
    out->final_fizzle_wait_ticks = WM_FW_FIZZLE_WAIT_TICKS;
    out->flare_sound_command = WM_FW_FLARE_SOUND;
    out->explosion_sound_command = WM_FW_EXPLOSION_SOUND;

    fw_append(out, WM_FW_PAL_CLEAN);
    fw_append(out, WM_FW_FADE_DOWN_32);
    fw_append(out, WM_FW_WAIT_30);
    fw_append(out, WM_FW_KILL_MATCH_UI_PROCESSES);
    fw_append(out, WM_FW_KNOCKOUT_DRONES);
    fw_append(out, WM_FW_DELETE_MATCH_UI_OBJECTS);
    fw_append(out, WM_FW_DISABLE_BOG_REDUCTION);
    fw_append(out, WM_FW_WAKE_CROWD);
    fw_append(out, WM_FW_PAL_CLEAN_AGAIN);
    fw_append(out, WM_FW_FLASH_WHITE);
    fw_append(out, WM_FW_PLAY_FLARE_SOUND);
    fw_append(out, WM_FW_SPAWN_22_FLARES);
    fw_append(out, WM_FW_FADE_UP_64);
    fw_append(out, WM_FW_WAIT_16);
    fw_append(out, WM_FW_FLASH_WHITE_AGAIN);
    fw_append(out, WM_FW_PLAY_FLARE_SOUND_AGAIN);
    fw_append(out, WM_FW_WAIT_60);
    fw_append(out, WM_FW_START_PAN_PROCESS);
    fw_append(out, WM_FW_EXPLOSIONS_FOR_360_TICKS);
    fw_append(out, WM_FW_SET_PAN_DOWN);
    fw_append(out, WM_FW_CHEER_FOR_120_TICKS);
    fw_append(out, WM_FW_SET_FLARES_TO_FIZZLE);
    fw_append(out, WM_FW_WAIT_FINAL_60);
}
