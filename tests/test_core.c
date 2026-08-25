#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wm/anim.h"
#include "wm/app.h"
#include "wm/character_assets.h"
#include "wm_fix39_runtime.h"
#include "wm/bmod.h"
#include "wm/source_clock.h"
#include "wm/bret_visuals.h"
#include "wm/composite.h"
#include "wm/demo.h"
#include "wm/game.h"
#include "wm/roster.h"
#include "wm/select.h"
#include "wm/source_data.h"
#include "wm/visual.h"

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
        exit(1); \
    } \
} while (0)

static int runs;
static void sleeper(wm_process *p, void *user) {
    wm_scheduler *s = (wm_scheduler *)user;
    ++runs;
    if (runs == 1) wm_process_sleep(s, p, 2);
    else wm_process_kill(p);
}

static int child_runs;
static void child_proc(wm_process *p, void *user) {
    (void)user;
    ++child_runs;
    wm_process_kill(p);
}
static void creator_proc(wm_process *p, void *user) {
    wm_scheduler *s = (wm_scheduler *)user;
    (void)wm_process_create(s, 0x44, child_proc, NULL);
    wm_process_kill(p);
}

static void test_scheduler(void) {
    wm_scheduler s;
    runs = 0;
    wm_scheduler_init(&s);
    CHECK(wm_process_create(&s, 1, sleeper, &s) != NULL);
    wm_scheduler_step(&s);
    wm_scheduler_step(&s);
    wm_scheduler_step(&s);
    CHECK(runs == 2);

    /* CREATE during one source process must not recursively execute the child
       during that same cooperative scheduler pass. */
    wm_scheduler_init(&s);
    child_runs = 0;
    CHECK(wm_process_create(&s, 0x33, creator_proc, &s) != NULL);
    wm_scheduler_step(&s);
    CHECK(child_runs == 0);
    CHECK(wm_process_find_id(&s, 0x44) != NULL);
    wm_scheduler_step(&s);
    CHECK(child_runs == 1);
    CHECK(wm_process_find_id(&s, 0x44) == NULL);
}

static void test_source_clock(void) {
    wm_source_clock c;
    wm_source_clock_init(&c);
    unsigned due = 0;
    for (unsigned i = 0; i < 60; ++i)
        if (wm_source_clock_video_frame(&c)) ++due;
    CHECK(due == 53);
    CHECK(c.video_frames == 60);
    CHECK(c.source_ticks == 53);
    CHECK(c.accumulator == 0);
}

static void test_bmod_decode(void) {
    /* First NTITLESC packed record from BGNDTBL.ASM. */
    const uint16_t words[] = {0x0140, 0x0000, 0x0085, 0x0000};
    wm_bmod_module m = {.width=403, .height=256, .block_count=1, .packed_words=words};
    wm_bmod_block b;
    CHECK(wm_bmod_decode_block(&m, 0, &b));
    CHECK(b.palette == 0);
    CHECK(b.flags == 4);
    CHECK((b.flags & WM_BMOD_TRANSPARENT) != 0);
    CHECK(b.z == 1);
    CHECK(b.x == 0);
    CHECK(b.y == 133);
    CHECK(b.header_index == 0);

    /* High palette nibble is stored in header bits 12..15. */
    const uint16_t synthetic[] = {0x0235, 0xfffe, 0x0010, 0xA123};
    m.packed_words = synthetic;
    CHECK(wm_bmod_decode_block(&m, 0, &b));
    CHECK(b.palette == 0xA5);
    CHECK(b.flags == 3);
    CHECK(b.z == 2);
    CHECK(b.x == -2);
    CHECK(b.header_index == 0x123);

    /* Midway background objects are Z/Y sorted.  These are the relationships
       that matter to NTITLESC: the stone strips at Y=182/134 must draw before
       the MIDWAY strip at Y=201 when all share Z=0x40. */
    const wm_bmod_block stone182 = {.z=0x40, .y=182};
    const wm_bmod_block stone134 = {.z=0x40, .y=134};
    const wm_bmod_block midway = {.z=0x40, .y=201};
    const wm_bmod_block back = {.z=0x01, .y=250};
    CHECK(wm_bmod_draw_before(&stone182, &midway));
    CHECK(wm_bmod_draw_before(&stone134, &midway));
    CHECK(!wm_bmod_draw_before(&midway, &stone182));
    CHECK(wm_bmod_draw_before(&back, &stone134));
    CHECK(!wm_bmod_draw_before(&stone134, &stone134));
}

