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
    /*
     * The `SLEEPK 20` (or `SLEEP 120`) every one of these routines
     * runs before looping back to #lp. It is not decoration: without
     * it a held button re-fires the move on the very next tick.
     */
    int32_t cooldown;
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
    /*
     * *a8(CLOSEST_NUM) resolved through process_ptrs -- the wrestler
     * WRESTLE.ASM:4489 get_opp_plyrmode reads, and the one the
     * grab_toss_air and charge families test the mode and ATTACK_TYPE
     * of. In a two-man match it is simply the other wrestler.
     */
    wm_arcade_actor_t *closest;
    /* @PCNT, for the one monitor that stamps SPECIAL_DAMAGE_TIME. */
    uint32_t pcnt;
    /* Filled in by wm_smove_tick for a generated monitor: the row
       whose constants this run is using. */
    const void *hdhold_row;
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
    /*
     * The head-hold family's own two: the bonus message's index
     * (`movk <n>,a10 / CREATE MESSAGE_PID,BONUS_MESS`), or -1 for a
     * move that awards none, and the IMMOBILIZE_TIME it puts on its
     * victim.
     */
    int32_t bonus;
    int32_t victim_immobilize;
    /* Who `victim_immobilize` is for: WHOIHIT on a slam, WHOHITME on
       a reversal, NULL when the move pins nobody. */
    wm_arcade_actor_t *victim;
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
    /* The trailing SLEEP, in ticks, before the routine loops. */
    int32_t cooldown;
    /*
     * Run once per tick during that sleep, with the ticks remaining.
     * One monitor needs it: SHAWN.ASM:570's flying-knee path waits
     * five ticks after the animation starts and only then writes
     * OBJ_YVEL, so the lift is part of the move rather than of the
     * frame that begins it.
     */
    void (*tail)(wm_arcade_actor_t *a, int32_t remaining);
    /*
     * The generated row this descriptor was built from, if any. One
     * of these four at most; the runtime hands it back through the
     * env so a single gate() and fire() can serve a whole family.
     */
    const struct wm_smove_hdhold *hdhold;
    const struct wm_smove_charge *charge;
    const struct wm_smove_grab *grab;
    const struct wm_smove_free *freemove;
} wm_smove_monitor_t;

/* ---- the head-hold family --------------------------------------- */

/*
 * 41 of the 62 monitors left after the three hand-written ones are
 * ONE routine written out with different constants. The shape, from
 * TAKER.ASM:748 und_hdhold_neckbrk and forty others:
 *
 *   #lp0  SLEEPK 1
 *   #lp   PLYRMODE is MODE_HEADHOLD or MODE_HEADHELD, else back to
 *         #lp0 -- so the monitor is live only while somebody has a
 *         head hold, either way round.
 *   #cont clr a11, then three WAITSWITCH_DWN: two stick inputs and a
 *         button, the first with no deadline and the other two
 *         sharing the routine's own #TIMEOUT.
 *   then  PLYRMODE decides WHO IS DOING IT. MODE_HEADHOLD means I
 *         have him and the move is mine; MODE_HEADHELD means he has
 *         me and the same input is a REVERSAL -- the source calls
 *         DO_REVERSAL and targets WHOHITME instead of WHOIHIT. Not
 *         every move has that path, which is why `reversal` is a
 *         column rather than an assumption.
 *   both  I_WILL_DIE and a non-zero IMMOBILIZE_TIME refuse; the
 *         target's IMMOBILIZE_TIME is set to 15;
 *         FIND_AND_KILL_ENDLESS; SPECIAL_MOVE_ADDR takes the move;
 *         SLEEPK 20; back to #lp.
 *
 * So six things vary, and they are what tools/wlsmove.py reads out of
 * the source into src/generated/smove_hdhold.c. Transcribing 41
 * copies of one routine by hand is what this tree generates instead.
 */
typedef struct wm_smove_hdhold {
    const char *name;
    const char *file;
    wm_smove_step_t step[3];
    int32_t timeout;
    /* `movk <n>,a10 / CREATE MESSAGE_PID,BONUS_MESS`, or -1 for a move
       that awards none. */
    int32_t bonus;
    /*
     * Which of the two modes the GATE lets in. Fifteen combo moves
     * are written `cmpi MODE_HEADHOLD,a0 / jrnz #lp0` and run only
     * for the man doing the holding; the rest admit both. Reading
     * both as admitting both hands a move to the wrong wrestler.
     */
    bool gate_headhold;
    bool gate_headheld;
    /*
     * And what a completed sequence then DOES for the man whose head
     * is held -- one of wm_smove_hh_result_t. Three answers, not two:
     * he reverses it (DO_REVERSAL, targeting WHOHITME), he performs
     * the same move himself (the fall-through lands on the slam
     * label, targeting WHOIHIT), or the tail tests MODE_HEADHOLD and
     * refuses him.
     */
    uint8_t headheld;
    /*
     * `SMRTTGT a8,WHOIHIT`. dnk_hdhold_buzz and yok_salt_throw call
     * it on neither path, so aiming their move at the nearest
     * wrestler would be this port's invention rather than the game's.
     */
    bool smart_target;
    /*
     * Two extra gates that appear before the sequence on some of
     * them, and refuse the whole monitor rather than one step:
     *
     *   `calla CHECK_COMBO_GO / jrlt #lp0` on fifteen combo moves,
     *   which are only available while a combo is running;
     *   `move *a8(GETUP_TIME),a0 / jrnz #lp0` on four of Shawn's,
     *   which refuse while he is getting up.
     */
    bool needs_combo;
    bool needs_getup_clear;
    /*
     * `movk 15,a14 / move a14,*a0(IMMOBILIZE_TIME)` -- how long the
     * move pins its target. Not one constant: seventeen say 15, five
     * say 32, 30 or 25, and eighteen have the write COMMENTED OUT and
     * pin nobody at all.
     */
    int32_t victim_immobilize;
    /* The animation SPECIAL_MOVE_ADDR takes. `anim_flipped` is
       non-NULL only where the source picks it with FACE24, in which
       case `anim` is the `_2_` form and this is the `_4_`. */
    const char *anim;
    const char *anim_flipped;
} wm_smove_hdhold_t;

