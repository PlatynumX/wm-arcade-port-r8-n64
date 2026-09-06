#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wm/anim.h"
#include "wm/app.h"
#include "wm/arcade/wm_arcade_confine.h"
#include "wm/arcade/wm_arcade_joystat.h"
#include "wm/arcade/wm_arcade_mode_dead.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm/bmod.h"
#include "wm/source_clock.h"
#include "wm/bret_visuals.h"
#include "wm/anim_program.h"
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

    /* WRESTLE.ASM:2691-2755 (#set0) real #teamX_starts placement,
       "first player on team" row (WRESTLE.ASM:2784/2790) -- RING_X_CENTER
       (RING.EQU:22) = 0x400+50 = 1074. */
    CHECK(m.actors[0].x_int == 1074 - 85);
    CHECK(m.actors[0].x_fixed == (int32_t)((1074 - 85) << 16));
    CHECK(m.actors[0].z_int == 1127 + 93);
    CHECK(m.actors[0].facing_dir == WM_MOVE_UP_RIGHT);
    CHECK(m.actors[0].new_facing_dir == WM_MOVE_UP_RIGHT);
    CHECK(m.actors[1].x_int == 1074 + 85);
    CHECK(m.actors[1].x_fixed == (int32_t)((1074 + 85) << 16));
    CHECK(m.actors[1].z_int == 1103 + 93);
    CHECK(m.actors[1].facing_dir == WM_MOVE_DOWN_LEFT);
    CHECK(m.actors[1].new_facing_dir == WM_MOVE_DOWN_LEFT);

    /* The two starting positions are chosen so each wrestler's real starting
       facing already points at the other's real starting position --
       recomputing NEW_FACING_DIR from these positions must land back on
       the exact same value, so nothing spuriously "turns" on tick 1 (see
       test_match_bret_idle_animates). */
    wm_arcade_update_newfacing(&m.actors[0], &m.actors[1]);
    wm_arcade_update_newfacing(&m.actors[1], &m.actors[0]);
    CHECK(m.actors[0].new_facing_dir == WM_MOVE_UP_RIGHT);
    CHECK(m.actors[1].new_facing_dir == WM_MOVE_DOWN_LEFT);
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
    /* HRTSEQ2.ASM has two different super-punch routines and the port used
       to conflate them. hrt_2/4_super_punch2_anim (:618/:677) is BRET.ASM
       #spunch_slap's own `FACE24 hrt,super_punch2_anim` pair, which is what
       do_super_punch selects in ordinary play; hrt_4_super_punch_anim
       (:223) is #scrt_cut's supercut target. Different frames, different
       attack boxes. */
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_SUPER_PUNCH2_2) ==
          &wm_bret_super_punch2_2_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_SUPER_PUNCH2_4) ==
          &wm_bret_super_punch2_4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_SUPER_PUNCH4) ==
          &wm_bret_power_punch_anim);
    CHECK(wm_bret_super_punch2_2_anim.frames != wm_bret_power_punch_anim.frames);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_KICK2) == &wm_bret_light_kick2_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_KICK4) == &wm_bret_light_kick4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_SUPER_KICK2) == &wm_bret_power_kick_anim);
    /* HRTSEQ2.ASM:1334-1335: hrt_4_super_kick_anim is a literal SUBR alias
       of hrt_2_super_kick_anim, not distinct artwork. */
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_SUPER_KICK4) == &wm_bret_power_kick_anim);
    /* Batch 1 of Bret's previously-unmapped strike set: four ids his own
       do_punch/do_kick already selected on the close-range branch, plus the
       crouching uppercut do_super_punch selects on a down input. */
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_BUTT2) == &wm_bret_butt2_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_BUTT4) == &wm_bret_butt4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_KNEE2) == &wm_bret_knee2_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_KNEE4) == &wm_bret_knee4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_UPPERCUT4) == &wm_bret_uppercut4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_STOMP2) == &wm_bret_stomp2_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_STOMP4) == &wm_bret_stomp4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_GROUND_PUNCH2) == &wm_bret_ground_punch2_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_GROUND_PUNCH4) == &wm_bret_ground_punch4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_PUSH4) == &wm_bret_push4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_JUMP_KICK4) == &wm_bret_jump_kick4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_KNEE_FALL4) == &wm_bret_knee_fall4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_KICK_TB) == &wm_bret_kick_tb_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_HEAD_HELD_STAND3) ==
          &wm_bret_head_held_stand3_anim);
    /* Batch 4: neither routine ends where its own text does --
       hrt_4_knee_to_head_anim ends in ANI_GOTO,#cont and
       hrt_3_fake_hold_anim in ANI_GOTO,#missed, both landing in the shared
       tail that follows. Before wlanim.py followed those, they extracted
       as 1 and 3 frames; the real streams are 8 and 6. */
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_KNEE_TO_HEAD4) ==
          &wm_bret_knee_to_head4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_FAKE_HOLD3) ==
          &wm_bret_fake_hold3_anim);
    CHECK(wm_bret_knee_to_head4_anim.frame_count == 8);
    CHECK(wm_bret_fake_hold3_anim.frame_count == 6);
    /* hrt_climb_down_anim passes tools/wlattack.py --audit, but its own
       third frame (H4HU4B+FR10) has no artwork: BRET.LOD carries only
       H4HU4B01/02/03/04/07, and hrt_jms.img itself has no FR10 either, so
       the frame the source asks for genuinely does not exist in the
       shipped art. Wiring it would mean inventing a shortened animation. */
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_CLIMB_DOWN) == NULL);

    /* Batch 5: all three carry an ANI_SET_RPTCOUNT,3 span, representable
       now that wm_visual_sequence carries the loop rather than needing it
       flattened. */
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_PIN2) == &wm_bret_pin2_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_PIN4) == &wm_bret_pin4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_KNEES_TO_HEAD) ==
          &wm_bret_knees_to_head_anim);
    CHECK(wm_bret_pin2_anim.loop_count == 3);
    CHECK(wm_bret_knees_to_head_anim.loop_count == 3);
    /* hrt_uppercuts_to_head_anim stays unmapped on purpose: its
       ANI_IF_RPTCOUNT branches FORWARD, so it is a first pass plus a
       separate repeated block sharing one RPT_COUNT, which a single loop
       span cannot represent -- wlanim.py refuses it rather than emitting
       an inverted or invented span. */
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_UPPERCUTS_TO_HEAD) == NULL);
    /* Batch 6: unlocked by recognising ANI_CHANGEANIM as the terminator
       ANIM.ASM:1301 makes it. Each of these ends by becoming another
       animation, so its own frame list stops there. */
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_BUTTS2) == &wm_bret_butts2_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_BUTTS4) == &wm_bret_butts4_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_FLYING_KICK) ==
          &wm_bret_flying_kick_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_TBUKL_LEAP) ==
          &wm_bret_tbukl_leap_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_RUNNING_GROUND_PUNCH) ==
          &wm_bret_running_ground_punch_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_COMBO_PUNCH) ==
          &wm_bret_combo_punch_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_COMBO_KICK) ==
          &wm_bret_combo_kick_anim);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_FALL_BACK) ==
          &wm_bret_fall_back_anim);
    /* fall_back reads as 12 frames, not the 55 a walk that ran past its
       ANI_CHANGEANIM produced; flying_kick as 9, not 38. */
    CHECK(wm_bret_fall_back_anim.frame_count == 12);
    CHECK(wm_bret_flying_kick_anim.frame_count == 9);
    CHECK(wm_bret_butts2_anim.loop_count == 3);
    CHECK(wm_bret_anim_sequence(WM_BRET_ANIM_FINISH1) == NULL);
}

/* WRESTLE.ASM:4650 xflip_table + update_joystat's own insert conditions
   (wm/arcade/wm_arcade_joystat.h). */
static void test_joystat_update(void) {
    wm_arcade_actor_t a;
    wm_arcade_joystat_t js;

    /* No stick/button activity at all: no insert. */
    memset(&a, 0, sizeof(a));
    wm_arcade_joystat_init(&js);
    wm_arcade_joystat_update(&js, &a, 12);
    CHECK(js.entries[0].value == 0 && js.entries[0].tickcount == 0);

    /* Facing right: raw stick value used as-is (no flip). */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_RIGHT;
    a.stick_val_cur = WM_MOVE_LEFT;
    a.stick_val_down = WM_MOVE_LEFT; /* fresh edge */
    wm_arcade_joystat_init(&js);
    wm_arcade_joystat_update(&js, &a, 50);
    CHECK((js.entries[0].value & 0x0Fu) == WM_MOVE_LEFT);
    CHECK(js.entries[0].tickcount == 50);

    /* Facing left: xflip_table swaps LEFT<->RIGHT, so raw LEFT records as
       the facing-relative WM_J_TOWARD. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_LEFT;
    a.stick_val_cur = WM_MOVE_LEFT;
    a.stick_val_down = WM_MOVE_LEFT;
    wm_arcade_joystat_init(&js);
    wm_arcade_joystat_update(&js, &a, 50);
    CHECK((js.entries[0].value & 0x0Fu) == WM_J_TOWARD);

    /* A button press inserts its own entry, tagged with whatever direction
       is currently held, even without a fresh vertical edge. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_RIGHT;
    a.stick_val_cur = WM_MOVE_DOWN;
    a.but_val_down = WM_BTN_PUNCH;
    wm_arcade_joystat_init(&js);
    wm_arcade_joystat_update(&js, &a, 77);
    CHECK(js.entries[0].value == (uint16_t)(WM_B_PUNCH | WM_J_DOWN));
    CHECK(js.entries[0].tickcount == 77);
}

/* WRESTLE.ASM:4851 check_secret_moves' own matching algorithm (wm/arcade/
   wm_arcade_joystat.h): step 0 (the trigger) must be the exact queue
   head, later steps get a shared 8-entry skip budget, and a full match
   must still fit inside max_ticks. */
static void test_joystat_matches(void) {
    wm_arcade_joystat_t js;
    static const wm_arcade_bret_sequence_step_t supercut[] = {
        { WM_B_PUNCH, WM_J_ALL }, { WM_J_DOWN, WM_J_REAL_LR }, { WM_J_DOWN, WM_J_REAL_LR }
    };
    static const wm_arcade_bret_sequence_step_t jump_kick[] = {
        { WM_B_SKICK, WM_J_ALL },
        { WM_J_AWAY, (uint16_t)(WM_J_REAL_LR | WM_J_UP | WM_J_DOWN) },
        { WM_J_AWAY, (uint16_t)(WM_J_REAL_LR | WM_J_UP | WM_J_DOWN) }
    };

    wm_arcade_joystat_init(&js);
    js.entries[0].value = WM_B_PUNCH; js.entries[0].tickcount = 100;
    js.entries[1].value = WM_J_DOWN;  js.entries[1].tickcount = 99;
    js.entries[2].value = WM_J_DOWN;  js.entries[2].tickcount = 98;
    CHECK(wm_arcade_joystat_matches(&js, 100, supercut, 3, 16));

    /* Too slow: elapsed (100-80=20) exceeds max_ticks (16). */
    js.entries[2].tickcount = 80;
    CHECK(!wm_arcade_joystat_matches(&js, 100, supercut, 3, 16));

    /* Wrong trigger button. */
    js.entries[2].tickcount = 98;
    js.entries[0].value = WM_B_SPUNCH;
    CHECK(!wm_arcade_joystat_matches(&js, 100, supercut, 3, 16));

    /* Wrong direction at an intermediate step. */
    js.entries[0].value = WM_B_PUNCH;
    js.entries[1].value = WM_J_UP;
    CHECK(!wm_arcade_joystat_matches(&js, 100, supercut, 3, 16));

    /* A masked-to-zero entry between real steps is skipped (up to the
       shared 8-entry budget), not treated as a mismatch. */
    wm_arcade_joystat_init(&js);
    js.entries[0].value = WM_B_SKICK;  js.entries[0].tickcount = 100;
    js.entries[1].value = WM_J_AWAY;   js.entries[1].tickcount = 99;
    js.entries[2].value = WM_J_LEFT;   js.entries[2].tickcount = 98; /* masks to 0 */
    js.entries[3].value = WM_J_AWAY;   js.entries[3].tickcount = 97;
    CHECK(wm_arcade_joystat_matches(&js, 100, jump_kick, 3, 32));

    /* Skip budget exhausted (all entries masked-to-zero, never finds the
       second step): rejected. */
    {
        int i;
        static const wm_arcade_bret_sequence_step_t two_step[] = {
            { WM_B_SKICK, WM_J_ALL },
            { WM_J_AWAY, (uint16_t)(WM_J_REAL_LR | WM_J_UP | WM_J_DOWN) }
        };
        wm_arcade_joystat_init(&js);
        js.entries[0].value = WM_B_SKICK; js.entries[0].tickcount = 100;
        for (i = 1; i < WM_JOYSTAT_DEPTH; ++i) {
            js.entries[i].value = WM_J_LEFT;
            js.entries[i].tickcount = (uint16_t)(100 - i);
        }
        CHECK(!wm_arcade_joystat_matches(&js, 100, two_step, 2, 32));
    }
}

/* WRESTLE.ASM:4059 update_joy_dtime's #update_but half: consecutive ticks
   held, reset to 0 the instant released. */
static void test_arcade_update_joy_dtime(void) {
    wm_arcade_actor_t a;
    int i;

    memset(&a, 0, sizeof(a));
    a.but_val_cur = WM_BTN_PUNCH;
    for (i = 1; i <= 5; ++i) {
        wm_arcade_update_joy_dtime(&a);
        CHECK(a.punch_dtime == (uint16_t)i);
        CHECK(a.powerp_dtime == 0);
        CHECK(a.powerk_dtime == 0);
    }

    a.but_val_cur = 0;
    wm_arcade_update_joy_dtime(&a);
    CHECK(a.punch_dtime == 0);

    a.but_val_cur = (uint16_t)(WM_BTN_SPUNCH | WM_BTN_SKICK);
    wm_arcade_update_joy_dtime(&a);
    wm_arcade_update_joy_dtime(&a);
    CHECK(a.powerp_dtime == 2);
    CHECK(a.powerk_dtime == 2);
    CHECK(a.punch_dtime == 0);
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

/* HRTSEQ2.ASM: 5 of the 6 wired attacks lead with a real, instant
   ANI_SETFACING (attack_sets_facing_on_start's own comment in
   src/core/bret_backend.c has the exact HRTSEQ2.ASM line numbers);
   hrt_4_kick_anim is the one real exception, verified by reading its
   header directly rather than assumed from the other 5. */
static void test_bret_backend_change_anim_sets_facing_on_attack_start(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    a.facing_dir = WM_MOVE_UP_RIGHT;
    a.new_facing_dir = WM_MOVE_DOWN_LEFT;
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_PUNCH2, &bva);
    CHECK(a.facing_dir == WM_MOVE_DOWN_LEFT); /* the instant ANI_SETFACING */
    CHECK(a.anim_mode & WM_MODE_UNINT);

    /* SUPER_KICK4 shares hrt_2_super_kick_anim's body (HRTSEQ2.ASM:
       1334-1335), including its ANI_SETFACING. */
    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    a.facing_dir = WM_MOVE_UP_RIGHT;
    a.new_facing_dir = WM_MOVE_DOWN_LEFT;
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_SUPER_KICK4, &bva);
    CHECK(a.facing_dir == WM_MOVE_DOWN_LEFT);
    CHECK(a.anim_mode & WM_MODE_UNINT);

    /* hrt_4_kick_anim: real exception, no ANI_SETFACING. */
    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    a.facing_dir = WM_MOVE_UP_RIGHT;
    a.new_facing_dir = WM_MOVE_DOWN_LEFT;
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_KICK4, &bva);
    CHECK(a.facing_dir == WM_MOVE_UP_RIGHT);  /* unchanged */
    CHECK(a.anim_mode & WM_MODE_UNINT);       /* KICK4 still sets MODE_UNINT */

    /* Not a one-shot attack id at all (an idle stance): neither side
       effect applies. */
    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    a.facing_dir = WM_MOVE_UP_RIGHT;
    a.new_facing_dir = WM_MOVE_DOWN_LEFT;
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_STAND2, &bva);
    CHECK(a.facing_dir == WM_MOVE_UP_RIGHT);
    CHECK(!(a.anim_mode & WM_MODE_UNINT));

    /* Reselecting the SAME already-playing attack (not a real restart)
       doesn't re-copy -- the source command fires once, on selection, not
       continuously for as long as the attack plays. */
    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    a.facing_dir = WM_MOVE_UP_RIGHT;
    a.new_facing_dir = WM_MOVE_DOWN_LEFT;
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_PUNCH2, &bva);
    CHECK(a.facing_dir == WM_MOVE_DOWN_LEFT);
    a.facing_dir = WM_MOVE_UP_LEFT;         /* something else changed it after */
    a.new_facing_dir = WM_MOVE_DOWN_RIGHT;  /* and new_facing_dir moved on too */
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_PUNCH2, &bva); /* same id, not a restart */
    CHECK(a.facing_dir == WM_MOVE_UP_LEFT); /* untouched */
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