static void test_source_sequence(void) {
    const uint16_t expected[] = {
        0x8002, 0x000c, 0x8003, 0x8026, 0x0100,
        0x801b, 0x805f, 0x8002, 0x0000, 0x8049
    };
    CHECK(wm_source_hrt_finish1_move.word_count == sizeof(expected)/sizeof(expected[0]));
    for (size_t i = 0; i < sizeof(expected)/sizeof(expected[0]); ++i)
        CHECK(wm_source_hrt_finish1_move.words[i] == expected[i]);
}

static void test_anim(void) {
    wm_object o = {.vx=1,.vy=2,.vz=3,.visible=true};
    wm_anim_state a;
    wm_anim_start(&a, wm_source_hrt_finish1_move.words,
                  wm_source_hrt_finish1_move.word_count);
    CHECK(!wm_anim_step(&a, &o));
    CHECK(a.ended);
    CHECK(o.vx == 0 && o.vy == 0 && o.vz == 0);
    CHECK(a.speed == 0x100);
    CHECK((a.mode & WM_MODE_END) != 0);
}

static void test_source_anim_typed_stream(void) {
    /* SET_YVEL LONG; SET_XVEL LONG+WORD relative; SET_ZVEL LONG+WORD;
       SET_WRESTLER_XFLIP; frame WORD+LONG; END. All numbers are big-endian,
       matching the original source stream rather than a uint16_t surrogate. */
    const uint8_t stream[] = {
        0x80,0x05, 0x12,0x34,0x56,0x78,
        0x80,0x2c, 0x00,0x01,0x00,0x00, 0x00,0x01,
        0x80,0x30, 0x00,0x02,0x00,0x00, 0x00,0x01,
        0x80,0x5f,
        0x00,0x02, 0xde,0xad,0xbe,0xef,
        0x80,0x49
    };
    wm_source_anim_state a;
    wm_object o = {.visible=true};
    wm_source_anim_start(&a, stream, sizeof(stream));
    a.facing_right = false; /* relative X negates */
    a.facing_down = true;   /* relative Z stays positive */
    CHECK(wm_source_anim_step(&a, &o));
    CHECK(o.vy == (int32_t)0x12345678);
    CHECK(o.vx == -0x00010000);
    CHECK(o.vz == 0x00020000);
    CHECK(a.xflip);
    CHECK(a.current_frame_ref == 0xdeadbeefU);
    CHECK(a.hold_ticks == 2);
    CHECK(!a.ended && !a.malformed && !a.unsupported);
    CHECK(wm_source_anim_step(&a, &o)); /* hold 2 -> 1 */
    CHECK(!wm_source_anim_step(&a, &o)); /* hold 1 -> 0, END */
    CHECK(a.ended && !a.malformed && !a.unsupported);

    /* Source SETSPEED scales positive frame tick WORD by /256. */
    const uint8_t speed_stream[] = {
        0x80,0x26, 0x00,0x80,
        0x00,0x04, 0x01,0x02,0x03,0x04,
        0x80,0x49
    };
    wm_source_anim_start(&a, speed_stream, sizeof(speed_stream));
    CHECK(wm_source_anim_step(&a, &o));
    CHECK(a.speed == 0x80);
    CHECK(a.hold_ticks == 2);

    /* Unknown command semantics are never guessed. */
    const uint8_t unknown[] = {0x80,0x06}; /* ATTACK_ON not ported here yet */
    wm_source_anim_start(&a, unknown, sizeof(unknown));
    CHECK(!wm_source_anim_step(&a, &o));
    CHECK(a.ended && a.unsupported && !a.malformed);

    /* Legacy WORD-only decoder must refuse LONG-operand velocity commands. */
    const uint16_t bad_legacy[] = {WM_ANI_SET_YVEL, 0x1234, WM_ANI_END};
    wm_anim_state legacy;
    wm_anim_start(&legacy, bad_legacy, 3);
    CHECK(!wm_anim_step(&legacy, &o));
    CHECK(legacy.ended && legacy.unsupported);
}

