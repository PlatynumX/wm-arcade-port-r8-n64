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
#include "wm/game.h"
#include "wm/human_input.h"
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
        wm_match_tick(&m, &cb, NULL);
    CHECK(m.tick_count == 500);
    /* Life is untouched: wm_bret_backend's callbacks don't wire
       adjust_health, and neither wrestler has a backend that could hit the
       other yet (see wm/match.h and wm/bret_backend.h). */
    CHECK(m.actors[0].life == 163);
    CHECK(m.actors[1].life == 163);

    /* Ticking an inactive match is a documented no-op. */
    wm_match_state inactive;
    wm_match_init(&inactive);
    wm_match_tick(&inactive, &cb, NULL);
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
    wm_bret_backend_tick(&bva, NULL, 0);
    wm_bret_backend_tick(&bva, NULL, 0);
    size_t frame_before = bva.visual.frame_index;
    wm_bret_backend_change_anim(NULL, WM_BRET_ANIM_STAND2, &bva);
    CHECK(bva.visual.frame_index == frame_before);

    /* An unmapped id no-ops rather than clearing the sequence. */
    wm_bret_backend_change_anim(NULL, WM_BRET_ANIM_FINISH1, &bva);
    CHECK(bva.visual.sequence == &wm_bret_stand2_anim);

    wm_bret_backend_change_torso_anim(NULL, WM_BRET_ANIM_TORSO4, &bva);
    CHECK(bva.torso_visual.sequence == &wm_bret_torso4_anim);
}

/* BRET.ASM:1325-1350 mode_normal's own I_WILL_DIE self-death resolution,
   through the real wm_arcade_move_bret dispatch (not a direct call into
   wm_arcade_adjust_health): wm_bret_backend_callbacks() now wires
   adjust_health for real, so this actually applies the -10 hit and SETMODE
   DEAD instead of silently no-opping (cb->adjust_health was NULL before). */
static void test_bret_backend_i_will_die_resolves_through_move_bret(void) {
    wm_arcade_actor_t actor;
    wm_bret_backend_actor bva;
    wm_arcade_bret_callbacks_t cb;
    wm_arcade_bret_env_t env;

    memset(&actor, 0, sizeof(actor));
    actor.life = 3;
    actor.player_mode = WM_PMODE_NORMAL;
    actor.i_will_die = 1;

    wm_bret_backend_init(&bva);
    bva.attract_mode = false;
    cb = wm_bret_backend_callbacks(&bva);
    memset(&env, 0, sizeof(env));

    (void)wm_arcade_move_bret(&actor, NULL, &env, &cb);

    /* Deliberately unmapped id (see wm/bret_backend.h): current_id is still
       recorded even though no wm_visual_sequence exists for it. */
    CHECK(bva.current_id == WM_BRET_ANIM_FALL_BACK);
    CHECK(actor.life == 0);
    CHECK(actor.player_mode == WM_PMODE_DEAD);
    CHECK(actor.i_will_die == 0);
}

/* HRTSEQ2.ASM:204 ANI_ATTACK_ON_Z, AMODE_PUNCH,30,91,-45,50,15,45, hand-traced
   to fire right before wm_bret_light_punch2_anim's frame index 5 (of 11) --
   see wm_bret_backend_tick's own comment for how that index was derived. */
static void test_bret_attack_window_punch2(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    int guard;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_PUNCH2, &bva);
    CHECK(bva.visual.sequence == &wm_bret_light_punch2_anim);
    CHECK(!(a.anim_mode & WM_MODE_CHECKHIT));

    for (guard = 0; guard < 20 && bva.visual.frame_index != 5; ++guard)
        wm_bret_backend_tick(&bva, &a, (uint16_t)guard);
    CHECK(bva.visual.frame_index == 5);
    CHECK(a.anim_mode & WM_MODE_CHECKHIT);
    CHECK(a.attack_mode == WM_AMODE_PUNCH);
    CHECK(a.attack_xoff == 30);
    CHECK(a.attack_yoff == 91);
    CHECK(a.attack_zoff == -45);
    CHECK(a.attack_width == 50);
    CHECK(a.attack_height == 15);
    CHECK(a.attack_depth == 45);

    /* Re-ticking the same active frame does not refire (attack_time only
       moves when ATTACK_OFF actually runs). */
    uint16_t attack_time_before = a.attack_time;
    wm_bret_backend_tick(&bva, &a, 999);
    CHECK(a.attack_time == attack_time_before);

    /* Advancing past the active frame turns WM_MODE_CHECKHIT back off
       (ATTACK_OFF, HRTSEQ2.ASM:206) and records attack_time. */
    for (guard = 0; guard < 20 && bva.visual.frame_index == 5; ++guard)
        wm_bret_backend_tick(&bva, &a, 42);
    CHECK(bva.visual.frame_index != 5);
    CHECK(!(a.anim_mode & WM_MODE_CHECKHIT));
    CHECK(a.attack_time == 42);
}

/* Spot-check the remaining 5 attack windows' literal ANI_ATTACK_ON(_Z) args
   directly (no frame stepping -- test_bret_attack_window_punch2 already
   proves the frame-index-driven activation/deactivation mechanism). */
static void test_bret_attack_windows_remaining(void) {
    static const struct {
        wm_arcade_bret_anim_id_t id;
        const wm_visual_sequence *seq;
        size_t frame;
        uint16_t attack_mode;
        int16_t xoff, yoff, zoff, width, height, depth;
        bool use_z;
    } cases[] = {
        { WM_BRET_ANIM_PUNCH4, &wm_bret_light_punch4_anim, 5,
          WM_AMODE_PUNCH, 30, 91, 0, 50, 15, 45, true },
        { WM_BRET_ANIM_SUPER_PUNCH2_4, &wm_bret_power_punch_anim, 5,
          WM_AMODE_UPRCUT, -6, 40, 0, 64, 90, 0, false },
        { WM_BRET_ANIM_KICK2, &wm_bret_light_kick2_anim, 5,
          WM_AMODE_KICK, 23, 73, 0, 50, 17, 0, false },
        { WM_BRET_ANIM_KICK4, &wm_bret_light_kick4_anim, 5,
          WM_AMODE_KICK, 23, 73, 0, 50, 17, 0, false },
        { WM_BRET_ANIM_SUPER_KICK2, &wm_bret_power_kick_anim, 4,
          WM_AMODE_SUPER_KICK, 5, 54, 0, 70, 34, 0, false },
    };
    size_t i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        wm_arcade_actor_t a;
        wm_bret_backend_actor bva;
        int guard;

        memset(&a, 0, sizeof(a));
        wm_bret_backend_init(&bva);
        wm_bret_backend_change_anim(&a, cases[i].id, &bva);
        CHECK(bva.visual.sequence == cases[i].seq);

        for (guard = 0; guard < 30 && bva.visual.frame_index != cases[i].frame; ++guard)
            wm_bret_backend_tick(&bva, &a, 0);
        CHECK(bva.visual.frame_index == cases[i].frame);
        CHECK(a.anim_mode & WM_MODE_CHECKHIT);
        CHECK(a.attack_mode == cases[i].attack_mode);
        CHECK(a.attack_xoff == cases[i].xoff);
        CHECK(a.attack_yoff == cases[i].yoff);
        CHECK(a.attack_width == cases[i].width);
        CHECK(a.attack_height == cases[i].height);
        if (cases[i].use_z) {
            CHECK(a.attack_zoff == cases[i].zoff);
            CHECK(a.attack_depth == cases[i].depth);
        } else {
            /* wm_arcade_ani_attack_on's own real, already-ported defaults
               (src/core/arcade/wm_arcade_anim_combat.c). */
            CHECK(a.attack_zoff == -40);
            CHECK(a.attack_depth == 80);
        }
    }
}

