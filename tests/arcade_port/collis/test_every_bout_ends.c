/*
 * Every bout ends -- not only the attract mode's.
 *
 * The stuck-state probe that test_quiet_bout.c and test_drone_running.c
 * came out of was only ever run on attract bouts: one drone against one
 * drone. Run on the one-player ladder (two and three opponents) and on
 * buddy mode (four drones), it found a match that could not end and six
 * wrestlers who could not move on, none of which an attract bout reaches:
 *
 *   - A round decided by the announcer -- the raise-arm animations'
 *     ANI_CODE,win_announce, which is how most live rounds end -- never
 *     started DO_WAIT when it was the deciding round. 15 of 20 bouts
 *     reached a winner and stood in the ring for good.
 *   - #dobuck's result was re-applied on every tick after a buckoff, so
 *     the convulse restarted from frame one for the rest of the match.
 *   - Bret's backend dropped an ANI_CHANGEANIM to a label with no typed
 *     id -- xxx_dead_anim, where a beaten wrestler goes to die -- and he
 *     lay ONGROUND instead, for 5644 ticks.
 *   - set_images' #no_2nd_piece was missing, so a torso turn kept playing
 *     under one-piece run frames and turned a running Bret round.
 *   - LOSE_BALANCE had no table, so a pushed man slid 25000 pixels.
 *   - move_wrestler ran BEFORE the wrestler was confined, the reverse of
 *     the source's own loop (WRESTLE.ASM:2457-2468), and a drone running
 *     along the ring's bottom edge never bounced: he ran on the spot
 *     against the left rope for 1800 ticks.
 *
 * Fixing the last exposed one more: only Bret ran his *_ani_init, so the
 * other seven had no animation and an empty hurt box until their own
 * dispatcher picked one -- harmless only while the dispatcher ran first.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/match.h"
#include "wm/anim_program.h"
#include "wm/wrestler_backend.h"
#include "wm/bret_backend.h"
#include "wm_arcade_roster.h"
#include "wm/arcade/wm_arcade_roster.h"
#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wm_arcade_drone_data.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wmania_ring_geometry.h"

static uint32_t g_tick, g_seed;
static uint32_t hc(void *u) { (void)u; return g_tick * 7u + 13u + g_seed * 101u; }
static uint32_t spf(void *u) { (void)u; return 0x1234u + g_tick + g_seed * 7919u; }

static const int ROSTER[8] = { 0, 1, 2, 3, 4, 5, 6, 8 };

static wm_match_state M;
static WmRng R;

typedef struct {
    unsigned ticks;
    unsigned longest[WM_MATCH_MAX_ACTORS][32];
    unsigned max_x_off;
    unsigned fix2_violations;
    unsigned against_ropes;
} bout_stats;

/*
 * kind 0: one-player ladder, the human standing idle.
 * kind 1: the same with the human handed to the drone AI.
 * kind 2: buddy mode, all four on the drone AI.
 */
static void bout(int kind, unsigned nopp, uint32_t seed, bout_stats *st)
{
    wm_arcade_drone_callbacks_t cb;
    unsigned run[WM_MATCH_MAX_ACTORS][32];
    unsigned t, i;

    memset(st, 0, sizeof *st);
    memset(run, 0, sizeof run);
    g_seed = seed;
    g_tick = 0;
    wm_rng_init(&R, seed, hc, spf, NULL);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    if (kind < 2) {
        uint8_t opps[3];
        for (i = 0; i < nopp; ++i)
            opps[i] = (uint8_t)ROSTER[(seed + 2 + i * 3) % 8];
        wm_match_start_one_player_team(&M, &R, 1, (uint8_t)ROSTER[seed % 8],
                                       0, opps, nopp);
        M.first_ladder = false;
        if (kind == 1) {
            M.actor_is_human[0] = false;
            M.actors[0].plyr_type = WM_PTYPE_DRONE;
        }
    } else {
        wm_match_start_two_player(&M, &R, 3, (uint8_t)ROSTER[seed % 8],
                                  (uint8_t)ROSTER[(seed + 3) % 8], false,
                                  128, 128);
        for (i = 0; i < M.actor_count; ++i) {
            M.actor_is_human[i] = false;
            M.actors[i].plyr_type = WM_PTYPE_DRONE;
        }
    }
    cb = wm_arcade_drone_data_callbacks(&R);

    for (t = 0; t < 20000 && M.active && !M.match_over; ++t) {
        g_tick = t;
        wm_match_tick(&M, &cb, NULL);
        for (i = 0; i < M.actor_count; ++i) {
            const wm_arcade_actor_t *a = &M.actors[i];
            unsigned k, pm = a->player_mode;
            int32_t off = a->x_int - WM_RING_X_CENTER;
            /* confine_wrestler_fix2 (WRESTLE.ASM:3736): whatever the first
               confine pass found him against, the tick still says -- for
               anyone the main loop's final_confine (:6163) leaves alone,
               which re-confines only a wrestler with an ATTACH_PROC and
               does it without fix2. */
            if (!a->attach_proc &&
                (a->can_move_dir & a->can_move_temp) != a->can_move_temp)
                ++st->fix2_violations;
            if (a->can_move_temp) ++st->against_ropes;
            unsigned u = (unsigned)(off < 0 ? -off : off);
            if (u > st->max_x_off) st->max_x_off = u;
            for (k = 0; k < 32; ++k) {
                if (k == pm) {
                    if (++run[i][k] > st->longest[i][k])
                        st->longest[i][k] = run[i][k];
                } else {
                    run[i][k] = 0;
                }
            }
        }
    }
    st->ticks = t;
}

