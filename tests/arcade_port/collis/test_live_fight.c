/*
 * A live match must actually turn into a fight.
 *
 * This is the end-to-end pin for ANIM.ASM:2497 _ani_end. That opcode
 * ORs MODE_END into the mode word, and BRET.ASM:2232 mode_bouncing --
 * with its eight counterparts in the other dispatchers -- leaves
 * PLYRMODE BOUNCING only on `btst MODE_END_BIT`. The port's animation
 * VM used to set an internal `ended` flag and never that bit, so the
 * first wrestler to touch the ropes stayed in BOUNCING for the rest of
 * the match: over 20000 ticks two Bret drones spent 39845 of 40000
 * actor-ticks bouncing, played five animations between them, threw no
 * strike and took no damage.
 *
 * The unit-level checks live in tests/arcade_port/anim/test_change_anim.c.
 * These are the observable consequences, which is what makes the bug
 * worth a test of its own: every piece of the chain below was already
 * translated and individually tested, and the match still could not
 * produce a single point of damage.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/match.h"
#include "wm_arcade_roster.h"
#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wm_arcade_drone_data.h"
#include "wm/arcade/wm_arcade_combat_defs.h"

/* The same shape of RNG feed the other live-match tests use: a
   deterministic pair of hardware-counter stand-ins, so a run is
   reproducible and a regression is not mistaken for luck. */
static uint32_t g_tick, g_seed;
static uint32_t hc(void *u) { (void)u; return g_tick * 7u + 13u + g_seed * 101u; }
static uint32_t spf(void *u) { (void)u; return 0x1234u + g_tick + g_seed * 7919u; }

#define WM_FIGHT_MODES 32u

typedef struct {
    unsigned bouncing;      /* actor-ticks in PLYRMODE BOUNCING */
    unsigned normal;        /* actor-ticks in PLYRMODE NORMAL */
    unsigned actor_ticks;
    unsigned longest_bounce;/* longest unbroken BOUNCING run, one actor */
    unsigned damage_events; /* ticks on which either life changed */
    /* Per-PLYRMODE occupancy and the longest unbroken run in it. When a
       mode's longest run EQUALS its total occupancy, one wrestler
       entered it and never left -- the signature of a one-way door, and
       the shape of all three defects this file guards. */
    unsigned mode_ticks[WM_FIGHT_MODES];
    unsigned mode_longest[WM_FIGHT_MODES];
    /* Furthest either wrestler got from the ring's centre in X. */
    unsigned max_x_off;
} fight_stats;

static void play(fight_stats *st, uint32_t seed, int w0, int w1,
                 unsigned ticks)
{
    static wm_match_state m;
    static WmRng rng;
    wm_arcade_drone_callbacks_t cb;
    unsigned t, run0 = 0, run1 = 0;
    static unsigned mode_run[2][WM_FIGHT_MODES];
    int32_t l0, l1;

    memset(mode_run, 0, sizeof mode_run);

    memset(st, 0, sizeof *st);
    g_seed = seed;
    g_tick = 0;
    wm_rng_init(&rng, seed, hc, spf, NULL);
    memset(&m, 0, sizeof m);
    wm_match_init(&m);
    wm_match_start_attract(&m, &rng);
    m.actors[0].wrestler_num = w0;
    m.actors[1].wrestler_num = w1;
    cb = wm_arcade_drone_data_callbacks(&rng);

    l0 = m.actors[0].life;
    l1 = m.actors[1].life;

    for (t = 0; t < ticks && m.active; ++t) {
        size_t i;
        g_tick = t;
        wm_match_tick(&m, &cb, NULL);

        for (i = 0; i < m.actor_count && i < 2; ++i) {
            unsigned *run = i == 0 ? &run0 : &run1;
            unsigned pm = m.actors[i].player_mode;
            st->actor_ticks++;
            if (pm < WM_FIGHT_MODES) {
                unsigned k;
                st->mode_ticks[pm]++;
                mode_run[i][pm]++;
                if (mode_run[i][pm] > st->mode_longest[pm])
                    st->mode_longest[pm] = mode_run[i][pm];
                for (k = 0; k < WM_FIGHT_MODES; ++k)
                    if (k != pm) mode_run[i][k] = 0;
            }
            {
                int32_t off = m.actors[i].x_int - (int32_t)(0x0400 + 50);
                unsigned u = (unsigned)(off < 0 ? -off : off);
                if (u > st->max_x_off) st->max_x_off = u;
            }
            if (m.actors[i].player_mode == WM_PMODE_BOUNCING) {
                st->bouncing++;
                if (++*run > st->longest_bounce) st->longest_bounce = *run;
            } else {
                *run = 0;
                if (m.actors[i].player_mode == WM_PMODE_NORMAL) st->normal++;
            }
        }

        if (m.actors[0].life != l0 || m.actors[1].life != l1) {
            st->damage_events++;
            l0 = m.actors[0].life;
            l1 = m.actors[1].life;
        }
    }
}