/* wm_bret_hurt_box_for_frame against real numbers independently confirmed
   from the original WIMP containers (tools/wimpimg.py against HRT_PNC.IMG
   and HRT_KIK.IMG) -- not just against the generated table that backs it. */
static void test_bret_hurt_box_for_frame_real_geometry(void) {
    wm_arcade_frame_box_t box = wm_bret_hurt_box_for_frame("H2PL3B04");
    CHECK(box.iani3x == -44);
    CHECK(box.iani3y == -107);
    CHECK(box.iani3z == 119);
    CHECK(box.iani3id == 110);

    box = wm_bret_hurt_box_for_frame("H2KM3A05");
    CHECK(box.iani3x == -41);
    CHECK(box.iani3y == -101);
    CHECK(box.iani3z == 104);
    CHECK(box.iani3id == 102);

    /* Unresolved frame names return an all-zero box (unhittable point)
       rather than guessing or crashing. */
    box = wm_bret_hurt_box_for_frame("NOT_A_REAL_FRAME");
    CHECK(box.iani3x == 0 && box.iani3y == 0 && box.iani3z == 0 && box.iani3id == 0);
    box = wm_bret_hurt_box_for_frame(NULL);
    CHECK(box.iani3x == 0 && box.iani3y == 0 && box.iani3z == 0 && box.iani3id == 0);
}

/* wm_bret_backend_tick actually calls wm_arcade_set_hurt_box every tick
   using the real current frame, not a placeholder. */
static void test_bret_backend_tick_sets_real_hurt_box(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    const wm_visual_frame *cur;
    wm_arcade_frame_box_t expect;

    memset(&a, 0, sizeof(a));
    a.x_int = 1000;
    a.z_int = 500;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_STAND2, &bva);
    CHECK(bva.visual.sequence == &wm_bret_stand2_anim);

    wm_bret_backend_tick(&bva, &a, 0);

    cur = wm_visual_current(&bva.visual);
    CHECK(cur != NULL);
    expect = wm_bret_hurt_box_for_frame(cur->source_frame);
    CHECK(expect.iani3z > 0 && expect.iani3id > 0);

    /* WM_PMODE_NORMAL (the memset-zeroed default): wm_arcade_set_hurt_box's
       zoff=-30/zdepth=60, and the unflipped-x branch since obj_control has
       no WM_OBJ_FLIPH here. */
    CHECK(!(a.obj_control & WM_OBJ_FLIPH));
    CHECK(a.hurt_box.x1 == a.x_int + expect.iani3x);
    CHECK(a.hurt_box.x2 == a.hurt_box.x1 + expect.iani3z);
    CHECK(a.hurt_box.y2 == a.y_int - expect.iani3y);
    CHECK(a.hurt_box.y1 == a.hurt_box.y2 - expect.iani3id);
    CHECK(a.hurt_box.z1 == a.z_int - 30);
    CHECK(a.hurt_box.z2 == a.hurt_box.z1 + 60);
}

static void test_hurt_box_adjust_health(wm_arcade_actor_t *victim, int16_t delta,
                                        wm_arcade_actor_t *source, void *user) {
    (void)source;
    (void)user;
    victim->life += delta;
}

/*
 * Capstone: a real per-frame hurt box (wm_bret_hurt_box_for_frame, set every
 * tick by wm_bret_backend_tick) combined with the real, already
 * ctest-verified-since-fix38 wm_arcade_check_wrestler_collisions() /
 * wm_arcade_wrestler_hit() (REACT1.ASM) / wm_arcade_wrestler_hit_
 * collision_callback bridge -- the exact pieces wm_match_tick now wires
 * together (match.c) -- actually lands a hit and reduces the victim's life.
 *
 * This drives bret_backend.c and the react/combat layer directly rather
 * than through wm_arcade_move_bret's button dispatch, matching how every
 * other attack-window test in this file already exercises an attack
 * (test_bret_attack_window_punch2 etc.): wm_arcade_move_bret's do_punch
 * (BRET.ASM) currently always treats the opponent as point-blank (near(a,
 * 50,45), since nothing in this port yet computes closest_xdist/closest_
 * zdist) and picks the unwired WM_BRET_ANIM_BUTT2/4 headbutt instead of a
 * mapped punch id -- a real, separate gap this test does not exercise or
 * claim to fix.
 */
static void test_hurt_box_connects_a_real_hit(void) {
    wm_arcade_actor_t attacker, victim;
    wm_bret_backend_actor bva_attacker, bva_victim;
    wm_arcade_actor_t *actors[2];
    wm_arcade_combat_runtime_t runtime;
    wm_arcade_react_callbacks_t react_cb;
    wm_arcade_react_bridge_t bridge;
    wm_arcade_combat_callbacks_t combat_cb;
    int32_t life_before;
    int guard;
    bool hit = false;

    memset(&attacker, 0, sizeof(attacker));
    memset(&victim, 0, sizeof(victim));
    attacker.active = 1;
    victim.active = 1;
    attacker.in_ring = 1;
    victim.in_ring = 1;
    attacker.life = 163;
    victim.life = 163;
    attacker.wrestler_num = WM_ROSTER_BRET;
    victim.wrestler_num = WM_ROSTER_BRET;
    attacker.player_mode = WM_PMODE_NORMAL;
    victim.player_mode = WM_PMODE_NORMAL;
    attacker.smart_target = &victim;
    victim.smart_target = &attacker;

    /* Real light-punch ATTACK_ON_Z box (xoff=30, width=50) reaches from
       attacker.x_int=0: [30,80]. Real WM_BRET_ANIM_STAND2 frame 0 hurt box
       (H2ST2A05, xani=19, width=54) at victim.x_int=60: [41,95] -- overlap
       on all three axes (z/y stay at the actors' shared defaults). */
    attacker.x_int = 0;
    victim.x_int = 60;

    wm_bret_backend_init(&bva_attacker);
    wm_bret_backend_init(&bva_victim);
    bva_attacker.opponent = &victim;
    bva_victim.opponent = &attacker;

    wm_bret_backend_change_anim(&attacker, WM_BRET_ANIM_PUNCH2, &bva_attacker);
    wm_bret_backend_change_anim(&victim, WM_BRET_ANIM_STAND2, &bva_victim);
    CHECK(bva_attacker.visual.sequence == &wm_bret_light_punch2_anim);

    life_before = victim.life;
    actors[0] = &attacker;
    actors[1] = &victim;

    wm_arcade_combat_runtime_init(&runtime);
    memset(&react_cb, 0, sizeof(react_cb));
    react_cb.adjust_health = test_hurt_box_adjust_health;
    bridge.runtime = &runtime;
    bridge.callbacks = &react_cb;
    memset(&bridge.last_result, 0, sizeof(bridge.last_result));
    memset(&combat_cb, 0, sizeof(combat_cb));
    combat_cb.wrestler_hit = wm_arcade_wrestler_hit_collision_callback;
    combat_cb.user = &bridge;

    for (guard = 0; guard < 20 && !hit; ++guard) {
        wm_bret_backend_tick(&bva_attacker, &attacker, (uint16_t)guard);
        wm_bret_backend_tick(&bva_victim, &victim, (uint16_t)guard);
        runtime.pcnt = (uint32_t)guard;
        if (wm_arcade_check_wrestler_collisions(actors, 2, (uint32_t)guard, &combat_cb))
            hit = true;
    }

    CHECK(hit);
    CHECK(bva_attacker.visual.frame_index == 5);
    CHECK(attacker.attack_mode == WM_AMODE_PUNCH);
    CHECK(victim.hurt_box.x2 > victim.hurt_box.x1);
    CHECK(bridge.last_result.status == WM_WRESTLER_HIT_OK);
    CHECK(bridge.last_result.health_hook_called);
    CHECK(victim.life < life_before);
}