static void test_every_ladder_and_buddy_bout_ends(void)
{
    unsigned worst_run = 0, worst_ground = 0, worst_off = 0, longest_bout = 0;
    unsigned against_ropes = 0;
    int kind;
    uint32_t seed;

    for (kind = 0; kind < 3; ++kind) {
        for (seed = 0; seed < 4; ++seed) {
            unsigned nopp;
            for (nopp = (kind < 2 ? 2u : 3u); nopp <= 3u; ++nopp) {
                bout_stats st;
                unsigned i;
                bout(kind, nopp, seed, &st);
                /* DO_WAIT ran to its end: match_over is 2. */
                if (M.match_over != 2)
                    printf("kind %d nopp %u seed %u did not end (%u ticks)\n",
                           kind, nopp, (unsigned)seed, st.ticks);
                assert(M.match_over == 2);
                if (st.ticks > longest_bout) longest_bout = st.ticks;
                if (st.max_x_off > worst_off) worst_off = st.max_x_off;
                assert(st.fix2_violations == 0);
                against_ropes += st.against_ropes;
                for (i = 0; i < M.actor_count; ++i) {
                    /* An idle human stands as long as he likes. */
                    if (kind == 0 && i == 0) continue;
                    if (st.longest[i][WM_PMODE_RUNNING] > worst_run)
                        worst_run = st.longest[i][WM_PMODE_RUNNING];
                    if (st.longest[i][WM_PMODE_ONGROUND] > worst_ground)
                        worst_ground = st.longest[i][WM_PMODE_ONGROUND];
                }
            }
        }
    }
    printf("20 bouts: longest %u ticks, RUNNING %u, ONGROUND %u, x off %u, "
           "%u actor-ticks against the ropes\n",
           longest_bout, worst_run, worst_ground, worst_off, against_ropes);
    assert(against_ropes > 1000);
    /* Measured 73, 263 and 578. Before: 1824 (the rope that never
       bounced), 5644 (the dropped get-up) and 24998 (the slide). */
    assert(worst_run < 400);
    assert(worst_ground < 1000);
    assert(worst_off < 800);
}

/*
 * ani_init (WRESTLE.ASM:2903, called from wrestler_main at :2373): every
 * wrestler stands in his own stance, with a hurt box, before the first
 * tick -- not only Bret.
 */
static void test_everyone_starts_in_his_own_stance(void)
{
    unsigned w;
    for (w = 0; w < 8; ++w) {
        unsigned i;
        wm_rng_init(&R, 7u, hc, spf, NULL);
        memset(&M, 0, sizeof M);
        wm_match_init(&M);
        wm_match_start_selected(&M, &R, (uint8_t)ROSTER[w]);
        for (i = 0; i < M.actor_count; ++i) {
            const wm_arcade_actor_t *a = &M.actors[i];
            assert(a->hurt_box.x2 > a->hurt_box.x1);
            if (a->wrestler_num != WM_ROSTER_BRET) {
                const wm_ani_init_row *row = &wm_ani_init_rows[a->wrestler_num];
                const char *want = (a->facing_dir & WM_MOVE_RIGHT)
                                       ? row->stand2 : row->stand4;
                assert(M.wrestler_visual[i].current_label != NULL);
                assert(strcmp(M.wrestler_visual[i].current_label, want) == 0);
            }
        }
    }
}

