#include "wm/app.h"
#include <string.h>

static const wm_input_state no_input = {0};

bool wm_app_any_attract_button(const wm_input_state *input) {
    if (!input) return false;
    return input->start || input->run || input->light_punch ||
           input->power_punch || input->light_kick || input->power_kick ||
           input->block;
}

bool wm_attract_call_is_translated(wm_attract_call call) {
    wm_port_status status = wm_attract_call_port_status(call);
    return status == WM_PORT_PARTIAL_SOURCE || status == WM_PORT_EXACT_SOURCE;
}

static void cycle_lava_proc(wm_process *proc, void *user) {
    wm_app *app = (wm_app *)user;
    if (!proc || !app) return;
    /* The source process changes one palette link, then SLEEP 5. The first
       scheduler visit models process startup; its first source palette write
       occurs after that five-tick sleep boundary. */
    if (proc->state == 0) {
        proc->state = 1;
        wm_process_sleep(&app->scheduler, proc, WM_TITLE_LAVA_PERIOD_TICKS);
        return;
    }
    app->attract.title_lava_step =
        (app->attract.title_lava_step + 1u) % WM_TITLE_LAVA_STEPS;
    wm_process_sleep(&app->scheduler, proc, WM_TITLE_LAVA_PERIOD_TICKS);
}

/* The title source calls two shared Williams/Midway helpers that are only
   referenced, not defined, in the checked-in WWF tree:
       SPRINKLE_GLINTS( A8=[102,7], A9=4, A10=WHERE_WRESTLMANIA_SPARKLES )
       RANDOM_SPARKLE
   Their original SPARKLE.IMG frames are available, so preserve those frames,
   the CREATE/KIL1C lifetime and the four-glint count.  Until the shared helper
   module and WHERE_WRESTLMANIA_SPARKLES table are recovered, the placement
   sites/cadence below are explicitly a provisional behavior layer rather than
   a claim of source-exact helper internals. */
static const int16_t title_glint_sites[WM_TITLE_GLINT_COUNT][2] = {
    { 48, 132 }, { 145, 116 }, { 255, 118 }, { 352, 134 }
};

static const int16_t title_random_sites[][2] = {
    { 181,  52 }, { 221,  62 },
    {  73, 151 }, { 116, 127 }, { 166, 159 }, { 211, 127 },
    { 260, 154 }, { 306, 126 }, { 354, 153 },
    { 118, 190 }, { 210, 184 }, { 302, 190 },
};

static uint32_t title_random_next(wm_attract_state *a) {
    /* RANDOM_SPARKLE's RNG call path is in the missing shared helper. Keep a
       deterministic local stream so tests/replays are stable; replace this
       when that source body is recovered. */
    uint32_t x = a->title_random_state ? a->title_random_state : 0x57574631u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    a->title_random_state = x;
    return x;
}

static void reset_title_sparkles(wm_attract_state *a) {
    for (size_t i = 0; i < WM_TITLE_GLINT_COUNT; ++i) {
        a->title_glints[i].x = title_glint_sites[i][0];
        a->title_glints[i].y = title_glint_sites[i][1];
        a->title_glints[i].family = (uint8_t)(i & 1u); /* BSPRKA/B */
        a->title_glints[i].frame = (uint8_t)((i * 4u) % WM_TITLE_GLINT_ANIM_FRAMES);
        a->title_glints[i].active = false;
    }
    a->title_random_sparkle = (wm_title_sparkle){0};
    a->title_random_state = 0x57574631u;
}

static void sprinkle_glints_proc(wm_process *proc, void *user) {
    wm_app *app = (wm_app *)user;
    if (!proc || !app) return;
    wm_attract_state *a = &app->attract;

    if (proc->state == 0) {
        proc->state = 1;
        for (size_t i = 0; i < WM_TITLE_GLINT_COUNT; ++i)
            a->title_glints[i].active = true;
        wm_process_sleep(&app->scheduler, proc, WM_TITLE_SPARKLE_INFERRED_FRAME_TICKS);
        return;
    }

    for (size_t i = 0; i < WM_TITLE_GLINT_COUNT; ++i)
        a->title_glints[i].frame = (uint8_t)((a->title_glints[i].frame + 1u) % WM_TITLE_GLINT_ANIM_FRAMES);
    wm_process_sleep(&app->scheduler, proc, WM_TITLE_SPARKLE_INFERRED_FRAME_TICKS);
}

