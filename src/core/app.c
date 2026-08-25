#include "wm/app.h"
#include "wm/arcade_sound_tables.h"
#include "wm/arcade_sound_bridge.h"
#include "wm_fix39_runtime.h"
#include "wm_arcade_wimp_frame.h"
#include "wm/character_assets.h"
#include "wm/bret_sprites.h"
#include "wm/visual.h"
#include <stdint.h>
#include <string.h>

static const wm_input_state no_input = {0};
static uint32_t app_sound_rng(void *user, uint32_t maximum) {
    wm_app *app=(wm_app *)user; uint32_t h=(app->scheduler.tick*8u)&0x1ffu;
    uint32_t sp=(uint32_t)(uintptr_t)&maximum; wm_fix39_rng_set_entropy(h,sp); return wm_fix39_rndrng0(maximum);
}
static uint32_t app_sound_hcount(void *user) { wm_app *app=(wm_app *)user; return (app->scheduler.tick*8u)&0x1ffu; }

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

/* Rev 1.30 ROM: WHERE_WRESTLMANIA_SPARKLES at TMS 0xFF9C1A40.
   The table stores offsets from the A8 origin [Y=102,X=7] and ends in
   FFFF,FFFF. */
static const uint16_t title_sparkle_sites[WM_TITLE_SPARKLE_SITE_COUNT][2] = {
    {  8,  5 }, { 22, 29 }, { 43,  4 }, { 50, 60 }, { 65,  4 },
    { 76, 28 }, { 80, 56 }, { 87, 15 }, { 94, 33 }, {105, 50 },
    {110, 32 }, {114,  4 }, {121, 64 }, {125, 56 }, {129, 28 },
    {158, 15 }, {159, 56 }, {177,  4 }, {186, 33 }, {200, 30 },
    {203, 15 }, {220, 35 }, {225,  4 }, {236, 40 }, {257,  4 },
    {260, 13 }, {271,  4 }, {273, 61 }, {277, 56 }, {286, 15 },
    {291, 33 }, {309, 15 }, {320, 37 }, {334, 56 }, {338, 33 },
    {345,  4 }, {359, 32 }, {370,  4 }, {373, 56 }, {383, 17 },
};


static uint32_t title_rndrng0(wm_app *app, const wm_process *proc,
                               uint32_t maximum) {
    /* Exact shared RNDRNG0 translation. The source scheduler phase supplies
       HCOUNT; a live stack address supplies the N64 SP entropy input. */
    uint32_t hcount = (app->scheduler.tick * 8u) & 0x1ffu;
    uint32_t sp_value = (uint32_t)(uintptr_t)&maximum;
    (void)proc;
    wm_fix39_rng_set_entropy(hcount, sp_value);
    return wm_fix39_rndrng0(maximum);
}

static unsigned title_sparkle_family_frames(unsigned family) {
    return family < 3u ? 13u : 15u;
}

static wm_title_sparkle *title_sparkle_alloc(wm_attract_state *a,
                                             size_t *slot_out) {
    for (size_t i = 0; i < WM_TITLE_SPARKLE_SLOT_COUNT; ++i) {
        if (!a->title_glints[i].active) {
            if (slot_out) *slot_out = i;
            return &a->title_glints[i];
        }
    }
    return NULL;
}

static void title_sparkle_child_proc(wm_process *proc, void *user) {
    wm_app *app = (wm_app *)user;
    if (!proc || !app || proc->state == 0) {
        wm_process_kill(proc);
        return;
    }
    size_t slot = (size_t)(proc->state - 1u);
    if (slot >= WM_TITLE_SPARKLE_SLOT_COUNT) {
        wm_process_kill(proc);
        return;
    }

    wm_title_sparkle *sp = &app->attract.title_glints[slot];
    if (!sp->active) {
        sp->family = (uint8_t)title_rndrng0(app, proc, 4u);
        sp->frame = 0;
        sp->active = true;
        wm_process_sleep(&app->scheduler, proc, WM_TITLE_SPARKLE_FRAME_TICKS);
        return;
    }

    ++sp->frame;
    if ((unsigned)sp->frame >= title_sparkle_family_frames(sp->family)) {
        sp->active = false;
        wm_process_kill(proc);
        return;
    }
    wm_process_sleep(&app->scheduler, proc, WM_TITLE_SPARKLE_FRAME_TICKS);
}