static void test_ropes(void) {
    wm_rope_system r;
    wm_ropes_init(&r);
    CHECK(wm_rope_command(&r, WM_ROPE_LEFT, WM_ROPE_CMD_2, 3, wm_fix_from_int(42)));
    CHECK(r.group[WM_ROPE_LEFT].generation == 1);
    CHECK(r.group[WM_ROPE_LEFT].position_or_magnitude == 3);
    CHECK(wm_fix_to_int(r.group[WM_ROPE_LEFT].wrestler_z) == 42);
}

static void test_secondary_composite_offsets(void) {
    int xoff = 0, yoff = 0;
    wm_secondary_display_offsets(41, 97, 18, 61, 22, 35, &xoff, &yoff);
    CHECK(xoff == 45);  /* 41 - 18 + 22 */
    CHECK(yoff == 71);  /* 97 - 61 + 35 */

    /* Attachment math itself must not change when the backend mirrors X.
       The renderer flips around this effective display offset, matching the
       arcade object renderer rather than manually negating a layer delta. */
    wm_secondary_display_offsets(-12, 80, -7, 44, 16, 21, &xoff, &yoff);
    CHECK(xoff == 11);
    CHECK(yoff == 57);
}

static void test_roster(void) {
    CHECK(wm_roster_count() == 8);
    CHECK(strcmp(wm_roster_get(WM_WRESTLER_BRET)->name, "Bret Hart") == 0);
    CHECK(wm_roster_get(WM_WRESTLER_BRET)->visual_backend_ready);
    CHECK(strcmp(wm_roster_get(WM_WRESTLER_UNDERTAKER)->source_module, "TAKER.ASM") == 0);
    CHECK(wm_roster_get((wm_wrestler_id)99) == NULL);
}

static void test_visual_sequences(void) {
    CHECK(wm_bret_stand2_anim.frame_count == 14);
    CHECK(wm_bret_stand4_anim.frame_count == 14);
    CHECK(wm_bret_torso2_anim.frame_count == 6 && wm_bret_torso2_anim.repeat);
    CHECK(wm_bret_torso4_anim.frame_count == 6 && wm_bret_torso4_anim.repeat);
    CHECK(wm_bret_walk2_f2_anim.frame_count == 16);
    CHECK(wm_bret_walk8_f2_anim.frame_count == 16);
    CHECK(wm_bret_walk4_f4_anim.frame_count == 16);
    CHECK(wm_bret_walk6_f4_anim.frame_count == 15);
    CHECK(wm_bret_run_anim.frame_count == 12 && wm_bret_run_anim.repeat);

    CHECK(wm_bret_light_punch2_anim.frame_count == 11 && !wm_bret_light_punch2_anim.repeat);
    CHECK(wm_bret_light_punch4_anim.frame_count == 11 && !wm_bret_light_punch4_anim.repeat);
    CHECK(wm_bret_power_punch_anim.frame_count == 10 && !wm_bret_power_punch_anim.repeat);
    CHECK(wm_bret_light_kick2_anim.frame_count == 12 && !wm_bret_light_kick2_anim.repeat);
    CHECK(wm_bret_light_kick4_anim.frame_count == 12 && !wm_bret_light_kick4_anim.repeat);
    CHECK(wm_bret_power_kick_anim.frame_count == 11 && !wm_bret_power_kick_anim.repeat);

    wm_visual_state v;
    wm_visual_start(&v, &wm_bret_stand4_anim);
    CHECK(wm_visual_current(&v) != NULL);
    for (int i = 0; i < 4; ++i) wm_visual_tick(&v);
    CHECK(v.frame_index == 0);
    wm_visual_tick(&v);
    CHECK(v.frame_index == 1);
}

