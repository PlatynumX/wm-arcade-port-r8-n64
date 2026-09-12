#ifndef WMANIA_RNG_H
#define WMANIA_RNG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Source-faithful port of the Williams/Midway TMS34010 RAND/RNDRNG family.
 *
 * WrestleMania's own main loop advances @RAND with:
 *
 *   move @RAND,a1,L
 *   rl   a1,a1
 *   move @HCOUNT,a14
 *   rl   a14,a1
 *   add  sp,a1
 *   move a1,@RAND,L
 *
 * The shared Williams utility implementation of RNDRNG/RNDRNG0/RNDRNGS
 * performs that same state advance before range scaling.
 *
 * IMPORTANT TMS34010 semantics:
 * - RL Rs,Rd rotates Rd left by the low 5 bits of Rs.
 *   Therefore "rl a1,a1" rotates RAND by RAND&31; it is NOT a one-bit ROL.
 * - MPYU A1,A0 with even destination A0 produces a 64-bit product in A0:A1.
 *   A0 receives the high 32 bits, which is the returned scaled random value.
 *
 * CRITICAL for integrators: `add sp,a1` is the *only* value-changing step in
 * that mix -- everything before it merely rotates RAND's bits, which cannot
 * change its popcount. So a caller that leaves both entropy inputs at zero
 * does not get a weak sequence, it gets a degenerate one: RAND can only
 * rotate, and a RAND of 0 (wm_rng_init's own natural BSS-matching seed)
 * rotates to 0 forever, making every rndrng0 return 0. Even a non-zero seed
 * collapses to a small fixed point within a few calls. Supply real HCOUNT
 * and SP values -- via the callbacks or wm_rng_set_latched_inputs -- or the
 * whole family silently stops being random. A constant non-zero SP is
 * already enough to restore a uniform distribution.
 */

typedef uint32_t (*WmRngReadValueFn)(void *user);

typedef struct {
    uint32_t rand_state;

    /*
     * HCOUNT and SP are runtime entropy inputs on the arcade hardware.
     * A source-accurate N64 integration should provide translated values
     * rather than silently substituting libc rand().
     *
     * If callbacks are NULL, the latched values below are used.
     */
    uint32_t latched_hcount;
    uint32_t latched_sp;
    WmRngReadValueFn read_hcount;
    WmRngReadValueFn read_sp;
    void *env_user;
} WmRng;

/* Initialize the global RAND-equivalent state and optional environment. */
void wm_rng_init(
    WmRng *rng,
    uint32_t initial_rand,
    WmRngReadValueFn read_hcount,
    WmRngReadValueFn read_sp,
    void *env_user);

/* Useful for deterministic tests/replays or a translated virtual TMS state. */
void wm_rng_set_latched_inputs(
    WmRng *rng,
    uint32_t hcount,
    uint32_t sp_value);

/* Exact TMS34010 RL helper: rotate destination by source low 5 bits. */
uint32_t wm_rng_rl(uint32_t source, uint32_t destination);

/*
 * Exact RAND state mix using explicitly supplied hardware/process values.
 * This is also the body WrestleMania executes once per main dispatch loop.
 */
uint32_t wm_rng_mix(
    uint32_t old_rand,
    uint32_t hcount,
    uint32_t sp_value);

/* Advance only the shared RAND state (WrestleMania main-loop randomize). */
uint32_t wm_rng_mainloop_step(WmRng *rng);

/*
 * RNDRNG0:
 *   input max_inclusive X
 *   output 0..X inclusive
 *
 * The source increments X before multiply-high. UINT32_MAX therefore wraps
 * the span to zero and returns zero, matching 32-bit source arithmetic.
 */
uint32_t wm_rng_rndrng0(
    WmRng *rng,
    uint32_t max_inclusive);

/*
 * RNDRNG:
 *   lower..upper inclusive.
 *
 * Arithmetic intentionally wraps as 32-bit TMS34010 arithmetic.
 */
uint32_t wm_rng_rndrng(
    WmRng *rng,
    uint32_t lower_inclusive,
    uint32_t upper_inclusive);

/*
 * RNDRNGS:
 *   -X..+X inclusive, returned as two's-complement int32_t.
 */
int32_t wm_rng_rndrngs(
    WmRng *rng,
    int32_t positive_x);

/* Pure helpers for validation/tests and source-derived callers. */
uint32_t wm_rng_mul_high_u32(uint32_t a, uint32_t b);
uint32_t wm_rng_rndrng0_with_inputs(
    WmRng *rng,
    uint32_t max_inclusive,
    uint32_t hcount,
    uint32_t sp_value);

/*
 * Drop-in callback shape used by the translated high-score initials system:
 * max argument is inclusive, exactly like RNDRNG0.
 */
uint32_t wm_rng_rndrng0_callback(
    void *user,
    uint32_t max_inclusive);

#ifdef __cplusplus
}
#endif

#endif
