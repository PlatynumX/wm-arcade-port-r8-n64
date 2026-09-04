#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wm/anim.h"
#include "wm/app.h"
#include "wm/bmod.h"
#include "wm/source_clock.h"
#include "wm/bret_visuals.h"
#include "wm/composite.h"
#include "wm/demo.h"
#include "wm/bret_backend.h"
#include "wm/bret_visuals.h"
#include "wm/game.h"
#include "wm/match.h"
#include "wm/movement.h"
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
    CHECK(d.p1.action == WM_DEMO_IDLE);
    CHECK(d.p1.facing == WM_DEMO_FACING_6);

    wm_input_state right = {.stick_x = 80};
    wm_demo_tick(&d, &right);
    CHECK(d.p1.action == WM_DEMO_WALK);
    CHECK(d.p1.facing == WM_DEMO_FACING_6);
    CHECK(d.p1.flip_x);
    CHECK(d.p1.visual.sequence == &wm_bret_walk6_f4_anim);

    wm_input_state left = {.stick_x = -80};
    wm_demo_tick(&d, &left);
    CHECK(d.p1.facing == WM_DEMO_FACING_4);
    CHECK(!d.p1.flip_x);
    CHECK(d.p1.visual.sequence == &wm_bret_walk4_f4_anim);

    wm_input_state up_run = {.stick_y = 80, .run = true};
    int y0 = d.p1.screen_y;
    wm_demo_tick(&d, &up_run);
    CHECK(d.p1.action == WM_DEMO_RUN);
    CHECK(d.p1.facing == WM_DEMO_FACING_8);
    CHECK(d.p1.screen_y < y0);
    CHECK(d.p1.visual.sequence == &wm_bret_run_anim);

    wm_input_state down = {.stick_y = -80};
    wm_demo_tick(&d, &down);
    CHECK(d.p1.action == WM_DEMO_WALK);
    CHECK(d.p1.facing == WM_DEMO_FACING_2);
    CHECK(d.p1.visual.sequence == &wm_bret_walk2_f2_anim);
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
                    &wm_bret_light_punch4_anim, 5);
    run_attack_case((wm_input_state){.power_punch=true}, WM_DEMO_POWER_PUNCH,
                    &wm_bret_power_punch_anim, 12);
    run_attack_case((wm_input_state){.light_kick=true}, WM_DEMO_LIGHT_KICK,
                    &wm_bret_light_kick4_anim, 5);
    run_attack_case((wm_input_state){.power_kick=true}, WM_DEMO_POWER_KICK,
                    &wm_bret_power_kick_anim, 15);
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

/* ATTRACT.ASM::show_gameplay draws RNDRNG0(7) and bumps a 7 draw to 8, so
   the result is never exactly 7 (wm_arcade_roster_id_t skips 7 for the
   same reason). Sweep every RAND seed byte so both branches are exercised. */
static void test_match_draw_wrestler_index_skips_seven(void) {
    for (uint32_t seed = 0; seed < 256; ++seed) {
        WmRng rng;
        wm_rng_init(&rng, seed, NULL, NULL, NULL);
        unsigned idx = wm_match_draw_wrestler_index(&rng);
        CHECK(idx <= 8);
        CHECK(idx != 7);
    }
}