static void test_demo_four_way_and_run(void) {
    wm_demo d;
    wm_demo_init(&d);
    d.ai_enabled = false;
    wm_demo_set_roster(&d, 4u, 0u);
    CHECK(d.p1.action == WM_DEMO_IDLE);
    CHECK(d.p1.facing == WM_DEMO_FACING_6);

    wm_input_state right = {.stick_x = 80};
    wm_demo_tick(&d, &right);
    CHECK(d.p1.action == WM_DEMO_WALK);
    CHECK(d.p1.facing == WM_DEMO_FACING_6);
    CHECK(d.p1.flip_x);
    CHECK(d.p1.visual.sequence == wm_character_visual(4u, WM_CV_WALK6));
    CHECK(d.p1.visual.sequence != wm_character_visual(0u, WM_CV_WALK6));

    wm_input_state left = {.stick_x = -80};
    wm_demo_tick(&d, &left);
    CHECK(d.p1.facing == WM_DEMO_FACING_4);
    CHECK(!d.p1.flip_x);
    CHECK(d.p1.visual.sequence == wm_character_visual(4u, WM_CV_WALK4));

    wm_input_state up_run = {.stick_y = 80, .run = true};
    int y0 = d.p1.screen_y;
    wm_demo_tick(&d, &up_run);
    CHECK(d.p1.action == WM_DEMO_RUN);
    CHECK(d.p1.facing == WM_DEMO_FACING_8);
    CHECK(d.p1.screen_y < y0);
    CHECK(d.p1.visual.sequence == wm_character_visual(4u, WM_CV_RUN));

    wm_input_state down = {.stick_y = -80};
    wm_demo_tick(&d, &down);
    CHECK(d.p1.action == WM_DEMO_WALK);
    CHECK(d.p1.facing == WM_DEMO_FACING_2);
    CHECK(d.p1.visual.sequence == wm_character_visual(4u, WM_CV_WALK2));
}

static bool is_attack_action(wm_demo_action a) {
    return a == WM_DEMO_LIGHT_PUNCH || a == WM_DEMO_POWER_PUNCH ||
           a == WM_DEMO_LIGHT_KICK || a == WM_DEMO_POWER_KICK;
}

static void finish_player_action(wm_demo *d) {
    wm_input_state none = {0};
    for (int i = 0; i < 256 && is_attack_action(d->p1.action); ++i)
        wm_demo_tick(d, &none);
    CHECK(d->p1.action == WM_DEMO_IDLE);
}

static void configure_hit(wm_demo *d) {
    d->ai_enabled = false;
    d->p1.screen_x = 120;
    d->p1.screen_y = 170;
    d->p1.facing = WM_DEMO_FACING_6;
    d->p2.screen_x = 158;
    d->p2.screen_y = 170;
    d->p2.facing = WM_DEMO_FACING_4;
    d->p2.stun_ticks = 0;
}

static void run_attack_case(wm_input_state attack, wm_demo_action expected_action,
                            const wm_visual_sequence *expected_sequence, int damage) {
    wm_demo d;
    wm_demo_init(&d);
    configure_hit(&d);
    int hp = d.p2.health;

    wm_demo_tick(&d, &attack);
    CHECK(d.p1.action == expected_action);
    CHECK(d.p1.visual.sequence == expected_sequence);
    CHECK(d.p1.action_count == 1);

    wm_input_state none = {0};
    for (int i = 0; i < 160 && d.p2.health == hp; ++i)
        wm_demo_tick(&d, &none);
    CHECK(d.p2.health == hp - damage);
    CHECK(d.p1.hit_count == 1);
    CHECK(d.total_hits == 1);

    finish_player_action(&d);
    CHECK(d.p2.health == hp - damage); /* one hit per swing */
}

static void test_four_attack_buttons(void) {
    run_attack_case((wm_input_state){.light_punch=true}, WM_DEMO_LIGHT_PUNCH,
                    wm_character_visual(0u, WM_CV_LP4), 5);
    run_attack_case((wm_input_state){.power_punch=true}, WM_DEMO_POWER_PUNCH,
                    wm_character_visual(0u, WM_CV_PP), 12);
    run_attack_case((wm_input_state){.light_kick=true}, WM_DEMO_LIGHT_KICK,
                    wm_character_visual(0u, WM_CV_LK4), 5);
    run_attack_case((wm_input_state){.power_kick=true}, WM_DEMO_POWER_KICK,
                    wm_character_visual(0u, WM_CV_PK), 15);
}

static void test_block(void) {
    wm_demo d;
    wm_demo_init(&d);
    d.ai_enabled = true;
    d.ai_cooldown = 0;
    d.p1.screen_x = 120;
    d.p1.screen_y = 170;
    d.p1.facing = WM_DEMO_FACING_6;
    d.p2.screen_x = 158;
    d.p2.screen_y = 170;
    d.p2.facing = WM_DEMO_FACING_4;
    int hp = d.p1.health;
    wm_input_state block = {.block=true};
    for (int i = 0; i < 160 && d.total_blocks == 0; ++i)
        wm_demo_tick(&d, &block);
    CHECK(d.p1.action == WM_DEMO_BLOCK || d.p1.stun_ticks > 0);
    CHECK(d.total_blocks > 0);
    CHECK(d.p1.health == hp);
}

