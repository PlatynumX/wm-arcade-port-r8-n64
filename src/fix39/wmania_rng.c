#include "wmania_rng.h"

#include <stddef.h>

static uint32_t read_hcount(const WmRng *rng)
{
    return rng->read_hcount != NULL
        ? rng->read_hcount(rng->env_user)
        : rng->latched_hcount;
}

static uint32_t read_sp(const WmRng *rng)
{
    return rng->read_sp != NULL
        ? rng->read_sp(rng->env_user)
        : rng->latched_sp;
}

void wm_rng_init(
    WmRng *rng,
    uint32_t initial_rand,
    WmRngReadValueFn read_hcount_fn,
    WmRngReadValueFn read_sp_fn,
    void *env_user)
{
    if (rng == NULL) {
        return;
    }

    rng->rand_state = initial_rand;
    rng->latched_hcount = 0u;
    rng->latched_sp = 0u;
    rng->read_hcount = read_hcount_fn;
    rng->read_sp = read_sp_fn;
    rng->env_user = env_user;
}

void wm_rng_set_latched_inputs(
    WmRng *rng,
    uint32_t hcount,
    uint32_t sp_value)
{
    if (rng == NULL) {
        return;
    }

    rng->latched_hcount = hcount;
    rng->latched_sp = sp_value;
}

uint32_t wm_rng_rl(uint32_t source, uint32_t destination)
{
    unsigned count = (unsigned)(source & 31u);

    if (count == 0u) {
        return destination;
    }

    return (destination << count) |
           (destination >> (32u - count));
}

uint32_t wm_rng_mix(
    uint32_t old_rand,
    uint32_t hcount,
    uint32_t sp_value)
{
    uint32_t value = old_rand;

    /* rl a1,a1 */
    value = wm_rng_rl(value, value);

    /* move @HCOUNT,a14 / rl a14,a1 */
    value = wm_rng_rl(hcount, value);

    /* add sp,a1 -- ordinary wrapping 32-bit add */
    value += sp_value;

    return value;
}

uint32_t wm_rng_mul_high_u32(uint32_t a, uint32_t b)
{
    uint64_t product = (uint64_t)a * (uint64_t)b;
    return (uint32_t)(product >> 32);
}

uint32_t wm_rng_mainloop_step(WmRng *rng)
{
    if (rng == NULL) {
        return 0u;
    }

    rng->rand_state = wm_rng_mix(
        rng->rand_state,
        read_hcount(rng),
        read_sp(rng));

    return rng->rand_state;
}

uint32_t wm_rng_rndrng0_with_inputs(
    WmRng *rng,
    uint32_t max_inclusive,
    uint32_t hcount,
    uint32_t sp_value)
{
    uint32_t span;

    if (rng == NULL) {
        return 0u;
    }

    /* RNDRNG0: addk 1,a0 (32-bit wrap). */
    span = max_inclusive + 1u;

    rng->rand_state = wm_rng_mix(
        rng->rand_state,
        hcount,
        sp_value);

    /*
     * mpyu a1,a0 with A0 even:
     * A0 = high 32 bits of RAND * span.
     */
    return wm_rng_mul_high_u32(rng->rand_state, span);
}

bool wm_rng_rndper_hi_with_inputs(
    WmRng *rng,
    uint16_t probability,
    uint32_t hcount,
    uint32_t sp_value)
{
    uint32_t sample;

    if (rng == NULL) {
        return false;
    }

    /*
     * UTIL.ASM::RNDPER, exact source order:
     *   move @RAND,a1,L
     *   rl   a1,a1
     *   move @HCOUNT,a14
     *   rl   a14,a1
     *   add  sp,a1
     *   move a1,@RAND,L
     *   move a0,a14       ; save probability (0..1000)
     *   movi 1000,a0
     *   mpyu a1,a0        ; A0 = 0..999
     *   cmp  a0,a14       ; HI iff probability > sample
     */
    rng->rand_state = wm_rng_mix(rng->rand_state, hcount, sp_value);
    sample = wm_rng_mul_high_u32(rng->rand_state, 1000u);
    return (uint32_t)probability > sample;
}

bool wm_rng_rndper_hi(WmRng *rng, uint16_t probability)
{
    if (rng == NULL) {
        return false;
    }
    return wm_rng_rndper_hi_with_inputs(
        rng, probability, read_hcount(rng), read_sp(rng));
}

uint32_t wm_rng_rnd_mask(WmRng *rng, uint32_t mask)
{
    if (rng == NULL) return 0u;
    rng->rand_state = wm_rng_mix(
        rng->rand_state,
        read_hcount(rng),
        read_sp(rng));
    return rng->rand_state & mask;
}

uint32_t wm_rng_rndrng0(
    WmRng *rng,
    uint32_t max_inclusive)
{
    if (rng == NULL) {
        return 0u;
    }

    return wm_rng_rndrng0_with_inputs(
        rng,
        max_inclusive,
        read_hcount(rng),
        read_sp(rng));
}

uint32_t wm_rng_rndrng(
    WmRng *rng,
    uint32_t lower_inclusive,
    uint32_t upper_inclusive)
{
    uint32_t span;
    uint32_t scaled;

    if (rng == NULL) {
        return lower_inclusive;
    }

    /*
     * RNDRNG:
     *   sub a0,a1
     *   addk 1,a1
     * All operations wrap at 32 bits.
     */
    span = (upper_inclusive - lower_inclusive) + 1u;

    rng->rand_state = wm_rng_mix(
        rng->rand_state,
        read_hcount(rng),
        read_sp(rng));

    scaled = wm_rng_mul_high_u32(rng->rand_state, span);
    return scaled + lower_inclusive;
}

int32_t wm_rng_rndrngs(
    WmRng *rng,
    int32_t positive_x)
{
    uint32_t upper = (uint32_t)positive_x;
    uint32_t lower = (uint32_t)(0u - upper);

    return (int32_t)wm_rng_rndrng(rng, lower, upper);
}

uint32_t wm_rng_rndrng0_callback(
    void *user,
    uint32_t max_inclusive)
{
    return wm_rng_rndrng0((WmRng *)user, max_inclusive);
}