extern const wm_smove_hdhold_t wm_smove_hdhold[];
extern const size_t wm_smove_hdhold_count;

/* The trailing `SLEEPK 20` every one of them shares. */
#define WM_SMOVE_HH_COOLDOWN 20

/*
 * Which of the two things the input did, once the sequence completes.
 * The source decides it on PLYRMODE at that moment, not on who
 * started the hold.
 */
typedef enum {
    WM_SMOVE_HH_NOTHING = 0,
    WM_SMOVE_HH_SLAM,        /* the move is mine, targeting WHOIHIT */
    WM_SMOVE_HH_REVERSAL     /* I am reversing his, targeting WHOHITME */
} wm_smove_hh_result_t;

/*
 * The tail every one of them shares, given the actor and the row.
 * `victim` is filled with WHOIHIT or WHOHITME as the outcome
 * requires, and `out` with the animation and the bonus index.
 *
 * Returns WM_SMOVE_HH_NOTHING when the guards refuse -- the wrong
 * mode, I_WILL_DIE, or an immobilised attacker.
 */
wm_smove_hh_result_t wm_smove_hdhold_fire(const wm_smove_hdhold_t *row,
                                          wm_arcade_actor_t *a,
                                          wm_arcade_actor_t **victim,
                                          wm_smove_fire_t *out);

/* ---- the guard lists -------------------------------------------- */

/*
 * The other three families all end the same way: a list of reasons
 * NOT to do the move, checked in order, any one of which sends the
 * monitor back to the top. What is in the list varies from routine to
 * routine more than anything else about them, so it is read out of
 * the source as data rather than flattened into columns.
 *
 * tools/wlsmove.py REFUSES a routine containing an instruction it
 * does not recognise here, which is the point: a test that quietly
 * failed to translate is a move that fires in this port and would not
 * in the arcade, and that is not a difference anyone would notice
 * until it mattered.
 */
typedef enum {
    /* `move *a8(PLYRMODE),a0 / cmpi MODE_x,a0 / jrz <top>` */
    WM_SMOVE_G_SELF_MODE = 0,
    /* the same on `calla get_opp_plyrmode`, which WRESTLE.ASM:4489
       shows IS process_ptrs[CLOSEST_NUM]->PLYRMODE, so the charge
       monitors' inline pointer walk is the same guard */
    WM_SMOVE_G_OPP_MODE,
    /* `move *a8(ANIMODE),a14 / btst MODE_UNINT_BIT,a14 / jrnz <top>` */
    WM_SMOVE_G_UNINT,
    /* `move *a8(GETUP_TIME),a0 / jrnz <top>` */
    WM_SMOVE_G_GETUP,
    /* `move *a8(IMMOBILIZE_TIME),a14 / jrnz <top>` */
    WM_SMOVE_G_IMMOBILIZE,
    /* `move *a8(I_WILL_DIE),a14 / jrnz <top>` */
    WM_SMOVE_G_I_WILL_DIE,
    /* WRESTLE.ASM:6016 ck_ignore / :6044 ck_ignore_a8 -- "if player is
       moving away from opponent... ignore button press". Both refuse
       on carry; the two spellings differ only in which register holds
       the wrestler. */
    WM_SMOVE_G_CK_IGNORE
} wm_smove_guard_kind_t;

#define WM_SMOVE_MAX_GUARDS 10

typedef struct {
    uint8_t kind;   /* wm_smove_guard_kind_t */
    uint8_t mode;   /* WM_PMODE_*, for the two mode guards */
} wm_smove_guard_t;

/* True when every guard in the list passes -- i.e. none of them
   refuses. `opp` may be NULL, in which case an opponent-mode guard
   cannot refuse, exactly as a null process pointer would not. */
bool wm_smove_guards_pass(const wm_smove_guard_t *guard, size_t count,
                          const wm_arcade_actor_t *a,
                          const wm_arcade_actor_t *opp);