static bool title_sparkle_create(wm_app *app, size_t site) {
    if (!app || site >= WM_TITLE_SPARKLE_SITE_COUNT) return false;

    size_t slot = 0;
    wm_title_sparkle *sp = title_sparkle_alloc(&app->attract, &slot);
    if (!sp) return false;

    sp->x = (int16_t)(WM_TITLE_SPARKLE_ORIGIN_X + title_sparkle_sites[site][0]);
    sp->y = (int16_t)(WM_TITLE_SPARKLE_ORIGIN_Y + title_sparkle_sites[site][1]);
    sp->family = 0;
    sp->frame = 0;
    sp->active = false;

    wm_process *child = wm_process_create(&app->scheduler, WM_PID_FLASH,
                                          title_sparkle_child_proc, app);
    if (!child) return false;
    child->state = (uint32_t)slot + 1u;
    return true;
}

static void reset_title_sparkles(wm_attract_state *a) {
    for (size_t i = 0; i < WM_TITLE_SPARKLE_SLOT_COUNT; ++i)
        a->title_glints[i] = (wm_title_sparkle){0};
    a->title_random_sparkle = (wm_title_sparkle){0};
    a->title_random_state = 0x57574631u;
    a->title_rng_counter = 0;
}

static void sprinkle_glints_proc(wm_process *proc, void *user) {
    wm_app *app = (wm_app *)user;
    if (!proc || !app) return;

    size_t site = (size_t)proc->state;
    if (site >= WM_TITLE_SPARKLE_SITE_COUNT) {
        wm_process_kill(proc);
        return;
    }

    (void)title_sparkle_create(app, site);
    proc->state = (uint32_t)(site + 1u);
    wm_process_sleep(&app->scheduler, proc, WM_TITLE_SPARKLE_SWEEP_PERIOD_TICKS);
}

