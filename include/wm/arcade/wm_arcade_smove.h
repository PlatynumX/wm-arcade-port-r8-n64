#ifndef WM_ARCADE_SMOVE_H
#define WM_ARCADE_SMOVE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wm_arcade_und_finish.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE2.ASM:4058 init_smoves and the watchdog processes it makes.
 *
 * The source's own description: "This gets called once each MATCH for
 * every wrestler, not each round. It creates the set of 'watchdog'
 * processes that look out for special moves that the usual method
 * can't handle; specifically, stuff that involves charging up with a
 * stick, detailed control over timing, or proximity to the bad guy."
 *
 * Each wrestler has a table of them (wm/wrestler_anim_tables.h's
 * wm_wrestler_smoves, read out of the source), init_smoves creates one
 * SMOVE_PID process per entry, and each process sits on a stick/button
 * sequence for the whole match.
 *
 * This matters beyond the moves themselves: und_finish_move1 -- the
 * only finishing move the game assembled, and the thing that raises
 * the coffin -- is an entry in und_smove_table and is reached by
 * nothing else. Without init_smoves the whole coffin sequence is
 * unreachable however completely it is translated.
 *
 * ------------------------------------------------------------------
 * WAITSWITCH_DWN (MACROS.H:652)
 *
 * Every one of these monitors is built out of this one macro, and all
 * five of its behaviours are load-bearing:
 *
 *   1. It compares `(BUT_VAL_DOWN << 4) | STICK_REL_NEW` -- buttons
 *      newly pressed this tick, shifted up four, ORed with the stick
 *      direction RELATIVE to facing (toward/away, not left/right) and
 *      only when the stick moved this tick. GAME.EQU's B_* constants
 *      are already shifted, so they compare directly.
 *
 *   2. It masks with `andni MASK`, then `jrz lp?` -- a tick with
 *      nothing new in the unmasked bits simply waits.
 *
 *   3. It FAILS on the wrong input: `cmpi SWITCHES,a0 / jrne
 *      FAILADDR`. This is the opposite of AWARD.ASM's PUPWAITSWITCH,
 *      which loops on junk; here a stray press restarts the sequence.
 *
 *   4. It fails if SPECIAL_MOVE_ADDR is already set -- he is in the
 *      middle of a special move, so he cannot be starting another.
 *
 *   5. The countdown is decremented BEFORE the test, and the first
 *      wait in every monitor runs with a11 cleared: `dec 0` wraps to
 *      -1, which is not zero, so the opening input has no deadline at
 *      all. `movi #TIMEOUT,a11` then runs ONCE, and every remaining
 *      step shares that one budget rather than getting its own.
 */

/* MACROS.H:652's three answers for one tick. */
typedef enum {
    WM_SMOVE_WAIT = 0,   /* `jrz lp?` -- nothing new in the masked bits */
    WM_SMOVE_MATCH,      /* exactly the switches this step wants */
    WM_SMOVE_FAIL        /* timed out, already busy, or the wrong input */
} wm_smove_tick_t;

/*
 * One tick of WAITSWITCH_DWN. `countdown` is the macro's a11 and is
 * decremented in place; pass 0 for a step with no deadline, exactly as
 * the source's `clr a11` does.
 *
 * `but_val_down` and `stick_rel_new` are the wrestler's cached
 * readings (wm/arcade/wm_arcade_switches.h), and `special_move_addr`
 * his SPECIAL_MOVE_ADDR.
 */
wm_smove_tick_t wm_smove_waitswitch(int32_t *countdown,
                                    uintptr_t special_move_addr,
                                    uint16_t but_val_down,
                                    uint16_t stick_rel_new,
                                    uint16_t switches,
                                    uint16_t mask);

/* The combined word the macro compares, exposed because it is the one
   piece of this that is easy to get subtly wrong. */
uint16_t wm_smove_switch_word(uint16_t but_val_down, uint16_t stick_rel_new);

/* ---- a monitor's input sequence -------------------------------- */

typedef struct {
    uint16_t switches;
    uint16_t mask;
} wm_smove_step_t;

/* No monitor in the game has more than nine steps: std_walk_fast and
   std_taunt are both a full eight-way stick rotation, and und_finish
   is three. */
#define WM_SMOVE_MAX_STEPS 12

/*
 * A monitor's state. The arcade gives each one its own process; this
 * is the same state machine without one, so a caller can tick every
 * monitor from wherever it runs its game loop.
 */
typedef struct {
    const struct wm_smove_monitor *monitor;
    size_t at;            /* steps matched so far */
    int32_t countdown;    /* the macro's a11 */
    bool armed;           /* past the gate, inside the sequence */
    bool dead;            /* DIEd; one-shots do this when they fire */
} wm_smove_run_t;

/*
 * What a monitor reads that does not live on the actor. Only what the
 * three translated monitors actually use -- a field is here because a
 * routine reads it, not to be general.
 */
typedef struct {
    /* @p1pins / @p2pins, already selected by the wrestler's side. */
    int32_t my_pins;
    /* The wrestler he is standing over: *a8(WHOIHIT). */
    wm_arcade_actor_t *victim;
    /* *a8(RING_TIME) -- negative means outside the ring. */
    int32_t ring_time;
    /* @WORLDTLX / @WORLDTLY for adjust_view. */
    int32_t world_tlx;
    int32_t world_tly;
    /*
     * und_finish_move1's own seams -- the world-origin writes, the
     * shaker, and @in_finish_move (wm/arcade/wm_arcade_und_finish.h).
     * The monitor does not reimplement that routine; it reaches the
     * translation already here.
     */
    const wm_arcade_und_finish_callbacks_t *und_cb;
} wm_smove_env_t;