/*
 * reset_wrestle (WRESTLE.ASM:2676) ends `callr ani_init` (:2775): a new
 * round opens in the stance, not on whatever the last one ended on.
 */
static void test_a_new_round_opens_in_the_stance(void)
{
    wm_input_state in;
    const unsigned opp = 0;             /* the Undertaker, idle */
    wm_rng_init(&R, 7u, hc, spf, NULL);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_match_start_selected(&M, &R, (uint8_t)WM_ROSTER_TAKER);
    memset(&in, 0, sizeof in);
    assert(M.actors[opp].wrestler_num == WM_ROSTER_TAKER);

    /* He ended the last round lying down. */
    {
        wm_arcade_roster_callbacks_t rc =
            wm_wrestler_roster_callbacks(&M.wrestler_visual[opp]);
        rc.change_anim_restart(&M.actors[opp], "und_liedown_anim", rc.user);
    }
    /* ...with no torso channel running at all. The stance comes back on
       the reset tick whatever happens -- his own #zip picks it -- so the
       torso is what says ani_init ran: its change_anim2a is the only
       thing on that tick that starts one. */
    M.wrestler_visual[opp].torso_prog.program = NULL;
    M.wrestler_visual[opp].torso_label = NULL;
    /* And the auto-pin had made him a drone (WRESTLE.ASM:3926). */
    M.actors[opp].plyr_type = WM_PTYPE_DRONE;
    M.round_reset_pending = true;
    wm_match_tick(&M, NULL, &in);
    assert(M.wrestler_visual[opp].current_label != NULL);
    assert(strstr(M.wrestler_visual[opp].current_label, "liedown") == NULL);
    assert(M.wrestler_visual[opp].torso_label != NULL);
    assert(strncmp(M.wrestler_visual[opp].torso_label, "und_torso", 9) == 0);
    /* reset_wrestle's `calla drone_change_back`: PLYRNUM 0, a player
       again for the new round. */
    assert(M.actors[opp].player_num == 0);
    assert(M.actors[opp].plyr_type == WM_PTYPE_PLAYER);
}

/* #dobuck's report is consumed, not re-applied every tick. */
static void test_the_buckoff_report_is_applied_once(void)
{
    wm_input_state in;
    wm_mode_dead_result_t zero;
    wm_rng_init(&R, 7u, hc, spf, NULL);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_match_start_selected(&M, &R, (uint8_t)WM_ROSTER_TAKER);
    memset(&in, 0, sizeof in);
    memset(&zero, 0, sizeof zero);

    /* Both backends carry the report; actor 0 is the Undertaker on the
       shared one, and actor 1 whoever was drawn. */
    {
        unsigned i;
        for (i = 0; i < M.actor_count; ++i) {
            wm_mode_dead_result_t *r = M.actors[i].wrestler_num == WM_ROSTER_BRET
                ? &M.bret_visual[i].mode_dead_result
                : &M.wrestler_visual[i].mode_dead_result;
            r->bucked_off = true;
            r->convulse = true;
        }
        wm_match_tick(&M, NULL, &in);
        for (i = 0; i < M.actor_count; ++i) {
            const wm_mode_dead_result_t *r = M.actors[i].wrestler_num == WM_ROSTER_BRET
                ? &M.bret_visual[i].mode_dead_result
                : &M.wrestler_visual[i].mode_dead_result;
            assert(memcmp(r, &zero, sizeof zero) == 0);
        }
    }
}

/*
 * The order. A runner holding down along the ring's bottom edge is pushed
 * one Z step past RING_BOT by veladd every tick; confine pulls him back.
 * bounce_off_ropes, inside move_wrestler, must see him after confine --
 * calc_line_x answers 0 for a Z past the rope (WRESTLE.ASM:5820), so
 * before confine there is no rope to bounce off.
 */