/* wm_arcade_adjust_health (wm/arcade/wm_arcade_lifebar.h), tested directly
   against LIFEBAR.ASM's literal branch structure -- see that header's own
   comment for the exact line numbers each case below translates. */
static void test_arcade_adjust_health_normal_damage(void) {
    wm_arcade_actor_t victim;
    memset(&victim, 0, sizeof(victim));
    victim.life = 100;
    /* LIFEBAR.ASM:1471-1521: -30 scaled by _85PCT (218/256) -> -26, not a
       plain -30 -- see wm_arcade_adjust_health's own comment. */
    wm_arcade_adjust_health(&victim, -30, NULL, false, 12345u, NULL);
    CHECK(victim.life == 74);
    CHECK(victim.player_mode == WM_PMODE_NORMAL);
    /* LIFEBAR.ASM:1593-1595: unconditional on every call. */
    CHECK(victim.last_damage == (uint16_t)12345u);
}

static void test_arcade_adjust_health_clamps_to_life_max(void) {
    wm_arcade_actor_t victim;
    memset(&victim, 0, sizeof(victim));
    victim.life = WM_ARCADE_LIFE_MAX - 5;
    wm_arcade_adjust_health(&victim, 50, NULL, false, 0, NULL);
    CHECK(victim.life == WM_ARCADE_LIFE_MAX);
}

/* LIFEBAR.ASM:1524-1528 speed_adjustment scaling: the factory-default
   ADJSPEED (WM_ARCADE_SPEED_ADJUSTMENT_16_16, see its own comment for why)
   is exactly 1.0x, so it must not perturb any delta -- unlike
   damage_mod_table, this step also runs on positive (healing) deltas. */
static void test_arcade_adjust_health_speed_adjustment_is_identity(void) {
    wm_arcade_actor_t victim;

    CHECK(WM_ARCADE_SPEED_ADJUSTMENT_16_16 == 0x10000L);

    memset(&victim, 0, sizeof(victim));
    victim.life = 50;
    wm_arcade_adjust_health(&victim, 7, NULL, false, 0, NULL); /* healing */
    CHECK(victim.life == 57);
}

static void test_arcade_adjust_health_fudge_saves_a_near_death_hit(void) {
    wm_arcade_actor_t victim;
    memset(&victim, 0, sizeof(victim));
    /* -24 scaled by _85PCT (LIFEBAR.ASM:1471-1521) -> -21: life+delta = -6,
       > -10 (not overkilled by 10+) and the scaled delta is <= -20 (a 20+
       point hit) -- LIFEBAR.ASM:1561-1569 fudge applies: life = 5. */
    victim.life = 15;
    wm_arcade_adjust_health(&victim, -24, NULL, false, 0, NULL);
    CHECK(victim.life == 5);
    CHECK(victim.player_mode == WM_PMODE_NORMAL);
}

static void test_arcade_adjust_health_attract_mode_never_dies(void) {
    wm_arcade_actor_t victim;
    memset(&victim, 0, sizeof(victim));
    victim.life = 5;
    wm_arcade_adjust_health(&victim, -5, NULL, true, 0, NULL);
    CHECK(victim.life == WM_ARCADE_LIFE_MAX);
    CHECK(victim.player_mode == WM_PMODE_NORMAL);
}

static void test_arcade_adjust_health_death(void) {
    wm_arcade_actor_t victim;
    memset(&victim, 0, sizeof(victim));
    victim.life = 5;
    victim.anim_mode = WM_MODE_CHECKHIT;
    wm_arcade_adjust_health(&victim, -5, NULL, false, 0, NULL);
    CHECK(victim.life == 0);
    CHECK(victim.player_mode == WM_PMODE_DEAD);
    /* LIFEBAR.ASM:1725 calla wres_collis_off. */
    CHECK(!(victim.anim_mode & WM_MODE_CHECKHIT));
}

static void test_arcade_adjust_health_combo_revival_defers_death(void) {
    wm_arcade_actor_t victim, attacker;
    memset(&victim, 0, sizeof(victim));
    memset(&attacker, 0, sizeof(attacker));
    victim.life = 5;
    attacker.combo_count = 3;
    wm_arcade_adjust_health(&victim, -5, &attacker, false, 0, NULL);
    CHECK(victim.life == 1);
    CHECK(victim.i_will_die == 1);
    CHECK(victim.player_mode == WM_PMODE_NORMAL);
}

/* LIFEBAR.ASM:1429-1447: "doing a combo" damage entirely replaces the
   original hit's delta with -max(10-COMBO_COUNT,4), regardless of what the
   original delta was, and clears dam_mult. */
static void test_arcade_adjust_health_combo_damage_overrides_delta(void) {
    wm_arcade_actor_t victim, attacker;
    int32_t dam_mult;

    memset(&victim, 0, sizeof(victim));
    memset(&attacker, 0, sizeof(attacker));
    victim.life = 100;
    attacker.combo_count = 1; /* magnitude = max(10-1,4) = 9, then scaled
                                 by _85PCT (LIFEBAR.ASM:1471-1521) -> -8 */
    dam_mult = 4;
    wm_arcade_adjust_health(&victim, -1, &attacker, false, 0, &dam_mult);
    CHECK(victim.life == 92); /* not 99: the original -1 delta is ignored */
    CHECK(dam_mult == 0);

    memset(&victim, 0, sizeof(victim));
    victim.life = 100;
    attacker.combo_count = 20; /* magnitude = max(10-20,4) = 4 (floored),
                                   scaled by _85PCT -> -4 (unchanged) */
    wm_arcade_adjust_health(&victim, -1, &attacker, false, 0, NULL);
    CHECK(victim.life == 96);
}

/* LIFEBAR.ASM:1449-1465: no combo, but a nonzero dam_mult scales delta by
   (1+dam_mult)/2 and clears it. Each result is then also scaled by _85PCT
   (LIFEBAR.ASM:1471-1521, see wm_arcade_adjust_health's own comment). */