/*
 * What firing one produces. The source writes SPECIAL_MOVE_ADDR and
 * calls change_anim1a; this hands the label back the way the rest of
 * the port does, with the side effects it also performs listed beside
 * it so a caller cannot silently drop half of a move.
 */
typedef struct {
    /* The animation to start on the wrestler himself, or NULL. */
    const char *anim;
    /* DOINK.ASM:1136 std_walk_fast's `movi 15*60,a0 / move
       a0,*a8(WALK_FAST)`; 0 when this monitor does not set it. */
    int32_t walk_fast;
    /* DOINK.ASM:1270 std_taunt's `movi 8000h+12*60,a0 / move
       a0,*a8(RISK)`; 0 when this monitor does not set it. */
    uint16_t risk;
    /* und_finish_move1 raised @in_finish_move and scrolled the view. */
    bool in_finish_move;
} wm_smove_fire_t;

typedef struct wm_smove_monitor {
    const char *name;       /* the smove table's own label */
    const char *file;       /* where the source defines it */
    /* The steps, first one deadline-free. */
    wm_smove_step_t step[WM_SMOVE_MAX_STEPS];
    size_t steps;
    /* The routine's own `#TIMEOUT .equ`, applied once after step 0. */
    int32_t timeout;
    /* A drone may not run this one at all (`move *a8(PLYR_TYPE),a14 /
       janz SUCIDE` at the top of std_taunt). */
    bool humans_only;
    /* The test at `#lp`, re-run every pass before the sequence starts
       and, for the two DOINK monitors, again when it completes. */
    bool (*gate)(const wm_arcade_actor_t *a, const wm_smove_env_t *env);
    /* The extra test between step 0 and the timeout (std_taunt's
       "is BLOCK still held"); NULL when there is none. */
    bool (*mid_gate)(const wm_arcade_actor_t *a, const wm_smove_env_t *env);
    /* Everything after the last step. Fills `out` and answers whether
       the move actually starts. */
    bool (*fire)(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                 wm_smove_fire_t *out);
    /* `DIE` rather than looping back to #lp0 once it fires. */
    bool one_shot;
} wm_smove_monitor_t;

/* The monitors this port implements, by their source label. NULL for
   one it does not -- which is the point: an unimplemented monitor is a
   named absence, not a silent one. */
const wm_smove_monitor_t *wm_smove_monitor_find(const char *name);
size_t wm_smove_monitor_count(void);
const wm_smove_monitor_t *wm_smove_monitor_at(size_t i);

/* ---- init_smoves ------------------------------------------------ */

/*
 * WRESTLE2.ASM:4058 init_smoves: `#special_moves` indexed by
 * WRESTLERNUM, then one SMOVE_PID process per non-zero entry until the
 * table's terminating zero. A wrestler whose table pointer is zero
 * (slot 7, Adam Bomb, and the Referee) gets none.
 *
 * Fills `runs` with one armed state machine per entry this port has a
 * monitor for and returns how many. `capacity` caps it; the largest
 * table in the game is Shawn's, at 13.
 *
 * The entries with no monitor are counted separately in *unported, so
 * the caller can see how much of a wrestler's special-move repertoire
 * is missing rather than being told a number that looks complete.
 */
#define WM_SMOVE_MAX_PER_WRESTLER 16

size_t wm_smove_init(int32_t wrestler_num, bool is_drone,
                     wm_smove_run_t *runs, size_t capacity,
                     size_t *unported);

/*
 * WRESTLE.ASM:6141 reset_smoves: every SMOVE_PID process is rewound to
 * its own SM_RESET_ADDRESS -- the PWAKE init_smoves saved when it made
 * it, which is the routine's entry -- and given a PTIME of 1. Here
 * that is: back to step 0, disarmed, and alive again.
 */
void wm_smove_reset(wm_smove_run_t *runs, size_t count);

/*
 * WRESTLE2.ASM:3896 kill_smove_procs: every SMOVE_PID process whose
 * PA8 is this wrestler dies. Here, all of his.
 */
void wm_smove_kill(wm_smove_run_t *runs, size_t count);

/*
 * One tick of one monitor. Returns true on the tick it fires, with
 * `out` filled in; the caller applies the animation and the fields.
 *
 * A monitor that is `dead` does nothing, which is what a DIEd process
 * does.
 */
bool wm_smove_tick(wm_smove_run_t *run, wm_arcade_actor_t *a,
                   const wm_smove_env_t *env, wm_smove_fire_t *out);

/* The three routines' own `#TIMEOUT .equ` values. */
#define WM_SMOVE_TIMEOUT_FINISH 53   /* TAKER.ASM:610, `.equ TSEC` */
#define WM_SMOVE_TIMEOUT_DOINK  61   /* DOINK.ASM:1096 and :1219 */

/* DOINK.ASM:1136 `movi 15*60,a0` and :1270 `movi 8000h+12*60,a0`.
   Both are in 60ths, not TSEC ticks -- the source's own constants. */
#define WM_SMOVE_WALK_FAST_TIME (15 * 60)
#define WM_SMOVE_TAUNT_RISK     (0x8000u + 12u * 60u)

#ifdef __cplusplus
}
#endif
#endif