static void random_sparkle_proc(wm_process *proc, void *user) {
    wm_app *app = (wm_app *)user;
    if (!proc || !app) return;
    wm_attract_state *a = &app->attract;
    wm_title_sparkle *sp = &a->title_random_sparkle;

    if (proc->state == 0) {
        proc->state = 1;
        sp->active = false;
        wm_process_sleep(&app->scheduler, proc, 9u);
        return;
    }

    if (!sp->active) {
        uint32_t r = title_random_next(a);
        size_t site = r % (sizeof(title_random_sites) / sizeof(title_random_sites[0]));
        sp->x = title_random_sites[site][0];
        sp->y = title_random_sites[site][1];
        sp->family = (uint8_t)(2u + ((r >> 8) % 3u)); /* SPRKLA/B/C */
        sp->frame = 0;
        sp->active = true;
        wm_process_sleep(&app->scheduler, proc, WM_TITLE_SPARKLE_INFERRED_FRAME_TICKS);
        return;
    }

    if ((unsigned)sp->frame + 1u < WM_TITLE_RANDOM_ANIM_FRAMES) {
        ++sp->frame;
        wm_process_sleep(&app->scheduler, proc, WM_TITLE_SPARKLE_INFERRED_FRAME_TICKS);
        return;
    }

    sp->active = false;
    uint32_t r = title_random_next(a);
    wm_process_sleep(&app->scheduler, proc, 13u + (r % 18u));
}

static void move_back_off_screen_proc(wm_process *proc, void *user) {
    wm_app *app = (wm_app *)user;
    if (!proc || !app) return;
    /* CREATE begins a cooperative process; defer the first SLOOP body until
       the following source scheduler pass rather than recursively executing it
       from the creator. */
    if (proc->state == 0) {
        proc->state = 1;
        wm_process_sleep(&app->scheduler, proc, 1);
        return;
    }
    app->attract.sports_world_x -= 2;
    app->attract.sports_world_y += 2;
    wm_process_sleep(&app->scheduler, proc, 1);
}

static void kill_call_processes(wm_app *app, wm_attract_call call) {
    if (!app) return;
    if (call == WM_ATTRACT_SHOW_TITLE) {
        wm_process_kill_id(&app->scheduler, WM_PID_CYCLE_LAVA);
        wm_process_kill_id(&app->scheduler, WM_PID_FLASH);
        wm_process_kill_id(&app->scheduler, WM_PID_ATTRACT_ANIM);
    } else if (call == WM_ATTRACT_SHOW_SPORTS_LOGO) {
        wm_process_kill_id(&app->scheduler, WM_PID_WATER);
    }
}

static void begin_call(wm_app *app, wm_attract_call call) {
    wm_attract_state *a = &app->attract;
    a->call = call;
    a->call_ticks = 0;
    a->phase_ticks = 0;

    switch (call) {
        case WM_ATTRACT_DCS_LOGO:
            a->dcs_phase = WM_DCS_STATIC;
            break;
        case WM_ATTRACT_SHOW_SPORTS_LOGO:
            a->sports_world_x = 0;
            a->sports_world_y = 0;
            wm_process_kill_id(&app->scheduler, WM_PID_WATER);
            break;
        case WM_ATTRACT_SHOW_TITLE:
            a->title_lava_step = 0;
            reset_title_sparkles(a);
            wm_process_kill_id(&app->scheduler, WM_PID_CYCLE_LAVA);
            wm_process_kill_id(&app->scheduler, WM_PID_FLASH);
            wm_process_kill_id(&app->scheduler, WM_PID_ATTRACT_ANIM);
            break;
        default:
            break;
    }
}

static void begin_base_loop(wm_app *app) {
    app->attract.flow = WM_ATTRACT_FLOW_BASE;
    app->attract.source_index = 0;
    begin_call(app, wm_source_attract_loop[0]);
}