static void test_arcade_adjust_health_dam_mult_scales_delta(void) {
    wm_arcade_actor_t victim;
    int32_t dam_mult;

    memset(&victim, 0, sizeof(victim));
    victim.life = 100;
    dam_mult = 2; /* x1.5: -10*3/2 = -15, then *_85PCT -> -13 */
    wm_arcade_adjust_health(&victim, -10, NULL, false, 0, &dam_mult);
    CHECK(victim.life == 87);
    CHECK(dam_mult == 0);

    memset(&victim, 0, sizeof(victim));
    victim.life = 100;
    dam_mult = 4; /* x2.5: -10*5/2 = -25, then *_85PCT -> -22 */
    wm_arcade_adjust_health(&victim, -10, NULL, false, 0, &dam_mult);
    CHECK(victim.life == 78);
    CHECK(dam_mult == 0);

    /* dam_mult == 0: no DAM_MULT scaling, but _85PCT still applies:
       -10 -> -9. */
    memset(&victim, 0, sizeof(victim));
    victim.life = 100;
    dam_mult = 0;
    wm_arcade_adjust_health(&victim, -10, NULL, false, 0, &dam_mult);
    CHECK(victim.life == 91);
}

/* WHOHITME's own reduced_damage window (REACT1.ASM/wm_arcade_wrestler_hit):
   a second hit on the same victim within 50 ticks deals reduced_damage
   instead of full_damage, but only once LAST_DAMAGE is real. */
static void test_arcade_adjust_health_stamps_last_damage_for_reduced_window(void) {
    wm_arcade_actor_t victim;
    memset(&victim, 0, sizeof(victim));
    victim.life = 100;
    CHECK(victim.last_damage == 0);

    wm_arcade_adjust_health(&victim, -8, NULL, false, 1000u, NULL);
    CHECK(victim.last_damage == 1000u);

    /* A later hit within REACT1.ASM's 50-tick window would now see a
       nonzero, recent last_damage -- see
       test_arcade_wrestler_hit_reduced_damage_within_window below for the
       actual reduced_damage effect. */
    wm_arcade_adjust_health(&victim, -8, NULL, false, 1010u, NULL);
    CHECK(victim.last_damage == 1010u);
}

static void test_hurt_box_real_adjust_health(wm_arcade_actor_t *victim, int16_t delta,
                                             wm_arcade_actor_t *damage_source, void *user) {
    wm_arcade_combat_runtime_t *runtime = (wm_arcade_combat_runtime_t *)user;
    /* Mirrors wm_match_adjust_health's adapter for a human match
       (attract_mode=false, matching wm_match_start_selected). */
    wm_arcade_adjust_health(victim, delta, damage_source, false,
                            runtime ? runtime->pcnt : 0,
                            runtime ? &runtime->dam_mult : NULL);
}

/* End-to-end through wm_arcade_wrestler_hit (REACT1.ASM): a second PUNCH
   within the 50-tick window now actually deals WM_RD_PUNCH (reduced)
   instead of WM_D_PUNCH (full) -- real once wm_arcade_adjust_health stamps
   LAST_DAMAGE for the real callers to read back. */
static void test_arcade_wrestler_hit_reduced_damage_within_window(void) {
    wm_arcade_actor_t attacker, victim;
    wm_arcade_combat_runtime_t runtime;
    wm_arcade_react_callbacks_t cb;
    wm_arcade_wrestler_hit_result_t r1, r2;
    int32_t life_after_first, full_damage_dealt, reduced_damage_dealt;

    memset(&attacker, 0, sizeof(attacker));
    memset(&victim, 0, sizeof(victim));
    attacker.wrestler_num = WM_ROSTER_BRET;
    victim.wrestler_num = WM_ROSTER_BRET;
    attacker.attack_mode = WM_AMODE_PUNCH;
    victim.life = 163;
    victim.player_mode = WM_PMODE_NORMAL;

    wm_arcade_combat_runtime_init(&runtime);
    memset(&cb, 0, sizeof(cb));
    cb.adjust_health = test_hurt_box_real_adjust_health;
    cb.user = &runtime;

    runtime.pcnt = 100;
    r1 = wm_arcade_wrestler_hit(&attacker, &victim, &runtime, &cb);
    CHECK(r1.status == WM_WRESTLER_HIT_OK);
    life_after_first = victim.life;
    full_damage_dealt = 163 - life_after_first;
    CHECK(full_damage_dealt > 0);
    CHECK(victim.last_damage == 100);

    /* WHOHITME must match the current attacker for the window to apply
       (REACT1.ASM's own WHOHITME/A10 check) -- wm_arcade_wrestler_hit sets
       this itself on the first hit. */
    CHECK(victim.who_hit_me == &attacker);

    runtime.pcnt = 130; /* 30 ticks later: elapsed_word(130,100)=30 <= 50 */
    r2 = wm_arcade_wrestler_hit(&attacker, &victim, &runtime, &cb);
    CHECK(r2.status == WM_WRESTLER_HIT_OK);
    reduced_damage_dealt = life_after_first - victim.life;

    CHECK(reduced_damage_dealt > 0);
    CHECK(reduced_damage_dealt < full_damage_dealt);
}

/* End-to-end: wm_arcade_wrestler_hit's own first-hit-of-the-round bonus
   (runtime->dam_mult=2, real and ctest-verified since fix38) now actually
   inflates damage through wm_arcade_adjust_health's DAM_MULT consumption,
   instead of being computed and then silently discarded. */
static void test_arcade_wrestler_hit_first_hit_bonus_deals_more_damage(void) {
    wm_arcade_actor_t attacker, victim_first, victim_no_bonus;
    wm_arcade_combat_runtime_t runtime_first, runtime_no_bonus;
    wm_arcade_react_callbacks_t cb;
    int32_t first_hit_damage, no_bonus_damage;

    memset(&attacker, 0, sizeof(attacker));
    attacker.wrestler_num = WM_ROSTER_BRET;
    attacker.attack_mode = WM_AMODE_PUNCH;

    memset(&cb, 0, sizeof(cb));
    cb.adjust_health = test_hurt_box_real_adjust_health;

    /* First hit of the round: any_hits==0 triggers dam_mult=2. */
    memset(&victim_first, 0, sizeof(victim_first));
    victim_first.wrestler_num = WM_ROSTER_BRET;
    victim_first.life = 163;
    victim_first.player_mode = WM_PMODE_NORMAL;
    wm_arcade_combat_runtime_init(&runtime_first);
    cb.user = &runtime_first;
    (void)wm_arcade_wrestler_hit(&attacker, &victim_first, &runtime_first, &cb);
    first_hit_damage = 163 - victim_first.life;

    /* Same hit, but a hit already landed this round (any_hits==1): no
       first-hit bonus. */
    memset(&victim_no_bonus, 0, sizeof(victim_no_bonus));
    victim_no_bonus.wrestler_num = WM_ROSTER_BRET;
    victim_no_bonus.life = 163;
    victim_no_bonus.player_mode = WM_PMODE_NORMAL;
    wm_arcade_combat_runtime_init(&runtime_no_bonus);
    runtime_no_bonus.any_hits = 1;
    cb.user = &runtime_no_bonus;
    (void)wm_arcade_wrestler_hit(&attacker, &victim_no_bonus, &runtime_no_bonus, &cb);
    no_bonus_damage = 163 - victim_no_bonus.life;

    CHECK(first_hit_damage > no_bonus_damage);
}

/* End-to-end: wm_match_tick's real collision path (wm_arcade_
   check_wrestler_collisions -> wm_arcade_wrestler_hit -> the real
   wm_arcade_adjust_health, via the same adapter shape wm_match_
   adjust_health uses) actually transitions a victim to WM_PMODE_DEAD once
   life reaches 0, not just decrementing it forever. Same setup as
   test_hurt_box_connects_a_real_hit, but with the victim's life preset to
   exactly the real light punch's damage (10, per WM_D_PUNCH=8 and the
   offense/defense-mod math in wm_arcade_wrestler_hit) so the hit is fatal
   but not a 20+pt fudge case. */
