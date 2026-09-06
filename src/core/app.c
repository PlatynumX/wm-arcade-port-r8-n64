#include "wm/app.h"
#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_drone_data.h"
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

static uint32_t title_rotl32(uint32_t v, unsigned n) {
    n &= 31u;
    return n ? (v << n) | (v >> (32u - n)) : v;
}

static uint32_t title_rndrng0(wm_app *app, const wm_process *proc,
                              uint32_t maximum) {
    wm_attract_state *a = &app->attract;
    uint32_t state = a->title_random_state ? a->title_random_state : 0x57574631u;
    uint32_t hcount = (app->scheduler.tick * 8u) & 0x1ffu;
    uint32_t stack_surrogate = proc ? (proc->generation << 4) ^ proc->state : 0u;
    uint32_t mixed = title_rotl32(state, state & 31u);
    mixed ^= title_rotl32(hcount | (hcount << 16), hcount & 31u);
    mixed ^= stack_surrogate;
    mixed += 0x9e3779b9u + a->title_rng_counter++;
    a->title_random_state = mixed;
    return (uint32_t)(((uint64_t)mixed * ((uint64_t)maximum + 1u)) >> 32);
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
            /* ATTR.ASM::DCS_LOGO SNDSND 1005 after display_unblank, gated
               by TURN_SOUNDS_OFF_IF_NEED (ATTRACT.ASM:669): once
               AMODE_LOOPS >= 2 the source sets SOUNDSUP and the attract
               loop plays silent. ADJMUSIC is not exposed yet; this
               frontend's default is enabled, so only the loop count gates
               it here.

               This guard used to sit below the send with no body of its
               own, so it captured the following `break` instead: the
               command was sent unconditionally, and on the third attract
               loop onward WM_ATTRACT_DCS_LOGO fell through into
               WM_ATTRACT_SHOW_SPORTS_LOGO and reset that call's own
               world/scroll state while the call was still DCS_LOGO. */
            if (a->amode_loops < 2u)
                (void)wm_audio_send_command(&app->audio, 1005);
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
    if (a->call == WM_ATTRACT_DCS_LOGO) {
        /* ATTR.ASM DCS screen stop/reset boundary. */
        (void)wm_audio_send_command(&app->audio, 0);
    }
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

/*
 * The shared @RAND stream's two hardware entropy inputs.
 *
 * WRESTLE.ASM's randomize step is `rl RAND,RAND / rl HCOUNT,RAND / add sp`
 * -- and `add sp` is the only part that can change RAND's value at all (see
 * wm/arcade/wmania_rng.h). Left at zero, as this port did until now, RAND
 * could only rotate: seeded from 0 it stayed 0, so every RNDRNG0 in the game
 * returned 0 forever. That silently pinned every drone decision to the first
 * entry of whatever table it rolled against -- the AI only ever chose
 * `#run`.
 *
 * Neither input has a true N64 equivalent: HCOUNT is the arcade's video-beam
 * line counter and SP is the TMS34010 stack pointer at the moment of the
 * call. Both are derived here from the real source clock instead, the same
 * surrogate approach (and the same `tick * 8 & 0x1ff` beam derivation) this
 * file's own title-sparkle RNG already uses. Deterministic, unlike the
 * hardware, which is what the host tests want.
 */
static uint32_t app_rng_hcount(void *user) {
    const wm_app *app = (const wm_app *)user;
    return app ? ((app->scheduler.tick * 8u) & 0x1ffu) : 0u;
}

static uint32_t app_rng_sp(void *user) {
    const wm_app *app = (const wm_app *)user;
    /* A TMS34010 stack pointer counts *down* from the top of its region as
       calls nest; this mirrors that shape rather than reusing the HCOUNT
       ramp, so the two inputs never move in lockstep. */
    return app ? (0x00010000u - (app->scheduler.tick * 64u)) : 0u;
}

/*
 * DCSSOUND.ASM triple_sound's seam for the animation VM's ANI_CODE sound
 * routines (wm/anim_program.h): the sound's own index goes into the port's
 * audio command queue unchanged. triple_sound's four-channel priority
 * arbitration is not modelled here -- that belongs to a DCSSOUND port.
 */
static void wm_app_anim_sound(void *user, uint16_t call) {
    (void)wm_audio_send_command((wm_audio_state *)user, call);
}

/* Hand the match the services its ANI_CODE routines reach for. Both live
   on the app, so this is done wherever a match is started. */
/* AWARD.ASM round_award, behind JJXM.H's RND_AWARD macro. The award
   arrays are per credit, so they live on the app; the wrestler's own
   PLYRNUM picks the row, exactly as `move :REG:,a0 / calla round_award`
   does with a13. */
static void wm_app_round_award(void *user, int player_num,
                               unsigned award_index) {
    wm_app *app = (wm_app *)user;
    if (!app || player_num < 0 ||
        (unsigned)player_num >= WM_AWARD_PLAYER_COUNT)
        return;
    wm_award_round_award(&app->awards, (unsigned)player_num, award_index);
}

static void wm_app_bind_anim_env(wm_app *app) {
    app->match.anim_rng = &app->rng;
    app->match.anim_sound_user = &app->audio;
    app->match.anim_sound = wm_app_anim_sound;
    app->match.anim_award_user = app;
    app->match.anim_round_award = wm_app_round_award;
    wm_anim_code_reset();
}

static bool tick_gameplay(wm_app *app, const wm_input_state *input) {
    wm_attract_state *a = &app->attract;

    /*
     * These two belong together: the bind hands the freshly started match
     * its RNG, audio and award seams. Without the braces the bind ran
     * every tick, and wm_anim_code_reset() inside it wiped the source's
     * timed sound state on each one -- so a CALL_x announcer process,
     * which sleeps 5-15 ticks before it says anything, was cleared before
     * it could ever fire.
     */
    if (a->call_ticks == 0) {
        wm_match_start_attract(&app->match, &app->rng);
        wm_app_bind_anim_env(app);
    }

    {
        wm_arcade_drone_callbacks_t cb = wm_arcade_drone_data_callbacks(&app->rng);
        wm_match_tick(&app->match, &cb, NULL);
    }

    ++a->call_ticks;

    if ((a->call_ticks > WM_GAMEPLAY_BUTTON_ENABLE_TICKS &&
         wm_app_any_attract_button(input)) ||
        a->call_ticks >= WM_GAMEPLAY_TOTAL_TICKS) {
        /* ATTRACT.ASM::show_gameplay ends by freezing wrestler processes
           (@HALT), fading, and display_blank -- the next occurrence starts
           a fresh start_match, not a continuation of this one. */
        app->match.active = false;
        return true;
    }
    return false;
}

void wm_app_init(wm_app *app) {
    memset(app, 0, sizeof(*app));
    app->mode = WM_APP_MODE_ATTRACT;
    wm_audio_init(&app->audio);
    wm_select_continue_init(&app->continue_select);
    wm_award_init(&app->awards);
    wm_demo_init(&app->demo);
    wm_match_init(&app->match);
    wm_rng_init(&app->rng, 0, app_rng_hcount, app_rng_sp, app);
    wm_source_clock_init(&app->source_clock);
    wm_scheduler_init(&app->scheduler);
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
        wm_app_bind_anim_env(app);
        wm_match_start_selected(&app->match, &app->rng,
                                app->pregame.player_source_wrestler);
        app->mode = WM_APP_MODE_MATCH;
        return;
    }
    if (app->mode == WM_APP_MODE_MATCH) {
        wm_arcade_drone_callbacks_t cb = wm_arcade_drone_data_callbacks(&app->rng);
        wm_match_tick(&app->match, &cb, input);
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

    skip_untranslated_calls(app);

    /*
     * SOURCE_SELECT_TITLE_START_BRIDGE
     * Cabinet coin/PSTATUS accounting is not yet a native N64 subsystem.
     * Start on the source title bridges one human player into SELECT.ASM.
     */
    if (app->attract.call == WM_ATTRACT_SHOW_TITLE &&
        app->attract.call_ticks > WM_TITLE_BUTTON_ENABLE_TICKS &&
        input && input->start) {
        kill_call_processes(app, app->attract.call);
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
        case WM_ATTRACT_SHOW_GAMEPLAY: done = tick_gameplay(app, input); break;
        default: break;
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
    skip_untranslated_calls(app);
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
    return true;
}