static void finish_base_loop(wm_app *app) {
    wm_attract_state *a = &app->attract;
    ++a->amode_loops;
    if (a->amode_loops & 1u) {
        begin_base_loop(app);
        return;
    }
    a->flow = WM_ATTRACT_FLOW_EVEN_CREDITS;
    begin_call(app, WM_ATTRACT_CREDITSCREEN);
}

static void advance_call(wm_app *app) {
    wm_attract_state *a = &app->attract;
    kill_call_processes(app, a->call);

    switch (a->flow) {
        case WM_ATTRACT_FLOW_BASE:
            ++a->source_index;
            if (a->source_index < wm_source_attract_loop_count) {
                begin_call(app, wm_source_attract_loop[a->source_index]);
            } else {
                finish_base_loop(app);
            }
            return;
        case WM_ATTRACT_FLOW_EVEN_CREDITS:
            a->flow = WM_ATTRACT_FLOW_TIME_DATE;
            begin_call(app, WM_ATTRACT_SHOW_TIME_DATE);
            return;
        case WM_ATTRACT_FLOW_TIME_DATE:
            if (a->amode_loops & 7u) begin_base_loop(app);
            else {
                a->flow = WM_ATTRACT_FLOW_COPYRIGHT;
                begin_call(app, WM_ATTRACT_SHOW_COPYRIGHT);
            }
            return;
        case WM_ATTRACT_FLOW_COPYRIGHT:
            a->flow = WM_ATTRACT_FLOW_AAMA;
            begin_call(app, WM_ATTRACT_AAMA_MESSAGE);
            return;
        case WM_ATTRACT_FLOW_AAMA:
            a->amode_loops = 0;
            begin_base_loop(app);
            return;
    }
}

static void skip_untranslated_calls(wm_app *app) {
    for (unsigned guard = 0; guard < 64 &&
         !wm_attract_call_is_translated(app->attract.call); ++guard)
        advance_call(app);
}

static void set_dcs_phase(wm_app *app, wm_dcs_phase phase) {
    app->attract.dcs_phase = phase;
    app->attract.phase_ticks = 0;
}

static bool tick_dcs_logo(wm_app *app, const wm_input_state *input) {
    wm_attract_state *a = &app->attract;
    ++a->call_ticks;
    ++a->phase_ticks;
    switch (a->dcs_phase) {
        case WM_DCS_STATIC:
            if (a->phase_ticks >= WM_DCS_STATIC_TICKS) set_dcs_phase(app, WM_DCS_ROT_SETUP);
            break;
        case WM_DCS_ROT_SETUP:
            if (a->phase_ticks >= WM_DCS_ROT_SETUP_TICKS) set_dcs_phase(app, WM_DCS_ROT_UNSKIPPABLE);
            break;
        case WM_DCS_ROT_UNSKIPPABLE:
            if (a->phase_ticks >= WM_DCS_ROT_UNSKIPPABLE_TICKS) set_dcs_phase(app, WM_DCS_ROT_SKIPPABLE);
            break;
        case WM_DCS_ROT_SKIPPABLE:
            if (wm_app_any_attract_button(input)) return true;
            if (a->phase_ticks >= WM_DCS_ROT_SKIPPABLE_TICKS) set_dcs_phase(app, WM_DCS_STATIC_RETURN);
            break;
        case WM_DCS_STATIC_RETURN:
            if (a->phase_ticks >= WM_DCS_STATIC_RETURN_TICKS) set_dcs_phase(app, WM_DCS_BURST_FLASH);
            break;
        case WM_DCS_BURST_FLASH:
            if (a->phase_ticks >= WM_DCS_BURST_FLASH_TICKS) set_dcs_phase(app, WM_DCS_BURST_WAIT);
            break;
        case WM_DCS_BURST_WAIT:
            if (a->phase_ticks >= WM_DCS_BURST_WAIT_TICKS) set_dcs_phase(app, WM_DCS_BURST_SKIPPABLE);
            break;
        case WM_DCS_BURST_SKIPPABLE:
            if (wm_app_any_attract_button(input) ||
                a->phase_ticks >= WM_DCS_BURST_SKIPPABLE_TICKS) return true;
            break;
    }
    return false;
}