static void test_hurt_box_hit_kills_at_zero_life(void) {
    wm_arcade_actor_t attacker, victim;
    wm_bret_backend_actor bva_attacker, bva_victim;
    wm_arcade_actor_t *actors[2];
    wm_arcade_combat_runtime_t runtime;
    wm_arcade_react_callbacks_t react_cb;
    wm_arcade_react_bridge_t bridge;
    wm_arcade_combat_callbacks_t combat_cb;
    int guard;
    bool hit = false;

    memset(&attacker, 0, sizeof(attacker));
    memset(&victim, 0, sizeof(victim));
    attacker.active = 1;
    victim.active = 1;
    attacker.in_ring = 1;
    victim.in_ring = 1;
    attacker.life = 163;
    victim.life = 10;
    attacker.wrestler_num = WM_ROSTER_BRET;
    victim.wrestler_num = WM_ROSTER_BRET;
    attacker.player_mode = WM_PMODE_NORMAL;
    victim.player_mode = WM_PMODE_NORMAL;
    attacker.smart_target = &victim;
    victim.smart_target = &attacker;
    attacker.x_int = 0;
    victim.x_int = 60;

    wm_bret_backend_init(&bva_attacker);
    wm_bret_backend_init(&bva_victim);
    bva_attacker.opponent = &victim;
    bva_victim.opponent = &attacker;

    wm_bret_backend_change_anim(&attacker, WM_BRET_ANIM_PUNCH2, &bva_attacker);
    wm_bret_backend_change_anim(&victim, WM_BRET_ANIM_STAND2, &bva_victim);

    actors[0] = &attacker;
    actors[1] = &victim;

    wm_arcade_combat_runtime_init(&runtime);
    memset(&react_cb, 0, sizeof(react_cb));
    react_cb.adjust_health = test_hurt_box_real_adjust_health;
    react_cb.user = &runtime;
    bridge.runtime = &runtime;
    bridge.callbacks = &react_cb;
    memset(&bridge.last_result, 0, sizeof(bridge.last_result));
    memset(&combat_cb, 0, sizeof(combat_cb));
    combat_cb.wrestler_hit = wm_arcade_wrestler_hit_collision_callback;
    combat_cb.user = &bridge;

    for (guard = 0; guard < 20 && !hit; ++guard) {
        wm_bret_backend_tick(&bva_attacker, &attacker, (uint16_t)guard);
        wm_bret_backend_tick(&bva_victim, &victim, (uint16_t)guard);
        runtime.pcnt = (uint32_t)guard;
        if (wm_arcade_check_wrestler_collisions(actors, 2, (uint32_t)guard, &combat_cb))
            hit = true;
    }

    CHECK(hit);
    CHECK(victim.life == 0);
    CHECK(victim.player_mode == WM_PMODE_DEAD);
    /* wres_collis_off (LIFEBAR.ASM:1725) is called on the dying victim, not
       the attacker -- the attacker's own CHECKHIT bit (its still-active
       punch window) is untouched by adjust_health. */
    CHECK(attacker.anim_mode & WM_MODE_CHECKHIT);
}

/* wm_arcade_get_live_bits (WRESTLE2.ASM::get_live_bits). */
static void test_arcade_get_live_bits(void) {
    wm_arcade_actor_t p1, p2;
    wm_arcade_actor_t *actors[2] = { &p1, &p2 };

    memset(&p1, 0, sizeof(p1));
    memset(&p2, 0, sizeof(p2));
    p1.active = 1;
    p2.active = 1;
    p1.player_side = 0;
    p2.player_side = 1;
    p1.player_mode = WM_PMODE_NORMAL;
    p2.player_mode = WM_PMODE_NORMAL;
    CHECK(wm_arcade_get_live_bits(actors, 2) == 3);

    p2.player_mode = WM_PMODE_DEAD;
    CHECK(wm_arcade_get_live_bits(actors, 2) == 1);

    p1.player_mode = WM_PMODE_DEAD;
    CHECK(wm_arcade_get_live_bits(actors, 2) == 0);

    /* A dead-but-ZOMBIE actor still counts as live. */
    p1.status_flags = WM_STATUS_ZOMBIE;
    CHECK(wm_arcade_get_live_bits(actors, 2) == 1);

    /* An inactive slot is skipped entirely, not counted as dead. */
    p1.status_flags = 0;
    p1.player_mode = WM_PMODE_NORMAL;
    p2.active = 0;
    CHECK(wm_arcade_get_live_bits(actors, 2) == 1);
}

/* wm_arcade_round_tick (WRESTLE2.ASM::match_timer's #1tmded 5-second
   pin-idiot-check countdown). */
static void test_arcade_round_tick_decides_after_pin_timeout(void) {
    wm_arcade_actor_t p1, p2;
    wm_arcade_actor_t *actors[2] = { &p1, &p2 };
    wm_arcade_round_state_t rs;
    int i;

    memset(&p1, 0, sizeof(p1));
    memset(&p2, 0, sizeof(p2));
    p1.active = 1;
    p2.active = 1;
    p1.player_side = 0;
    p2.player_side = 1;
    p1.player_mode = WM_PMODE_NORMAL;
    p2.player_mode = WM_PMODE_DEAD; /* side 1 is fully dead from tick 0 */

    wm_arcade_round_state_init(&rs);
    CHECK(!rs.decided);

    for (i = 0; i < WM_ARCADE_PIN_TIMEOUT_TICKS - 1; ++i) {
        wm_arcade_round_tick(&rs, actors, 2);
        CHECK(!rs.decided);
    }
    wm_arcade_round_tick(&rs, actors, 2);
    CHECK(rs.decided);
    CHECK(rs.decided_winner_side == 0);

    /* Already decided: further ticks are no-ops. */
    wm_arcade_round_tick(&rs, actors, 2);
    CHECK(rs.decided_winner_side == 0);
}

static void test_arcade_round_tick_cancels_on_revival(void) {
    wm_arcade_actor_t p1, p2;
    wm_arcade_actor_t *actors[2] = { &p1, &p2 };
    wm_arcade_round_state_t rs;

    memset(&p1, 0, sizeof(p1));
    memset(&p2, 0, sizeof(p2));
    p1.active = 1;
    p2.active = 1;
    p1.player_side = 0;
    p2.player_side = 1;
    p1.player_mode = WM_PMODE_NORMAL;
    p2.player_mode = WM_PMODE_DEAD;

    wm_arcade_round_state_init(&rs);
    wm_arcade_round_tick(&rs, actors, 2);
    wm_arcade_round_tick(&rs, actors, 2);
    CHECK(rs.pin_timeout == WM_ARCADE_PIN_TIMEOUT_TICKS - 2);

    /* Both sides alive again cancels the countdown. */
    p2.player_mode = WM_PMODE_NORMAL;
    wm_arcade_round_tick(&rs, actors, 2);
    CHECK(rs.pin_timeout == 0);
    CHECK(!rs.decided);
}

