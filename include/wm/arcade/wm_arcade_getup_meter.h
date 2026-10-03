#ifndef WM_ARCADE_GETUP_METER_H
#define WM_ARCADE_GETUP_METER_H

/*
 * WRESTLE2.ASM:950 getup_meter, :1180 slide_offscr, :1235
 * ditch_getup_meter_a9 and :1242 ditch_getup_meter -- the bar that comes
 * up beside a downed wrestler while he mashes his way back to his feet.
 *
 * getup_meter is a PROCESS, and like announce_rnd_winner it is modelled
 * as a state machine ticked once per frame rather than as a coroutine.
 * What is here is its decisions; what is not here is BEGINOBJ, OXVAL,
 * OSIZEY and OSAG, which are the display's, the same split the life bar
 * already makes.
 *
 * The routine matters beyond the picture, and that is why it is here:
 * slide_offscr's first instruction is `movi 18*60,a0 / move a0,
 * *a10(DELAY_METER)`, and nothing in this port was writing it. DELAY_METER
 * is a real gate -- wm_arcade_roll.c and wm_arcade_combat.c both read it
 * -- so without this a meter that had just gone away could come straight
 * back, and the eighteen seconds the arcade makes you wait did not exist.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WRESTLE2.ASM:938-943, the block's own equs. INV_MULT is declared there
   and used nowhere, so it is not carried. */
#define WM_GETUP_SIZE 80          /* `GETUP_SIZE equ 80 ;102 ;174 ;99` */
#define WM_GETUP_MAX_TIME (6 * 53)     /* `MAX_TIME equ 6*TSEC` */
#define WM_GETUP_ONSCR_X 173
#define WM_GETUP_OFFSCR_X 221

/* GAME.EQU:510 `FLUNG_TIME .equ 170-20-30`. */
#define WM_FLUNG_TIME (170 - 20 - 30)

/* slide_offscr's `movi 18*60,a0`, with the source's own abandoned value
   still beside it: `;13`. */
#define WM_GETUP_METER_DELAY (18 * 60)

/* The ten ticks slide_offscr spends before it starts asking whether the
   meter may come out. It is a one-off grace and NOT a period: the source
   decrements a11 down to zero and never reloads it, so from the eleventh
   tick on the question is asked every tick. */
#define WM_GETUP_METER_GRACE 10

/* `movi 120,a6` -- the tick the dufus-message check fires on, once. */
#define WM_GETUP_DUFUS_TICK 120
/* `cmpi 175,a5` -- how little getup time may have been burned off in
   those 120 ticks before the game starts explaining the buttons. */
#define WM_GETUP_DUFUS_BURN 175
/* `MOVI 0BDH,A0 / CALLA triple_sound` -- the meter announce sound. */
#define WM_GETUP_METER_SOUND 0xBD

/*
 * getup_meter's opening decision, in the order the source makes it:
 *
 *   - A human always gets a meter.
 *   - A drone with a HUMAN teammate never does, and the source does more
 *     than decline: `#die` clears METER_PROC and kills the process.
 *   - A lone drone gets one only if drone_meters_on is set and NUM_OPPS
 *     is less than 2.
 *
 * The commented-out `move @GETUP_POWER,a14` above the live
 * `move @drone_meters_on,a14` is the source's own history and is left
 * as history: drone_meters_on is what it reads.
 *
 * `actors`/`actor_count` stand in for the `process_ptrs` walk, which
 * skips inactive slots, itself and the other team.
 */
typedef enum {
    /* `#yes` */
    WM_GETUP_METER_YES = 0,
    /* `#die` -- and METER_PROC is cleared on the way out. */
    WM_GETUP_METER_NO
} wm_getup_meter_eligibility_t;

wm_getup_meter_eligibility_t wm_arcade_getup_meter_eligible(
    wm_arcade_actor_t *actor, wm_arcade_actor_t *const *actors,
    size_t actor_count, bool drone_meters_on, int32_t num_opps);

/*
 * The royal-rumble hack, which the source labels as one and explains in
 * full: "In royal rumble mode, player 1 is on PLAYER 0's TEAM, so this
 * code is gonna want to put his getup meter on the left. To get it over
 * on the right where it belongs, we temporarily put him on the other
 * team. This shouldn't break anything."
 *
 * Returns the side the meter is drawn for, which is PLYR_SIDE except for
 * that one case. It is the only thing the hack affects -- a9 is read for
 * the X sign and for RECVR_L vs RECVR_R and for nothing else.
 */