/* WRESTLE.ASM's real check_secret_moves/update_joystat (wm/arcade/
   wm_arcade_joystat.h): a human genuinely typing the supercut input
   (down, down, punch, each a distinct press) now fires the real secret
   move (wm_arcade_bret_fire_secret's own WM_BRET_SECRET_SUPERCUT case),
   not just a direct call with a hand-picked id -- and it survives the
   rest of the very same wm_arcade_move_bret call, because
   WM_BRET_SECRET_SUPERCUT now dispatches to WM_BRET_ANIM_SUPER_PUNCH2_4
   (BRET.ASM's real hrt_4_super_punch_anim, the exact animation #scrt_cut
   itself selects) instead of a separate, permanently-unmapped id: real
   frame data means wm_bret_backend_change_anim sets WM_MODE_UNINT for
   real, so mode_normal's own top-of-function guard (`if (ANIMODE&
   MODE_UNINT) return`) stops the switch statement's own continuation,
   still within this same call, from immediately reselecting an idle/
   attack animation over it. */
static void test_bret_backend_secret_move_fires_through_real_input(void) {
    wm_arcade_actor_t actor, opp;
    wm_bret_backend_actor bva;
    wm_human_input_state hs;
    wm_input_state in;
    wm_arcade_bret_env_t env;
    wm_arcade_bret_callbacks_t cb;

    memset(&actor, 0, sizeof(actor));
    actor.facing_dir = WM_MOVE_UP_RIGHT; /* no LEFT bit -- no xflip needed */
    actor.player_mode = WM_PMODE_NORMAL;

    memset(&opp, 0, sizeof(opp));
    opp.player_mode = WM_PMODE_NORMAL;

    wm_bret_backend_init(&bva);
    bva.opponent = &opp;
    wm_human_input_init(&hs);
    cb = wm_bret_backend_callbacks(&bva);
    memset(&env, 0, sizeof(env));

    /* Tick 0: press DOWN. */
    memset(&in, 0, sizeof(in));
    in.stick_y = -50;
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 0;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
    CHECK(bva.current_id != WM_BRET_ANIM_SUPER_PUNCH2_4);

    /* Tick 1: release to neutral, so the next DOWN is a fresh edge too. */
    memset(&in, 0, sizeof(in));
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 1;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);

    /* Tick 2: press DOWN again. */
    memset(&in, 0, sizeof(in));
    in.stick_y = -50;
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 2;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);

    /* Tick 3: release, then press PUNCH -- completes the real supercut
       sequence (BRET.ASM's #supercut: B_PUNCH, J_DOWN, J_DOWN) well
       within its real 16-tick window. A single wm_arcade_move_bret call
       both fires the secret move AND runs its own player_mode switch
       right after -- this checks the animation survives that same call. */
    memset(&in, 0, sizeof(in));
    in.light_punch = true;
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 3;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);

    /* #scrt_cut's own target is hrt_4_super_punch_anim, i.e.
       WM_BRET_ANIM_SUPER_PUNCH4 -- not the SUPER_PUNCH2_* pair
       do_super_punch selects, which is hrt_2/4_super_punch2_anim. */
    CHECK(bva.current_id == WM_BRET_ANIM_SUPER_PUNCH4);
    CHECK(bva.visual.sequence == &wm_bret_power_punch_anim);
    CHECK(actor.anim_mode & WM_MODE_UNINT);

    /* A further wm_arcade_move_bret call the very next tick -- with the
       human now pressing nothing at all -- still doesn't get reselected
       out from under it while the real animation is still playing. */
    memset(&in, 0, sizeof(in));
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 4;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
    CHECK(bva.current_id == WM_BRET_ANIM_SUPER_PUNCH4);
}

/* WM_BRET_ANIM_JUMP_KICK4 (hrt_4_jump_kick_anim) has no real extracted
   frame data, unlike SUPER_PUNCH2_4 above -- secret_move_sets_mode_uninit
   still lets it survive the same-tick reselection (its own real header
   does set MODE_UNINT), but with no wm_visual_state to time a real end
   from, wm_bret_backend_tick clears that protection back off the very
   next tick instead of leaving it set indefinitely -- see both functions'
   own comments for why. This checks both halves: survives its own tick,
   free again the next. */
static void test_bret_backend_secret_move_no_frame_data_survives_one_tick(void) {
    wm_arcade_actor_t actor, opp;
    wm_bret_backend_actor bva;
    wm_human_input_state hs;
    wm_input_state in;
    wm_arcade_bret_env_t env;
    wm_arcade_bret_callbacks_t cb;

    memset(&actor, 0, sizeof(actor));
    actor.facing_dir = WM_MOVE_UP_RIGHT; /* no LEFT bit -- no xflip needed */
    actor.player_mode = WM_PMODE_NORMAL;

    memset(&opp, 0, sizeof(opp));
    opp.player_mode = WM_PMODE_NORMAL;

    wm_bret_backend_init(&bva);
    bva.opponent = &opp;
    wm_human_input_init(&hs);
    cb = wm_bret_backend_callbacks(&bva);
    memset(&env, 0, sizeof(env));

    /* Tick 0: push AWAY (LEFT, since facing right). */
    memset(&in, 0, sizeof(in));
    in.stick_x = -50;
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 0;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
    wm_bret_backend_tick(&bva, &actor, 0);

    /* Tick 1: release to neutral, so the next AWAY is a fresh edge too. */
    memset(&in, 0, sizeof(in));
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 1;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
    wm_bret_backend_tick(&bva, &actor, 1);

    /* Tick 2: push AWAY again. */
    memset(&in, 0, sizeof(in));
    in.stick_x = -50;
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 2;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
    wm_bret_backend_tick(&bva, &actor, 2);

    /* Tick 3: release, then power-kick -- completes the real jump-kick
       sequence (BRET.ASM's #jump_kick: B_SKICK, J_AWAY, J_AWAY). It
       survives this same wm_arcade_move_bret call's own player_mode
       switch continuation. */
    memset(&in, 0, sizeof(in));
    in.power_kick = true;
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 3;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
    CHECK(bva.current_id == WM_BRET_ANIM_JUMP_KICK4);
    CHECK(actor.anim_mode & WM_MODE_UNINT);

    /* JUMP_KICK4 now has real extracted hrt_4_jump_kick_anim frame data
       and a real attack window, so its MODE_UNINT is timed off the actual
       animation the way every wired attack's is -- it is no longer the
       one-tick stopgap this test used to assert (that stopgap's own stated
       premise, "no real frame duration to time a longer one from", stopped
       being true when the animation was extracted). Ticking once leaves it
       set, mid-swing. */
    wm_bret_backend_tick(&bva, &actor, 3);
    CHECK(actor.anim_mode & WM_MODE_UNINT);
    CHECK(bva.visual.sequence == &wm_bret_jump_kick4_anim);

    /* Tick 4: still uninterruptible, so an ordinary punch genuinely does
       not displace the jump kick -- which is the real behavior the source
       has and the stopgap could not reproduce. */
    memset(&in, 0, sizeof(in));
    in.light_punch = true;
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 4;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
    CHECK(bva.current_id == WM_BRET_ANIM_JUMP_KICK4);

    /* Once the animation genuinely ends, its own trailing
       ANI_SETMODE,MODE_NORMAL clears the protection and Bret is free
       again -- no soft-lock. */
    {
        int guard;
        for (guard = 0; guard < 80 && (actor.anim_mode & WM_MODE_UNINT); ++guard)
            wm_bret_backend_tick(&bva, &actor, (uint16_t)(5 + guard));
        CHECK(!(actor.anim_mode & WM_MODE_UNINT));
    }
    /* Release first: mode_normal dispatches on but_val_down (the press
       edge), so a button already held since tick 4 is not a new press. */
    memset(&in, 0, sizeof(in));
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 89;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);

    memset(&in, 0, sizeof(in));
    in.light_punch = true;
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 90;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
    CHECK(bva.current_id != WM_BRET_ANIM_JUMP_KICK4);
    CHECK(bva.current_id != WM_BRET_ANIM_NONE);
}

/* BRET.ASM:543 hrt_charge_flying_kick: hold SKICK 100+ ticks then release
   it -- a real, independent "charge and release" watcher, translated via
   wm_arcade_update_joy_dtime's real per-button hold-duration counter
   (wm/arcade/wm_arcade_combat.h) and wm_arcade_bret_release_charge_
   flying_kick's own already-real preconditions/dispatch. The fired move
   itself is deferred through the real SPECIAL_MOVE_ADDR handoff
   (WRESTLE.ASM:3843-3849 move_wrestler) -- picked up by the very next
   wm_arcade_move_bret call, not the one that fires it. */