static void test_arcade_round_tick_double_ko_is_a_draw(void) {
    wm_arcade_actor_t p1, p2;
    wm_arcade_actor_t *actors[2] = { &p1, &p2 };
    wm_arcade_round_state_t rs;
    int i;

    memset(&p1, 0, sizeof(p1));
    memset(&p2, 0, sizeof(p2));
    p1.active = 1;
    p2.active = 1;
    p1.player_side = 0;
    p2.player_side = 1;
    p1.player_mode = WM_PMODE_DEAD;
    p2.player_mode = WM_PMODE_DEAD;

    wm_arcade_round_state_init(&rs);
    for (i = 0; i < WM_ARCADE_PIN_TIMEOUT_TICKS; ++i)
        wm_arcade_round_tick(&rs, actors, 2);

    CHECK(rs.decided);
    CHECK(rs.decided_winner_side == -1);
}

/* wm_arcade_match_score_award_round (LIFEBAR.ASM::set_winner's KO branch,
   specialized to this port's fixed 1-on-1 case -- see its own comment). */
static void test_arcade_match_score_awards_rounds_and_sets_match_winner(void) {
    wm_arcade_match_score_t score;

    wm_arcade_match_score_init(&score);
    CHECK(score.p1rounds == 0 && score.p2rounds == 0 && score.match_winner == 0);

    wm_arcade_match_score_award_round(&score, 0);
    CHECK(score.p1rounds == 1);
    CHECK(score.p2rounds == 0);
    CHECK(score.match_winner == 0); /* best-of-3: one win isn't the match yet */

    wm_arcade_match_score_award_round(&score, 1);
    CHECK(score.p1rounds == 1);
    CHECK(score.p2rounds == 1);
    CHECK(score.match_winner == 0);

    wm_arcade_match_score_award_round(&score, 0);
    CHECK(score.p1rounds == 2);
    CHECK(score.match_winner == 1); /* side 0 reached 2 round wins */

    /* Once match_winner is set, further awards are a no-op. */
    wm_arcade_match_score_award_round(&score, 1);
    CHECK(score.p2rounds == 1);
    CHECK(score.match_winner == 1);
}

static void test_arcade_match_score_draw_awards_nothing(void) {
    wm_arcade_match_score_t score;

    wm_arcade_match_score_init(&score);
    wm_arcade_match_score_award_round(&score, -1);
    CHECK(score.p1rounds == 0 && score.p2rounds == 0 && score.match_winner == 0);
}

/* End-to-end: a real fatal hit through wm_match_tick (same setup as
   test_hurt_box_hit_kills_at_zero_life) eventually decides the match via
   round_state, not just flipping player_mode. The opponent must also draw
   WM_ROSTER_BRET (like test_match_bret_idle_animates), since only a Bret
   actor gets a real per-frame hurt_box set at all. */
static void test_match_round_decided_after_real_kill(void) {
    WmRng rng;
    wm_match_state m;
    wm_arcade_drone_callbacks_t cb;
    wm_input_state punch;
    uint32_t seed;
    bool found = false;
    int guard;

    for (seed = 0; seed < 4096 && !found; ++seed) {
        WmRng probe;
        wm_rng_init(&probe, seed, NULL, NULL, NULL);
        if (wm_match_draw_wrestler_index(&probe) == WM_ROSTER_BRET) found = true;
    }
    CHECK(found);

    wm_rng_init(&rng, seed, NULL, NULL, NULL);
    wm_match_init(&m);
    wm_match_start_selected(&m, &rng, WM_ROSTER_BRET);
    CHECK(m.actors[1].wrestler_num == WM_ROSTER_BRET);

    /* Force the opponent to a known, low life total and a range that lands
       the human's real light punch (as in
       test_match_human_punch_from_range_selects_real_punch). */
    m.actors[1].x_int = 60;
    m.actors[1].x_fixed = 60 << 16;
    m.actors[1].life = 10;

    memset(&cb, 0, sizeof(cb));
    cb.rndrng0_upto = test_match_rndrng0_cb;
    cb.user = &rng;

    memset(&punch, 0, sizeof(punch));
    punch.light_punch = true;
    wm_match_tick(&m, &cb, &punch);
    memset(&punch, 0, sizeof(punch));

    for (guard = 0; guard < 20 && m.actors[1].player_mode != WM_PMODE_DEAD; ++guard)
        wm_match_tick(&m, &cb, &punch);
    CHECK(m.actors[1].player_mode == WM_PMODE_DEAD);
    CHECK(!m.round_state.decided);

    for (guard = 0; guard < WM_ARCADE_PIN_TIMEOUT_TICKS + 5 && !m.round_state.decided; ++guard)
        wm_match_tick(&m, &cb, &punch);

    CHECK(m.round_state.decided);
    CHECK(m.round_state.decided_winner_side == m.actors[0].player_side);

    /* wm_match_tick awarded the real LIFEBAR.ASM::set_winner round on the
       exact tick round_state.decided flipped -- one round isn't the match
       yet (best-of-3). */
    CHECK(m.score.p1rounds == 1);
    CHECK(m.score.p2rounds == 0);
    CHECK(m.score.match_winner == 0);

    /* The award happens exactly once: many more ticks past the decided
       edge must not double-count it. */
    for (guard = 0; guard < 10; ++guard)
        wm_match_tick(&m, &cb, &punch);
    CHECK(m.score.p1rounds == 1);
}

/* Spot-checks against BRET.ASM:2897 hrt_leg_anims_table's literal contents
   (transcribed as leg_table in src/core/bret_backend.c), not just against
   the C code that mirrors it. */
static void test_bret_leg_anim_table(void) {
    /* Block 0 "(#1 - UP)", entries 1 (UP) and 4 (DOWN_RIGHT). */
    CHECK(wm_bret_leg_anim(0, 0) == &wm_bret_walk1_f2_anim);
    CHECK(wm_bret_leg_anim(0, 3) == &wm_bret_walk1_f4_anim);
    /* Block 2 "(#3 - RIGHT)", entries 1 (UP) and 6 (DOWN_LEFT). */
    CHECK(wm_bret_leg_anim(2, 0) == &wm_bret_walk2_f2_anim);
    CHECK(wm_bret_leg_anim(2, 5) == &wm_bret_walk8_f4_anim);
    /* Block 4 "(#5 - DOWN)", entry 3 (RIGHT). */
    CHECK(wm_bret_leg_anim(4, 2) == &wm_bret_walk5_f4_anim);
    /* Block 6 "(#7 - LEFT)", entries 1 (UP) and 4 (DOWN_RIGHT). */
    CHECK(wm_bret_leg_anim(6, 0) == &wm_bret_walk2_f2_anim);
    CHECK(wm_bret_leg_anim(6, 3) == &wm_bret_walk8_f4_anim);
    /* Block 7 "(#8 - UP-LEFT)", entry 5 (DOWN). */
    CHECK(wm_bret_leg_anim(7, 4) == &wm_bret_walk6_f4_anim);

    CHECK(wm_bret_leg_anim(-1, 0) == NULL);
    CHECK(wm_bret_leg_anim(0, -1) == NULL);
    CHECK(wm_bret_leg_anim(8, 0) == NULL);
    CHECK(wm_bret_leg_anim(0, 8) == NULL);
}