static void test_match_start_attract(void) {
    WmRng rng;
    wm_match_state m;
    wm_rng_init(&rng, 0x1234u, NULL, NULL, NULL);
    wm_match_init(&m);
    CHECK(!m.active);

    wm_match_start_attract(&m, &rng);
    CHECK(m.active);
    CHECK(m.actor_count == WM_MATCH_MAX_ACTORS);
    CHECK(m.index1 <= 8 && m.index1 != 7);
    CHECK(m.opponent_wrestler <= 8 && m.opponent_wrestler != 7);

    /* LIFEBAR.ASM::init_life_data: LIFE_MAX = 163. */
    CHECK(m.actors[0].life == 163);
    CHECK(m.actors[1].life == 163);
    CHECK(m.actors[0].active && m.actors[0].in_ring);
    CHECK(m.actors[1].active && m.actors[1].in_ring);

    /* WRESTLE.ASM #0plyr: P1 drone PLYRNUM=2/PSIDE_PLYR1=0,
       opponent PLYRNUM=3/PSIDE_PLYR2=1. */
    CHECK(m.actors[0].player_num == 2);
    CHECK(m.actors[0].player_side == 0);
    CHECK(m.actors[0].wrestler_num == (int32_t)m.index1);
    CHECK(m.actors[1].player_num == 3);
    CHECK(m.actors[1].player_side == 1);
    CHECK(m.actors[1].wrestler_num == (int32_t)m.opponent_wrestler);

    CHECK(m.actors[0].smart_target == &m.actors[1]);
    CHECK(m.actors[1].smart_target == &m.actors[0]);
    CHECK(m.tick_count == 0);
}

static uint32_t test_match_rndrng0_cb(uint32_t max_inclusive, void *user) {
    return wm_rng_rndrng0((WmRng *)user, max_inclusive);
}

static void test_match_tick_runs_without_crashing(void) {
    WmRng rng;
    wm_match_state m;
    wm_arcade_drone_callbacks_t cb;
    wm_rng_init(&rng, 0x9999u, NULL, NULL, NULL);
    wm_match_init(&m);
    wm_match_start_attract(&m, &rng);

    memset(&cb, 0, sizeof(cb));
    cb.rndrng0_upto = test_match_rndrng0_cb;
    cb.user = &rng;

    for (unsigned i = 0; i < 500; ++i)
        wm_match_tick(&m, &cb);
    CHECK(m.tick_count == 500);
    /* Life is untouched: wm_bret_backend's callbacks don't wire
       adjust_health, and neither wrestler has a backend that could hit the
       other yet (see wm/match.h and wm/bret_backend.h). */
    CHECK(m.actors[0].life == 163);
    CHECK(m.actors[1].life == 163);

    /* Ticking an inactive match is a documented no-op. */
    wm_match_state inactive;
    wm_match_init(&inactive);
    wm_match_tick(&inactive, &cb);
    CHECK(inactive.tick_count == 0);
}

static void test_bret_anim_sequence_mapping(void) {
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_STAND2) == &wm_bret_stand2_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_STAND4) == &wm_bret_stand4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_TORSO2) == &wm_bret_torso2_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_TORSO4) == &wm_bret_torso4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_PUNCH2) == &wm_bret_light_punch2_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_PUNCH4) == &wm_bret_light_punch4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_SUPER_PUNCH2_4) == &wm_bret_power_punch_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_KICK2) == &wm_bret_light_kick2_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_KICK4) == &wm_bret_light_kick4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_SUPER_KICK2) == &wm_bret_power_kick_anim);

    /* Deliberately unmapped: no "hrt_2_super_punch_anim" exists in the
       source tree, and "hrt_4_super_kick_anim" (HRTSEQ2.ASM:1335) exists
       but has not been extracted yet -- see wm/bret_backend.h. */
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_SUPER_PUNCH2_2) == NULL);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_SUPER_PUNCH4) == NULL);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_SUPER_KICK4) == NULL);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_PIN2) == NULL);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_FINISH1) == NULL);
}

static void test_bret_backend_change_anim(void) {
    wm_bret_backend_actor bva;
    wm_bret_backend_init(&bva);
    CHECK(bva.visual.sequence == NULL);

    wm_bret_backend_change_anim(NULL, WM_BRET_ANIM_STAND2, &bva);
    CHECK(bva.visual.sequence == &wm_bret_stand2_anim);
    CHECK(bva.visual.just_started);

    /* Advance a few frames, then re-request the same id: must not restart
       (same "only restart on change" rule as wm/demo.c's set_action). */
    wm_bret_backend_tick(&bva);
    wm_bret_backend_tick(&bva);
    size_t frame_before = bva.visual.frame_index;
    wm_bret_backend_change_anim(NULL, WM_BRET_ANIM_STAND2, &bva);
    CHECK(bva.visual.frame_index == frame_before);

    /* An unmapped id no-ops rather than clearing the sequence. */
    wm_bret_backend_change_anim(NULL, WM_BRET_ANIM_FINISH1, &bva);
    CHECK(bva.visual.sequence == &wm_bret_stand2_anim);

    wm_bret_backend_change_torso_anim(NULL, WM_BRET_ANIM_TORSO4, &bva);
    CHECK(bva.torso_visual.sequence == &wm_bret_torso4_anim);
}