static void test_bret_backend_charge_flying_kick_fires_on_release(void) {
    wm_arcade_actor_t actor, opp;
    wm_bret_backend_actor bva;
    wm_human_input_state hs;
    wm_input_state in;
    wm_arcade_bret_env_t env;
    wm_arcade_bret_callbacks_t cb;
    int i;

    memset(&actor, 0, sizeof(actor));
    actor.facing_dir = WM_MOVE_UP_RIGHT;
    actor.player_mode = WM_PMODE_NORMAL;

    memset(&opp, 0, sizeof(opp));
    opp.player_mode = WM_PMODE_NORMAL;

    wm_bret_backend_init(&bva);
    bva.opponent = &opp;
    wm_human_input_init(&hs);
    cb = wm_bret_backend_callbacks(&bva);
    memset(&env, 0, sizeof(env));

    memset(&in, 0, sizeof(in));
    in.power_kick = true;
    for (i = 0; i < 100; ++i) {
        wm_human_input_commit(&actor, &hs, &in);
        bva.pcnt = (uint32_t)i;
        (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
        /* Same reason test_bret_backend_charge_face_rake_fires_on_release
           ticks: the press edge on tick 0 really does select an attack
           (do_super_kick's own knee-fall branch at these zero distances),
           whose header sets MODE_UNINT. Advancing the animation lets it
           finish, which is what makes the charge release below reachable
           at all -- and is what the live match loop does every tick. */
        wm_bret_backend_tick(&bva, &actor, (uint16_t)i);
    }
    CHECK(actor.powerk_dtime == 100);
    CHECK(actor.special_move_addr == 0);
    CHECK(!(actor.anim_mode & WM_MODE_UNINT));

    /* Release: fires for real, but only sets INAIR + queues the anim. */
    memset(&in, 0, sizeof(in));
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 100;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
    CHECK(actor.player_mode == WM_PMODE_INAIR);
    CHECK(actor.special_move_addr == (uintptr_t)WM_BRET_ANIM_FLYING_KICK);
    CHECK(bva.current_id != WM_BRET_ANIM_FLYING_KICK);

    /* The next call picks up SPECIAL_MOVE_ADDR for real. */
    bva.pcnt = 101;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
    CHECK(actor.special_move_addr == 0);
    CHECK(bva.current_id == WM_BRET_ANIM_FLYING_KICK);
}

/* BRET.ASM:614 hrt_charge_face_rake: same shape as the flying kick above,
   holding ordinary PUNCH instead of SKICK, and never leaving player_mode
   NORMAL (no setmode call in the source's own #scrt_facerake-adjacent
   release path). */
static void test_bret_backend_charge_face_rake_fires_on_release(void) {
    wm_arcade_actor_t actor;
    wm_bret_backend_actor bva;
    wm_human_input_state hs;
    wm_input_state in;
    wm_arcade_bret_env_t env;
    wm_arcade_bret_callbacks_t cb;
    int i;

    memset(&actor, 0, sizeof(actor));
    actor.facing_dir = WM_MOVE_UP_RIGHT;
    actor.player_mode = WM_PMODE_NORMAL;

    wm_bret_backend_init(&bva);
    wm_human_input_init(&hs);
    cb = wm_bret_backend_callbacks(&bva);
    memset(&env, 0, sizeof(env));

    memset(&in, 0, sizeof(in));
    in.light_punch = true;
    for (i = 0; i < 100; ++i) {
        wm_human_input_commit(&actor, &hs, &in);
        bva.pcnt = (uint32_t)i;
        (void)wm_arcade_move_bret(&actor, NULL, &env, &cb);
        /* The press edge on tick 0 selects a real close-range headbutt
           (do_punch's own near(50,45) branch), whose HRTSEQ2.ASM header
           leads with ANI_SETMODE,MODE_UNINT|MODE_NOAUTOFLIP. Advancing the
           animation here -- the same wm_bret_backend_tick the live match
           loop runs every tick -- lets that 11-frame animation reach its
           own closing ANI_SETMODE,MODE_NORMAL and drop MODE_UNINT, which
           is what makes the charge release below reachable at all. Without
           it the headbutt would stay mid-swing for all 100 ticks and
           wm_arcade_bret_release_charge_face_rake's real MODE_UNINT
           precondition would (correctly) refuse to fire. */
        wm_bret_backend_tick(&bva, &actor, (uint16_t)i);
    }
    CHECK(actor.punch_dtime == 100);
    CHECK((actor.anim_mode & WM_MODE_UNINT) == 0);

    memset(&in, 0, sizeof(in));
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 100;
    (void)wm_arcade_move_bret(&actor, NULL, &env, &cb);
    CHECK(actor.special_move_addr == (uintptr_t)WM_BRET_ANIM_RAKE_FACE);

    bva.pcnt = 101;
    (void)wm_arcade_move_bret(&actor, NULL, &env, &cb);
    CHECK(bva.current_id == WM_BRET_ANIM_RAKE_FACE);
    CHECK(actor.special_move_addr == 0);
}

/* BRET.ASM:253 #charge_ddt (bret_secret_moves' own real first entry):
   holding SPUNCH 100+ ticks then releasing it takes priority over the
   joystick-history table scan, exactly like the source's own
   "call a0 / jrc #done" -- fires wm_arcade_bret_fire_secret's
   WM_BRET_SECRET_CHARGE_DDT case for real. Unlike the charge flying kick/
   face rake above, #scrt_ddt calls change_anim1a directly (BRET.ASM:277
   "calla change_anim1a") rather than going through SPECIAL_MOVE_ADDR --
   #charge_ddt runs synchronously inside the wrestler's own process (via
   check_secret_moves), not as a separate CREATEd process like
   hrt_charge_flying_kick/hrt_charge_face_rake, so it never needed the
   cross-process handoff those two do. */
static void test_bret_backend_charge_ddt_fires_on_release(void) {
    wm_arcade_actor_t actor, opp;
    wm_bret_backend_actor bva;
    wm_human_input_state hs;
    wm_input_state in;
    wm_arcade_bret_env_t env;
    wm_arcade_bret_callbacks_t cb;
    int i;

    memset(&actor, 0, sizeof(actor));
    actor.facing_dir = WM_MOVE_UP_RIGHT;
    actor.player_mode = WM_PMODE_NORMAL;
    actor.new_facing_dir = WM_MOVE_DOWN_LEFT; /* != STICK_VAL_CUR at release -> hh_2_ddt_anim path */

    memset(&opp, 0, sizeof(opp));
    opp.player_mode = WM_PMODE_NORMAL;

    wm_bret_backend_init(&bva);
    bva.opponent = &opp;
    wm_human_input_init(&hs);
    cb = wm_bret_backend_callbacks(&bva);
    memset(&env, 0, sizeof(env));

    memset(&in, 0, sizeof(in));
    in.power_punch = true;
    for (i = 0; i < 100; ++i) {
        wm_human_input_commit(&actor, &hs, &in);
        bva.pcnt = (uint32_t)i;
        (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
        /* The press edge on tick 0 really selects an attack
           (do_super_punch's own close-range butts branch at these zero
           distances), whose header sets MODE_UNINT. Advancing the
           animation -- what the live match loop does every tick -- lets it
           finish, which is what makes the charge release below reachable;
           wm_arcade_bret_try_charge_ddt's own MODE_UNINT precondition
           would otherwise (correctly) refuse. */
        wm_bret_backend_tick(&bva, &actor, (uint16_t)i);
    }
    CHECK(actor.powerp_dtime == 100);
    CHECK(!(actor.anim_mode & WM_MODE_UNINT));

    memset(&in, 0, sizeof(in));
    wm_human_input_commit(&actor, &hs, &in);
    bva.pcnt = 100;
    (void)wm_arcade_move_bret(&actor, &opp, &env, &cb);
    CHECK(actor.special_move_addr == 0);
    CHECK(bva.current_id == WM_BRET_ANIM_HH_DDT2);
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
        { WM_BRET_ANIM_SUPER_PUNCH4, &wm_bret_power_punch_anim, 5,
          WM_AMODE_UPRCUT, -6, 40, 0, 64, 90, 0, false },
        /* the ordinary super punch's own, genuinely different, window */
        { WM_BRET_ANIM_SUPER_PUNCH2_2, &wm_bret_super_punch2_2_anim, 3,
          WM_AMODE_URN, 19, 75, 0, 35, 24, 0, false },
        { WM_BRET_ANIM_SUPER_PUNCH2_4, &wm_bret_super_punch2_4_anim, 3,
          WM_AMODE_URN, 19, 75, 0, 35, 24, 0, false },
        { WM_BRET_ANIM_KICK2, &wm_bret_light_kick2_anim, 5,
          WM_AMODE_KICK, 23, 73, 0, 50, 17, 0, false },
        { WM_BRET_ANIM_KICK4, &wm_bret_light_kick4_anim, 5,
          WM_AMODE_KICK, 23, 73, 0, 50, 17, 0, false },
        { WM_BRET_ANIM_SUPER_KICK2, &wm_bret_power_kick_anim, 4,
          WM_AMODE_SUPER_KICK, 5, 54, 0, 70, 34, 0, false },
        /* HRTSEQ2.ASM:1334-1335: identical shared body, so the identical
           hand-traced window applies. */
        { WM_BRET_ANIM_SUPER_KICK4, &wm_bret_power_kick_anim, 4,
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

/* Batch 1's own attack windows, traced out of HRTSEQ2.ASM with
   tools/wlattack.py (which reproduces all six hand-traced windows above
   exactly, so the same reading applies here):

     hrt_2/4_butt_anim      ANI_ATTACK_ON,AMODE_HDBUTT,19,75,35,24 @ [5]
     hrt_2/4_knee_anim      ANI_ATTACK_ON,AMODE_KNEE,11,44,51,49   @ [4]
     hrt_4_uppercut_anim    ANI_ATTACK_ON,AMODE_UPRCUT,-6,22,64,100 @ [5]

   All five also lead with ANI_SETMODE,MODE_UNINT|MODE_NOAUTOFLIP and
   ANI_SETFACING before their own frame [0], which is what the added
   attack_sets_facing_on_start entries and the MODE_UNINT check below
   cover. None of these is an ANI_ATTACK_ON_Z, so the z offset/depth take
   wm_arcade_ani_attack_on's real defaults. */
static void test_bret_attack_windows_batch1(void) {
    static const struct {
        wm_arcade_bret_anim_id_t id;
        const wm_visual_sequence *seq;
        size_t frame;
        uint16_t attack_mode;
        int16_t xoff, yoff, width, height;
    } cases[] = {
        { WM_BRET_ANIM_BUTT2, &wm_bret_butt2_anim, 5, WM_AMODE_HDBUTT, 19, 75, 35, 24 },
        { WM_BRET_ANIM_BUTT4, &wm_bret_butt4_anim, 5, WM_AMODE_HDBUTT, 19, 75, 35, 24 },
        { WM_BRET_ANIM_KNEE2, &wm_bret_knee2_anim, 4, WM_AMODE_KNEE, 11, 44, 51, 49 },
        { WM_BRET_ANIM_KNEE4, &wm_bret_knee4_anim, 4, WM_AMODE_KNEE, 11, 44, 51, 49 },
        { WM_BRET_ANIM_UPPERCUT4, &wm_bret_uppercut4_anim, 5, WM_AMODE_UPRCUT, -6, 22, 64, 100 },
    };
    size_t i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        wm_arcade_actor_t a;
        wm_bret_backend_actor bva;
        int guard;

        memset(&a, 0, sizeof(a));
        a.new_facing_dir = WM_MOVE_UP_RIGHT;
        wm_bret_backend_init(&bva);
        wm_bret_backend_change_anim(&a, cases[i].id, &bva);
        CHECK(bva.visual.sequence == cases[i].seq);
        /* the header's own ANI_SETMODE + ANI_SETFACING, both instant */
        CHECK(a.anim_mode & WM_MODE_UNINT);
        CHECK(a.anim_mode & WM_MODE_NOAUTOFLIP);
        CHECK(a.facing_dir == WM_MOVE_UP_RIGHT);
        CHECK(!(a.anim_mode & WM_MODE_CHECKHIT));

        for (guard = 0; guard < 40 && bva.visual.frame_index != cases[i].frame; ++guard)
            wm_bret_backend_tick(&bva, &a, 0);
        CHECK(bva.visual.frame_index == cases[i].frame);
        CHECK(a.anim_mode & WM_MODE_CHECKHIT);
        CHECK(a.attack_mode == cases[i].attack_mode);
        CHECK(a.attack_xoff == cases[i].xoff);
        CHECK(a.attack_yoff == cases[i].yoff);
        CHECK(a.attack_width == cases[i].width);
        CHECK(a.attack_height == cases[i].height);
        CHECK(a.attack_zoff == -40);
        CHECK(a.attack_depth == 80);

        /* ANI_ATTACK_OFF one frame later, then the closing
           ANI_SETMODE,MODE_NORMAL once the animation runs out. */
        for (guard = 0; guard < 40 && (a.anim_mode & WM_MODE_UNINT); ++guard)
            wm_bret_backend_tick(&bva, &a, 7);
        CHECK(!(a.anim_mode & WM_MODE_UNINT));
        CHECK(!(a.anim_mode & WM_MODE_NOAUTOFLIP));
        CHECK(!(a.anim_mode & WM_MODE_CHECKHIT));
        CHECK(a.attack_time == 7);
    }
}

/* Batch 2: the first wired animations carrying more than one real
   ANI_ATTACK_ON pulse each, so this checks the whole ON/OFF/ON shape in
   order rather than one window in isolation. Traced with
   tools/wlattack.py out of HRTSEQ2.ASM:

     hrt_2_stomp_anim         [4] AMODE_HITCHECK,7,-10,-40,28,31,50
                              [7] AMODE_STOMP2,7,-10,-40,28,31,50
     hrt_4_stomp_anim         [4] AMODE_HITCHECK,7,-12,-10,29,35,50
                              [7] AMODE_STOMP2,7,-12,-10,29,35,50
     hrt_2_ground_punch_anim  [2] AMODE_HITCHECK,5-10,-8,-40,32,32,50
                              [5] AMODE_LBOWDROP2,5,-8,-40,32,32,50
                              [8] AMODE_LBOWDROP2,5,-8,-40,32,32,50
     hrt_4_ground_punch_anim  [2] AMODE_HITCHECK,5,-6,-10,36,30,50
                              [5] AMODE_LBOWDROP2,5,-6,-10,36,30,50
                              [8] AMODE_LBOWDROP2,5,-6,-10,36,30,50

   All four are ANI_ATTACK_ON_Z throughout, and all four lead with
   ANI_SETMODE,MODE_UNINT|MODE_NOAUTOFLIP|MODE_OVERLAP (the only wired
   attacks that carry the third bit) plus ANI_ZEROVELS, and no
   ANI_SETFACING. */
static void test_bret_attack_windows_multi_pulse(void) {
    typedef struct { size_t frame; uint16_t mode; int16_t x, y, z, w, h, d; } pulse_t;
    static const struct {
        wm_arcade_bret_anim_id_t id;
        const wm_visual_sequence *seq;
        bool sets_plyrmode_normal;
        size_t count;
        pulse_t pulses[3];
    } cases[] = {
        { WM_BRET_ANIM_STOMP2, &wm_bret_stomp2_anim, true, 2, {
            { 4, WM_AMODE_HITCHECK, 7, -10, -40, 28, 31, 50 },
            { 7, WM_AMODE_STOMP2,   7, -10, -40, 28, 31, 50 },
            { 0, 0, 0, 0, 0, 0, 0, 0 } } },
        { WM_BRET_ANIM_STOMP4, &wm_bret_stomp4_anim, true, 2, {
            { 4, WM_AMODE_HITCHECK, 7, -12, -10, 29, 35, 50 },
            { 7, WM_AMODE_STOMP2,   7, -12, -10, 29, 35, 50 },
            { 0, 0, 0, 0, 0, 0, 0, 0 } } },
        { WM_BRET_ANIM_GROUND_PUNCH2, &wm_bret_ground_punch2_anim, false, 3, {
            { 2, WM_AMODE_HITCHECK,   5 - 10, -8, -40, 32, 32, 50 },
            { 5, WM_AMODE_LBOWDROP2,  5,      -8, -40, 32, 32, 50 },
            { 8, WM_AMODE_LBOWDROP2,  5,      -8, -40, 32, 32, 50 } } },
        { WM_BRET_ANIM_GROUND_PUNCH4, &wm_bret_ground_punch4_anim, false, 3, {
            { 2, WM_AMODE_HITCHECK,   5, -6, -10, 36, 30, 50 },
            { 5, WM_AMODE_LBOWDROP2,  5, -6, -10, 36, 30, 50 },
            { 8, WM_AMODE_LBOWDROP2,  5, -6, -10, 36, 30, 50 } } },
    };
    size_t i, k;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        wm_arcade_actor_t a;
        wm_bret_backend_actor bva;
        int guard;
        int seen_off_between = 0;

        memset(&a, 0, sizeof(a));
        a.player_mode = WM_PMODE_RUNNING;
        a.new_facing_dir = WM_MOVE_UP_RIGHT;
        a.facing_dir = WM_MOVE_DOWN_LEFT;
        a.x_vel = 0x1234;
        a.z_vel = -0x1234;
        wm_bret_backend_init(&bva);
        wm_bret_backend_change_anim(&a, cases[i].id, &bva);
        CHECK(bva.visual.sequence == cases[i].seq);

        /* the header's own instant commands */
        CHECK(a.anim_mode & WM_MODE_UNINT);
        CHECK(a.anim_mode & WM_MODE_NOAUTOFLIP);
        CHECK(a.anim_mode & WM_MODE_OVERLAP);
        CHECK(a.x_vel == 0);
        CHECK(a.z_vel == 0);
        /* no ANI_SETFACING in any of these four headers */
        CHECK(a.facing_dir == WM_MOVE_DOWN_LEFT);
        if (cases[i].sets_plyrmode_normal)
            CHECK(a.player_mode == WM_PMODE_NORMAL);
        else
            CHECK(a.player_mode == WM_PMODE_RUNNING);

        for (k = 0; k < cases[i].count; ++k) {
            for (guard = 0; guard < 60 && bva.visual.frame_index != cases[i].pulses[k].frame;
                 ++guard) {
                wm_bret_backend_tick(&bva, &a, 5);
                if (!(a.anim_mode & WM_MODE_CHECKHIT)) seen_off_between = 1;
            }
            CHECK(bva.visual.frame_index == cases[i].pulses[k].frame);
            CHECK(a.anim_mode & WM_MODE_CHECKHIT);
            CHECK(a.attack_mode == cases[i].pulses[k].mode);
            CHECK(a.attack_xoff == cases[i].pulses[k].x);
            CHECK(a.attack_yoff == cases[i].pulses[k].y);
            CHECK(a.attack_zoff == cases[i].pulses[k].z);
            CHECK(a.attack_width == cases[i].pulses[k].w);
            CHECK(a.attack_height == cases[i].pulses[k].h);
            CHECK(a.attack_depth == cases[i].pulses[k].d);
        }
        /* the ANI_ATTACK_OFF between two pulses genuinely ran -- without it
           the second ANI_ATTACK_ON would never be reached */
        CHECK(seen_off_between == 1);

        /* the closing ANI_SETMODE,MODE_NORMAL clears all three bits */
        for (guard = 0; guard < 80 && (a.anim_mode & WM_MODE_UNINT); ++guard)
            wm_bret_backend_tick(&bva, &a, 11);
        CHECK(!(a.anim_mode & WM_MODE_UNINT));
        CHECK(!(a.anim_mode & WM_MODE_NOAUTOFLIP));
        CHECK(!(a.anim_mode & WM_MODE_OVERLAP));
        CHECK(!(a.anim_mode & WM_MODE_CHECKHIT));
    }
}

/* Batch 3: the animations tools/wlattack.py --audit says a flat
   wlanim.py --slice genuinely represents. Traced out of HRTSEQ2.ASM:

     hrt_4_push_anim       [3] AMODE_PUSH,11,83,70,20
     hrt_4_jump_kick_anim  [4] AMODE_FLYKICK,15,69,64,38
     hrt_4_knee_fall_anim  [2] AMODE_BIGKNEE,11,44,51,49
     hrt_kick_TB_anim      [2] AMODE_SPINKICK,5,54,70,34, ATTACK_OFF two
                               frames later rather than one

   None is an ANI_ATTACK_ON_Z, so all take wm_arcade_ani_attack_on's real
   z defaults. */
static void test_bret_attack_windows_batch3(void) {
    static const struct {
        wm_arcade_bret_anim_id_t id;
        const wm_visual_sequence *seq;
        size_t frame;
        uint16_t attack_mode;
        int16_t xoff, yoff, width, height;
        bool sets_facing;
    } cases[] = {
        { WM_BRET_ANIM_PUSH4, &wm_bret_push4_anim, 3,
          WM_AMODE_PUSH, 11, 83, 70, 20, true },
        { WM_BRET_ANIM_JUMP_KICK4, &wm_bret_jump_kick4_anim, 4,
          WM_AMODE_FLYKICK, 15, 69, 64, 38, true },
        { WM_BRET_ANIM_KNEE_FALL4, &wm_bret_knee_fall4_anim, 2,
          WM_AMODE_BIGKNEE, 11, 44, 51, 49, false },
        { WM_BRET_ANIM_KICK_TB, &wm_bret_kick_tb_anim, 2,
          WM_AMODE_SPINKICK, 5, 54, 70, 34, true },
    };
    size_t i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        wm_arcade_actor_t a;
        wm_bret_backend_actor bva;
        int guard;

        memset(&a, 0, sizeof(a));
        a.new_facing_dir = WM_MOVE_UP_RIGHT;
        a.facing_dir = WM_MOVE_DOWN_LEFT;
        a.x_vel = 0x999;
        a.z_vel = 0x999;
        wm_bret_backend_init(&bva);
        wm_bret_backend_change_anim(&a, cases[i].id, &bva);
        CHECK(bva.visual.sequence == cases[i].seq);
        CHECK(a.anim_mode & WM_MODE_UNINT);
        CHECK(a.anim_mode & WM_MODE_NOAUTOFLIP);
        /* Every one leads with ANI_ZEROVELS -- but hrt_kick_TB_anim then
           sets a real leap velocity in the same header
           (ANI_SET_XVEL,-20000h,AM_FACE_REL and ANI_SET_YVEL,48000h), which
           the blanket "zero x and z on every attack" approximation this
           replaced could not represent. Its z stays zeroed; its x and y do
           not. */
        if (cases[i].id == WM_BRET_ANIM_KICK_TB) {
            CHECK(a.x_vel != 0);
            CHECK(a.y_vel == 0x70000);
        } else {
            CHECK(a.x_vel == 0);
            CHECK(a.y_vel == 0);
        }
        CHECK(a.z_vel == 0);
        if (cases[i].sets_facing)
            CHECK(a.facing_dir == WM_MOVE_UP_RIGHT);
        else
            CHECK(a.facing_dir == WM_MOVE_DOWN_LEFT);

        for (guard = 0; guard < 60 && bva.visual.frame_index != cases[i].frame; ++guard)
            wm_bret_backend_tick(&bva, &a, 0);
        CHECK(bva.visual.frame_index == cases[i].frame);
        CHECK(a.anim_mode & WM_MODE_CHECKHIT);
        CHECK(a.attack_mode == cases[i].attack_mode);
        CHECK(a.attack_xoff == cases[i].xoff);
        CHECK(a.attack_yoff == cases[i].yoff);
        CHECK(a.attack_width == cases[i].width);
        CHECK(a.attack_height == cases[i].height);
        CHECK(a.attack_zoff == -40);
        CHECK(a.attack_depth == 80);

        for (guard = 0; guard < 80 && (a.anim_mode & WM_MODE_UNINT); ++guard)
            wm_bret_backend_tick(&bva, &a, 3);
        CHECK(!(a.anim_mode & WM_MODE_UNINT));
        CHECK(!(a.anim_mode & WM_MODE_CHECKHIT));
    }
}

/* ANIM.ASM's ANI_SETPLYRMODE where it falls partway through an animation
   rather than in the header. hrt_kick_TB_anim is the first wired animation
   that has one: MODE_INAIR2 before its own frame 0 (Bret really is
   airborne through the leap) and back to MODE_NORMAL before frame 4, once
   he lands. */
static void test_bret_midanim_setplyrmode(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    int guard;

    memset(&a, 0, sizeof(a));
    a.player_mode = WM_PMODE_RUNNING;
    wm_bret_backend_init(&bva);

    /* the header's own ANI_SETPLYRMODE, instant on selection */
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_KICK_TB, &bva);
    CHECK(a.player_mode == WM_PMODE_INAIR2);

    /* still airborne right up to the frame the source lands on */
    for (guard = 0; guard < 40 && bva.visual.frame_index < 3; ++guard) {
        wm_bret_backend_tick(&bva, &a, 0);
        if (bva.visual.frame_index < 4) CHECK(a.player_mode == WM_PMODE_INAIR2);
    }
    for (guard = 0; guard < 40 && bva.visual.frame_index != 4; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(bva.visual.frame_index == 4);
    CHECK(a.player_mode == WM_PMODE_NORMAL);

    /* hrt_3_head_held_stand_anim's own header ANI_SETPLYRMODE,MODE_NORMAL
       is what actually releases mode_headhold -- the whole point of the
       animation, and it has no attack window at all, so it exercises the
       header path for a non-attacking wired animation. */
    memset(&a, 0, sizeof(a));
    a.player_mode = WM_PMODE_HEADHOLD;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_HEAD_HELD_STAND3, &bva);
    CHECK(bva.visual.sequence == &wm_bret_head_held_stand3_anim);
    CHECK(a.player_mode == WM_PMODE_NORMAL);
    CHECK(a.anim_mode & WM_MODE_UNINT);
    CHECK(a.anim_mode & WM_MODE_NOAUTOFLIP);
    for (guard = 0; guard < 40 && (a.anim_mode & WM_MODE_UNINT); ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(!(a.anim_mode & WM_MODE_UNINT));
}

/* Batch 4. hrt_4_knee_to_head_anim's ANI_ATTACK_ON sits at frame 2 of its
   chained 8-frame stream -- past the end of the 1-frame fragment its own
   label alone yields, so this window simply could not exist before the
   extractor followed the routine's trailing ANI_GOTO.
   hrt_3_fake_hold_anim carries no ANI_ATTACK_ON at all (it is a feint),
   but does carry the MODE_UNINT|MODE_OVERLAP header and an
   ANI_SETPLYRMODE,MODE_NORMAL, so it exercises the non-attacking wired
   path. */
static void test_bret_attack_windows_batch4(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    int guard;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_KNEE_TO_HEAD4, &bva);
    CHECK(bva.visual.sequence == &wm_bret_knee_to_head4_anim);
    CHECK(a.anim_mode & WM_MODE_UNINT);

    for (guard = 0; guard < 40 && bva.visual.frame_index != 2; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(bva.visual.frame_index == 2);
    CHECK(a.anim_mode & WM_MODE_CHECKHIT);
    CHECK(a.attack_mode == WM_AMODE_KNEE);
    CHECK(a.attack_xoff == 11);
    CHECK(a.attack_yoff == 44);
    CHECK(a.attack_width == 51);
    CHECK(a.attack_height == 49);

    for (guard = 0; guard < 60 && (a.anim_mode & WM_MODE_UNINT); ++guard)
        wm_bret_backend_tick(&bva, &a, 9);
    CHECK(!(a.anim_mode & WM_MODE_UNINT));
    CHECK(!(a.anim_mode & WM_MODE_CHECKHIT));

    /* fake_hold3: real frame data now, so its MODE_UNINT is timed off the
       animation instead of the one-tick stopgap it used to get. */
    memset(&a, 0, sizeof(a));
    a.player_mode = WM_PMODE_HEADHOLD;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_FAKE_HOLD3, &bva);
    CHECK(bva.visual.sequence == &wm_bret_fake_hold3_anim);
    CHECK(a.anim_mode & WM_MODE_UNINT);
    CHECK(a.anim_mode & WM_MODE_OVERLAP);
    CHECK(a.player_mode == WM_PMODE_NORMAL);
    wm_bret_backend_tick(&bva, &a, 0);
    CHECK(a.anim_mode & WM_MODE_UNINT);   /* not cleared same tick any more */
    for (guard = 0; guard < 60 && (a.anim_mode & WM_MODE_UNINT); ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(!(a.anim_mode & WM_MODE_UNINT));
    CHECK(!(a.anim_mode & WM_MODE_OVERLAP));
}

/* ANIM.ASM's RPT_COUNT loop (_ani_set_rptcount:3530, _ani_dec_rptcount:3552,
   _ani_if_rptcount:3257): ANI_SET_RPTCOUNT,N seeds the count, the span
   between the loop label and ANI_IF_RPTCOUNT plays N times, then the stream
   continues past it. hrt_knees_to_head_anim is the case that matters most,
   because its FIRST ANI_ATTACK_ON sits inside that span -- so the window has
   to fire once per pass, exactly like the source, rather than once. */
static void test_bret_rptcount_loop(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    int guard;
    int visits_to_loop_start = 0;
    int attack_on_edges = 0;
    bool was_on = false;
    size_t prev_index;

    CHECK(wm_bret_knees_to_head_anim.loop_count == 3);
    CHECK(wm_bret_knees_to_head_anim.loop_first == 1);
    CHECK(wm_bret_knees_to_head_anim.loop_last == 5);

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_KNEES_TO_HEAD, &bva);
    CHECK(bva.visual.sequence == &wm_bret_knees_to_head_anim);
    CHECK(bva.visual.rpt_count == 3);

    prev_index = bva.visual.frame_index;
    for (guard = 0; guard < 400 && !bva.visual.ended; ++guard) {
        bool on;
        wm_bret_backend_tick(&bva, &a, 4);
        if (bva.visual.frame_index != prev_index &&
            bva.visual.frame_index == wm_bret_knees_to_head_anim.loop_first)
            ++visits_to_loop_start;
        prev_index = bva.visual.frame_index;
        on = (a.anim_mode & WM_MODE_CHECKHIT) != 0;
        if (on && !was_on) ++attack_on_edges;
        was_on = on;
    }
    CHECK(bva.visual.ended);

    /* Entered the span once by falling into it, then branched back twice
       (count 3 -> 2 -> 1, the third decrement reaching 0 and falling
       through), so the loop's first frame is reached three times. */
    CHECK(visits_to_loop_start == 3);

    /* Three passes each firing the in-loop window at frame 3, plus the one
       after the loop at frame 8. */
    CHECK(attack_on_edges == 4);

    /* Without the loop the stream would be its 13 frames once; the pins
       carry the same shape and no attack window at all. */
    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_PIN2, &bva);
    CHECK(bva.visual.rpt_count == 3);
    CHECK(a.anim_mode & WM_MODE_UNINT);
    CHECK(a.anim_mode & WM_MODE_OVERLAP);
    /* The pin is genuinely long -- 35 frames with real holds, plus its
       own 10-frame span three times, ~1300 ticks end to end. */
    for (guard = 0; guard < 3000 && !bva.visual.ended; ++guard) {
        wm_bret_backend_tick(&bva, &a, 0);
        CHECK(!(a.anim_mode & WM_MODE_CHECKHIT));
    }
    CHECK(bva.visual.ended);
}