/* ---- the charge family ------------------------------------------ */

/*
 * BRET.ASM:543 hrt_charge_flying_kick and five others. These have no
 * input sequence at all, which is why the head-hold reader refuses
 * them rather than reading them badly:
 *
 *   #start_over  #CHARGE_TIME = 0
 *   #loop1       SLEEPK 1; ++#CHARGE_TIME; button still held -> #loop1
 *                #CHARGE_TIME < threshold -> #start_over
 *                <the guard list>
 *                SPECIAL_MOVE_ADDR = <anim>; [SETMODE]; back to the top
 *
 * `#CHARGE_TIME .equ SM_USRW1` is a word on the MONITOR's own process,
 * not on the wrestler, so each of the six counts independently and a
 * wrestler can be charging two of them at once.
 */
typedef struct wm_smove_charge {
    const char *name;
    const char *file;
    /* The WM_BTN_* bit tested in BUT_VAL_CUR, held not newly pressed. */
    uint16_t button;
    /* `cmpi 100,a14 / jrlt #start_over` -- all six say 100. */
    int32_t threshold;
    wm_smove_guard_t guard[WM_SMOVE_MAX_GUARDS];
    size_t guards;
    const char *anim;
    /* Shawn's and Bam Bam's have a second animation for a wrestler
       who is already running: `cmpi MODE_RUNNING,a0 / jrnz #cont`. */
    const char *anim_running;
    /* `SETMODE INAIR` on the two flying kicks; -1 for no mode change. */
    int32_t set_mode;
    /* The trailing SLEEP before #start_over, in ticks; 0 for none. */
    int32_t cooldown;
} wm_smove_charge_t;

extern const wm_smove_charge_t wm_smove_charge[];
extern const size_t wm_smove_charge_count;

/*
 * One tick of a charge monitor. `held` is the counter the source
 * keeps on the monitor's process; it is incremented or zeroed in
 * place. Returns true when the move fires.
 */
bool wm_smove_charge_tick(const wm_smove_charge_t *row, int32_t *held,
                          wm_arcade_actor_t *a, const wm_smove_env_t *env,
                          wm_smove_fire_t *out);

/* ---- the grab_toss_air family ----------------------------------- */

/*
 * TAKER.ASM:1149 und_grab_toss_air and seven others, one per
 * wrestler. Three switch steps -- away, away, punch -- and then a
 * TWO-WAY choice the head-hold row cannot hold, which is why these
 * eight were the largest group the first pass refused:
 *
 *   an opponent who is MODE_INAIR, MODE_INAIR2, or in the middle of
 *   an AT_LEAPING attack takes the `2` animation wherever he is;
 *   anyone else has to be within CLOSEST_DIST of the threshold, and
 *   takes the `1` animation.
 *
 * The threshold is per wrestler (68h, 6ch or 70h), and five of the
 * eight pick each animation with FACE24 while three name it outright.
 */
typedef struct wm_smove_grab {
    const char *name;
    const char *file;
    wm_smove_step_t step[3];
    int32_t timeout;
    /* `move *a8(CLOSEST_DIST),a0 / cmpi 68h,a0 / jrgt #lp`. */
    int32_t near_dist;
    const char *near_anim;
    const char *near_anim_flipped;
    const char *air_anim;
    const char *air_anim_flipped;
    /* The Undertaker alone drops his attachment: `clr a0 / move
       a0,*a8(ATTACH_PROC),L`. */
    bool clear_attach;
    int32_t set_mode;      /* SETMODE NORMAL on the Undertaker; -1 else */
    int32_t cooldown;
} wm_smove_grab_t;

extern const wm_smove_grab_t wm_smove_grab[];
extern const size_t wm_smove_grab_count;

/* ---- the free-move family --------------------------------------- */

/*
 * The head-hold shape with the head-hold GATE taken off: three switch
 * steps, a guard list, one animation. These are done from a neutral
 * stance, and four of the six actively REFUSE while anyone has a head
 * hold -- which is how they were first read, because matching on the
 * mode names alone sees MODE_HEADHOLD and MODE_HEADHELD in the body
 * and cannot tell a requirement from a refusal. Four moves were
 * gated on the exact opposite of their real condition.
 */
typedef struct wm_smove_free {
    const char *name;
    const char *file;
    wm_smove_step_t step[3];
    int32_t timeout;
    /* Tested before the sequence starts and again on every restart:
       the Undertaker's two spirit moves refuse while he is in a head
       hold either way round. */
    wm_smove_guard_t gate[WM_SMOVE_MAX_GUARDS];
    size_t gates;
    /* And these once it completes. */
    wm_smove_guard_t guard[WM_SMOVE_MAX_GUARDS];
    size_t guards;
    const char *anim;
    /* `clr a0 / move a0,*a8(RUN_TIME)` -- the move stops a run. */
    bool clear_run_time;
    int32_t set_mode;
    int32_t cooldown;
} wm_smove_free_t;

extern const wm_smove_free_t wm_smove_free[];
extern const size_t wm_smove_free_count;

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