static void test_bret_ani_init_facing(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    wm_arcade_bret_callbacks_t cb;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    cb = wm_bret_backend_callbacks(&bva);

    a.facing_dir = 0; /* not WM_MOVE_RIGHT -> the "4" (STAND4/TORSO4) branch */
    wm_arcade_bret_ani_init(&a, &cb);
    CHECK(bva.visual.sequence == &wm_bret_stand4_anim);
    CHECK(bva.torso_visual.sequence == &wm_bret_torso4_anim);

    wm_bret_backend_init(&bva);
    cb = wm_bret_backend_callbacks(&bva);
    a.facing_dir = WM_MOVE_RIGHT;
    wm_arcade_bret_ani_init(&a, &cb);
    CHECK(bva.visual.sequence == &wm_bret_stand2_anim);
    CHECK(bva.torso_visual.sequence == &wm_bret_torso2_anim);
}

static void test_convert_facing(void) {
    CHECK(wm_convert_facing(WM_MOVE_UP) == 0);
    CHECK(wm_convert_facing(WM_MOVE_UP_RIGHT) == 1);
    CHECK(wm_convert_facing(WM_MOVE_RIGHT) == 2);
    CHECK(wm_convert_facing(WM_MOVE_DOWN_RIGHT) == 3);
    CHECK(wm_convert_facing(WM_MOVE_DOWN) == 4);
    CHECK(wm_convert_facing(WM_MOVE_DOWN_LEFT) == 5);
    CHECK(wm_convert_facing(WM_MOVE_LEFT) == 6);
    CHECK(wm_convert_facing(WM_MOVE_UP_LEFT) == 7);
    /* WRESTLE.ASM:4514/4517/4521/4525-4529: 0, 3, 7, 11-15 all mean "zip". */
    CHECK(wm_convert_facing(WM_MOVE_ZIP) == -1);
    CHECK(wm_convert_facing(3) == -1);
    CHECK(wm_convert_facing(11) == -1);
}

static void test_set_velocities_normal(void) {
    wm_arcade_actor_t a;
    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_RIGHT;
    a.facing_dir = WM_MOVE_RIGHT; /* not moving backward */
    wm_set_velocities(&a, NULL, wm_bret_velocity_table);
    CHECK(a.x_vel == WM_BRET_WALK_VEL);
    CHECK(a.z_vel == 0);
}

static void test_set_velocities_backward_reduction(void) {
    wm_arcade_actor_t a;
    int32_t expect_x;
    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_LEFT;
    a.facing_dir = WM_MOVE_RIGHT; /* opposite horizontal -> backward */
    wm_set_velocities(&a, NULL, wm_bret_velocity_table);
    /* WRESTLE.ASM MULT = 256*90/100 = 230. */
    expect_x = (int32_t)(((int64_t)-WM_BRET_WALK_VEL * 230) >> 8);
    CHECK(a.x_vel == expect_x);
    CHECK(a.x_vel != -WM_BRET_WALK_VEL);
}

static void test_set_velocities_ground_boost(void) {
    wm_arcade_actor_t a, opp;
    int32_t expect_x;
    memset(&a, 0, sizeof(a));
    memset(&opp, 0, sizeof(opp));
    a.move_dir = WM_MOVE_RIGHT;
    a.facing_dir = WM_MOVE_RIGHT;
    opp.player_mode = WM_PMODE_ONGROUND;
    wm_set_velocities(&a, &opp, wm_bret_velocity_table);
    /* WRESTLE.ASM GRND_MULT = 256*150/100 = 384. */
    expect_x = (int32_t)(((int64_t)WM_BRET_WALK_VEL * 384) >> 8);
    CHECK(a.x_vel == expect_x);

    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_RIGHT;
    a.facing_dir = WM_MOVE_RIGHT;
    a.walk_fast = 1; /* same boost without any opponent at all */
    wm_set_velocities(&a, NULL, wm_bret_velocity_table);
    CHECK(a.x_vel == expect_x);
}