/* Batch 6's attack windows and header commands, all traced with
   tools/wlattack.py after ANI_CHANGEANIM was recognised as a terminator.
   hrt_2/4_butts_anim's window is inside their ANI_SET_RPTCOUNT,3 span, so
   it fires three times; the two combos carry three pulses each. */
static void test_bret_attack_windows_batch6(void) {
    static const struct {
        wm_arcade_bret_anim_id_t id;
        const wm_visual_sequence *seq;
        size_t frame;
        uint16_t attack_mode;
        int16_t xoff, yoff, width, height;
    } cases[] = {
        { WM_BRET_ANIM_BUTTS2, &wm_bret_butts2_anim, 3,
          WM_AMODE_HDBUTT_STAY, 19, 75, 35, 24 },
        { WM_BRET_ANIM_BUTTS4, &wm_bret_butts4_anim, 3,
          WM_AMODE_HDBUTT_STAY, 19, 75, 35, 24 },
        { WM_BRET_ANIM_FLYING_KICK, &wm_bret_flying_kick_anim, 4,
          WM_AMODE_FLYKICK, -3, 26, 61, 21 },
        { WM_BRET_ANIM_RUNNING_GROUND_PUNCH,
          &wm_bret_running_ground_punch_anim, 5,
          WM_AMODE_BUTTSTOMP, -50, -6, 36, 23 },
        { WM_BRET_ANIM_COMBO_KICK, &wm_bret_combo_kick_anim, 3,
          WM_AMODE_KICK, 23, 53, 50, 27 },
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
        CHECK(a.anim_mode & WM_MODE_UNINT);

        for (guard = 0; guard < 80 && bva.visual.frame_index != cases[i].frame; ++guard)
            wm_bret_backend_tick(&bva, &a, 0);
        CHECK(bva.visual.frame_index == cases[i].frame);
        CHECK(a.anim_mode & WM_MODE_CHECKHIT);
        CHECK(a.attack_mode == cases[i].attack_mode);
        CHECK(a.attack_xoff == cases[i].xoff);
        CHECK(a.attack_yoff == cases[i].yoff);
        CHECK(a.attack_width == cases[i].width);
        CHECK(a.attack_height == cases[i].height);
    }

    /* hrt_tbukl_leap_anim's header carries three extra mode bits beyond
       UNINT|NOAUTOFLIP, and its box is an ANI_ATTACK_ON_Z. */
    {
        wm_arcade_actor_t a;
        wm_bret_backend_actor bva;
        int guard;

        memset(&a, 0, sizeof(a));
        wm_bret_backend_init(&bva);
        wm_bret_backend_change_anim(&a, WM_BRET_ANIM_TBUKL_LEAP, &bva);
        CHECK(a.anim_mode & WM_MODE_OVERLAP);
        CHECK(a.anim_mode & WM_MODE_NOCONFINE);
        CHECK(a.anim_mode & WM_MODE_NOGRAVITY);
        for (guard = 0; guard < 60 && bva.visual.frame_index != 4; ++guard)
            wm_bret_backend_tick(&bva, &a, 0);
        CHECK(a.attack_mode == WM_AMODE_BSTOMP);
        CHECK(a.attack_zoff == -10);
        CHECK(a.attack_depth == 70);
        /* lands on the ground partway through */
        for (guard = 0; guard < 60 && bva.visual.frame_index != 6; ++guard)
            wm_bret_backend_tick(&bva, &a, 0);
        CHECK(a.player_mode == WM_PMODE_ONGROUND);
    }

    /* hrt_fall_back_anim carries no attack window, an OVERLAP|NOCOLLIS
       header, and its own landing partway through. */
    {
        wm_arcade_actor_t a;
        wm_bret_backend_actor bva;
        int guard;

        memset(&a, 0, sizeof(a));
        wm_bret_backend_init(&bva);
        wm_bret_backend_change_anim(&a, WM_BRET_ANIM_FALL_BACK, &bva);
        CHECK(a.anim_mode & WM_MODE_UNINT);
        CHECK(a.anim_mode & WM_MODE_OVERLAP);
        CHECK(a.anim_mode & WM_MODE_NOCOLLIS);
        for (guard = 0; guard < 200 && bva.visual.frame_index != 11; ++guard) {
            wm_bret_backend_tick(&bva, &a, 0);
            CHECK(!(a.anim_mode & WM_MODE_CHECKHIT));
        }
        CHECK(bva.visual.frame_index == 11);
        CHECK(a.player_mode == WM_PMODE_ONGROUND);
    }
}

/* ANI_CHANGEANIM's hand-off (ANIM.ASM:1301): an animation that ends by
   BECOMING another does not stop -- the target starts, with its own header
   commands. hrt_tbukl_leap_anim is the case worth checking end to end
   because it transitions twice: leap -> hit the ground face down -> get
   up, each step a real routine with its own ANI_CHANGEANIM. */
static void test_bret_anim_transition_chain(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    int guard;
    int saw_hitground = 0, saw_getup = 0;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_TBUKL_LEAP, &bva);
    CHECK(bva.current_id == WM_BRET_ANIM_TBUKL_LEAP);

    for (guard = 0; guard < 600; ++guard) {
        wm_bret_backend_tick(&bva, &a, 0);
        if (bva.current_id == WM_BRET_ANIM_HITONGROUND_FACEDOWN) saw_hitground = 1;
        if (bva.current_id == WM_BRET_ANIM_FACEUP_GETUP) saw_getup = 1;
    }
    CHECK(saw_hitground == 1);
    CHECK(saw_getup == 1);
    CHECK(bva.current_id == WM_BRET_ANIM_FACEUP_GETUP);
    CHECK(bva.visual.sequence == &wm_bret_faceup_getup_anim);
    /* the getup's own header put him back on his feet */
    CHECK(a.player_mode == WM_PMODE_NORMAL);

    /* fall_back -> faceup_getup, the single-step case, and the one the
       death path already selects. */
    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_FALL_BACK, &bva);
    CHECK(a.anim_mode & WM_MODE_NOCOLLIS);
    for (guard = 0; guard < 600 &&
                    bva.current_id != WM_BRET_ANIM_FACEUP_GETUP; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(bva.current_id == WM_BRET_ANIM_FACEUP_GETUP);
    /* the target's own header replaced the source animation's mode bits
       rather than inheriting them */
    CHECK(a.anim_mode & WM_MODE_NOCOLLIS);
    CHECK(!(a.anim_mode & WM_MODE_OVERLAP));

    /* An animation with no transition still simply ends. */
    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_BUTTS2, &bva);
    for (guard = 0; guard < 600 && !bva.visual.ended; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(bva.visual.ended);
    CHECK(bva.current_id == WM_BRET_ANIM_BUTTS2);
}

/* ANIM.ASM's inline motion commands, generated per animation by
   tools/wlcommands.py. A wired attack has always played its real frames and
   set its real attack box, but never moved -- these are what move it. The
   port previously approximated the headers by zeroing x and z velocity on
   every attack, which is wrong for any attack that leaps. */
static void test_bret_frame_motion_commands(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    int guard;

    /* hrt_kick_TB_anim's header: ANI_ZEROVELS, then ANI_SET_YVEL,70000h and
       ANI_SET_XVEL,-20000h,AM_FACE_REL, then ANI_OFFSET,5,0,0. The x value
       is negated when the facing-right bit is clear (ANIM.ASM:1626), and
       the offset moves him instantly. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_UP_RIGHT;   /* right bit set -> x used as written */
    a.x_int = 100;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_KICK_TB, &bva);
    CHECK(a.y_vel == 0x70000);
    CHECK(a.x_vel == -0x20000);
    CHECK(a.x_int == 105);
    CHECK(a.x_fixed == (105 << 16));
    CHECK(a.z_vel == 0);

    /* Facing left, the same command negates. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_UP_LEFT;
    a.x_int = 100;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_KICK_TB, &bva);
    CHECK(a.x_vel == 0x20000);
    CHECK(a.x_int == 95);   /* the offset flips too */

    /* hrt_2_punch_anim: ANI_ZEROVELS in the header and a real
       ANI_SET_YVEL,30000h partway through, on the connected-hit path. The
       mid-animation one must fire when the animation reaches that frame,
       not on selection. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_UP_RIGHT;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_PUNCH2, &bva);
    CHECK(a.y_vel == 0);
    for (guard = 0; guard < 60 && bva.visual.frame_index != 6; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(bva.visual.frame_index == 6);
    CHECK(a.y_vel == 0x30000);

    /* hrt_4_knee_fall_anim carries an ANI_OFFSET,23,0,0 at frame 1 and both
       a y and a z velocity at frame 3 -- three different command kinds in
       one animation, all fired off the generated table. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_UP_RIGHT;
    a.x_int = 0;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_KNEE_FALL4, &bva);
    for (guard = 0; guard < 60 && bva.visual.frame_index != 1; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(a.x_int == 23);
    for (guard = 0; guard < 60 && bva.visual.frame_index != 3; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(a.y_vel == 0x50000);
    CHECK(a.z_vel != 0);
}

/* ANIM.ASM:71 _ani_ifbuttons -- "and a1,a0 / cmp a1,a0 / jrne #fail": EVERY
   named button must be held, not just any of them. Across HRTSEQ2-4 the only
   case is PLAYER_PUNCH_VAL|PLAYER_KICK_VAL -> start_run_anim, the run cancel
   out of an attack's opening frames. Getting the mask into the wrong operand
   slot made the test `(but_val_cur & 0) == 0`, which is true on every frame,
   so every attack cancelled itself into a run on tick 0 -- this checks the
   condition, not just that a cancel is possible. */
