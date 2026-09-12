/*
 * AWARD.ASM's powerup codes -- the hidden match options two players can
 * turn on at the credit screen by holding a button sequence.
 *
 * Several headers in this port already name these flags as things it
 * did not have: wm_arcade_mode_dead.h's `instant_combos_on`,
 * wm_arcade_combat.h's `hyper_speed_on`, wm_arcade_lifebar.h's
 * move-names toggle. This is where they come from.
 *
 * Translated from AWARD.ASM:1967-2290 and the PUPWAITSWITCH macro at
 * AWARD.ASM:113.
 */
#ifndef WM_ARCADE_POWERUP_H
#define WM_ARCADE_POWERUP_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GAME.EQU:558-576 */
#define WM_PU_BLOCKING_OFF 1
#define WM_PU_COMBOS_ON 2
#define WM_PU_RING_OUTS_ON 4
#define WM_PU_NO_RING 8
#define WM_PU_MOVE_NAMES_ON 16
#define WM_PU_D_METERS_ON 32
#define WM_PU_HYPER_MATCH_ON 64
#define WM_PU_BUDDY_MODE 128
#define WM_PU_BOTH_P_MASK (WM_PU_BLOCKING_OFF | WM_PU_COMBOS_ON | \
                           WM_PU_RING_OUTS_ON | WM_PU_NO_RING | \
                           WM_PU_HYPER_MATCH_ON | WM_PU_BUDDY_MODE)

/*
 * AWARD.ASM:113-122. PUPWAITSWITCH builds what it compares as
 * `(stick_down << 5) | buttons_down`, so a step of a code is one value
 * in that space and these constants are already shifted.
 */
#define WM_PUP_PUNCH 1
#define WM_PUP_BLOCK 2
#define WM_PUP_SUPERP 4
#define WM_PUP_KICK 8
#define WM_PUP_UP 32
#define WM_PUP_DOWN 64
#define WM_PUP_LEFT 128
#define WM_PUP_RIGHT 256

/* DISPLAY.EQU:46 TSEC, and the `movi TSEC*2,a11` budget. */
#define WM_TSEC 53
#define WM_PUP_WINDOW (WM_TSEC * 2)

#define WM_PUP_MAX_STEPS 5

typedef struct wm_powerup_code {
    const char *name;
    uint32_t grants;                /* the WM_PU_* bit it requests */
    int steps;
    int32_t step[WM_PUP_MAX_STEPS];
    bool blocked_in_royal_rumble;   /* `move @royal_rumble / jrnz #die` */
    bool spawned;                   /* CREATEd by player_powerup_checker */
} wm_powerup_code;

extern const wm_powerup_code wm_powerup_codes[];
extern const int wm_powerup_code_count;

/*
 * One player's attempt at one code. The arcade runs each as its own
 * process; this is the same state machine without one.
 */
typedef struct wm_powerup_attempt {
    const wm_powerup_code *code;
    int at;                     /* steps matched so far */
    int32_t timer;              /* the macro's A11 */
    bool dead;
} wm_powerup_attempt;

void wm_powerup_attempt_start(wm_powerup_attempt *att,
                              const wm_powerup_code *code);

/*
 * One tick of PUPWAITSWITCH. `switches` is this tick's freshly-pressed
 * input, already `(stick << 5) | buttons`. Returns true on the tick the
 * code completes.
 *
 * Three things about this loop are worth knowing, and all three are the
 * macro's, not a simplification:
 *
 *   The first step has no deadline. `clr a11` then `dec a11` wraps to
 *   -1, so the countdown never reaches zero and the code waits forever
 *   for its opening press.
 *
 *   Every step after that shares ONE two-second budget. `movi TSEC*2`
 *   runs once, after the first press, and is never reset -- so a
 *   five-press code needs all four remaining presses inside 106 ticks
 *   in total, not 106 ticks each.
 *
 *   A wrong press does not restart the code. The macro loops on
 *   anything that is not the value it wants, so junk input costs time
 *   and nothing else.
 */
bool wm_powerup_attempt_tick(wm_powerup_attempt *att, int32_t switches);

/* Whether the code has been entered. True from the start for
 * drone_meters, whose sequence is commented out in the source. */
bool wm_powerup_attempt_done(const wm_powerup_attempt *att);

/*
 * get_powerups (AWARD.ASM:2230): reconcile the two players' requests
 * and distribute them.
 *
 * Everything in BOTH_P_MASK survives only where both players asked;
 * D_METERS_ON and MOVE_NAMES_ON, the two outside it, need only one.
 * The reconciled sets are written back over both request words before
 * distribution, which matters because MOVE_NAMES_ON has no output flag
 * of its own -- get_powerups never writes `move_names_on`, and its one
 * consumer (LIFEBAR.ASM:3552) reads the per-player request word and
 * tests MOVE_NAMES_ON_BIT itself.
 */
typedef struct wm_powerup_flags {
    uint32_t p_request[2];
    /* `movi 2020h,a8` -- not a 0/1 flag. */
    int32_t blocking_off;
    /* These three keep the bit value rather than being normalised. */
    int32_t instant_combos_on;
    int32_t ring_out_on;
    int32_t no_ring_on;
    /* This one IS normalised to 1. */
    int32_t hyper_speed_on;
    int32_t drone_meters_on;
} wm_powerup_flags;

#define WM_BLOCKING_OFF_VALUE 0x2020

void wm_get_powerups(wm_powerup_flags *f);

/* powerup_check (AWARD.ASM:2198): clear both requests and every output
 * flag before the codes start listening. */
void wm_powerup_reset(wm_powerup_flags *f);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_POWERUP_H */