static void test_a_runner_on_the_bottom_edge_bounces(void)
{
    wm_input_state in;
    wm_arcade_actor_t *a;
    unsigned t;
    int bounced = 0;

    wm_rng_init(&R, 7u, hc, spf, NULL);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_match_start_selected(&M, &R, (uint8_t)WM_ROSTER_BAM);
    a = &M.actors[0];
    assert(a->wrestler_num == WM_ROSTER_BAM && M.actor_is_human[0]);

    /* The other man well out of the way, up by the top rope. */
    M.actors[1].x_int = WM_RING_X_CENTER + 150;
    M.actors[1].z_int = WM_RING_TOP + 20;

    a->x_int = WM_RING_X_CENTER - 100;
    a->z_int = WM_RING_BOT;
    a->facing_dir = WM_MOVE_LEFT;
    a->move_dir = WM_MOVE_LEFT | WM_MOVE_DOWN;
    a->player_mode = WM_PMODE_RUNNING;
    {
        wm_arcade_roster_callbacks_t rc =
            wm_wrestler_roster_callbacks(&M.wrestler_visual[0]);
        rc.change_anim_restart(a, "bam_run_anim", rc.user);
    }
    memset(&in, 0, sizeof in);
    in.stick_x = -100;          /* left */
    in.stick_y = -100;          /* and down, into the edge */
    in.run = true;

    for (t = 0; t < 60 && !bounced; ++t) {
        wm_match_tick(&M, NULL, &in);
        if (a->player_mode == WM_PMODE_BOUNCING) bounced = 1;
        assert(a->z_int <= WM_RING_BOT);
    }
    assert(bounced);
}

/*
 * Bret's backend: an ANI_CHANGEANIM to a label with no typed id still
 * starts it. xxx_dead_anim is the one that mattered -- ANI_WAITROLL's
 * #die (ANIM.ASM:3135) hands a beaten man to it, and its
 * ANI_SETPLYRMODE,MODE_DEAD (WRESTLE2.ASM:3995) is what makes him dead.
 */
static void test_bret_becomes_a_label_with_no_id(void)
{
    wm_bret_backend_actor bva;
    wm_arcade_actor_t a;
    const wm_anim_program *dead = wm_anim_program_find("xxx_dead_anim");

    assert(dead != NULL);
    assert(wm_bret_anim_id_for_label("xxx_dead_anim") < 0);
    memset(&a, 0, sizeof a);
    a.facing_dir = WM_MOVE_RIGHT;
    a.player_mode = WM_PMODE_ONGROUND;
    wm_bret_backend_init(&bva);
    wm_anim_exec_start(&bva.prog, wm_anim_program_find("hrt_stand2_anim"),
                       &a, 0, &bva.anim_env);
    bva.prog.ended = true;
    bva.prog.become = "xxx_dead_anim";
    wm_bret_backend_tick(&bva, &a, 1);
    assert(bva.prog.program == dead);
    assert(a.player_mode == WM_PMODE_DEAD);
}

/*
 * #no_2nd_piece (ANIM.ASM:4818) on Bret's own tracks: his stand frames
 * are one piece and end the torso; change_anim2 restarts it.
 */
static void test_bret_one_piece_frames_end_his_torso(void)
{
    wm_bret_backend_actor bva;
    wm_arcade_actor_t a;
    wm_arcade_bret_callbacks_t cb;
    int i;

    memset(&a, 0, sizeof a);
    a.facing_dir = WM_MOVE_RIGHT;
    a.new_facing_dir = WM_MOVE_RIGHT;
    wm_bret_backend_init(&bva);
    cb = wm_bret_backend_callbacks(&bva);
    wm_arcade_bret_ani_init(&a, &cb);
    assert(!bva.torso_visual.ended);
    wm_bret_backend_tick(&bva, &a, 1);
    assert(bva.torso_visual.ended);

    /* Walking: the leg frames carry the torso, and change_walk_anim's
       torso half restarts it and it keeps running. */
    for (i = 0; i < 30; ++i) {
        a.stick_val_cur = WM_MOVE_RIGHT;
        a.move_dir = WM_MOVE_RIGHT;
        cb.execute_walk(&a, cb.user);
        wm_bret_backend_tick(&bva, &a, (uint16_t)(2 + i));
    }
    /* change_anim1 on the leg animation replaced the stance program
       (WRESTLE.ASM:5017) -- the walk is what is playing... */
    assert(bva.prog.program == NULL);
    assert(bva.visual.sequence != NULL &&
           strncmp(bva.visual.sequence->source_label, "hrt_walk", 8) == 0);
    /* ...and its frames carry the torso, which keeps running. */
    assert(!bva.torso_visual.ended);
}

int main(void)
{
    test_everyone_starts_in_his_own_stance();
    test_a_new_round_opens_in_the_stance();
    test_the_buckoff_report_is_applied_once();
    test_a_runner_on_the_bottom_edge_bounces();
    test_bret_becomes_a_label_with_no_id();
    test_bret_one_piece_frames_end_his_torso();
    test_every_ladder_and_buddy_bout_ends();
    printf("every bout ends\n");
    return 0;
}