static void test_execute_walk_flip_and_zip(void) {
    wm_arcade_actor_t a;

    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_LEFT;
    wm_execute_walk(&a, NULL, wm_bret_velocity_table);
    CHECK(a.obj_control & WM_OBJ_FLIPH);
    CHECK(a.move_dir == WM_MOVE_LEFT);
    CHECK(a.x_vel == -WM_BRET_WALK_VEL);

    /* WRESTLE.ASM #right/#up_right/#down_right clear M_FLIPH. */
    memset(&a, 0, sizeof(a));
    a.obj_control = WM_OBJ_FLIPH;
    a.move_dir = WM_MOVE_RIGHT;
    wm_execute_walk(&a, NULL, wm_bret_velocity_table);
    CHECK(!(a.obj_control & WM_OBJ_FLIPH));

    /* #up/#down never touch OBJ_CONTROL. */
    memset(&a, 0, sizeof(a));
    a.obj_control = WM_OBJ_FLIPH;
    a.move_dir = WM_MOVE_UP;
    wm_execute_walk(&a, NULL, wm_bret_velocity_table);
    CHECK(a.obj_control & WM_OBJ_FLIPH);
    CHECK(a.x_vel == 0);
    CHECK(a.z_vel == -WM_BRET_WALK_VEL);

    /* #zip clears MOVE_DIR and both velocities. */
    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_ZIP;
    a.x_vel = 12345;
    a.z_vel = 6789;
    wm_execute_walk(&a, NULL, wm_bret_velocity_table);
    CHECK(a.move_dir == 0);
    CHECK(a.x_vel == 0);
    CHECK(a.z_vel == 0);
}

static void test_integrate_position(void) {
    wm_arcade_actor_t a;
    memset(&a, 0, sizeof(a));
    a.x_vel = WM_BRET_WALK_VEL;
    a.z_vel = -WM_BRET_WALK_VEL;
    for (int i = 0; i < 10; ++i) wm_integrate_position(&a);
    CHECK(a.x_fixed == (int32_t)(WM_BRET_WALK_VEL * 10));
    CHECK(a.z_fixed == (int32_t)(-WM_BRET_WALK_VEL * 10));
    CHECK(a.x_int == a.x_fixed >> 16);
    CHECK(a.z_int == a.z_fixed >> 16);
}

/* End-to-end through wm_match: when the RNG draws Bret for P1, the actor's
   idle stand animation is real (set once by wm_arcade_bret_ani_init inside
   wm_match_start_attract) and stays selected and running across ticks, since
   the idle drone AI never gives mode_normal a reason to change it -- see
   wm/match.h. (wm_execute_walk itself is real and wired; it just has no
   input to act on yet.) */
