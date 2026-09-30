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

typedef struct {
    unsigned bouncing;      /* actor-ticks in PLYRMODE BOUNCING */
    unsigned normal;        /* actor-ticks in PLYRMODE NORMAL */
    unsigned actor_ticks;
    unsigned longest_bounce;/* longest unbroken BOUNCING run, one actor */
    unsigned damage_events; /* ticks on which either life changed */
} fight_stats;

static void play(fight_stats *st, uint32_t seed, int w0, int w1,
                 unsigned ticks)
{
    static wm_match_state m;
    static WmRng rng;
    wm_arcade_drone_callbacks_t cb;
    unsigned t, run0 = 0, run1 = 0;
    int32_t l0, l1;

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
            st->actor_ticks++;
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

int main(void)
{
    test_a_rope_bounce_ends();
    test_the_wrestlers_do_not_live_in_the_ropes();
    test_somebody_takes_damage();
    printf("live fight: rope bounce ends, drones leave the ropes, damage lands\n");
    return 0;
}