/* Spot-checks against BRET.ASM:2981 hrt_torso_anims_table's literal
   diagonal contents (transcribed as torso_diag_table in
   src/core/bret_backend.c): compass 0/1 (UP/UP_RIGHT) and 6/7 (LEFT/
   UP_LEFT) -> hrt_torso2_anim; compass 2/3 (RIGHT/DOWN_RIGHT) and 4/5
   (DOWN/DOWN_LEFT) -> hrt_torso4_anim (matching hrt_torso8_anim/
   hrt_torso6_anim's own SUBR aliasing of those two). */
static void test_bret_torso_anim_table(void) {
    CHECK(wm_bret_torso_anim(0) == &wm_bret_torso2_anim); /* UP */
    CHECK(wm_bret_torso_anim(1) == &wm_bret_torso2_anim); /* UP_RIGHT */
    CHECK(wm_bret_torso_anim(2) == &wm_bret_torso4_anim); /* RIGHT */
    CHECK(wm_bret_torso_anim(3) == &wm_bret_torso4_anim); /* DOWN_RIGHT */
    CHECK(wm_bret_torso_anim(4) == &wm_bret_torso4_anim); /* DOWN */
    CHECK(wm_bret_torso_anim(5) == &wm_bret_torso4_anim); /* DOWN_LEFT */
    CHECK(wm_bret_torso_anim(6) == &wm_bret_torso2_anim); /* LEFT */
    CHECK(wm_bret_torso_anim(7) == &wm_bret_torso2_anim); /* UP_LEFT */

    CHECK(wm_bret_torso_anim(-1) == NULL);
    CHECK(wm_bret_torso_anim(8) == NULL);
}

/* HRTSEQ1.ASM:103-104/115-116: hrt_torso8_anim and hrt_torso6_anim are
   literal SUBR aliases for hrt_torso2_anim/hrt_torso4_anim, not distinct
   artwork -- confirm the extracted frame data actually matches, not just
   that the symbols exist. */
static void test_bret_torso_alias_frames_match_source(void) {
    CHECK(wm_bret_torso8_anim.frame_count == wm_bret_torso2_anim.frame_count);
    CHECK(wm_bret_torso6_anim.frame_count == wm_bret_torso4_anim.frame_count);
    for (size_t i = 0; i < wm_bret_torso2_anim.frame_count; ++i) {
        CHECK(strcmp(wm_bret_torso8_anim.frames[i].source_frame,
                     wm_bret_torso2_anim.frames[i].source_frame) == 0);
        CHECK(wm_bret_torso8_anim.frames[i].ticks == wm_bret_torso2_anim.frames[i].ticks);
    }
    for (size_t i = 0; i < wm_bret_torso4_anim.frame_count; ++i) {
        CHECK(strcmp(wm_bret_torso6_anim.frames[i].source_frame,
                     wm_bret_torso4_anim.frames[i].source_frame) == 0);
        CHECK(wm_bret_torso6_anim.frames[i].ticks == wm_bret_torso4_anim.frames[i].ticks);
    }
}

static void test_bret_backend_execute_walk_selects_leg_anim(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    a.move_dir = WM_MOVE_RIGHT;
    wm_bret_backend_execute_walk(&a, &bva);
    /* facing_dir substituted from move_dir (see wm/bret_backend.h) ->
       leg_table[RIGHT][RIGHT] == wm_bret_walk2_f2_anim, and the torso
       half's own diagonal reselection (wm_bret_torso_anim(RIGHT)) ->
       wm_bret_torso4_anim. */
    CHECK(a.facing_dir == WM_MOVE_RIGHT);
    CHECK(bva.visual.sequence == &wm_bret_walk2_f2_anim);
    CHECK(bva.torso_visual.sequence == &wm_bret_torso4_anim);

    /* #zip (move_dir cleared to 0 by wm_execute_walk) leaves the leg
       sprite alone -- no leg animation for "not moving" -- and, since
       facing_dir is still its zeroed default (never having moved),
       wm_convert_facing returns -1 so the torso half no-ops too. */
    wm_bret_backend_init(&bva);
    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_ZIP;
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_STAND4, &bva);
    wm_bret_backend_execute_walk(&a, &bva);
    CHECK(bva.visual.sequence == &wm_bret_stand4_anim);
    CHECK(bva.torso_visual.sequence == NULL);

    /* MODE_UNINT skips the torso half only (WRESTLE.ASM:4973-4976) -- the
       leg half is unaffected. */
    wm_bret_backend_init(&bva);
    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_RIGHT;
    a.anim_mode = WM_MODE_UNINT;
    wm_bret_backend_execute_walk(&a, &bva);
    CHECK(bva.visual.sequence == &wm_bret_walk2_f2_anim);
    CHECK(bva.torso_visual.sequence == NULL);
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

static void test_human_input_commit(void) {
    wm_arcade_actor_t a;
    wm_human_input_state hs;
    wm_input_state in;

    memset(&a, 0, sizeof(a));
    wm_human_input_init(&hs);

    memset(&in, 0, sizeof(in));
    in.light_punch = true;
    in.block = true;
    in.stick_x = 100; /* > deadzone */
    wm_human_input_commit(&a, &hs, &in);
    CHECK(a.but_val_cur == (WM_BTN_PUNCH | WM_BTN_BLOCK));
    CHECK(a.but_val_down == (WM_BTN_PUNCH | WM_BTN_BLOCK)); /* fresh press */
    CHECK(a.but_val_up == 0);
    CHECK(a.stick_val_cur == WM_MOVE_RIGHT);

    /* Block held, punch edge gone (wm_input_state's attack fields are
       platform-pre-edge-detected -- see wm/human_input.h): PUNCH lifts,
       BLOCK stays down without a fresh but_val_down for it. */
    memset(&in, 0, sizeof(in));
    in.block = true;
    wm_human_input_commit(&a, &hs, &in);
    CHECK(a.but_val_cur == WM_BTN_BLOCK);
    CHECK(a.but_val_down == 0);
    CHECK(a.but_val_up == WM_BTN_PUNCH);

    /* Small stick deflection stays inside the deadzone. */
    memset(&in, 0, sizeof(in));
    in.stick_x = 5;
    in.stick_y = -5;
    wm_human_input_commit(&a, &hs, &in);
    CHECK(a.stick_val_cur == 0);

    /* NULL input is a defined "everything released" tick, not a crash. */
    wm_human_input_commit(&a, &hs, NULL);
    CHECK(a.but_val_cur == 0);
    CHECK(a.stick_val_cur == 0);
}

static void test_match_start_selected(void) {
    WmRng rng;
    wm_match_state m;
    wm_rng_init(&rng, 0xabcdu, NULL, NULL, NULL);
    wm_match_init(&m);

    wm_match_start_selected(&m, &rng, WM_ROSTER_BRET);
    CHECK(m.active);
    CHECK(m.has_human);
    CHECK(m.human_actor_index == 0);
    CHECK(m.opponent_wrestler <= 8 && m.opponent_wrestler != 7);

    /* WRESTLE.ASM #1plyr: human PLYRNUM=0/PSIDE_PLYR1, drone PLYRNUM=2/
       PSIDE_PLYR2 -- different from #0plyr's 2/3 (see wm/match.h). */
    CHECK(m.actors[0].player_num == 0);
    CHECK(m.actors[0].player_side == 0);
    CHECK(m.actors[0].wrestler_num == WM_ROSTER_BRET);
    CHECK(m.actors[0].life == 163);
    CHECK(m.actors[1].player_num == 2);
    CHECK(m.actors[1].player_side == 1);
    CHECK(m.actors[0].smart_target == &m.actors[1]);
    CHECK(m.actors[1].smart_target == &m.actors[0]);

    /* wm_arcade_bret_ani_init already ran for the human Bret actor. */
    CHECK(m.bret_visual[0].visual.sequence != NULL);
}