static void test_cpu_chases(void) {
    wm_demo d;
    wm_demo_init(&d);
    d.p1.screen_x = 80;
    d.p1.screen_y = 170;
    d.p2.screen_x = 240;
    d.p2.screen_y = 170;
    d.ai_enabled = true;
    int x0 = d.p2.screen_x;
    wm_input_state none = {0};
    for (int i = 0; i < 8; ++i)
        wm_demo_tick(&d, &none);
    CHECK(d.p2.screen_x < x0);
    CHECK(d.p2.action == WM_DEMO_RUN || d.p2.action == WM_DEMO_WALK);
}

static void test_demo_bounds(void) {
    wm_demo d;
    wm_demo_init(&d);
    d.ai_enabled = false;
    wm_input_state up = {.stick_y = 100};
    wm_input_state left = {.stick_x = -100};
    for (int i = 0; i < 200; ++i) wm_demo_tick(&d, &up);
    CHECK(d.p1.screen_y >= 142);
    for (int i = 0; i < 200; ++i) wm_demo_tick(&d, &left);
    CHECK(d.p1.screen_x >= 68);
}

static void test_source_attract_sequence(void) {
    const wm_attract_call expected[] = {
        WM_ATTRACT_SHOW_HSTD,
        WM_ATTRACT_DCS_LOGO,
        WM_ATTRACT_SHOW_SPORTS_LOGO,
        WM_ATTRACT_SHOW_GAMEPLAY,
        WM_ATTRACT_CREDITSCREEN,
        WM_ATTRACT_SHOW_TITLE,
        WM_ATTRACT_SHOW_GAMEPLAY,
        WM_ATTRACT_CREDITSCREEN,
        WM_ATTRACT_DO_HINTS,
        WM_ATTRACT_SHOW_GEN_TIPS,
        WM_ATTRACT_SHOW_BIOS,
        WM_ATTRACT_SHOW_BIOS_TIPS,
        WM_ATTRACT_SHOW_OPERATORMSG,
    };
    CHECK(wm_source_attract_loop_count == sizeof(expected) / sizeof(expected[0]));
    for (size_t i = 0; i < wm_source_attract_loop_count; ++i)
        CHECK(wm_source_attract_loop[i] == expected[i]);
}

