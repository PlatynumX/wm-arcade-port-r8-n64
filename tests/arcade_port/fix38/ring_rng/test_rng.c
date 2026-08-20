#include "wmania_rng.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t hcount;
    uint32_t sp;
} Env;

static uint32_t get_hcount(void *user)
{
    return ((Env *)user)->hcount;
}

static uint32_t get_sp(void *user)
{
    return ((Env *)user)->sp;
}

static void test_rl(void)
{
    /* RL Rs,Rd rotates by Rs low 5 bits, not by one. */
    assert(wm_rng_rl(0u, 0x12345678u) == 0x12345678u);
    assert(wm_rng_rl(4u, 0x12345678u) == 0x23456781u);

    /* Same-register source/dest form used by RAND: count = value&31. */
    assert(wm_rng_rl(0x12345678u, 0x12345678u) == 0x78123456u);
}

static void test_known_vector(void)
{
    WmRng rng;
    uint32_t out;

    wm_rng_init(&rng, 0x12345678u, 0, 0, 0);

    out = wm_rng_rndrng0_with_inputs(
        &rng,
        6u,
        0x00000123u,
        0x01020304u);

    assert(rng.rand_state == 0xc193a5b7u);
    assert(out == 5u);
}

static void test_callbacks_and_inclusive_range(void)
{
    WmRng rng;
    Env env = {0x1fu, 0x00200000u};
    uint32_t out;

    wm_rng_init(&rng, 0xdeadbeefu, get_hcount, get_sp, &env);
    out = wm_rng_rndrng0(&rng, 0x00080000u);

    assert(rng.rand_state == 0x6fdbf7abu);
    assert(out == 229088u);
    assert(out <= 0x00080000u);
}

static void test_mainloop_step(void)
{
    WmRng rng;
    uint32_t expected;

    wm_rng_init(&rng, 0x89abcdefu, 0, 0, 0);
    wm_rng_set_latched_inputs(&rng, 0x12u, 0x12345678u);

    expected = wm_rng_mix(0x89abcdefu, 0x12u, 0x12345678u);
    assert(wm_rng_mainloop_step(&rng) == expected);
    assert(expected == 0x258bf257u);
}

static void test_multiply_high(void)
{
    assert(wm_rng_mul_high_u32(0xffffffffu, 7u) == 6u);
    assert(wm_rng_mul_high_u32(0u, 7u) == 0u);
}

static void test_full_span_wrap(void)
{
    WmRng rng;

    wm_rng_init(&rng, 0x12345678u, 0, 0, 0);
    wm_rng_set_latched_inputs(&rng, 1u, 2u);

    /*
     * Source ADDK 1 wraps X=FFFFFFFF to zero, then MPYU by zero.
     */
    assert(wm_rng_rndrng0(&rng, 0xffffffffu) == 0u);
}

static void test_rndrng_and_signed(void)
{
    WmRng rng;
    uint32_t v;
    int32_t s;

    wm_rng_init(&rng, 0x11111111u, 0, 0, 0);
    wm_rng_set_latched_inputs(&rng, 7u, 0x1000u);

    v = wm_rng_rndrng(&rng, 10u, 20u);
    assert(v >= 10u && v <= 20u);

    s = wm_rng_rndrngs(&rng, 100);
    assert(s >= -100 && s <= 100);
}

int main(void)
{
    test_rl();
    test_known_vector();
    test_callbacks_and_inclusive_range();
    test_mainloop_step();
    test_multiply_high();
    test_full_span_wrap();
    test_rndrng_and_signed();

    puts("wmania_rng tests: PASS");
    return 0;
}