static void test_bret_ifbuttons_run_cancel(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    int guard;

    /* Punch alone: no cancel. The headbutt plays as itself. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_UP_RIGHT;
    a.but_val_cur = WM_BTN_PUNCH;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_BUTT4, &bva);
    CHECK(bva.current_id == WM_BRET_ANIM_BUTT4);
    for (guard = 0; guard < 10; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(bva.current_id == WM_BRET_ANIM_BUTT4);
    CHECK(a.player_mode != WM_PMODE_RUNNING);

    /* Punch AND kick: the animation becomes start_run_anim, which is
       WRESTLE2.ASM:3443's state setup plus Bret's own run animation. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_UP_RIGHT;
    a.new_facing_dir = WM_MOVE_UP_RIGHT;
    a.but_val_cur = (uint16_t)(WM_BTN_PUNCH | WM_BTN_KICK);
    a.getup_time = 5;
    a.run_time = 9;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_BUTT4, &bva);
    wm_bret_backend_tick(&bva, &a, 0);
    CHECK(bva.current_id == WM_BRET_ANIM_START_RUN);
    CHECK(bva.visual.sequence == &wm_bret_run_anim);
    /* #dorun's own state: timers cleared, MODE RUNNING, DELAY_BUTNS set. */
    CHECK(a.player_mode == WM_PMODE_RUNNING);
    CHECK(a.getup_time == 0);
    CHECK(a.run_time == 0);
    CHECK(a.delay_butns == 1);

    /* Kick alone is not enough either -- the mask needs both bits. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_UP_RIGHT;
    a.but_val_cur = WM_BTN_KICK;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_BUTT4, &bva);
    for (guard = 0; guard < 10; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(bva.current_id == WM_BRET_ANIM_BUTT4);
}

/* The instant state commands ANIM.ASM carries inline, each self-contained:
   ANI_SETSPEED (:38), ANI_STARTATTACK (:117), ANI_SET_WRESTLER_XFLIP (:95),
   ANI_FACEUP (:33) / ANI_FACEDOWN (:34), ANI_CLR_BUTCOUNT (:97),
   ANI_SAFE_TIME (:104), ANI_GRAVITY_ON (:15), ANI_CLR_STATUS (:58). */
static void test_bret_instant_state_commands(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    int guard;

    /* hrt_2_punch_anim's header: ANI_SETSPEED,100h,
       ANI_SET_WRESTLER_XFLIP, then ANI_STARTATTACK,AT_PUNCH,5 at frame 3. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_UP_RIGHT;
    a.obj_control = WM_OBJ_FLIPH;    /* stale flip, facing right */
    a.punchb_count = 7;
    a.spunchb_count = 7;
    a.skickb_count = 7;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_PUNCH2, &bva);

    /* Every ANI_SETSPEED across HRTSEQ2-4 is 100h, the identity rate. */
    CHECK(a.ani_speed == 0x100);
    /* facing right -> the mirror is cleared */
    CHECK((a.obj_control & WM_OBJ_FLIPH) == 0);

    for (guard = 0; guard < 40 && bva.visual.frame_index != 3; ++guard)
        wm_bret_backend_tick(&bva, &a, 11);
    CHECK(bva.visual.frame_index == 3);
    /* AT_PUNCH is 0 (DAMAGE.EQU:174); ATTACK_TIME is round_tickcount + 5. */
    CHECK(a.attack_type == 0);
    CHECK(a.attack_time == 11 + 5);

    /* Facing left, the same header command sets the mirror instead. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_UP_LEFT;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_PUNCH2, &bva);
    CHECK(a.obj_control & WM_OBJ_FLIPH);

    /* hrt_facedown_getup_anim carries ANI_SAFE_TIME,50 and, being a getup,
       an ANI_FACEDOWN at the end. hrt_2_butts_anim carries
       ANI_CLR_BUTCOUNT inside its repeat span. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_UP_RIGHT;
    a.punchb_count = 4;
    a.spunchb_count = 4;
    a.skickb_count = 4;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_BUTTS2, &bva);
    CHECK(a.punchb_count == 0);
    CHECK(a.spunchb_count == 0);
    CHECK(a.skickb_count == 0);

    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_UP_RIGHT;
    wm_bret_backend_init(&bva);
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_FACEDOWN_GETUP, &bva);
    for (guard = 0; guard < 80 && a.safe_time == 0; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(a.safe_time == 50);
}

/* The program interpreter (wm/anim_program.h) against the flat runtime it
   is meant to replace. Driven side by side, a tick at a time, comparing the
   frame each shows.

   On the CONNECTED-HIT path they agree, which is the equivalence that
   matters: the flat extractor always kept the hit path, so reproducing it
   is what proves the interpreter is not inventing anything. On the MISS
   path they must NOT agree -- the whole point is that a miss really plays
   fewer frames, because ANI_SLIDE_BACK and ANI_IFNOTSTATUS branch past the
   connected-hit ones, and a flat list cannot express that. */
static int program_vs_flat(const char *label, const wm_visual_sequence *seq,
                           bool hit, int *ticks_out) {
    wm_anim_exec ex;
    wm_visual_state vs;
    wm_arcade_actor_t a;
    const wm_anim_program *p = wm_anim_program_find(label);
    int t, diff = 0, n = 0;

    if (!p) return -1;
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_UP_RIGHT;
    wm_anim_exec_start(&ex, p, &a, 0);
    wm_visual_start(&vs, seq);

    for (t = 0; t < 3000; ++t) {
        const char *pf = wm_anim_exec_frame(&ex);
        const wm_visual_frame *vf = wm_visual_current(&vs);
        if (!pf && vs.ended) break;
        if (!pf || vs.ended) { ++diff; break; }
        ++n;
        if (strcmp(pf, vf->source_frame) != 0) ++diff;
        if (hit) a.anim_mode |= (uint16_t)WM_MODE_STATUS;
        else a.anim_mode &= (uint16_t)~WM_MODE_STATUS;
        wm_anim_exec_tick(&ex, &a, 0);
        wm_visual_tick(&vs);
    }
    if (ticks_out) *ticks_out = n;
    return diff;
}

static void test_anim_program_interpreter(void) {
    int hit_ticks = 0, miss_ticks = 0;

    /* Every wired animation has a program. */
    CHECK(wm_anim_program_find("hrt_2_punch_anim") != NULL);
    CHECK(wm_anim_program_find("hrt_4_block_anim") != NULL);
    CHECK(wm_anim_program_find("hrt_fall_back_anim") != NULL);
    CHECK(wm_anim_program_find("no_such_animation") == NULL);

    /* Hit path: the interpreter reproduces the shipped frame data exactly,
       including the ANI_SET_RPTCOUNT loop in knees_to_head. */
    CHECK(program_vs_flat("hrt_2_punch_anim", &wm_bret_light_punch2_anim,
                          true, NULL) == 0);
    CHECK(program_vs_flat("hrt_4_punch_anim", &wm_bret_light_punch4_anim,
                          true, NULL) == 0);
    CHECK(program_vs_flat("hrt_2_kick_anim", &wm_bret_light_kick2_anim,
                          true, NULL) == 0);
    CHECK(program_vs_flat("hrt_2_butt_anim", &wm_bret_butt2_anim,
                          true, NULL) == 0);
    CHECK(program_vs_flat("hrt_4_uppercut_anim", &wm_bret_uppercut4_anim,
                          true, NULL) == 0);
    CHECK(program_vs_flat("hrt_2_stomp_anim", &wm_bret_stomp2_anim,
                          true, NULL) == 0);
    CHECK(program_vs_flat("hrt_4_block_anim", &wm_bret_block4_anim,
                          true, NULL) == 0);
    CHECK(program_vs_flat("hrt_knees_to_head_anim", &wm_bret_knees_to_head_anim,
                          true, NULL) == 0);

    /* hrt_4_push_anim is the one that does NOT match even on the hit path,
       and it is right not to: its ANI_IFSTATUS skips a 5-tick frame when
       the shove connected. The flat list keeps that frame unconditionally,
       so this is the approximation showing, not an interpreter fault. */
    CHECK(program_vs_flat("hrt_4_push_anim", &wm_bret_push4_anim,
                          true, &hit_ticks) != 0);

    /* Miss path: genuinely shorter, because the branches skip the
       connected-hit frames. This is the playthrough the flat model could
       never represent. */
    (void)program_vs_flat("hrt_4_push_anim", &wm_bret_push4_anim,
                          false, &miss_ticks);
    CHECK(miss_ticks < hit_ticks);

    {
        int punch_hit = 0, punch_miss = 0;
        (void)program_vs_flat("hrt_2_punch_anim", &wm_bret_light_punch2_anim,
                              true, &punch_hit);
        (void)program_vs_flat("hrt_2_punch_anim", &wm_bret_light_punch2_anim,
                              false, &punch_miss);
        /* hrt_2_punch_anim's ANI_SLIDE_BACK skips ANI_SET_YVEL and a
           3-tick frame when the punch missed. */
        CHECK(punch_miss < punch_hit);
    }
}

/* ATTRACT.ASM:669 TURN_SOUNDS_OFF_IF_NEED gates the DCS_LOGO music on
   AMODE_LOOPS < 2 -- it suppresses sound, and never skips or re-enters an
   attract call. app.c's guard for that had no body of its own and captured
   the following `break` instead, so from the third attract loop onward
   WM_ATTRACT_DCS_LOGO fell straight through into
   WM_ATTRACT_SHOW_SPORTS_LOGO's entry body and ran it while the call was
   still DCS_LOGO.

   The observable that separates the two is that body's own
   `sports_world_x = 0; sports_world_y = 0;`. Driving the real app past two
   attract loops reaches DCS_LOGO with AMODE_LOOPS >= 2 in ~12k ticks, and
   at that moment the sports offsets legitimately hold -900/900; the
   fallthrough zeroed them. Confirmed to fail against the original code and
   pass against the fix, rather than merely asserting the shape of it. */
static void test_attract_dcs_logo_does_not_fall_through(void) {
    static wm_app app;
    wm_attract_call prev;
    int i;
    int entries = 0;

    wm_app_init(&app);
    prev = app.attract.call;
    for (i = 0; i < 200000 && app.attract.amode_loops < 3u; ++i) {
        wm_app_tick(&app, NULL);
        if (app.attract.call == WM_ATTRACT_DCS_LOGO &&
            prev != WM_ATTRACT_DCS_LOGO &&
            app.attract.amode_loops >= 2u) {
            ++entries;
            CHECK(app.attract.sports_world_x != 0 ||
                  app.attract.sports_world_y != 0);
        }
        prev = app.attract.call;
    }
    CHECK(app.attract.amode_loops >= 3u);
    CHECK(entries >= 1);
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
    /* A real actor always has a facing, and hrt_2_punch_anim's header
       carries ANI_SET_WRESTLER_XFLIP (ANIM.ASM:95), which mirrors the
       sprite -- and so the attack box -- whenever the MOVE_RIGHT bit is
       clear. Leaving facing_dir memset to 0 made the attacker face nowhere,
       and once that command was translated his box mirrored away from the
       victim. He faces right, toward the victim at +60. */
    attacker.facing_dir = WM_MOVE_UP_RIGHT;
    attacker.new_facing_dir = WM_MOVE_UP_RIGHT;
    victim.facing_dir = WM_MOVE_UP_LEFT;
    victim.new_facing_dir = WM_MOVE_UP_LEFT;

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
    wm_arcade_adjust_health(&victim, -30, NULL, false, 12345u, NULL, NULL);
    CHECK(victim.life == 74);
    CHECK(victim.player_mode == WM_PMODE_NORMAL);
    /* LIFEBAR.ASM:1593-1595: unconditional on every call. */
    CHECK(victim.last_damage == (uint16_t)12345u);
}

static void test_arcade_adjust_health_clamps_to_life_max(void) {
    wm_arcade_actor_t victim;
    memset(&victim, 0, sizeof(victim));
    victim.life = WM_ARCADE_LIFE_MAX - 5;
    wm_arcade_adjust_health(&victim, 50, NULL, false, 0, NULL, NULL);
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
    wm_arcade_adjust_health(&victim, 7, NULL, false, 0, NULL, NULL); /* healing */
    CHECK(victim.life == 57);
}

static void test_arcade_adjust_health_fudge_saves_a_near_death_hit(void) {
    wm_arcade_actor_t victim;
    memset(&victim, 0, sizeof(victim));
    /* -24 scaled by _85PCT (LIFEBAR.ASM:1471-1521) -> -21: life+delta = -6,
       > -10 (not overkilled by 10+) and the scaled delta is <= -20 (a 20+
       point hit) -- LIFEBAR.ASM:1561-1569 fudge applies: life = 5. */
    victim.life = 15;
    wm_arcade_adjust_health(&victim, -24, NULL, false, 0, NULL, NULL);
    CHECK(victim.life == 5);
    CHECK(victim.player_mode == WM_PMODE_NORMAL);
}

static void test_arcade_adjust_health_attract_mode_never_dies(void) {
    wm_arcade_actor_t victim;
    memset(&victim, 0, sizeof(victim));
    victim.life = 5;
    wm_arcade_adjust_health(&victim, -5, NULL, true, 0, NULL, NULL);
    CHECK(victim.life == WM_ARCADE_LIFE_MAX);
    CHECK(victim.player_mode == WM_PMODE_NORMAL);
}

static void test_arcade_adjust_health_death(void) {
    wm_arcade_actor_t victim;
    memset(&victim, 0, sizeof(victim));
    victim.life = 5;
    victim.anim_mode = WM_MODE_CHECKHIT;
    wm_arcade_adjust_health(&victim, -5, NULL, false, 0, NULL, NULL);
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
    wm_arcade_adjust_health(&victim, -5, &attacker, false, 0, NULL, NULL);
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
    wm_arcade_adjust_health(&victim, -1, &attacker, false, 0, &dam_mult, NULL);
    CHECK(victim.life == 92); /* not 99: the original -1 delta is ignored */
    CHECK(dam_mult == 0);

    memset(&victim, 0, sizeof(victim));
    victim.life = 100;
    attacker.combo_count = 20; /* magnitude = max(10-20,4) = 4 (floored),
                                   scaled by _85PCT -> -4 (unchanged) */
    wm_arcade_adjust_health(&victim, -1, &attacker, false, 0, NULL, NULL);
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
    wm_arcade_adjust_health(&victim, -10, NULL, false, 0, &dam_mult, NULL);
    CHECK(victim.life == 87);
    CHECK(dam_mult == 0);

    memset(&victim, 0, sizeof(victim));
    victim.life = 100;
    dam_mult = 4; /* x2.5: -10*5/2 = -25, then *_85PCT -> -22 */
    wm_arcade_adjust_health(&victim, -10, NULL, false, 0, &dam_mult, NULL);
    CHECK(victim.life == 78);
    CHECK(dam_mult == 0);

    /* dam_mult == 0: no DAM_MULT scaling, but _85PCT still applies:
       -10 -> -9. */
    memset(&victim, 0, sizeof(victim));
    victim.life = 100;
    dam_mult = 0;
    wm_arcade_adjust_health(&victim, -10, NULL, false, 0, &dam_mult, NULL);
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

    wm_arcade_adjust_health(&victim, -8, NULL, false, 1000u, NULL, NULL);
    CHECK(victim.last_damage == 1000u);

    /* A later hit within REACT1.ASM's 50-tick window would now see a
       nonzero, recent last_damage -- see
       test_arcade_wrestler_hit_reduced_damage_within_window below for the
       actual reduced_damage effect. */
    wm_arcade_adjust_health(&victim, -8, NULL, false, 1010u, NULL, NULL);
    CHECK(victim.last_damage == 1010u);
}

static int g_death_anim_calls;
static wm_arcade_react1_anim_group_t g_death_anim_last;
static void test_death_anim_recorder(wm_arcade_actor_t *victim,
                                     wm_arcade_react1_anim_group_t anim,
                                     void *user) {
    (void)victim;
    ++g_death_anim_calls;
    g_death_anim_last = anim;
    (void)user;
}

/* LIFEBAR.ASM #fallbk: real death-dispatch tail (see
   wm_arcade_lifebar.h) -- fall_back anim + knockback away from the
   attacker, unless already moving away fast enough. */
static void test_arcade_adjust_health_death_fallback_anim_and_knockback(void) {
    wm_arcade_actor_t victim, attacker;
    wm_arcade_death_anim_callback_t death_anim;
    death_anim.change_anim = test_death_anim_recorder;
    death_anim.user = NULL;

    /* Attacker to the left of victim -> knocked right (+2.0 px/tick). */
    memset(&victim, 0, sizeof(victim));
    memset(&attacker, 0, sizeof(attacker));
    victim.life = 5;
    victim.player_mode = WM_PMODE_RUNNING;
    victim.x_int = 100;
    attacker.x_int = 50;
    g_death_anim_calls = 0;
    wm_arcade_adjust_health(&victim, -5, &attacker, false, 0, NULL, &death_anim);
    CHECK(victim.player_mode == WM_PMODE_DEAD);
    CHECK(victim.x_vel == (2 << 16));
    CHECK(g_death_anim_calls == 1);
    CHECK(g_death_anim_last == WM_R1_ANIM_FALL_BACK);

    /* Attacker to the right of victim -> knocked left (-2.0 px/tick). */
    memset(&victim, 0, sizeof(victim));
    memset(&attacker, 0, sizeof(attacker));
    victim.life = 5;
    victim.player_mode = WM_PMODE_NORMAL;
    victim.x_int = 50;
    attacker.x_int = 100;
    g_death_anim_calls = 0;
    wm_arcade_adjust_health(&victim, -5, &attacker, false, 0, NULL, &death_anim);
    CHECK(victim.x_vel == -(2 << 16));
    CHECK(g_death_anim_calls == 1);

    /* Already moving away faster than 2.0 px/tick -- left alone. */
    memset(&victim, 0, sizeof(victim));
    memset(&attacker, 0, sizeof(attacker));
    victim.life = 5;
    victim.player_mode = WM_PMODE_NORMAL;
    victim.x_vel = 5 << 16;
    g_death_anim_calls = 0;
    wm_arcade_adjust_health(&victim, -5, &attacker, false, 0, NULL, &death_anim);
    CHECK(victim.x_vel == (5 << 16));
    CHECK(g_death_anim_calls == 1);

    /* ROLL_POS always reset on genuine death. */
    CHECK(victim.roll_pos == 0);
}

/* LIFEBAR.ASM:1691-1710's own pre-checks: certain attack_mode ids, or the
   victim's own WM_STATUS_DEAD_ANIM bit, skip the whole death-anim dispatch
   (no anim call, velocities untouched). */
static void test_arcade_adjust_health_death_anim_skip_cases(void) {
    wm_arcade_actor_t victim, attacker;
    wm_arcade_death_anim_callback_t death_anim;
    death_anim.change_anim = test_death_anim_recorder;
    death_anim.user = NULL;

    memset(&victim, 0, sizeof(victim));
    memset(&attacker, 0, sizeof(attacker));
    victim.life = 5;
    victim.player_mode = WM_PMODE_NORMAL;
    victim.x_vel = 42;
    attacker.attack_mode = WM_AMODE_BSTOMP;
    g_death_anim_calls = 0;
    wm_arcade_adjust_health(&victim, -5, &attacker, false, 0, NULL, &death_anim);
    CHECK(g_death_anim_calls == 0);
    CHECK(victim.x_vel == 42); /* untouched, unlike the catch-all's zeroing */
    CHECK(victim.player_mode == WM_PMODE_DEAD);

    memset(&victim, 0, sizeof(victim));
    memset(&attacker, 0, sizeof(attacker));
    victim.life = 5;
    victim.player_mode = WM_PMODE_NORMAL;
    victim.x_vel = 42;
    victim.status_flags = WM_STATUS_DEAD_ANIM;
    g_death_anim_calls = 0;
    wm_arcade_adjust_health(&victim, -5, NULL, false, 0, NULL, &death_anim);
    CHECK(g_death_anim_calls == 0);
    CHECK(victim.x_vel == 42);

    /* AMODE_BUZZ only skips when the victim was unblocked (not MODE_BLOCK). */
    memset(&victim, 0, sizeof(victim));
    memset(&attacker, 0, sizeof(attacker));
    victim.life = 5;
    victim.player_mode = WM_PMODE_BLOCK;
    attacker.attack_mode = WM_AMODE_BUZZ;
    g_death_anim_calls = 0;
    wm_arcade_adjust_health(&victim, -5, &attacker, false, 0, NULL, &death_anim);
    CHECK(g_death_anim_calls == 1); /* blocked -- real anim dispatch runs */
}

/* LIFEBAR.ASM's own unmatched-mode catch-all: zero all velocities, no anim. */
static void test_arcade_adjust_health_death_unmatched_mode_zeroes_vels(void) {
    wm_arcade_actor_t victim;
    wm_arcade_death_anim_callback_t death_anim;
    death_anim.change_anim = test_death_anim_recorder;
    death_anim.user = NULL;

    memset(&victim, 0, sizeof(victim));
    victim.life = 5;
    victim.player_mode = WM_PMODE_ATTACHED;
    victim.x_vel = 7 << 16; victim.y_vel = 3 << 16; victim.z_vel = -(2 << 16);
    g_death_anim_calls = 0;
    wm_arcade_adjust_health(&victim, -5, NULL, false, 0, NULL, &death_anim);
    CHECK(victim.x_vel == 0);
    CHECK(victim.y_vel == 0);
    CHECK(victim.z_vel == 0);
    CHECK(g_death_anim_calls == 0);
}

static void test_hurt_box_real_adjust_health(wm_arcade_actor_t *victim, int16_t delta,
                                             wm_arcade_actor_t *damage_source, void *user) {
    wm_arcade_combat_runtime_t *runtime = (wm_arcade_combat_runtime_t *)user;
    /* Mirrors wm_match_adjust_health's adapter for a human match
       (attract_mode=false, matching wm_match_start_selected). */
    wm_arcade_adjust_health(victim, delta, damage_source, false,
                            runtime ? runtime->pcnt : 0,
                            runtime ? &runtime->dam_mult : NULL, NULL);
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
    /* Same reason as the hurt-box test above: hrt_2_punch_anim's
       ANI_SET_WRESTLER_XFLIP mirrors the attack box when the MOVE_RIGHT bit
       is clear, so the attacker needs a real facing. */
    attacker.facing_dir = WM_MOVE_UP_RIGHT;
    attacker.new_facing_dir = WM_MOVE_UP_RIGHT;
    victim.facing_dir = WM_MOVE_UP_LEFT;
    victim.new_facing_dir = WM_MOVE_UP_LEFT;

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
       test_match_human_punch_from_range_selects_real_punch), relative to
       P1's own real match-start position, not the origin. */
    m.actors[1].x_int = m.actors[0].x_int + 60;
    m.actors[1].x_fixed = m.actors[1].x_int << 16;
    m.actors[1].z_int = m.actors[0].z_int;
    m.actors[1].z_fixed = m.actors[0].z_fixed;
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

    /* wm_arcade_adjust_health's real death-dispatch tail reaches
       wm_match_death_change_anim -> wm_bret_backend_change_anim for real
       through the actual hit path, not just a direct call -- see
       wm_arcade_lifebar.h's own derivation for why WM_PMODE_NORMAL (this
       actor's mode at the moment of the killing hit) selects the real
       fall_back anim. */
    CHECK(m.bret_visual[1].current_id == WM_BRET_ANIM_FALL_BACK);

    /* wm_arcade_move_bret's real MODE_DEAD dispatch now reaches the wired
       mode_dead callback (wm_bret_backend_mode_dead ->
       wm_arcade_mode_dead) on the very next tick, setting
       WM_STATUS_NO_BUCKOFF exactly like DOINK.ASM's shared mode_dead --
       not just when called directly, but through real match play. */
    wm_match_tick(&m, &cb, &punch);
    CHECK((m.actors[1].status_flags & WM_STATUS_NO_BUCKOFF) != 0);

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
   contents (transcribed as torso_table in src/core/bret_backend.c),
   diagonal and off-diagonal alike: diag 0 (UP/UP_RIGHT) and diag 3 (LEFT/
   UP_LEFT) -> hrt_torso2_anim; diag 1 (RIGHT/DOWN_RIGHT) and diag 2 (DOWN/
   DOWN_LEFT) -> hrt_torso4_anim (matching hrt_torso8_anim/hrt_torso6_anim's
   own SUBR aliasing of those two). facing_compass and new_facing_compass
   only need to share a diag, not be equal -- WRESTLE.ASM's own >>1 fold
   (WRESTLE.ASM:4979/4985) does the same. Off-diagonal (different diag,
   i.e. mid-turn) now resolves to the real turn-transition sequence,
   including the reverse-rotation SUBR aliasing (e.g. diag3->diag0 reuses
   the same pointer as diag0->diag3, since hrt_8_to_2_turn2_anim IS
   hrt_2_to_8_turn2_anim in HRTSEQ1.ASM). */
static void test_bret_torso_anim_table(void) {
    CHECK(wm_bret_torso_anim(0, 0) == &wm_bret_torso2_anim); /* UP/UP, diag 0 */
    CHECK(wm_bret_torso_anim(1, 0) == &wm_bret_torso2_anim); /* UP_RIGHT/UP, diag 0 */
    CHECK(wm_bret_torso_anim(2, 3) == &wm_bret_torso4_anim); /* RIGHT/DOWN_RIGHT, diag 1 */
    CHECK(wm_bret_torso_anim(4, 5) == &wm_bret_torso4_anim); /* DOWN/DOWN_LEFT, diag 2 */
    CHECK(wm_bret_torso_anim(6, 7) == &wm_bret_torso2_anim); /* LEFT/UP_LEFT, diag 3 */
    CHECK(wm_bret_torso_anim(7, 6) == &wm_bret_torso2_anim); /* UP_LEFT/LEFT, diag 3 */

    /* Off-diagonal: the 12 real turn-transition entries, genuinely
       reachable now (FACING_DIR lagging NEW_FACING_DIR mid-walk). */
    CHECK(wm_bret_torso_anim(0, 2) == &wm_bret_2_to_4_turn2_anim); /* diag0->diag1 */
    CHECK(wm_bret_torso_anim(6, 4) == &wm_bret_2_to_4_turn2_anim); /* diag3->diag2, aliases diag0->diag1 */
    CHECK(wm_bret_torso_anim(2, 0) == &wm_bret_4_to_2_turn2_anim); /* diag1->diag0 */
    CHECK(wm_bret_torso_anim(4, 6) == &wm_bret_4_to_2_turn2_anim); /* diag2->diag3, aliases diag1->diag0 */
    CHECK(wm_bret_torso_anim(2, 4) == &wm_bret_4_to_6_turn2_anim); /* diag1->diag2 */
    CHECK(wm_bret_torso_anim(4, 2) == &wm_bret_4_to_6_turn2_anim); /* diag2->diag1, aliases diag1->diag2 */
    CHECK(wm_bret_torso_anim(0, 6) == &wm_bret_2_to_8_turn2_anim); /* diag0->diag3 */
    CHECK(wm_bret_torso_anim(6, 0) == &wm_bret_2_to_8_turn2_anim); /* diag3->diag0, aliases diag0->diag3 */
    CHECK(wm_bret_torso_anim(2, 6) == &wm_bret_4_to_8_turn2_anim); /* diag1->diag3 */
    CHECK(wm_bret_torso_anim(4, 0) == &wm_bret_4_to_8_turn2_anim); /* diag2->diag0, aliases diag1->diag3 */
    CHECK(wm_bret_torso_anim(0, 4) == &wm_bret_2_to_6_turn2_anim); /* diag0->diag2 */
    CHECK(wm_bret_torso_anim(6, 2) == &wm_bret_2_to_6_turn2_anim); /* diag3->diag1, aliases diag0->diag2 */

    CHECK(wm_bret_torso_anim(-1, 0) == NULL);
    CHECK(wm_bret_torso_anim(0, -1) == NULL);
    CHECK(wm_bret_torso_anim(8, 0) == NULL);
    CHECK(wm_bret_torso_anim(0, 8) == NULL);
}

/* BRET.ASM:2871 hrt_rotate_anims_table -- same shape/aliasing as
   hrt_torso_anims_table above, but the STAND-suffix leg/idle-turn table
   (wm_bret_rotate_anim/rotate_table). */
static void test_bret_rotate_anim_table(void) {
    CHECK(wm_bret_rotate_anim(0, 0) == &wm_bret_stand2_anim);
    CHECK(wm_bret_rotate_anim(2, 3) == &wm_bret_stand4_anim);
    CHECK(wm_bret_rotate_anim(6, 7) == &wm_bret_stand2_anim);

    CHECK(wm_bret_rotate_anim(0, 2) == &wm_bret_2_to_4_turn_anim);
    CHECK(wm_bret_rotate_anim(6, 4) == &wm_bret_2_to_4_turn_anim); /* diag3->diag2 aliases diag0->diag1 */
    CHECK(wm_bret_rotate_anim(2, 0) == &wm_bret_4_to_2_turn_anim);
    CHECK(wm_bret_rotate_anim(4, 6) == &wm_bret_4_to_2_turn_anim); /* diag2->diag3 aliases diag1->diag0 */
    CHECK(wm_bret_rotate_anim(2, 4) == &wm_bret_4_to_6_turn_anim);
    CHECK(wm_bret_rotate_anim(4, 2) == &wm_bret_4_to_6_turn_anim); /* diag2->diag1 aliases diag1->diag2 */
    CHECK(wm_bret_rotate_anim(0, 6) == &wm_bret_2_to_8_turn_anim);
    CHECK(wm_bret_rotate_anim(6, 0) == &wm_bret_2_to_8_turn_anim); /* diag3->diag0 aliases diag0->diag3 */
    CHECK(wm_bret_rotate_anim(2, 6) == &wm_bret_4_to_8_turn_anim);
    CHECK(wm_bret_rotate_anim(4, 0) == &wm_bret_4_to_8_turn_anim); /* diag2->diag0 aliases diag1->diag3 */
    CHECK(wm_bret_rotate_anim(0, 4) == &wm_bret_2_to_6_turn_anim);
    CHECK(wm_bret_rotate_anim(6, 2) == &wm_bret_2_to_6_turn_anim); /* diag3->diag1 aliases diag0->diag2 */

    CHECK(wm_bret_rotate_anim(-1, 0) == NULL);
    CHECK(wm_bret_rotate_anim(0, -1) == NULL);
    CHECK(wm_bret_rotate_anim(8, 0) == NULL);
    CHECK(wm_bret_rotate_anim(0, 8) == NULL);
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
    /* facing_dir/new_facing_dir are real now (wm/arcade/wm_arcade_closest.h,
       wm/movement.h) -- set as if a prior idle tick already caught FACING_DIR
       up to face right, same direction as this walk. */
    a.facing_dir = WM_MOVE_RIGHT;
    a.new_facing_dir = WM_MOVE_RIGHT;
    wm_bret_backend_execute_walk(&a, &bva);
    /* leg_table[RIGHT][RIGHT] == wm_bret_walk2_f2_anim, and the torso
       half's diagonal reselection (wm_bret_torso_anim(RIGHT,RIGHT)) ->
       wm_bret_torso4_anim. */
    CHECK(a.facing_dir == WM_MOVE_RIGHT); /* change_walk_anim's leg half never writes it */
    CHECK(bva.visual.sequence == &wm_bret_walk2_f2_anim);
    CHECK(bva.torso_visual.sequence == &wm_bret_torso4_anim);

    /* #zip (move_dir cleared to 0 by wm_execute_walk) leaves the leg
       sprite alone -- no leg animation for "not moving" -- and, since
       facing_dir/new_facing_dir are both still their zeroed defaults
       (never having moved), wm_convert_facing returns -1 so the torso half
       no-ops too. */
    wm_bret_backend_init(&bva);
    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_ZIP;
    wm_bret_backend_change_anim(&a, WM_BRET_ANIM_STAND4, &bva);
    wm_bret_backend_execute_walk(&a, &bva);
    CHECK(bva.visual.sequence == &wm_bret_stand4_anim);
    CHECK(bva.torso_visual.sequence == NULL);
    CHECK(a.facing_dir == 0); /* set_rotate_anim's catch-up copy: 0 == 0 */

    /* MODE_UNINT skips the torso half only (WRESTLE.ASM:4973-4976) -- the
       leg half is unaffected. */
    wm_bret_backend_init(&bva);
    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_RIGHT;
    a.facing_dir = WM_MOVE_RIGHT;
    a.anim_mode = WM_MODE_UNINT;
    wm_bret_backend_execute_walk(&a, &bva);
    CHECK(bva.visual.sequence == &wm_bret_walk2_f2_anim);
    CHECK(bva.torso_visual.sequence == NULL);
}

/* The real, now-wired gap wm/bret_backend.h documents: change_walk_anim's
   leg half never writes FACING_DIR, only WRESTLE.ASM's set_rotate_anim does
   (WM_MOVE_ZIP case, wm/movement.h) -- so FACING_DIR can lag NEW_FACING_DIR
   while actually walking, e.g. after running past a repositioned opponent.
   The leg table only ever indexes on MOVE_DIR/FACING_DIR, so it's
   unaffected; the torso table's matching off-diagonal turn-transition entry
   is now real and wired (wm_bret_torso_anim/torso_table), including its
   real ANI_SETFACING promotion -- see
   test_bret_backend_tick_promotes_facing_dir_via_setfacing for that half. */
static void test_bret_backend_execute_walk_facing_lags_new_facing_while_moving(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    a.move_dir = WM_MOVE_RIGHT;
    a.facing_dir = WM_MOVE_RIGHT;      /* frozen from before the opponent moved */
    a.new_facing_dir = WM_MOVE_LEFT;   /* opponent is now behind */
    wm_bret_backend_change_torso_anim(&a, WM_BRET_ANIM_TORSO2, &bva);
    wm_bret_backend_execute_walk(&a, &bva);

    CHECK(a.facing_dir == WM_MOVE_RIGHT); /* still frozen -- leg half doesn't touch it */
    CHECK(bva.visual.sequence == &wm_bret_walk2_f2_anim); /* leg table: FACING_DIR unaffected by NEW_FACING_DIR */
    /* torso_table[RIGHT diag=1][LEFT diag=3] -- the real off-diagonal
       turn-transition entry, not the old torso2_anim placeholder. */
    CHECK(bva.torso_visual.sequence == &wm_bret_4_to_8_turn2_anim);
}

/* WRESTLE.ASM::set_rotate_anim, called from #zip/do_stance
   (WRESTLE.ASM:5286). Real off-diagonal case (not the "never moved"
   zeroed default test_bret_backend_execute_walk_selects_leg_anim covers):
   set_rotate_anim reads FACING_DIR as it was *before* this tick's
   catch-up, so the selected anim reflects the turn actually being made. */
static void test_bret_backend_execute_walk_zip_selects_rotate_anim(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    a.move_dir = WM_MOVE_ZIP;
    a.facing_dir = WM_MOVE_UP_RIGHT;       /* old facing, diag0 */
    a.new_facing_dir = WM_MOVE_DOWN_RIGHT; /* opponent has moved, diag1 */
    wm_bret_backend_execute_walk(&a, &bva);

    /* rotate_table[diag0][diag1] == wm_bret_2_to_4_turn_anim, and the
       WM_MOVE_ZIP catch-up (wm/movement.h) still applies regardless. */
    CHECK(bva.visual.sequence == &wm_bret_2_to_4_turn_anim);
    CHECK(a.facing_dir == WM_MOVE_DOWN_RIGHT);
    CHECK(a.move_dir == 0);
}

/*
 * HRTSEQ1.ASM:460-524 ANI_SETFACING: an unconditional, live
 * FACING_DIR=NEW_FACING_DIR copy fired exactly once per marked frame
 * (torso_turn_setfacing in src/core/bret_backend.c), hand-traced against
 * hrt_4_to_8_turn2_anim's real 4-frame, 3-ticks-each body (HRTSEQ1.ASM:
 * 500-511): markers before frame index 1 and again before index 3.
 */
static void test_bret_backend_tick_promotes_facing_dir_via_setfacing(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    int guard;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    a.facing_dir = WM_MOVE_RIGHT;
    a.new_facing_dir = WM_MOVE_LEFT;
    wm_visual_start(&bva.torso_visual, &wm_bret_4_to_8_turn2_anim);

    for (guard = 0; guard < 10 && a.facing_dir != WM_MOVE_LEFT; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(a.facing_dir == WM_MOVE_LEFT);
    CHECK(bva.torso_visual.frame_index == 1);
    CHECK(!bva.torso_visual.ended);

    /* ANI_SETFACING reads whatever NEW_FACING_DIR is live at the instant it
       fires, not a value captured when the turn started -- change it before
       the second marker and confirm the second firing picks up the change. */
    a.new_facing_dir = WM_MOVE_DOWN;
    for (guard = 0; guard < 10 && a.facing_dir != WM_MOVE_DOWN; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(a.facing_dir == WM_MOVE_DOWN);
    CHECK(bva.torso_visual.frame_index == 3);

    /* Frame index 3 is the sequence's last frame -- it still has to play
       out its own 3-tick duration before the one-shot, non-repeat sequence
       actually ends. */
    for (guard = 0; guard < 10 && !bva.torso_visual.ended; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(bva.torso_visual.ended);
    CHECK(a.facing_dir == WM_MOVE_DOWN); /* no further/spurious promotion */
}

/*
 * HRTSEQ1.ASM:390-454 ANI_XFLIP (leg_turn_xflip in src/core/bret_backend.c):
 * toggles OBJ_CONTROL's M_FLIPH bit, hand-traced against
 * hrt_4_to_8_turn_anim's real 4-frame, 3-ticks-each body (HRTSEQ1.ASM:
 * 432-442) -- the marker falls right before frame index 3.
 */
static void test_bret_backend_tick_toggles_flip_via_xflip(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    int guard;
    uint16_t start_flip, after_first;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    start_flip = (uint16_t)(a.obj_control & WM_OBJ_FLIPH);
    wm_visual_start(&bva.visual, &wm_bret_4_to_8_turn_anim);

    for (guard = 0; guard < 15 && (a.obj_control & WM_OBJ_FLIPH) == start_flip; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK((a.obj_control & WM_OBJ_FLIPH) != start_flip);
    CHECK(bva.visual.frame_index == 3);
    CHECK(!bva.visual.ended);

    /* One shot: further ticks (through frame 3's remaining duration and
       past the sequence naturally ending) don't toggle it again. */
    after_first = (uint16_t)(a.obj_control & WM_OBJ_FLIPH);
    for (guard = 0; guard < 10 && !bva.visual.ended; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(bva.visual.ended);
    CHECK((uint16_t)(a.obj_control & WM_OBJ_FLIPH) == after_first);
}

/* hrt_2_to_4_turn_anim/hrt_4_to_2_turn_anim are the real exception: no
   ANI_XFLIP at all (adjacent-quadrant turns never cross the sprite's own
   left/right mirror line) -- confirm OBJ_CONTROL never toggles across a
   full play-through. */
static void test_bret_backend_tick_no_xflip_for_adjacent_quadrant_turn(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;
    int guard;
    uint16_t start_flip;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    start_flip = (uint16_t)(a.obj_control & WM_OBJ_FLIPH);
    wm_visual_start(&bva.visual, &wm_bret_2_to_4_turn_anim);

    for (guard = 0; guard < 10 && !bva.visual.ended; ++guard)
        wm_bret_backend_tick(&bva, &a, 0);
    CHECK(bva.visual.ended);
    CHECK((uint16_t)(a.obj_control & WM_OBJ_FLIPH) == start_flip);
}

/*
 * WRESTLE.ASM::execute_walk's own top-of-function INTURN freeze
 * (WRESTLE.ASM:5222-5252): "if our INTURN bit is set ... treat it like
 * UNINT" -- movement and reselection are held while a turn (leg or torso)
 * is still playing. Not cosmetic: without it, set_rotate_anim's instant
 * FACING_DIR=NEW_FACING_DIR copy would truncate every turn to one tick.
 */
static void test_bret_backend_execute_walk_freezes_while_leg_inturn(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    wm_visual_start(&bva.visual, &wm_bret_2_to_4_turn_anim); /* mid leg turn */
    a.move_dir = WM_MOVE_RIGHT; /* player presses a direction mid-turn */
    a.x_vel = 12345;            /* residual velocity from before the turn */
    wm_bret_backend_execute_walk(&a, &bva);

    /* WRESTLE.ASM's #inturn branch: MOVE_DIR!=0 -> jrnz #rets, literally
       does nothing -- move_dir and any stale velocity are left untouched,
       and no reselection happens. */
    CHECK(a.move_dir == WM_MOVE_RIGHT);
    CHECK(a.x_vel == 12345);
    CHECK(bva.visual.sequence == &wm_bret_2_to_4_turn_anim);

    /* Stick released mid-turn: velocities are cleared, still no reselect. */
    wm_bret_backend_init(&bva);
    memset(&a, 0, sizeof(a));
    wm_visual_start(&bva.visual, &wm_bret_2_to_4_turn_anim);
    a.move_dir = 0;
    a.x_vel = 12345;
    a.z_vel = 6789;
    wm_bret_backend_execute_walk(&a, &bva);
    CHECK(a.x_vel == 0);
    CHECK(a.z_vel == 0);
    CHECK(bva.visual.sequence == &wm_bret_2_to_4_turn_anim); /* unchanged */
}

static void test_bret_backend_execute_walk_freezes_while_torso_inturn(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    wm_visual_start(&bva.torso_visual, &wm_bret_4_to_8_turn2_anim);
    a.move_dir = WM_MOVE_RIGHT;
    a.facing_dir = WM_MOVE_RIGHT;
    a.new_facing_dir = WM_MOVE_LEFT;
    wm_bret_backend_execute_walk(&a, &bva);

    CHECK(a.move_dir == WM_MOVE_RIGHT);          /* untouched by wm_execute_walk */
    CHECK(bva.torso_visual.sequence == &wm_bret_4_to_8_turn2_anim); /* not reselected */
    CHECK(bva.visual.sequence == NULL);          /* everything froze, not just the torso */
}

/* Once a turn anim naturally ends (ended==true), the freeze lifts and
   ordinary dispatch resumes on the very next call. */
static void test_bret_backend_execute_walk_resumes_after_leg_turn_ends(void) {
    wm_arcade_actor_t a;
    wm_bret_backend_actor bva;

    memset(&a, 0, sizeof(a));
    wm_bret_backend_init(&bva);
    wm_visual_start(&bva.visual, &wm_bret_2_to_4_turn_anim);
    bva.visual.ended = true; /* the turn already finished naturally */
    a.move_dir = WM_MOVE_RIGHT;
    a.facing_dir = WM_MOVE_RIGHT;
    a.new_facing_dir = WM_MOVE_RIGHT;
    wm_bret_backend_execute_walk(&a, &bva);

    CHECK(bva.visual.sequence == &wm_bret_walk2_f2_anim); /* normal dispatch resumed */
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

    /* #zip clears MOVE_DIR and both velocities, and -- WRESTLE.ASM:5286
       `callr set_rotate_anim ;or stance` -- catches FACING_DIR up to
       NEW_FACING_DIR (WRESTLE.ASM:5082-5083), regardless of what FACING_DIR
       was before. */
    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_ZIP;
    a.x_vel = 12345;
    a.z_vel = 6789;
    a.facing_dir = WM_MOVE_LEFT;
    a.new_facing_dir = WM_MOVE_UP_RIGHT;
    wm_execute_walk(&a, NULL, wm_bret_velocity_table);
    CHECK(a.move_dir == 0);
    CHECK(a.x_vel == 0);
    CHECK(a.z_vel == 0);
    CHECK(a.facing_dir == WM_MOVE_UP_RIGHT);

    /* A walk_table index that aliases #zip (WRESTLE.ASM:5262 etc, e.g. 11)
       takes the same path, including the FACING_DIR catch-up. */
    memset(&a, 0, sizeof(a));
    a.move_dir = 11;
    a.facing_dir = WM_MOVE_LEFT;
    a.new_facing_dir = WM_MOVE_DOWN;
    wm_execute_walk(&a, NULL, wm_bret_velocity_table);
    CHECK(a.move_dir == 0);
    CHECK(a.facing_dir == WM_MOVE_DOWN);
}

/* WRESTLE.ASM #down_right/#down_left (WRESTLE.ASM:5339-5384): redirects to
   the pure right/left handler when actor->can_move_dir's real MOVE_DOWN
   bit (wm/arcade/wm_arcade_confine.h) blocks downward movement. */
static void test_execute_walk_can_move_dir_redirect(void) {
    wm_arcade_actor_t a;

    /* MOVE_DOWN blocked: down_right becomes plain right. */
    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_DOWN_RIGHT;
    a.can_move_dir = WM_MOVE_DOWN;
    wm_execute_walk(&a, NULL, wm_bret_velocity_table);
    CHECK(a.move_dir == WM_MOVE_RIGHT);
    CHECK(a.x_vel == WM_BRET_WALK_VEL);
    CHECK(a.z_vel == 0);
    CHECK(!(a.obj_control & WM_OBJ_FLIPH));

    /* MOVE_DOWN blocked: down_left becomes plain left. */
    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_DOWN_LEFT;
    a.can_move_dir = WM_MOVE_DOWN;
    wm_execute_walk(&a, NULL, wm_bret_velocity_table);
    CHECK(a.move_dir == WM_MOVE_LEFT);
    CHECK(a.x_vel == -WM_BRET_WALK_VEL);
    CHECK(a.z_vel == 0);
    CHECK(a.obj_control & WM_OBJ_FLIPH);

    /* MOVE_DOWN not blocked: takes the real diagonal, unaffected by other
       can_move_dir bits (only MOVE_DOWN triggers this specific redirect). */
    memset(&a, 0, sizeof(a));
    a.move_dir = WM_MOVE_DOWN_RIGHT;
    a.can_move_dir = WM_MOVE_LEFT;
    wm_execute_walk(&a, NULL, wm_bret_velocity_table);
    CHECK(a.move_dir == WM_MOVE_DOWN_RIGHT);
    CHECK(a.x_vel == WM_BRET_WALK_DVEL);
    CHECK(a.z_vel == WM_BRET_WALK_DVEL);
}

/*
 * WRESTLE.ASM:3018 SUBRP update_newfacing, wired as wm_arcade_update_newfacing
 * (wm/arcade/wm_arcade_closest.h): NEW_FACING_DIR = toward the opponent on
 * both axes independently, using the same CMP/JRGT "strictly greater than"
 * reading verified elsewhere in this port.
 */
static void test_arcade_update_newfacing(void) {
    wm_arcade_actor_t a, o;

    memset(&a, 0, sizeof(a));
    memset(&o, 0, sizeof(o));
    o.x_int = 50; o.z_int = 50; /* opponent down-right of self */
    wm_arcade_update_newfacing(&a, &o);
    CHECK(a.new_facing_dir == WM_MOVE_DOWN_RIGHT);

    memset(&a, 0, sizeof(a));
    a.x_int = 50; a.z_int = 50;
    memset(&o, 0, sizeof(o)); /* opponent up-left of self */
    wm_arcade_update_newfacing(&a, &o);
    CHECK(a.new_facing_dir == WM_MOVE_UP_LEFT);

    /* Equal coordinates: the source's jrgt only takes the branch on a
       strict >, so equal falls through to LEFT/UP, not RIGHT/DOWN. */
    memset(&a, 0, sizeof(a));
    memset(&o, 0, sizeof(o));
    wm_arcade_update_newfacing(&a, &o);
    CHECK(a.new_facing_dir == WM_MOVE_UP_LEFT);

    /* NULL actor/opponent: no-op, doesn't crash. */
    wm_arcade_update_newfacing(NULL, &o);
    wm_arcade_update_newfacing(&a, NULL);
}

/*
 * WRESTLE.ASM:5814 calc_line_x, translated as wm_ring_calc_line_x (real
 * direct interpolation instead of the source's cached per-Z table -- see
 * that function's own comment in wm/arcade/wmania_ring_geometry.h). Values
 * hand-derived from the closed form (top_x -/+ (i+1)*delta,
 * delta=(width<<16)/(depth+1) truncating) and cross-checked independently
 * in Python before being hardcoded here.
 */
static void test_ring_calc_line_x(void) {
    const WmRingBoundarySeed *left = wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_ROPE);
    const WmRingBoundarySeed *right = wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE);

    /* Left rope: top_x(856) > bottom_x(805), so values step DOWN as Z
       increases -- index 0 (z==top_z) is already one delta step below
       top_x itself, a genuine source quirk (see the function's own
       comment), not top_x exactly. */
    CHECK(wm_ring_calc_line_x(left, WM_RING_TOP) == 855);
    CHECK(wm_ring_calc_line_x(left, WM_RING_BOT) == 805);
    CHECK(wm_ring_calc_line_x(left, 1184) == 830);

    /* Right rope: top_x(1297) < bottom_x(1348), values step UP. */
    CHECK(wm_ring_calc_line_x(right, WM_RING_TOP) == 1297);
    CHECK(wm_ring_calc_line_x(right, WM_RING_BOT) == 1347);
    CHECK(wm_ring_calc_line_x(right, 1184) == 1322);

    /* Out of [top_z, bottom_z]: 0, exactly matching calc_line_x's own
       out-of-range return. */
    CHECK(wm_ring_calc_line_x(left, WM_RING_TOP - 1) == 0);
    CHECK(wm_ring_calc_line_x(left, WM_RING_BOT + 1) == 0);

    CHECK(wm_ring_calc_line_x(NULL, 1184) == 0);
}

/* WRESTLE.ASM:3074 confine_wrestler, in-ring branch (wm/arcade/
   wm_arcade_confine.h) -- well inside the ring: no clamp, CAN_MOVE_DIR
   stays 0. */
static void test_arcade_confine_wrestler_inside_ring_no_clamp(void) {
    wm_arcade_actor_t a;
    memset(&a, 0, sizeof(a));
    a.x_int = WM_RING_X_CENTER; a.x_fixed = WM_RING_X_CENTER << 16;
    a.z_int = 1184; a.z_fixed = 1184 << 16;
    a.hurt_box.x1 = a.x_int - 30;
    a.hurt_box.x2 = a.x_int + 30;

    wm_arcade_confine_wrestler(&a);

    CHECK(a.can_move_dir == 0);
    CHECK(a.x_int == WM_RING_X_CENTER);
    CHECK(a.z_int == 1184);
}

/* Z bounds: WRESTLE.ASM:3091-3129. */
static void test_arcade_confine_wrestler_clamps_z(void) {
    wm_arcade_actor_t a;

    memset(&a, 0, sizeof(a));
    a.x_int = WM_RING_X_CENTER; a.x_fixed = WM_RING_X_CENTER << 16;
    a.hurt_box.x1 = a.x_int - 30;
    a.hurt_box.x2 = a.x_int + 30;
    a.z_int = WM_RING_TOP - 50; a.z_fixed = a.z_int << 16;
    wm_arcade_confine_wrestler(&a);
    CHECK(a.can_move_dir == WM_MOVE_UP);
    CHECK(a.z_int == WM_RING_TOP);
    CHECK(a.z_fixed == (int32_t)WM_RING_TOP << 16);

    memset(&a, 0, sizeof(a));
    a.x_int = WM_RING_X_CENTER; a.x_fixed = WM_RING_X_CENTER << 16;
    a.hurt_box.x1 = a.x_int - 30;
    a.hurt_box.x2 = a.x_int + 30;
    a.z_int = WM_RING_BOT + 50; a.z_fixed = a.z_int << 16;
    wm_arcade_confine_wrestler(&a);
    CHECK(a.can_move_dir == WM_MOVE_DOWN);
    CHECK(a.z_int == WM_RING_BOT);

    /* Sitting exactly on the boundary: bit set, but no clamp needed. */
    memset(&a, 0, sizeof(a));
    a.x_int = WM_RING_X_CENTER; a.x_fixed = WM_RING_X_CENTER << 16;
    a.hurt_box.x1 = a.x_int - 30;
    a.hurt_box.x2 = a.x_int + 30;
    a.z_int = WM_RING_TOP; a.z_fixed = WM_RING_TOP << 16;
    wm_arcade_confine_wrestler(&a);
    CHECK(a.can_move_dir == WM_MOVE_UP);
    CHECK(a.z_int == WM_RING_TOP);
}

/* X bounds: WRESTLE.ASM:3132-3421, using hurt_box.x1/x2 exactly where the
   source reads OBJ_COLLX1/OBJ_COLLX2. */
static void test_arcade_confine_wrestler_clamps_x(void) {
    wm_arcade_actor_t a;
    int32_t left_rope, right_rope;

    left_rope = wm_ring_calc_line_x(wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_ROPE), 1184);
    right_rope = wm_ring_calc_line_x(wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE), 1184);

    /* Past the left rope: pushed back so hurt_box.x1 lands exactly on it
       (hurt_box.x1 == x_int here, offset 0, so x_int itself ends up
       exactly at left_rope). */
    memset(&a, 0, sizeof(a));
    a.z_int = 1184; a.z_fixed = 1184 << 16;
    a.x_int = left_rope - 20; a.x_fixed = a.x_int << 16;
    a.hurt_box.x1 = a.x_int; a.hurt_box.x2 = a.x_int + 60;
    wm_arcade_confine_wrestler(&a);
    CHECK(a.can_move_dir == WM_MOVE_LEFT);
    CHECK(a.x_int == left_rope);
    CHECK(a.x_fixed == (int32_t)(left_rope << 16));

    /* Past the right rope: pushed back so hurt_box.x2 lands exactly on it
       (hurt_box.x2 == x_int here, offset 0, so x_int ends up at
       right_rope). */
    memset(&a, 0, sizeof(a));
    a.z_int = 1184; a.z_fixed = 1184 << 16;
    a.x_int = right_rope + 20; a.x_fixed = a.x_int << 16;
    a.hurt_box.x1 = a.x_int - 60; a.hurt_box.x2 = a.x_int;
    wm_arcade_confine_wrestler(&a);
    CHECK(a.can_move_dir == WM_MOVE_RIGHT);
    CHECK(a.x_int == right_rope);
}

/* WRESTLE.ASM:3078-3084 #no_confine: MODE_NOCONFINE and PLYRMODE==ATTACHED
   both skip straight to CAN_MOVE_DIR=0, no clamp, even from way outside
   the ropes. */
static void test_arcade_confine_wrestler_noconfine_and_attached_skip(void) {
    wm_arcade_actor_t a;

    memset(&a, 0, sizeof(a));
    a.anim_mode = WM_MODE_NOCONFINE;
    a.x_int = 0; a.x_fixed = 0;
    a.z_int = 0; a.z_fixed = 0;
    wm_arcade_confine_wrestler(&a);
    CHECK(a.can_move_dir == 0);
    CHECK(a.x_int == 0);
    CHECK(a.z_int == 0);

    memset(&a, 0, sizeof(a));
    a.player_mode = WM_PMODE_ATTACHED;
    a.x_int = 0; a.x_fixed = 0;
    a.z_int = 0; a.z_fixed = 0;
    wm_arcade_confine_wrestler(&a);
    CHECK(a.can_move_dir == 0);
    CHECK(a.x_int == 0);
    CHECK(a.z_int == 0);

    /* NULL: no-op, doesn't crash. */
    wm_arcade_confine_wrestler(NULL);
}

static void test_arcade_mode_dead(void) {
    wm_arcade_actor_t a;

    /* First dead tick: real routine's own royal_rumble/is_8_on_1/
       CHECK_COMBO_GO checks all provably resolve to "#nobuck" in this
       port (see wm_arcade_mode_dead.h) -- sets WM_STATUS_NO_BUCKOFF. */
    memset(&a, 0, sizeof(a));
    wm_arcade_mode_dead(&a);
    CHECK((a.status_flags & WM_STATUS_NO_BUCKOFF) != 0);
    CHECK((a.status_flags & WM_STATUS_ZOMBIE) == 0);

    /* Every following tick: real top-of-function B_NO_BUCKOFF early-out,
       a genuine no-op. */
    a.status_flags = WM_STATUS_NO_BUCKOFF;
    wm_arcade_mode_dead(&a);
    CHECK(a.status_flags == WM_STATUS_NO_BUCKOFF);

    /* Same real early-out for WM_STATUS_DID_BUCKOFF/DO_BUCKOFF. */
    a.status_flags = WM_STATUS_DID_BUCKOFF;
    wm_arcade_mode_dead(&a);
    CHECK(a.status_flags == WM_STATUS_DID_BUCKOFF);

    a.status_flags = WM_STATUS_DO_BUCKOFF;
    wm_arcade_mode_dead(&a);
    CHECK(a.status_flags == WM_STATUS_DO_BUCKOFF);

    /* WM_STATUS_ZOMBIE: real #zmb early-out, also untouched (the
       zombie-transform tail is out of this port's scope, see header). */
    a.status_flags = WM_STATUS_ZOMBIE;
    wm_arcade_mode_dead(&a);
    CHECK(a.status_flags == WM_STATUS_ZOMBIE);

    /* NULL: no-op, doesn't crash. */
    wm_arcade_mode_dead(NULL);
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

    /* Same real #teamX_starts placement as wm_match_start_attract -- see
       that test's own comment for the derivation. */
    CHECK(m.actors[0].x_int == 1074 - 85);
    CHECK(m.actors[0].facing_dir == WM_MOVE_UP_RIGHT);
    CHECK(m.actors[1].x_int == 1074 + 85);
    CHECK(m.actors[1].facing_dir == WM_MOVE_DOWN_LEFT);

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
    int32_t opp_start_x;

    wm_rng_init(&rng, 0x42u, NULL, NULL, NULL);
    wm_match_init(&m);
    wm_match_start_selected(&m, &rng, WM_ROSTER_BRET);
    int32_t start_x = m.actors[0].x_fixed;

    /* Keep the opponent far to the right for the whole 30-tick walk (a few
       WM_BRET_WALK_VEL steps in 16.16 fixed move x_int by nothing close to
       this), so NEW_FACING_DIR keeps pointing right throughout and never
       drifts into the torso-turn/FACING_DIR-lag scenario
       test_bret_backend_execute_walk_facing_lags_new_facing_while_moving
       exercises directly -- this test is about ordinary straight-line
       walking, not that. */
    opp_start_x = 10000;
    m.actors[1].x_int = opp_start_x;
    m.actors[1].x_fixed = opp_start_x << 16;

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
    /* facing_dir stays WM_MOVE_UP_RIGHT (P1's real match-start facing, see
       wm_match_start_selected) the whole walk: the opponent is to the
       right on both ticks that matter (before the walk and throughout),
       so NEW_FACING_DIR never disagrees with it and hrt_leg_anims_table
       never needs to reselect off of RIGHT/UP_RIGHT. */
    CHECK(m.actors[0].facing_dir == WM_MOVE_UP_RIGHT);
    /* hrt_leg_anims_table[RIGHT][UP_RIGHT] -- see test_bret_leg_anim_table. */
    CHECK(m.bret_visual[0].visual.sequence == &wm_bret_walk2_f2_anim);

    /* The CPU opponent (a non-Bret wrestler unless the placeholder draw
       happened to also land on Bret) never receives human input and keeps
       stepping its drone core as before. */
    if (m.actors[1].wrestler_num != WM_ROSTER_BRET)
        CHECK(m.actors[1].x_fixed == opp_start_x << 16);
}

/*
 * End-to-end: wm_arcade_confine_wrestler (wm/arcade/wm_arcade_confine.h),
 * wired into wm_match_tick, actually stops a human-controlled Bret actor
 * from walking through the ring rope -- through ordinary human-input play,
 * not a direct/synthetic wm_arcade_confine_wrestler call.
 */
static void test_match_human_bret_stopped_by_right_rope(void) {
    WmRng rng;
    wm_match_state m;
    wm_arcade_drone_callbacks_t cb;
    wm_input_state right;
    int32_t right_rope;
    int guard;

    wm_rng_init(&rng, 0x42u, NULL, NULL, NULL);
    wm_match_init(&m);
    wm_match_start_selected(&m, &rng, WM_ROSTER_BRET);

    /* Real starting Z (WM_MATCH_P1_START_Z, see wm/match.h's
       place_wrestler) never changes in this test (pure X walking), so the
       right rope's X at that Z is a fixed target throughout. */
    right_rope = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE), m.actors[0].z_int);

    /* Start close to the right rope. */
    m.actors[0].x_int = right_rope - 15;
    m.actors[0].x_fixed = m.actors[0].x_int << 16;

    memset(&cb, 0, sizeof(cb));
    cb.rndrng0_upto = test_match_rndrng0_cb;
    cb.user = &rng;

    memset(&right, 0, sizeof(right));
    right.stick_x = 100;
    for (guard = 0; guard < 60 && !(m.actors[0].can_move_dir & WM_MOVE_RIGHT); ++guard)
        wm_match_tick(&m, &cb, &right);

    CHECK(m.actors[0].can_move_dir & WM_MOVE_RIGHT);

    /* Held right for many more ticks: WM_BRET_WALK_VEL (~3.6 px/tick in
       16.16) would carry x_int roughly 250+ px past the rope over 60 more
       ticks with no confinement at all. The real per-tick correction only
       ever re-anchors the *current* frame's hurt_box exactly on the rope,
       and the walk cycle's own hurt_box width varies frame to frame, so a
       wider next frame can still poke a few pixels past before the very
       next tick corrects it again -- a real, faithful "held at the
       boundary, not a hard wall" wobble, not unconfined drift. */
    for (guard = 0; guard < 60; ++guard) {
        wm_match_tick(&m, &cb, &right);
        CHECK(m.actors[0].hurt_box.x2 < right_rope + 20);
    }
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

    /* Opponent stands 200 units away on X (same Z as P1) -- outside
       do_punch's near(a,50,45) headbutt range, but still a plausible
       sparring distance, not a contrived edge case. Relative to P1's own
       real match-start position (WRESTLE.ASM:2691-2755's #teamX_starts,
       wm/match.h's place_wrestler), not the origin. */
    m.actors[1].x_int = m.actors[0].x_int + 200;
    m.actors[1].x_fixed = m.actors[1].x_int << 16;
    m.actors[1].z_int = m.actors[0].z_int;
    m.actors[1].z_fixed = m.actors[0].z_fixed;

    memset(&cb, 0, sizeof(cb));
    cb.rndrng0_upto = test_match_rndrng0_cb;
    cb.user = &rng;

    memset(&punch, 0, sizeof(punch));
    punch.light_punch = true;
    wm_match_tick(&m, &cb, &punch);

    CHECK(m.actors[0].closest_xdist == 200);
    /* face_is_2 (wm_arcade_bret.c) picks PUNCH2 because FACING_DIR&MOVE_UP
       is set: P1 starts facing WM_MOVE_UP_RIGHT (WRESTLE.ASM:2753-2755/2784,
       wm_match_start_selected), a real value now, not a zeroed default. */
    CHECK(m.bret_visual[0].current_id == WM_BRET_ANIM_PUNCH2);
    CHECK(m.bret_visual[0].visual.sequence == &wm_bret_light_punch2_anim);
    /* wm_arcade_update_newfacing, wired into wm_match_tick for every actor
       (wm/arcade/wm_arcade_closest.h): the opponent is to the right, so
       actor 0's NEW_FACING_DIR picks up WM_MOVE_RIGHT. */
    CHECK(m.actors[0].new_facing_dir & WM_MOVE_RIGHT);

    /* Let the already-started punch animation play out to its real,
       hand-traced ATTACK_ON frame through ordinary wm_match_tick calls. */
    memset(&punch, 0, sizeof(punch));
    for (guard = 0; guard < 10 && !(m.actors[0].anim_mode & WM_MODE_CHECKHIT); ++guard)
        wm_match_tick(&m, &cb, &punch);

    CHECK(m.actors[0].anim_mode & WM_MODE_CHECKHIT);
    CHECK(m.actors[0].attack_mode == WM_AMODE_PUNCH);
}