/*
 * The rope bounce has to end. hrt_bounce_anim (HRTSEQ1.ASM:575) is 33
 * ops long and holds each frame one to three ticks, so a bounce that is
 * still running hundreds of ticks later is not a slow animation -- it
 * is a wrestler who never got told his animation finished.
 */
static void test_a_rope_bounce_ends(void)
{
    fight_stats st;
    play(&st, 0, WM_ROSTER_BRET, WM_ROSTER_BRET, 4000);

    /* It must happen at all, or the rest of the test proves nothing. */
    assert(st.bouncing > 0);
    /* ...and it must finish. The animation is ~60 ticks end to end. */
    assert(st.longest_bounce < 200);
}

/*
 * And the match must be spent mostly out of the ropes. Before the fix
 * this ratio was 99.6% BOUNCING against 0.15% NORMAL; after it, 1%
 * against 49%. The thresholds sit far from both so the check fails on
 * the regression and not on ordinary variation in how the drones play.
 */
static void test_the_wrestlers_do_not_live_in_the_ropes(void)
{
    fight_stats st;
    play(&st, 0, WM_ROSTER_BRET, WM_ROSTER_BRET, 4000);

    assert(st.actor_ticks > 0);
    assert(st.bouncing * 2u < st.actor_ticks);   /* under half */
    assert(st.normal * 5u > st.actor_ticks);     /* over a fifth */
}

/*
 * The point of all of it: somebody gets hurt.
 *
 * Two drones at the same skill spend a lot of the round blocking each
 * other, and a single pairing can go a long time without connecting --
 * so this asks for damage across a spread of pairings rather than from
 * any one of them. Before the fix no pairing and no seed produced a
 * single point over 20000 ticks, because no strike animation was ever
 * selected at all.
 */
static void test_somebody_takes_damage(void)
{
    static const struct { uint32_t seed; int w0, w1; } bouts[] = {
        { 0, WM_ROSTER_YOKO,  WM_ROSTER_SHAWN },
        { 3, WM_ROSTER_BRET,  WM_ROSTER_BRET  },
        { 1, WM_ROSTER_LEX,   WM_ROSTER_RAZOR },
        { 0, WM_ROSTER_BAM,   WM_ROSTER_DOINK },
        { 0, WM_ROSTER_RAZOR, WM_ROSTER_TAKER },
    };
    unsigned total = 0;
    size_t i;

    for (i = 0; i < sizeof bouts / sizeof bouts[0]; ++i) {
        fight_stats st;
        play(&st, bouts[i].seed, bouts[i].w0, bouts[i].w1, 20000);
        total += st.damage_events;
    }
    assert(total > 0);
}

/*
 * No PLYRMODE may be a one-way door.
 *
 * A mode whose longest unbroken run equals its whole occupancy was
 * entered once and never left. That single measurement caught all three
 * defects this file exists for:
 *
 *   BOUNCING   39845 ticks, longest run 39845  (ANI_END set no MODE_END)
 *   ONTURNBKL   7854 ticks, longest run  7854  (the drone never acted)
 *   BLOCK       7093 ticks, longest run  7057  (same cause)
 *
 * After both fixes the longest runs are 28, 0 and 40. NORMAL is exempt
 * only in the sense that it is the resting state -- it still must not
 * hold a wrestler for the entire bout, which it did (8000 of 8000).
 */
static void test_no_player_mode_is_a_one_way_door(void)
{
    fight_stats st;
    unsigned pm;
    play(&st, 0, WM_ROSTER_BRET, WM_ROSTER_BRET, 4000);

    assert(st.actor_ticks > 0);
    for (pm = 0; pm < WM_FIGHT_MODES; ++pm) {
        if (st.mode_ticks[pm] == 0) continue;
        /* A mode occupied only briefly cannot show the pattern, so only
           judge the ones a wrestler spends real time in. */
        if (st.mode_ticks[pm] < 200u) continue;
        assert(st.mode_longest[pm] < st.mode_ticks[pm]);
        /* And no mode may swallow most of the bout in one sitting. */
        assert(st.mode_longest[pm] * 2u < 4000u);
    }
}

