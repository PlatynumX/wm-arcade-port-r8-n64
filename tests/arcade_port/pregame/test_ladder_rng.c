/*
 * PROGRESS.ASM's ladder, drawn from the game's real RNDRNG0.
 *
 * The ladder algorithm was already ported; what it drew from was a local
 * xorshift, added on the stated grounds that RNDRNG0 was not in the
 * source tree. It is -- UTIL.ASM:1713 -- and the port already had it.
 * These checks are about the substitution being gone: the ladder now
 * shares the one RAND the arcade has, and it behaves the way that RAND
 * behaves, including when nothing is stirring it.
 */
#include "wm/pregame.h"
#include "wm/arcade/wmania_rng.h"
#include "wm/audio.h"
#include "wm/roster.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Stand-ins for the beam position and stack pointer the cabinet mixes
 * in. app.c derives both from the scheduler tick; these just have to
 * move, and not in lockstep. */
static uint32_t g_tick;

static uint32_t fake_hcount(void *user)
{
    (void)user;
    return (g_tick * 8u) & 0x1FFu;
}

static uint32_t fake_sp(void *user)
{
    (void)user;
    return 0x13F73E0u - (g_tick % 97u) * 32u;
}

/* The ladder is built by enter_progress, which the belt phase reaches
 * on its own; drive the state machine until it exists. */
static void build_ladder(wm_pregame_state *s)
{
    wm_audio_state audio;
    wm_input_state in;
    int i;

    memset(&audio, 0, sizeof(audio));
    for (i = 0; i < 4000 && s->ladder[0].packed == 0u; ++i) {
        memset(&in, 0, sizeof(in));
        in.start = (i % 8) == 0;
        ++g_tick;
        wm_pregame_tick(s, &in, &audio);
    }
    assert(s->ladder[0].packed != 0u);
}

static unsigned ladder_entries(const wm_pregame_state *s, uint32_t *seen)
{
    unsigned n = 0;
    unsigned i;
    *seen = 0;
    for (i = 0; i < WM_PREGAME_PLAYABLE_LADDER_ENTRIES; ++i) {
        uint32_t packed = s->ladder[i].packed;
        unsigned count = (packed >> 24) & 0xFFu;
        unsigned k;
        if (count == 0) {
            continue;
        }
        ++n;
        for (k = 0; k < count && k < WM_PREGAME_MAX_OPPONENTS; ++k) {
            *seen |= 1u << ((packed >> (8u * k)) & 0x7u);
        }
    }
    return n;
}

static void test_a_driven_rng_produces_a_varied_ladder(void)
{
    WmRng rng;
    wm_pregame_state s;
    uint32_t seen = 0;
    unsigned entries;

    g_tick = 1;
    wm_rng_init(&rng, 0, fake_hcount, fake_sp, NULL);
    wm_pregame_init(&s, 0, WM_WRESTLER_BRET, &rng);
    build_ladder(&s);

    entries = ladder_entries(&s, &seen);
    /* The intercontinental table is 4+2+1 runs, so seven battles. */
    assert(entries == WM_PREGAME_PLAYABLE_LADDER_ENTRIES);
    /* INIT_TEMP_TABLE seeds 7..0 and FETCH_NEXT_OPPONENT walks it, so
     * the first eight picks are the eight wrestlers in some order --
     * a real shuffle reaches most of them. */
    assert(__builtin_popcount(seen) >= 6);
}

static void test_the_shuffle_actually_shuffles(void)
{
    /* Two runs off the same RNG must not produce the same ladder --
     * that is what the substitution's removal has to preserve. */
    WmRng rng;
    wm_pregame_state a;
    wm_pregame_state b;
    unsigned i;
    bool differ = false;

    g_tick = 1;
    wm_rng_init(&rng, 0, fake_hcount, fake_sp, NULL);
    wm_pregame_init(&a, 0, WM_WRESTLER_BRET, &rng);
    build_ladder(&a);
    g_tick = 40;
    wm_pregame_init(&b, 0, WM_WRESTLER_BRET, &rng);
    build_ladder(&b);

    for (i = 0; i < WM_PREGAME_PLAYABLE_LADDER_ENTRIES; ++i) {
        if (a.ladder[i].packed != b.ladder[i].packed) {
            differ = true;
        }
    }
    assert(differ);
}

static void test_an_unstirred_rand_draws_zero(void)
{
    /*
     * wm/arcade/wmania_rng.h documents the trap: RAND is stirred only
     * by HCOUNT and the stack pointer, so a WmRng with no live inputs
     * settles on one value and RNDRNG0 returns the same thing forever.
     * That is the arcade's behaviour, not a bug in the port, and the
     * ladder inherits it -- with no shuffle at all the temp table is
     * still 7,6,5,...,0 and the picks come out in that order.
     */
    WmRng rng;
    wm_pregame_state s;
    unsigned i;

    wm_rng_init(&rng, 0, NULL, NULL, NULL);
    wm_rng_set_latched_inputs(&rng, 0, 0);
    wm_pregame_init(&s, 0, WM_WRESTLER_BRET, &rng);
    build_ladder(&s);

    assert(wm_rng_rndrng0(&rng, 7u) == 0u);
    /* One opponent per battle on the intercontinental ladder's first
     * four rows, handed out from the top of the unshuffled table. */
    for (i = 0; i < 4u; ++i) {
        assert(((s.ladder[i].packed >> 24) & 0xFFu) == 1u);
    }
    assert((s.ladder[0].packed & 0xFFu) == 7u);
    assert((s.ladder[1].packed & 0xFFu) == 6u);
    assert((s.ladder[2].packed & 0xFFu) == 5u);
    assert((s.ladder[3].packed & 0xFFu) == 4u);
}

static void test_no_rng_at_all_is_safe(void)
{
    /* A pregame state nobody wired must not crash; every draw is zero,
     * which is the same shape as the unstirred case above. */
    wm_pregame_state s;
    wm_pregame_init(&s, 0, WM_WRESTLER_BRET, NULL);
    assert(s.rng == NULL);
    build_ladder(&s);
    assert(((s.ladder[0].packed >> 24) & 0xFFu) == 1u);
    assert((s.ladder[0].packed & 0xFFu) == 7u);
}

static void test_the_belt_decides_the_shape(void)
{
    /* LADDER_TABLE_ICONT is 4x1, 2x2, 1x3; LADDER_TABLE_WCHAMP is
     * 4x2, 2x3, 1x3. The counts live in the top byte of each entry. */
    WmRng rng;
    wm_pregame_state s;
    unsigned i;
    static const unsigned icont[7] = { 1, 1, 1, 1, 2, 2, 3 };

    g_tick = 7;
    wm_rng_init(&rng, 0, fake_hcount, fake_sp, NULL);
    wm_pregame_init(&s, 0, WM_WRESTLER_BRET, &rng);
    build_ladder(&s);
    for (i = 0; i < 7u; ++i) {
        assert(((s.ladder[i].packed >> 24) & 0xFFu) == icont[i]);
    }
}

int main(void)
{
    test_a_driven_rng_produces_a_varied_ladder();
    test_the_shuffle_actually_shuffles();
    test_an_unstirred_rand_draws_zero();
    test_no_rng_at_all_is_safe();
    test_the_belt_decides_the_shape();
    printf("PROGRESS.ASM ladder on the real RNDRNG0: all checks passed\n");
    return 0;
}