static void random_sparkle_proc(wm_process *proc, void *user) {
    wm_app *app = (wm_app *)user;
    if (!proc || !app) return;

    if (proc->state == 0) {
        proc->state = 1;
        wm_process_sleep(&app->scheduler, proc, WM_TITLE_SPARKLE_RANDOM_POLL_TICKS);
        return;
    }

    if (proc->state == 2) {
        proc->state = 1;
        wm_process_sleep(&app->scheduler, proc, WM_TITLE_SPARKLE_RANDOM_POLL_TICKS);
        return;
    }

    if (wm_process_find_id(&app->scheduler, WM_PID_FLASH)) {
        wm_process_sleep(&app->scheduler, proc, WM_TITLE_SPARKLE_RANDOM_POLL_TICKS);
        return;
    }

    size_t site = (size_t)title_rndrng0(app, proc, WM_TITLE_SPARKLE_SITE_COUNT - 1u);
    (void)title_sparkle_create(app, site);
    proc->state = 2;
    wm_process_sleep(&app->scheduler, proc, WM_TITLE_SPARKLE_RANDOM_POST_TICKS);
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
            /* Source-owned DCS command: start at DCS_LOGO entry, tick 0. */
            (void)wm_audio_send_command(&app->audio, 1005);
            /* ATTR.ASM::DCS_LOGO SNDSND 1005 after display_unblank.
               Source suppresses it once AMODE_LOOPS >= 2. ADJMUSIC is
               not exposed yet; this frontend's default is enabled. */
            if (a->amode_loops < 2u)
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

static bool fix39_attract_frontend_call(const WmAttractStep *step,
                                          wm_attract_call *out) {
    if (!step || !out) return false;
    switch (step->screen) {
        case WM_FIX39_ATTRACT_HISCORES: *out = WM_ATTRACT_SHOW_HSTD; return true;
        case WM_FIX39_ATTRACT_DCS_LOGO: *out = WM_ATTRACT_DCS_LOGO; return true;
        case WM_FIX39_ATTRACT_SPORTS_LOGO: *out = WM_ATTRACT_SHOW_SPORTS_LOGO; return true;
        case WM_FIX39_ATTRACT_GAMEPLAY_DEMO_1:
        case WM_FIX39_ATTRACT_GAMEPLAY_DEMO_2: *out = WM_ATTRACT_SHOW_GAMEPLAY; return true;
        case WM_FIX39_ATTRACT_CREDITS_1:
        case WM_FIX39_ATTRACT_CREDITS_2:
        case WM_FIX39_ATTRACT_EVEN_LOOP_CREDITS: *out = WM_ATTRACT_CREDITSCREEN; return true;
        case WM_FIX39_ATTRACT_TITLE: *out = WM_ATTRACT_SHOW_TITLE; return true;
        case WM_FIX39_ATTRACT_DESIGNER_HINT: *out = WM_ATTRACT_DO_HINTS; return true;
        case WM_FIX39_ATTRACT_GENERAL_TIPS: *out = WM_ATTRACT_SHOW_GEN_TIPS; return true;
        case WM_FIX39_ATTRACT_BIO: *out = WM_ATTRACT_SHOW_BIOS; return true;
        case WM_FIX39_ATTRACT_BIO_TIPS: *out = WM_ATTRACT_SHOW_BIOS_TIPS; return true;
        case WM_FIX39_ATTRACT_OPERATOR_MESSAGE: *out = WM_ATTRACT_SHOW_OPERATORMSG; return true;
        case WM_FIX39_ATTRACT_TIME_DATE: *out = WM_ATTRACT_SHOW_TIME_DATE; return true;
        case WM_FIX39_ATTRACT_COPYRIGHT: *out = WM_ATTRACT_SHOW_COPYRIGHT; return true;
        case WM_FIX39_ATTRACT_AAMA: *out = WM_ATTRACT_AAMA_MESSAGE; return true;
    }
    return false;
}
static bool begin_fix39_attract_step(wm_app *app, size_t index) {
    const WmAttractStep *step = wm_fix39_attract_step(index);
    wm_attract_call call;
    if (!step || !fix39_attract_frontend_call(step, &call)) return false;
    app->attract.source_index = (uint8_t)index;
    app->attract.amode_loops = step->source_amode_loops;
    begin_call(app, call);
    /* V11 source ownership: this begins the translated screen runner only
       when the platform has explicitly bound its exact presentation. */
    (void)wm_fix39_attract_screen_begin(index);
    return true;
}
static void begin_base_loop(wm_app *app) {
    app->attract.flow = WM_ATTRACT_FLOW_BASE;
    if (wm_fix39_attract_cycle_begin() == 0u) return;
    (void)begin_fix39_attract_step(app, 0u);
}


static void advance_call(wm_app *app) {
    wm_attract_state *a = &app->attract;
    size_t next_index;
    if (a->call == WM_ATTRACT_DCS_LOGO) {
        /* ATTR.ASM DCS screen stop/reset boundary. */
        (void)wm_audio_send_command(&app->audio, 0);
    }
    kill_call_processes(app, a->call);
    next_index = (size_t)a->source_index + 1u;
    if (wm_fix39_attract_step(next_index) != NULL) {
        (void)begin_fix39_attract_step(app, next_index);
        return;
    }
    begin_base_loop(app);
}

static void skip_fix39_pending_calls(wm_app *app) {
    for (unsigned guard = 0; guard < 64u; ++guard) {
        size_t index = (size_t)app->attract.source_index;
        if (wm_fix39_attract_step_runnable(index)) return;
        wm_fix39_attract_note_pending_skip(index);
        advance_call(app);
    }
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

static bool fix39_tick_gameplay_demo(wm_app *app, const wm_input_state *input) {
    wm_attract_state *a = &app->attract;
    ++a->call_ticks;
    if (a->call_ticks == 1u) {
        /* ATTR.ASM SHOW_GAMEPLAY enters the real match path. No wm_demo simulation. */
        wm_fix39_match_begin((unsigned)app->p1_choice, (unsigned)app->p2_choice);
        wm_fix39_match_set_cpu_vs_cpu(true);
    }
    wm_fix39_match_tick(0, 0, false, false, false, false, false, false);
    if (a->call_ticks > 180u && wm_app_any_attract_button(input)) { wm_fix39_match_set_cpu_vs_cpu(false); return true; }
    if (a->call_ticks >= (180u + 10u * WM_SOURCE_TICKS_PER_SEC)) { wm_fix39_match_set_cpu_vs_cpu(false); return true; }
    return false;
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
    wm_fix39_runtime_init();
    app->mode = WM_APP_MODE_ATTRACT;
    wm_audio_init(&app->audio);
    wm_arcade_sound_init(&app->sound, wm_arcade_sound_emit_to_audio, &app->audio, app_sound_rng, app);
    wm_arcade_sound_bind_default_tables(&app->sound);
    wm_arcade_sound_bind_hcount(&app->sound, app_sound_hcount, app);
    wm_fix39_bind_arcade_sound(&app->sound);
    wm_select_continue_init(&app->continue_select);
    wm_award_init(&app->awards);
    wm_demo_init(&app->demo);
    wm_source_clock_init(&app->source_clock);
    wm_scheduler_init(&app->scheduler);
    wm_cabinet_bridge_init(&app->cabinet);
    if (wm_hs_sdcard_backend_init(&app->hiscore_sdcard_backend,
                                  &app->hiscore_save_backend,
                                  NULL)) {
        wm_fix39_hiscore_bind_persistence(&app->hiscore_save_backend);
    }
    app->p1_choice = WM_WRESTLER_BRET;
    app->p2_choice = WM_WRESTLER_BAM_BAM;
    app->attract.amode_loops = 0;
    app->attract.call = WM_ATTRACT_SHOW_HSTD;
    app->attract_started = false;
}

void wm_app_tick_dual(wm_app *app,
                      const wm_input_state *p1_input,
                      const wm_input_state *p2_input) {
    const wm_input_state *input = p1_input;
    if (!app) return;
    wm_audio_source_tick(&app->audio);
    wm_arcade_sound_tick(&app->sound);
    /* SOURCE_SELECT_MODE_TICK */
    if (app->mode == WM_APP_MODE_SELECT) {
        wm_select_screen_tick(&app->select,
                              p1_input ? p1_input->stick_x : 0,
                              p1_input ? p1_input->stick_y : 0,
                              p1_input ? p1_input->start : false,
                              p1_input ? p1_input->light_punch : false,
                              p1_input ? p1_input->power_punch : false,
                              p1_input ? p1_input->light_kick : false,
                              p1_input ? p1_input->power_kick : false,
                              &app->audio, &app->p1_choice);
        app->done_howard = wm_select_screen_howard_done(&app->select);

        /*
         * SOURCE_SELECT_P2_START_BRIDGE
         * Arcade SELECT.ASM sees PSTATUS change when player two buys in.
         * N64 has no coin/PSTATUS subsystem yet, so Controller 2 Start is the
         * platform boundary that activates the otherwise direct P2 process.
         */
        bool p2_joined_now = false;
        if (p2_input && p2_input->start && !app->select.p2_joined) {
            (void)wm_cabinet_bridge_accept_player_start(&app->cabinet, 1u);
            wm_select_screen_join_p2(&app->select);
            (void)wm_award_prepare_select_bonus(&app->awards, 1u);
            p2_joined_now = true;
        }

        /* Consume the buy-in Start edge; it must not also start random-select. */
        if (app->select.p2_joined && !p2_joined_now) {
            wm_select_screen_tick_p2(&app->select,
                                     p2_input ? p2_input->stick_x : 0,
                                     p2_input ? p2_input->stick_y : 0,
                                     p2_input ? p2_input->start : false,
                                     p2_input ? p2_input->light_punch : false,
                                     p2_input ? p2_input->power_punch : false,
                                     p2_input ? p2_input->light_kick : false,
                                     p2_input ? p2_input->power_kick : false,
                                     &app->audio, &app->p2_choice);
        }

        if (app->select.finished) {
            /*
             * Fix36 deliberately ends at the SELECT boundary.  The current
             * pregame/progression port is still P1-oriented; do not invent a
             * new two-player pregame here.
             */
            wm_pregame_init(&app->pregame,
                            app->select.selected_source_wrestler,
                            app->p1_choice);
            app->pregame.win_streak = app->awards.win_streak[0];
            app->mode = WM_APP_MODE_PREGAME;
        }
        return;
    }
    if (app->mode == WM_APP_MODE_PREGAME) {
        wm_pregame_tick(&app->pregame, input, &app->audio);
        if (app->pregame.finished)
            app->mode = WM_APP_MODE_MATCH_INIT;
        return;
    }
    if (app->mode == WM_APP_MODE_MATCH_INIT) {
        /* SELECT/PROGRESS source handoff: CURRENT_LADDER's first live opponent
           uses the same source-id conversion routine as SELECT.ASM, then
           reset_start enters the already translated WRESTLE runtime. */
        uint8_t source_opp = wm_pregame_opponent_at(&app->pregame, 0u);
        wm_wrestler_id roster_opp;
        if (!wm_select_source_to_roster(source_opp, &roster_opp)) return;
        app->p2_choice = roster_opp;
        wm_fix39_match_begin((unsigned)app->p1_choice, (unsigned)app->p2_choice);
        if (wm_fix39_match_started()) app->mode = WM_APP_MODE_MATCH;
        return;
    }
    if (app->mode == WM_APP_MODE_MATCH) {
        const wm_input_state *mi = input ? input : &no_input;
        wm_fix39_match_tick(mi->stick_x, mi->stick_y, mi->run,
                            mi->light_punch, mi->power_punch,
                            mi->light_kick, mi->power_kick, mi->block);
        return;
    }
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
        /* ATTRACT.ASM startup SNDSND command 0 after SLEEPK 8. */
        (void)wm_audio_send_command(&app->audio, 0);
        begin_base_loop(app);
    }

    skip_fix39_pending_calls(app);

    /*
     * SOURCE_SELECT_TITLE_START_BRIDGE
     * Cabinet coin/PSTATUS accounting is not yet a native N64 subsystem.
     * Start on the source title bridges one human player into SELECT.ASM.
     */
    if (app->attract.call == WM_ATTRACT_SHOW_TITLE &&
        app->attract.call_ticks > WM_TITLE_BUTTON_ENABLE_TICKS &&
        input && input->start) {
        kill_call_processes(app, app->attract.call);
        (void)wm_cabinet_bridge_accept_player_start(&app->cabinet, 0u);
        app->mode = WM_APP_MODE_SELECT;
        wm_select_screen_init(&app->select);
        wm_select_screen_set_howard_done(&app->select, app->done_howard);
        /* SELECT.ASM::show_bonus_icons with SHOW_ACCUM_ICONS=0 clears the
           accumulated total on a fresh (zero-winstreak) player. */
        (void)wm_award_prepare_select_bonus(&app->awards, 0u);
        return;
    }

    bool done = false;
    switch (app->attract.call) {
            case WM_ATTRACT_DCS_LOGO: done = tick_dcs_logo(app, input); break;
            case WM_ATTRACT_SHOW_SPORTS_LOGO: done = tick_sports_logo(app, input); break;
            case WM_ATTRACT_SHOW_TITLE: done = tick_title(app, input); break;
        case WM_ATTRACT_SHOW_GAMEPLAY: done = fix39_tick_gameplay_demo(app, input); break;
            default:
                done = wm_fix39_attract_screen_tick(
                    wm_app_any_attract_button(input));
                break;
        }

    /* Source CREATEd processes are stepped on the same global source clock.
       Individual translated process bodies explicitly preserve their initial
       cooperative-yield boundary. */
    wm_scheduler_step(&app->scheduler);

    if (done) {
        if (app->attract.call == WM_ATTRACT_DCS_LOGO) {
            /* ATTR.ASM::DCS_LOGO exit SNDSND command 0 at nobutn1. */
            (void)wm_audio_send_command(&app->audio, 0);
        }
        advance_call(app);
    }
    skip_fix39_pending_calls(app);
}


void wm_app_tick(wm_app *app, const wm_input_state *input) {
    wm_app_tick_dual(app, input, NULL);
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
    /* WRESTLE.ASM mainpok randomizes RAND after process_dispatch. */
    {
        uint32_t hcount = (app->scheduler.tick * 8u) & 0x1ffu;
        uint32_t sp_value = (uint32_t)(uintptr_t)&tick_input;
        (void)wm_fix39_mainloop_step(hcount, sp_value);
    }
    return true;
}