int32_t wm_arcade_getup_meter_side(int32_t player_side, int32_t player_num,
                                   bool royal_rumble);

/*
 * The offscreen X, as the source computes it: `movi [OFFSCR_X,0],a10`,
 * and then for side 0 `dec a10 / neg a10`.
 *
 * The decrement is on the whole LONG, not on the integer half, so it
 * costs one 1/65536th and nothing else: 221<<16 decremented and negated
 * is 0xff230001, whose integer half is still -221 and whose fraction is
 * 1 instead of 0. Carried as written, and written down as harmless,
 * because the interesting thing about it is that it is harmless -- the
 * instruction looks like an off-by-one on a screen position and is not
 * one.
 */
int32_t wm_arcade_getup_meter_offscr_x(int32_t side);

/*
 * #set_x, which is why the routine is called slide_offscr rather than
 * put_offscr: it does not snap the objects to the target, it moves them
 * a QUARTER of the remaining distance each tick (`sub a1,a0 / sra 2,a0`)
 * and adds that same step to every object, computed once from the
 * frame's own OXVAL.
 *
 * `target` is the screen X the source builds as `[ONSCR_X,0]` or
 * `[OFFSCR_X,0]`, negated for side 0, plus `[200-1,0]`. Returns the
 * step to add, which is a signed arithmetic shift and so eases in from
 * either direction and stalls three quarters of a unit short.
 *
 * The loop that applies it is `movk 3-1,a1` over IPTR_FRAME and
 * IPTR_GREEN -- two objects, with a counter that says three. Whatever
 * the third one was, it is not in this listing.
 */
int32_t wm_arcade_getup_meter_target_x(int32_t side, bool onscreen);
int32_t wm_arcade_getup_meter_x_step(int32_t target, int32_t current);

/*
 * slide_offscr's `#update` test: may the meter come out?
 *
 * THE HEALTH CHECK IS DEAD, and this is the finding worth keeping. The
 * source reads:
 *
 *      calla get_health
 *      cmpi  20,a0
 *      jrgt  #norm
 *      move  *a10(GETUP_TIME),a14
 *      cmpi  FLUNG_TIME,a14
 *      jrz   #onscr
 *   #norm
 *      move  *a10(GETUP_TIME),a14
 *      jrnz  #onscr
 *
 * with the comment "If health meter is down low, don't have getup meter
 * come out. Unless it was a fling!" above it. But the low-health arm
 * FALLS THROUGH into #norm, and #norm shows the meter whenever GETUP_TIME
 * is non-zero -- which FLUNG_TIME (120) is. So both arms reach the same
 * answer for every health value there is, and the suppression the comment
 * describes was never assembled. get_health is taken as a callback anyway,
 * called exactly where the source calls it, so a test can demonstrate
 * that rather than take my word for it.
 *
 * The three gates above it are real and do refuse: the wrestler who hit
 * him is mid-combo, DELAY_METER has not run out, or he is MODE_DEAD.
 */
typedef struct {
    /* LIFEBAR.ASM get_health, as the source calls it: `move
       *a10(PLYRNUM),a1 / calla get_health`. May be NULL, which stands for
       the health the port cannot see yet; it changes no answer. */
    int32_t (*get_health)(const wm_arcade_actor_t *actor, void *user);
    void *user;
} wm_getup_meter_env_t;

bool wm_arcade_getup_meter_may_show(const wm_arcade_actor_t *actor,
                                    const wm_getup_meter_env_t *env);

/*
 * One step of #update_meter's smoothing, which is the whole of what the
 * displayed bar is: the new scaled value is averaged with the old
 * (`add a0,a7 / srl 1,a7`) and stored back into DISPLAY_VAL. The green
 * bar's height is that value; `a1`, the rows cropped off the top of
 * RECVRBLK, is GETUP_SIZE minus it, clamped both ends.
 *
 * `srl` is a LOGICAL shift, so this is an unsigned average; DISPLAY_VAL
 * is a WORD in the process's own PDATA and never goes negative.
 */
int32_t wm_arcade_getup_meter_smooth(int32_t display_val, int32_t scaled);
int32_t wm_arcade_getup_meter_crop(int32_t display_val);