/* End-to-end: a human holding the stick right actually walks Bret to the
   right, through the real wm_match_tick -> wm_human_input_commit ->
   wm_arcade_move_bret -> execute_walk -> set_velocities -> position chain,
   not a synthetic call into wm_execute_walk directly. */
static void test_match_human_bret_walks_right(void) {
    WmRng rng;
    wm_match_state m;
    wm_arcade_drone_callbacks_t cb;
    wm_input_state right;

    wm_rng_init(&rng, 0x42u, NULL, NULL, NULL);
    wm_match_init(&m);
    wm_match_start_selected(&m, &rng, WM_ROSTER_BRET);
    int32_t start_x = m.actors[0].x_fixed;

    memset(&cb, 0, sizeof(cb));
    cb.rndrng0_upto = test_match_rndrng0_cb;
    cb.user = &rng;

    memset(&right, 0, sizeof(right));
    right.stick_x = 100;
    for (unsigned i = 0; i < 30; ++i)
        wm_match_tick(&m, &cb, &right);

    CHECK(m.actors[0].stick_val_cur == WM_MOVE_RIGHT);
    CHECK(m.actors[0].move_dir == WM_MOVE_RIGHT);
    CHECK(m.actors[0].x_vel == WM_BRET_WALK_VEL);
    CHECK(m.actors[0].x_fixed > start_x);
    CHECK(!(m.actors[0].obj_control & WM_OBJ_FLIPH));
    /* hrt_leg_anims_table[RIGHT][RIGHT] -- see test_bret_leg_anim_table. */
    CHECK(m.bret_visual[0].visual.sequence == &wm_bret_walk2_f2_anim);

    /* The CPU opponent (a non-Bret wrestler unless the placeholder draw
       happened to also land on Bret) never receives human input and keeps
       stepping its drone core as before. */
    if (m.actors[1].wrestler_num != WM_ROSTER_BRET)
        CHECK(m.actors[1].x_fixed == 0);
}

/*
 * Regression for the do_punch/near() gap this port previously documented as
 * its next real blocker (README, port/translation_manifest.json): before
 * wm_arcade_calc_closest (wm/arcade/wm_arcade_closest.h) was wired into
 * wm_match_tick, closest_xdist/closest_zdist stayed 0 forever, so
 * BRET.ASM's do_punch always treated the opponent as point-blank
 * (near(a,50,45)) and picked the unwired WM_BRET_ANIM_BUTT2/4 headbutt
 * instead of a mapped punch id -- even through a full human-input
 * wm_match_tick call. With real per-tick distances now computed, a human
 * punching from a realistic (non-point-blank) range gets the real, wired
 * punch animation and its real ATTACK_ON window, through ordinary play --
 * no direct bret_backend/wm_arcade_move_bret calls, unlike
 * test_hurt_box_connects_a_real_hit's lower-level check.
 */
static void test_match_human_punch_from_range_selects_real_punch(void) {
    WmRng rng;
    wm_match_state m;
    wm_arcade_drone_callbacks_t cb;
    wm_input_state punch;
    int guard;

    wm_rng_init(&rng, 0x1234u, NULL, NULL, NULL);
    wm_match_init(&m);
    wm_match_start_selected(&m, &rng, WM_ROSTER_BRET);

    /* Opponent stands 200 units away on X -- outside do_punch's
       near(a,50,45) headbutt range, but still a plausible sparring
       distance, not a contrived edge case. */
    m.actors[1].x_int = 200;
    m.actors[1].x_fixed = 200 << 16;

    memset(&cb, 0, sizeof(cb));
    cb.rndrng0_upto = test_match_rndrng0_cb;
    cb.user = &rng;

    memset(&punch, 0, sizeof(punch));
    punch.light_punch = true;
    wm_match_tick(&m, &cb, &punch);

    CHECK(m.actors[0].closest_xdist == 200);
    CHECK(m.bret_visual[0].current_id == WM_BRET_ANIM_PUNCH4);
    CHECK(m.bret_visual[0].visual.sequence == &wm_bret_light_punch4_anim);

    /* Let the already-started punch animation play out to its real,
       hand-traced ATTACK_ON frame through ordinary wm_match_tick calls. */
    memset(&punch, 0, sizeof(punch));
    for (guard = 0; guard < 10 && !(m.actors[0].anim_mode & WM_MODE_CHECKHIT); ++guard)
        wm_match_tick(&m, &cb, &punch);

    CHECK(m.actors[0].anim_mode & WM_MODE_CHECKHIT);
    CHECK(m.actors[0].attack_mode == WM_AMODE_PUNCH);
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
        wm_match_tick(&m, &cb, NULL);
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
    test_bret_backend_i_will_die_resolves_through_move_bret();
    test_bret_attack_window_punch2();
    test_bret_attack_windows_remaining();
    test_bret_hurt_box_for_frame_real_geometry();
    test_bret_backend_tick_sets_real_hurt_box();
    test_hurt_box_connects_a_real_hit();
    test_arcade_adjust_health_normal_damage();
    test_arcade_adjust_health_clamps_to_life_max();
    test_arcade_adjust_health_speed_adjustment_is_identity();
    test_arcade_adjust_health_fudge_saves_a_near_death_hit();
    test_arcade_adjust_health_attract_mode_never_dies();
    test_arcade_adjust_health_death();
    test_arcade_adjust_health_combo_revival_defers_death();
    test_arcade_adjust_health_combo_damage_overrides_delta();
    test_arcade_adjust_health_dam_mult_scales_delta();
    test_arcade_adjust_health_stamps_last_damage_for_reduced_window();
    test_arcade_wrestler_hit_reduced_damage_within_window();
    test_arcade_wrestler_hit_first_hit_bonus_deals_more_damage();
    test_hurt_box_hit_kills_at_zero_life();
    test_arcade_get_live_bits();
    test_arcade_round_tick_decides_after_pin_timeout();
    test_arcade_round_tick_cancels_on_revival();
    test_arcade_round_tick_double_ko_is_a_draw();
    test_arcade_match_score_awards_rounds_and_sets_match_winner();
    test_arcade_match_score_draw_awards_nothing();
    test_bret_ani_init_facing();
    test_bret_leg_anim_table();
    test_bret_torso_anim_table();
    test_bret_torso_alias_frames_match_source();
    test_bret_backend_execute_walk_selects_leg_anim();
    test_convert_facing();
    test_set_velocities_normal();
    test_set_velocities_backward_reduction();
    test_set_velocities_ground_boost();
    test_execute_walk_flip_and_zip();
    test_integrate_position();
    test_human_input_commit();
    test_match_start_selected();
    test_match_human_bret_walks_right();
    test_match_human_punch_from_range_selects_real_punch();
    test_match_round_decided_after_real_kill();
    test_match_bret_idle_animates();
    test_source_attract_sequence();
    test_attract_source_flow();
    test_video_frame_source_clock_and_input_latch();
    puts("all core tests passed");
    return 0;
}