/*
 * wm_arcade_update_newfacing (WRESTLE.ASM:3018 update_newfacing) is wired
 * into wm_match_tick unconditionally, for every actor, mirroring WRESTLE.
 * ASM's own per-process main loop (WRESTLE.ASM:2418) which runs it before
 * the human/drone branch and regardless of movement -- confirm both actors
 * of a fixed 1-on-1 match get real, opposite NEW_FACING_DIR values purely
 * from ordinary wm_match_tick play, no direct calc_closest/bret_backend
 * calls.
 */
static void test_match_tick_updates_newfacing_for_both_actors(void) {
    WmRng rng;
    wm_match_state m;
    wm_arcade_drone_callbacks_t cb;
    wm_input_state no_input;

    wm_rng_init(&rng, 0x1234u, NULL, NULL, NULL);
    wm_match_init(&m);
    wm_match_start_selected(&m, &rng, WM_ROSTER_BRET);

    /* Both _int and _fixed need setting: wm_bret_backend_tick_position's
       wm_integrate_position resyncs x_int/z_int from x_fixed/z_fixed every
       tick (actor 0 is a Bret actor, ticked within this same call), so a
       stale _fixed would silently overwrite the _int override below before
       actor 1's own wm_arcade_update_newfacing call reads it.

       Positions are chosen well inside the real ring (RING_TOP=1023,
       RING_BOT=1345, ropes around x=835-1317 at these Z values) so
       wm_arcade_confine_wrestler -- now real, and run on actor 0 (always
       Bret) before actor 1's own wm_arcade_update_newfacing call reads
       actor 0's position -- has nothing to correct. Out-of-ring coordinates
       like the origin would get actor 0 pulled back into the ring by that
       same real confinement before this tick finishes, changing what actor
       1 sees; that's correct behavior, just not what this test is about. */
    m.actors[0].x_int = 1074;  m.actors[0].x_fixed = 1074 << 16;
    m.actors[0].z_int = 1150;  m.actors[0].z_fixed = 1150 << 16;
    m.actors[1].x_int = 1174;  m.actors[1].x_fixed = 1174 << 16;
    m.actors[1].z_int = 1100;  m.actors[1].z_fixed = 1100 << 16; /* opponent up-right */

    memset(&cb, 0, sizeof(cb));
    cb.rndrng0_upto = test_match_rndrng0_cb;
    cb.user = &rng;
    memset(&no_input, 0, sizeof(no_input));

    wm_match_tick(&m, &cb, &no_input);

    CHECK(m.actors[0].new_facing_dir == WM_MOVE_UP_RIGHT);
    CHECK(m.actors[1].new_facing_dir == WM_MOVE_DOWN_LEFT);
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
    test_joystat_update();
    test_joystat_matches();
    test_arcade_update_joy_dtime();
    test_bret_backend_change_anim();
    test_bret_backend_change_anim_sets_facing_on_attack_start();
    test_bret_backend_i_will_die_resolves_through_move_bret();
    test_bret_backend_secret_move_fires_through_real_input();
    test_bret_backend_secret_move_no_frame_data_survives_one_tick();
    test_bret_backend_charge_flying_kick_fires_on_release();
    test_bret_backend_charge_face_rake_fires_on_release();
    test_bret_backend_charge_ddt_fires_on_release();
    test_bret_attack_window_punch2();
    test_bret_attack_windows_remaining();
    test_bret_attack_windows_batch1();
    test_bret_attack_windows_multi_pulse();
    test_bret_attack_windows_batch3();
    test_bret_midanim_setplyrmode();
    test_bret_attack_windows_batch4();
    test_bret_rptcount_loop();
    test_bret_attack_windows_batch6();
    test_bret_anim_transition_chain();
    test_bret_frame_motion_commands();
    test_bret_ifbuttons_run_cancel();
    test_bret_instant_state_commands();
    test_anim_program_interpreter();
    test_attract_dcs_logo_does_not_fall_through();
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
    test_arcade_adjust_health_death_fallback_anim_and_knockback();
    test_arcade_adjust_health_death_anim_skip_cases();
    test_arcade_adjust_health_death_unmatched_mode_zeroes_vels();
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
    test_bret_rotate_anim_table();
    test_bret_torso_alias_frames_match_source();
    test_bret_backend_execute_walk_selects_leg_anim();
    test_bret_backend_execute_walk_facing_lags_new_facing_while_moving();
    test_bret_backend_execute_walk_zip_selects_rotate_anim();
    test_bret_backend_tick_promotes_facing_dir_via_setfacing();
    test_bret_backend_tick_toggles_flip_via_xflip();
    test_bret_backend_tick_no_xflip_for_adjacent_quadrant_turn();
    test_bret_backend_execute_walk_freezes_while_leg_inturn();
    test_bret_backend_execute_walk_freezes_while_torso_inturn();
    test_bret_backend_execute_walk_resumes_after_leg_turn_ends();
    test_convert_facing();
    test_set_velocities_normal();
    test_set_velocities_backward_reduction();
    test_set_velocities_ground_boost();
    test_execute_walk_flip_and_zip();
    test_execute_walk_can_move_dir_redirect();
    test_arcade_update_newfacing();
    test_ring_calc_line_x();
    test_arcade_confine_wrestler_inside_ring_no_clamp();
    test_arcade_confine_wrestler_clamps_z();
    test_arcade_confine_wrestler_clamps_x();
    test_arcade_confine_wrestler_noconfine_and_attached_skip();
    test_arcade_mode_dead();
    test_integrate_position();
    test_human_input_commit();
    test_match_start_selected();
    test_match_human_bret_walks_right();
    test_match_human_bret_stopped_by_right_rope();
    test_match_human_punch_from_range_selects_real_punch();
    test_match_tick_updates_newfacing_for_both_actors();
    test_match_round_decided_after_real_kill();
    test_match_bret_idle_animates();
    test_source_attract_sequence();
    test_attract_source_flow();
    test_video_frame_source_clock_and_input_latch();
    puts("all core tests passed");
    return 0;
}