static void test_match_bret_idle_animates(void) {
    WmRng rng;
    wm_match_state m;
    wm_arcade_drone_callbacks_t cb;
    uint32_t seed;
    bool found = false;

    for (seed = 0; seed < 4096 && !found; ++seed) {
        WmRng probe;
        wm_rng_init(&probe, seed, NULL, NULL, NULL);
        if (wm_match_draw_wrestler_index(&probe) == WM_ROSTER_BRET) found = true;
    }
    CHECK(found);

    wm_rng_init(&rng, seed, NULL, NULL, NULL);
    wm_match_init(&m);
    wm_match_start_attract(&m, &rng);
    CHECK(m.actors[0].wrestler_num == WM_ROSTER_BRET);
    CHECK(m.bret_visual[0].visual.sequence != NULL);
    const wm_visual_sequence *initial = m.bret_visual[0].visual.sequence;
    CHECK(initial == &wm_bret_stand2_anim || initial == &wm_bret_stand4_anim);
    uint16_t ticks_left_at_start = m.bret_visual[0].visual.ticks_left;
    size_t frame_at_start = m.bret_visual[0].visual.frame_index;

    memset(&cb, 0, sizeof(cb));
    cb.rndrng0_upto = test_match_rndrng0_cb;
    cb.user = &rng;
    for (unsigned i = 0; i < 50; ++i) {
        wm_match_tick(&m, &cb);
        CHECK(m.bret_visual[0].visual.sequence == initial);
    }
    /* wm_visual_tick actually advanced frames over 50 ticks; it isn't a
       frozen sprite. */
    CHECK(m.bret_visual[0].visual.frame_index != frame_at_start ||
          m.bret_visual[0].visual.ticks_left != ticks_left_at_start);
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
    CHECK(wm_attract_call_port_status(WM_ATTRACT_SHOW_GAMEPLAY) == WM_PORT_PARTIAL_SOURCE);
    CHECK(wm_attract_call_port_status(WM_ATTRACT_SHOW_TITLE) == WM_PORT_PARTIAL_SOURCE);
    CHECK(wm_attract_call_is_translated(WM_ATTRACT_SHOW_GAMEPLAY));
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
    /* WRESTLE.ASM::start_match's #0plyr path runs on the first gameplay
       tick, not the transition tick that entered this call. */
    CHECK(!app.match.active);

    wm_app_tick(&app, &button);
    CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);
    CHECK(app.attract.call_ticks == 1);
    CHECK(app.match.active);
    CHECK(app.match.index1 <= 8 && app.match.index1 != 7);
    CHECK(app.match.actors[0].life == 163);
    CHECK(app.match.actors[1].life == 163);
    CHECK(app.match.actors[0].player_num == 2);
    CHECK(app.match.actors[1].player_num == 3);
    CHECK(app.match.tick_count == 1);

    for (unsigned i = 1; i < WM_GAMEPLAY_BUTTON_ENABLE_TICKS; ++i) {
        wm_app_tick(&app, &button);
        CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);
    }
    CHECK(app.attract.call_ticks == WM_GAMEPLAY_BUTTON_ENABLE_TICKS);
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
    CHECK(app.attract.title_random_state != 0x57574631u);
    wm_app_tick(&app, &button);
    CHECK(wm_process_find_id(&app.scheduler, WM_PID_CYCLE_LAVA) == NULL);
    CHECK(wm_process_find_id(&app.scheduler, WM_PID_FLASH) == NULL);
    CHECK(wm_process_find_id(&app.scheduler, WM_PID_ATTRACT_ANIM) == NULL);
    /* Second show_gameplay occurrence in the loop (see
       test_source_attract_sequence); drive through it the same way. */
    CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);
    CHECK(!app.match.active);
    for (unsigned i = 0; i < WM_GAMEPLAY_BUTTON_ENABLE_TICKS; ++i) {
        wm_app_tick(&app, &button);
        CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);
    }
    CHECK(app.match.active);
    wm_app_tick(&app, &button);
    CHECK(app.attract.call == WM_ATTRACT_DCS_LOGO);
    CHECK(app.attract.amode_loops == 1);

    CHECK(app.demo.total_hits == 0);
    CHECK(app.demo.total_blocks == 0);

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
    test_match_draw_wrestler_index_skips_seven();
    test_match_start_attract();
    test_match_tick_runs_without_crashing();
    test_bret_anim_sequence_mapping();
    test_bret_backend_change_anim();
    test_bret_ani_init_facing();
    test_convert_facing();
    test_set_velocities_normal();
    test_set_velocities_backward_reduction();
    test_set_velocities_ground_boost();
    test_execute_walk_flip_and_zip();
    test_integrate_position();
    test_match_bret_idle_animates();
    test_source_attract_sequence();
    test_attract_source_flow();
    test_video_frame_source_clock_and_input_latch();
    puts("all core tests passed");
    return 0;
}