static void test_attract_source_flow(void) {
    wm_app app;
    wm_app_init(&app);
    CHECK(!app.attract_started);
    CHECK(app.attract.call == WM_ATTRACT_SHOW_HSTD);
    CHECK(app.attract.amode_loops == 0);

    CHECK(wm_attract_call_port_status(WM_ATTRACT_DCS_LOGO) == WM_PORT_PARTIAL_SOURCE);
    CHECK(wm_attract_call_port_status(WM_ATTRACT_SHOW_SPORTS_LOGO) == WM_PORT_PARTIAL_SOURCE);
    CHECK(wm_attract_call_port_status(WM_ATTRACT_SHOW_GAMEPLAY) == WM_PORT_HARNESS_ONLY);
    CHECK(wm_attract_call_port_status(WM_ATTRACT_SHOW_TITLE) == WM_PORT_PARTIAL_SOURCE);
    CHECK(!wm_attract_call_is_translated(WM_ATTRACT_SHOW_GAMEPLAY));
    CHECK(wm_attract_call_is_translated(WM_ATTRACT_SHOW_TITLE));

    wm_input_state button = {.run=true};
    for (unsigned i = 0; i < WM_ATTRACT_BOOT_DELAY_TICKS - 1; ++i) {
        wm_app_tick(&app, &button);
        CHECK(!app.attract_started);
        CHECK(app.attract.call == WM_ATTRACT_SHOW_HSTD);
    }
    wm_app_tick(&app, &button);
    CHECK(app.attract_started);
    CHECK(app.attract.call == WM_ATTRACT_DCS_LOGO);
    CHECK(app.attract.call_ticks == 1);

    for (unsigned i = 1; i < WM_DCS_STATIC_TICKS + WM_DCS_ROT_SETUP_TICKS +
                             WM_DCS_ROT_UNSKIPPABLE_TICKS; ++i) {
        wm_app_tick(&app, &button);
        CHECK(app.attract.call == WM_ATTRACT_DCS_LOGO);
    }
    CHECK(app.attract.dcs_phase == WM_DCS_ROT_SKIPPABLE);
    wm_app_tick(&app, &button);
    CHECK(app.attract.call == WM_ATTRACT_SHOW_SPORTS_LOGO);

    for (unsigned i = 0; i < WM_SPORTS_LOGO_BUTTON_ENABLE_TICKS; ++i) {
        wm_app_tick(&app, &button);
        CHECK(app.attract.call == WM_ATTRACT_SHOW_SPORTS_LOGO);
    }
    CHECK(wm_process_find_id(&app.scheduler, WM_PID_WATER) != NULL);
    CHECK(app.attract.sports_world_x < 0);
    CHECK(app.attract.sports_world_y > 0);
    wm_app_tick(&app, &button);
    CHECK(wm_process_find_id(&app.scheduler, WM_PID_WATER) == NULL);
    CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);
    /* ATTR.ASM show_gameplay creates start_match, then executes literal
       SLEEP 3*60 before entering wait_on_butn(10*TSEC). A held button returns
       from that wait at the first eligible tick, but show_gameplay does NOT
       return yet: source sets HALT, holds 60 ticks, starts fade_down, then
       SLEEPK 32 before display_blank/nosounds/RETP. */
    for (unsigned i = 0; i < 180u; ++i) {
        wm_app_tick(&app, &button);
        CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);
    }

    /* R37E source parity: after the held button ends wait_on_butn, preserve
       the exact 60 + 32 tick frozen/fade tail. */
    for (unsigned i = 0; i < (60u + 32u) - 1u; ++i) {
        wm_app_tick(&app, &button);
        CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);
    }
    wm_app_tick(&app, &button);
    CHECK(app.attract.call == WM_ATTRACT_SHOW_TITLE);
    unsigned initial_lava = app.attract.title_lava_step;
    for (unsigned i = 0; i < WM_TITLE_BUTTON_ENABLE_TICKS; ++i) {
        wm_app_tick(&app, &button);
        CHECK(app.attract.call == WM_ATTRACT_SHOW_TITLE);
    }
    CHECK(wm_process_find_id(&app.scheduler, WM_PID_CYCLE_LAVA) != NULL);
    CHECK(wm_process_find_id(&app.scheduler, WM_PID_FLASH) != NULL);
    CHECK(wm_process_find_id(&app.scheduler, WM_PID_ATTRACT_ANIM) != NULL);
    CHECK(app.attract.title_lava_step != initial_lava);
    bool any_glint = false;
    for (size_t i = 0; i < WM_TITLE_GLINT_COUNT; ++i)
        any_glint |= app.attract.title_glints[i].active;
    CHECK(any_glint);
    /* Fix39 replaced the provisional per-title RNG with the shared source RAND/RNDRNG0 state. */
    CHECK(wm_fix39_rng_state() != 0u);
    wm_app_tick(&app, &button);
    CHECK(wm_process_find_id(&app.scheduler, WM_PID_CYCLE_LAVA) == NULL);
    CHECK(wm_process_find_id(&app.scheduler, WM_PID_FLASH) == NULL);
    CHECK(wm_process_find_id(&app.scheduler, WM_PID_ATTRACT_ANIM) == NULL);
    CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);
    /* ATTRACT.ASM invokes the same show_gameplay routine here, so the
       second demo has the identical 3*60 live period followed by the
       60-tick HALT/freeze and 32-tick fade tail. */
    for (unsigned i = 0; i < 180u; ++i) {
        wm_app_tick(&app, &button);
        CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);
    }
    for (unsigned i = 0; i < (60u + 32u) - 1u; ++i) {
        wm_app_tick(&app, &button);
        CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);
    }
    wm_app_tick(&app, &button);
    CHECK(app.attract.call == WM_ATTRACT_DCS_LOGO);
    CHECK(app.attract.amode_loops == 1);

    /* Fix39: live attract gameplay mutates demo combat counters. */
    /* Counter values are outcome-dependent once CPU-vs-CPU gameplay runs. */

    bool old_debug = app.show_debug;
    wm_app_tick(&app, &(wm_input_state){.z=true});
    CHECK(app.show_debug != old_debug);
}