static bool tick_sports_logo(wm_app *app, const wm_input_state *input) {
    wm_attract_state *a = &app->attract;
    ++a->call_ticks;

    if (a->call_ticks == WM_SPORTS_LOGO_SCROLL_START_TICKS &&
        !wm_process_find_id(&app->scheduler, WM_PID_WATER))
        (void)wm_process_create(&app->scheduler, WM_PID_WATER,
                                move_back_off_screen_proc, app);

    if (a->call_ticks > WM_SPORTS_LOGO_BUTTON_ENABLE_TICKS &&
        wm_app_any_attract_button(input)) return true;
    return a->call_ticks >= WM_SPORTS_LOGO_TOTAL_TICKS;
}

static bool tick_title(wm_app *app, const wm_input_state *input) {
    wm_attract_state *a = &app->attract;
    ++a->call_ticks;

    if (a->call_ticks == WM_TITLE_SETUP_TICKS) {
        if (!wm_process_find_id(&app->scheduler, WM_PID_CYCLE_LAVA))
            (void)wm_process_create(&app->scheduler, WM_PID_CYCLE_LAVA,
                                    cycle_lava_proc, app);
        if (!wm_process_find_id(&app->scheduler, WM_PID_FLASH))
            (void)wm_process_create(&app->scheduler, WM_PID_FLASH,
                                    sprinkle_glints_proc, app);
        if (!wm_process_find_id(&app->scheduler, WM_PID_ATTRACT_ANIM))
            (void)wm_process_create(&app->scheduler, WM_PID_ATTRACT_ANIM,
                                    random_sparkle_proc, app);
    }

    if (a->call_ticks > WM_TITLE_BUTTON_ENABLE_TICKS &&
        wm_app_any_attract_button(input)) return true;
    return a->call_ticks >= WM_TITLE_TOTAL_TICKS;
}

void wm_app_init(wm_app *app) {
    memset(app, 0, sizeof(*app));
    wm_demo_init(&app->demo);
    wm_source_clock_init(&app->source_clock);
    wm_scheduler_init(&app->scheduler);
    app->p1_choice = WM_WRESTLER_BRET;
    app->p2_choice = WM_WRESTLER_BAM_BAM;
    app->attract.amode_loops = 0;
    app->attract.call = WM_ATTRACT_SHOW_HSTD;
    app->attract_started = false;
}

void wm_app_tick(wm_app *app, const wm_input_state *input) {
    if (!app) return;
    if (!input) input = &no_input;

    if (input->z) app->show_debug = !app->show_debug;

    /* ATTRACT.ASM performs display_blank/WIPEOUT and SLEEPK 8 before entering
       #loop. Preserve that boot boundary rather than beginning DCS immediately. */
    if (!app->attract_started) {
        if (++app->boot_ticks < WM_ATTRACT_BOOT_DELAY_TICKS) {
            wm_scheduler_step(&app->scheduler);
            return;
        }
        app->attract_started = true;
        begin_base_loop(app);
    }

    skip_untranslated_calls(app);

    bool done = false;
    switch (app->attract.call) {
        case WM_ATTRACT_DCS_LOGO: done = tick_dcs_logo(app, input); break;
        case WM_ATTRACT_SHOW_SPORTS_LOGO: done = tick_sports_logo(app, input); break;
        case WM_ATTRACT_SHOW_TITLE: done = tick_title(app, input); break;
        default: break;
    }

    /* Source CREATEd processes are stepped on the same global source clock.
       Individual translated process bodies explicitly preserve their initial
       cooperative-yield boundary. */
    wm_scheduler_step(&app->scheduler);

    if (done) advance_call(app);
    skip_untranslated_calls(app);
}

static void latch_input(wm_input_state *dst, const wm_input_state *src) {
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

bool wm_app_video_frame(wm_app *app, const wm_input_state *input) {
    if (!app) return false;
    if (!input) input = &no_input;
    latch_input(&app->latched_input, input);
    if (!wm_source_clock_video_frame(&app->source_clock)) return false;

    wm_input_state tick_input = app->latched_input;
    memset(&app->latched_input, 0, sizeof(app->latched_input));
    wm_app_tick(app, &tick_input);
    return true;
}