/*
 * The scale: `mpyu GETUP_SIZE,a7 / divu a11,a7`, where a7 is the current
 * GETUP_TIME and a11 the value it started at.
 *
 * Both of the source's two guards are here, and the second one is
 * strange enough to be worth naming. The first is plain: a current
 * getup time above the starting one would put the scale over the top, so
 * `a11` is raised to it. The second fires when the freshly scaled value
 * is bigger than DISPLAY_VAL -- "has getup been incremented?" -- and its
 * fix is `move a7,a11` followed by the same multiply and divide, with a7
 * now equal to a11. That pegs the bar at exactly GETUP_SIZE and leaves
 * a11 holding a SCALED value, which every later tick then divides by.
 * It is not what the comment intends, and it is what the arcade does.
 *
 * `*start_getup` is a11, carried in and out because the source keeps it
 * in a register across the whole onscreen loop and both guards write it.
 */
int32_t wm_arcade_getup_meter_scale(int32_t getup_time, int32_t display_val,
                                    int32_t *start_getup);

/*
 * getup_meter as a ticked process.
 *
 * WM_GETUP_PHASE_NONE is "no process", which is what `#die` leaves and
 * what a wrestler who was never eligible has. The other two are the
 * source's own two loops.
 */
typedef enum {
    WM_GETUP_PHASE_NONE = 0,
    WM_GETUP_PHASE_OFFSCR,   /* slide_offscr's #offscr_loop */
    WM_GETUP_PHASE_ONSCR     /* #onscr_loop */
} wm_getup_meter_phase_t;

typedef struct {
    uint8_t phase;
    /* a11 in #offscr_loop: the ten-tick grace, counted down once. */
    int16_t grace;
    /* a11 in #onscr_loop: the getup time the bar is scaled against. */
    int32_t start_getup;
    /* DISPLAY_VAL, the process's own PDATA word. */
    int32_t display_val;
    /* a6, counting down to the one dufus-message check. */
    int16_t dufus_countdown;
    /* a5: GETUP_TIME as it was when the meter came out, which the dufus
       check subtracts the current value from. */
    int32_t getup_at_onscr;
    /* The side the hack settled on, for the display. */
    int32_t side;
} wm_getup_meter_t;

/* What one tick asks the display for. Nothing here is state. */
typedef struct {
    bool visible;          /* on the ONSCR_X mark rather than OFFSCR_X */
    int32_t x;             /* the X the three objects are centred on */
    int32_t green_height;  /* OSIZEY for the green bar */
    int32_t crop_rows;     /* the rows of RECVRBLK skipped, via OSAG */
    bool announce_sound;   /* the tick `triple_sound 0BDh` is reached */
    bool dufus_message;    /* the tick `CREATE AWARD_PID,dufus_msg_on` is */
} wm_getup_meter_frame_t;

/* `SLEEPK 2`, then the eligibility test, then slide_offscr. Returns
   false when the process dies before it ever draws anything -- which
   also clears METER_PROC, as `#die` does. */
bool wm_arcade_getup_meter_start(wm_getup_meter_t *st,
                                 wm_arcade_actor_t *actor,
                                 wm_arcade_actor_t *const *actors,
                                 size_t actor_count,
                                 bool drone_meters_on, int32_t num_opps,
                                 bool royal_rumble);

/* One tick of whichever loop the process is in. */
void wm_arcade_getup_meter_tick(wm_getup_meter_t *st,
                                wm_arcade_actor_t *actor,
                                const wm_getup_meter_env_t *env,
                                wm_getup_meter_frame_t *out);

/*
 * WRESTLE2.ASM:1242 ditch_getup_meter -- "makes your getup meter go away
 * if you've got one out."
 *
 * Two guards, and they are both the wrong way round from what the
 * comment under them suggests: it does nothing unless GETUP_TIME is
 * non-zero AND PLYR_DIZZY is zero. Then, if METER_PROC is set, it
 * XFERPROCs that process to slide_offscr -- which is to say the meter
 * restarts at the top of the offscreen loop, and so re-stamps
 * DELAY_METER with its eighteen seconds.
 *
 * Returns true when the transfer happened.
 */
bool wm_arcade_ditch_getup_meter(wm_arcade_actor_t *actor,
                                 wm_getup_meter_t *st);

/*
 * WRESTLE2.ASM:1235 ditch_getup_meter_a9. Three instructions: push a13,
 * move a9 into it, call ditch_getup_meter, pull. It is the same routine
 * against a DIFFERENT wrestler -- whoever a9 holds at the call site, not
 * the process's own a13 -- and that is the only reason it exists.
 * WRESTLE2.ASM:2271 is its one caller.
 */
bool wm_arcade_ditch_getup_meter_a9(wm_arcade_actor_t *other,
                                    wm_getup_meter_t *st);

#ifdef __cplusplus
}
#endif
#endif