/*
 * The match must produce a real exchange, measured over a spread.
 *
 * Twenty bouts once produced 10 damage events between them; the drone's
 * opponent lookup took that to 1248. This file then recorded that one
 * bout (Bret v Bret, seed 1) still produced ZERO and called whether that
 * was arcade behaviour "NOT established". It was not arcade behaviour.
 * Both drones stood in NORMAL for all 20000 ticks because drn_ontb threw
 * away drone_seekxz's stick (DRONE.ASM:3078), and under that sat five
 * more one-way states -- see test_quiet_bout.c, which pins each cause:
 *
 *   a thrown man slid off the arena on his throw velocity and lay
 *     ONGROUND for 18000 ticks (start_run_flung's #ok2 untranslated);
 *   a held man stayed a PUPPET for 515 ticks after his holder walked off
 *     (a torso ANI_SETMODE wiped the body's MODE_UNINT);
 *   a man killed by an animation in ATTRACT mode lay DEAD for 15464
 *     ticks (ANI_DAMAGEOPP ignored "if we're in attract mode, don't die");
 *   drones outside the ring walked at a corner post for good (drn_ontb's
 *     #ering, and the drone layer's inverted INRING reads);
 *   every attract bout ran with the first rung's "no aggressive" cap.
 *
 * Then drn_run's in-ring arm, read inside out: a runner CLEAR of the
 * rope goes to #rpok and considers a strike (DRONE.ASM:2426), and the
 * port only steered him -- see test_drone_running.c, which also pins
 * the two out-of-ring arms (drone_chkrun's #out, drn_run's #ering and
 * #brkseek) that became reachable with the ring-out.
 *
 * Measured after all of it, over these twenty: 7499 damage events, the
 * quietest bout 263, longest ONGROUND run 419, longest PUPPET run 139,
 * longest RUNNING run 194 (was 3776), no DEAD tick at all, nobody
 * further than 913 from ring centre. The thresholds sit well clear of
 * both sides.
 *
 * Still NOT established, and left on the record: a drone outside the
 * ring at a corner. Seed 3 Bam v Doink leaves Doink at (809, 962) for
 * the last 4617 ticks: drn_enterring aims him at the ring's left end
 * exactly as DRONE.ASM:2778-2795 does, and the mat-edge confine stands
 * him at the corner without ck_climb_in_top's 0xC0-from-centre window
 * admitting him. Whether the arcade drone sticks there too is not
 * something this port can settle from the code, so it is not tuned
 * away. (The same trap held Razor in seed 3 Lex v Razor until drn_run
 * stopped running him into it.)
 */
static void test_the_match_produces_a_real_exchange(void)
{
    static const int pairs[5][2] = {
        { WM_ROSTER_BRET,  WM_ROSTER_BRET  },
        { WM_ROSTER_YOKO,  WM_ROSTER_SHAWN },
        { WM_ROSTER_BAM,   WM_ROSTER_DOINK },
        { WM_ROSTER_RAZOR, WM_ROSTER_TAKER },
        { WM_ROSTER_LEX,   WM_ROSTER_RAZOR },
    };
    unsigned total = 0, bouts = 0, drew_blood = 0;
    unsigned longest_ground = 0, longest_puppet = 0, dead = 0, max_x = 0;
    unsigned longest_run = 0;
    uint32_t seed;
    int p;

    for (seed = 0; seed < 4u; ++seed) {
        for (p = 0; p < 5; ++p) {
            fight_stats st;
            play(&st, seed, pairs[p][0], pairs[p][1], 20000);
            total += st.damage_events;
            ++bouts;
            if (st.damage_events > 0) ++drew_blood;
            if (st.mode_longest[WM_PMODE_ONGROUND] > longest_ground)
                longest_ground = st.mode_longest[WM_PMODE_ONGROUND];
            if (st.mode_longest[WM_PMODE_PUPPET] > longest_puppet)
                longest_puppet = st.mode_longest[WM_PMODE_PUPPET];
            if (st.mode_longest[WM_PMODE_RUNNING] > longest_run)
                longest_run = st.mode_longest[WM_PMODE_RUNNING];
            dead += st.mode_ticks[WM_PMODE_DEAD];
            if (st.max_x_off > max_x) max_x = st.max_x_off;
        }
    }
    assert(bouts == 20u);
    assert(total >= 3000u);         /* measured 7499; 1248 before, 10 first */
    assert(drew_blood == 20u);      /* measured 20 of 20, the least 263 */
    assert(longest_ground < 4000u); /* measured 419; was 18000+ */
    assert(longest_run < 1500u);    /* measured 194; was 3776 */
    assert(longest_puppet < 400u);  /* measured 139; was 515 */
    /* LIFEBAR.ASM:1578: nobody dies in attract mode, from any blow. */
    assert(dead == 0u);             /* was 15464 */
    assert(max_x < 1500u);          /* measured 913; was 3814 */
}

int main(void)
{
    test_a_rope_bounce_ends();
    test_the_wrestlers_do_not_live_in_the_ropes();
    test_somebody_takes_damage();
    test_no_player_mode_is_a_one_way_door();
    test_the_match_produces_a_real_exchange();
    printf("live fight: rope bounce ends, drones leave the ropes, damage lands\n");
    return 0;
}