static void test_video_frame_source_clock_and_input_latch(void) {
    wm_app app;
    wm_app_init(&app);
    unsigned advanced = 0;
    for (unsigned i = 0; i < 60; ++i) {
        wm_input_state in = {0};
        if (i == 0) in.z = true; /* edge occurs even if first video frame has no source tick */
        if (wm_app_video_frame(&app, &in)) ++advanced;
    }
    CHECK(advanced == 53);
    CHECK(app.source_clock.source_ticks == 53);
    CHECK(app.show_debug); /* latched edge survived until first source tick */
    CHECK(app.attract_started);
}

static void test_source_select_core(void) {
    CHECK(WM_SELECT_TIME_TICKS == 53 * 15);
    CHECK(WM_SELECT_FINAL_WAIT_TICKS == 30);
    CHECK(WM_SELECT_RND_MOVE_SPEED == 5);
    CHECK(WM_SELECT_RND_WANDER == 14);

    const wm_select_point expected_pos[WM_SELECT_VISIBLE_SLOTS] = {
        {164,45},{204,45},{164,90},{204,90},
        {164,135},{204,135},{164,180},{204,180}
    };
    for (unsigned i = 0; i < WM_SELECT_VISIBLE_SLOTS; ++i) {
        CHECK(wm_select_crouton_positions[i].x == expected_pos[i].x);
        CHECK(wm_select_crouton_positions[i].y == expected_pos[i].y);
    }

    CHECK(strcmp(wm_select_background_modules[0].bmod_name, "wwfselbkBMOD") == 0);
    CHECK(wm_select_background_modules[0].x == -40 && wm_select_background_modules[0].y == 0);
    CHECK(strcmp(wm_select_background_modules[1].bmod_name, "choiceBMOD") == 0);
    CHECK(wm_select_background_modules[1].x == 3 && wm_select_background_modules[1].y == 256);

    CHECK(wm_select_slot_to_source_wrestler(0) == WM_SOURCE_WRESTLER_DOINK);
    CHECK(wm_select_slot_to_source_wrestler(1) == WM_SOURCE_WRESTLER_RAZOR);
    CHECK(wm_select_slot_to_source_wrestler(6) == WM_SOURCE_WRESTLER_BRET);
    CHECK(wm_select_slot_to_source_wrestler(7) == WM_SOURCE_WRESTLER_LEX);
    CHECK(wm_select_slot_to_source_wrestler(8) == 0xff);

    CHECK(wm_select_source_attributes[WM_SOURCE_WRESTLER_BRET].power == 4);
    CHECK(wm_select_source_attributes[WM_SOURCE_WRESTLER_BRET].speed == 9);
    CHECK(wm_select_source_attributes[WM_SOURCE_WRESTLER_BRET].agility == 9);
    CHECK(wm_select_source_attributes[WM_SOURCE_WRESTLER_BRET].recovery == 3);
    CHECK(wm_select_source_attributes[WM_SOURCE_WRESTLER_DOINK].recovery == 8);
    CHECK(wm_select_source_attributes[WM_SOURCE_WRESTLER_LEX].power == 9);

    CHECK(wm_select_players[0].start_index == 0);
    CHECK(wm_select_players[0].mug_position.x == 20);
    CHECK(wm_select_players[0].mug_position.y == 175);
    CHECK(!wm_select_players[0].mug_flip_x);
    CHECK(wm_select_players[0].move_sound == 0xc8);
    CHECK(wm_select_players[0].select_sound == 0xcb);
    CHECK(wm_select_players[1].start_index == 1);
    CHECK(wm_select_players[1].mug_position.x == 382);
    CHECK(wm_select_players[1].mug_flip_x);
    CHECK(wm_select_players[1].move_sound == 0xc7);
    CHECK(wm_select_players[1].select_sound == 0xcc);

    CHECK(wm_select_move(0, WM_SELECT_DIR_DOWN) == 2);
    CHECK(wm_select_move(6, WM_SELECT_DIR_DOWN) == 6);
    CHECK(wm_select_move(7, WM_SELECT_DIR_DOWN) == 7);
    CHECK(wm_select_move(2, WM_SELECT_DIR_UP) == 0);
    CHECK(wm_select_move(1, WM_SELECT_DIR_UP) == 1);
    CHECK(wm_select_move(1, WM_SELECT_DIR_LEFT) == 0);
    CHECK(wm_select_move(0, WM_SELECT_DIR_LEFT) == 0);
    CHECK(wm_select_move(0, WM_SELECT_DIR_RIGHT) == 1);
    CHECK(wm_select_move(1, WM_SELECT_DIR_RIGHT) == 1);

    /* Source random direction 0 means sideways. Illegal vertical moves use
       a second source RNG bit to choose sideways vs the legal vertical. */
    CHECK(wm_select_random_move(4, 0, false) == 5);
    CHECK(wm_select_random_move(0, 2, false) == 1);
    CHECK(wm_select_random_move(0, 2, true) == 2);
    CHECK(wm_select_random_move(6, 1, false) == 7);
    CHECK(wm_select_random_move(6, 1, true) == 4);

    CHECK(wm_select_home_move(4, 5, 1, 0, false) == 5); /* same row */
    CHECK(wm_select_home_move(6, 2, 1, 0, false) == 4); /* home upward */
    CHECK(wm_select_home_move(0, 6, 1, 0, false) == 2); /* home downward */
    CHECK(wm_select_home_move(4, 0, 0, 0, false) == 5); /* one-in-three wander */

    wm_wrestler_id rid = WM_WRESTLER_COUNT;
    CHECK(wm_select_source_to_roster(WM_SOURCE_WRESTLER_RAZOR, &rid));
    CHECK(rid == WM_WRESTLER_RAZOR);
    CHECK(wm_select_source_to_roster(WM_SOURCE_WRESTLER_LEX, &rid));
    CHECK(rid == WM_WRESTLER_LEX);
    CHECK(!wm_select_source_to_roster(WM_SOURCE_WRESTLER_ADAM_BOMB, &rid));

    wm_select_cursor c;
    wm_select_cursor_init(&c, 0);
    CHECK(c.index == 0 && c.start_index == 0 && c.random_dest == -1);
    CHECK(wm_select_random_can_begin(&c, true, true));
    c.index = 2;
    CHECK(!wm_select_random_can_begin(&c, true, true));
    c.index = 0;
    CHECK(wm_select_begin_random(&c, 7));
    CHECK(c.random_dest == 7 && c.random_delay == 5 && c.random_wander == 14);
    for (unsigned i = 0; i < 14; ++i)
        (void)wm_select_random_event(&c, 1, 0, false);
    CHECK(c.random_wander == 0);

    c.index = 6;
    c.random_dest = 6;
    CHECK(wm_select_random_event(&c, 1, 0, false));
    uint8_t chosen = 0xff;
    CHECK(wm_select_choose(&c, &chosen));
    CHECK(chosen == WM_SOURCE_WRESTLER_BRET);
    CHECK(c.selected && c.selected_source_wrestler == WM_SOURCE_WRESTLER_BRET);
    CHECK(!wm_select_choose(&c, &chosen));


    wm_select_clock clock;
    wm_select_clock_init(&clock, 1);
    CHECK(clock.ticks_remaining == WM_SELECT_TIME_TICKS && !clock.time_out);
    for (unsigned i = 0; i < WM_SELECT_TIME_TICKS - 1; ++i)
        wm_select_clock_tick(&clock, 1, 0);
    CHECK(clock.ticks_remaining == 1 && !clock.time_out);
    wm_select_clock_tick(&clock, 1, 0);
    CHECK(clock.ticks_remaining == 0 && clock.time_out);

    wm_select_clock_init(&clock, 1);
    wm_select_clock_tick(&clock, 3, 0); /* new buy-in / PSTATUS change resets */
    CHECK(clock.pstatus_snapshot == 3);
    CHECK(clock.ticks_remaining == WM_SELECT_TIME_TICKS);
    for (unsigned i = 0; i < WM_SELECT_TIME_TICKS; ++i)
        wm_select_clock_tick(&clock, 3, 2);
    CHECK(!clock.time_out); /* OLD_PSTATUS sends source back to #reset */
    CHECK(clock.ticks_remaining == WM_SELECT_TIME_TICKS);
}

int main(void) {
    test_scheduler();
    test_source_clock();
    test_source_select_core();
    test_bmod_decode();
    test_source_sequence();
    test_anim();
    test_source_anim_typed_stream();
    test_ropes();
    test_roster();
    test_secondary_composite_offsets();
    test_visual_sequences();
    test_demo_four_way_and_run();
    test_four_attack_buttons();
    test_block();
    test_cpu_chases();
    test_demo_bounds();
    test_source_attract_sequence();
    test_attract_source_flow();
    test_video_frame_source_clock_and_input_latch();
    puts("all core tests passed");
    return 0;
}
