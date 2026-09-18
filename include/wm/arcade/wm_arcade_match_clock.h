#ifndef WM_ARCADE_MATCH_CLOCK_H
#define WM_ARCADE_MATCH_CLOCK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The round clock. WRESTLE2.ASM:4098 SUBR match_timer, created once per
 * match by WRESTLE.ASM:1631 as its own TIMER_PID process, and the
 * LIFEBAR.ASM:5149 `#tmout` branch of set_winner that decides a round
 * when it reaches zero.
 *
 * Without it a round could only end by knockout or pin. Five other
 * translated routines already read @match_time and were all reading a
 * permanent zero:
 *
 *   ANIM.ASM:73 animate_wrestler stops animating entirely at zero;
 *   WRESTLE.ASM:2541 releases a wrestler's GETUP_TIME at zero;
 *   AWARD.ASM:996 is_it_a_really_quick_win scores the win by what is
 *     left on it, so every win scored zero and the award never fired;
 *   LIFEBAR.ASM's set_winner has a whole branch for it;
 *   and the main loop at WRESTLE.ASM:2102 ends the round on it.
 *
 * ------------------------------------------------------------------
 * WRESTLE.ASM:219 `BSSX match_time, 16*3` -- three WORDS, and the
 * order in memory is the order that BSSX comment gives: `frac, 1's,
 * 10's`, with @match_time itself the 10's. Every reader takes it as
 * one LONG, which on this machine puts the 10's in the low half and
 * the 1's in the high half -- so `cmpi 090009h,a0` is the test for
 * a full 99 and `jrz` is the test for expiry.
 */
typedef struct {
    /* @match_time: the tens digit, 0-9. */
    int32_t tens;
    /* @match_time+10h: the units digit, 0-9. */
    int32_t ones;
    /*
     * @match_time+20h: a 16-bit fractional remainder, and the only
     * part of this that is not a digit. It starts at zero and has
     * `rate` subtracted from it every tick; each time that borrows,
     * one unit comes off the clock. So the clock does NOT advance
     * once a second -- it advances every 65536/rate ticks.
     */
    uint16_t frac;
    /*
     * a10, chosen once at the top of match_timer and then constant
     * for the whole match. See wm_match_clock_rate.
     */
    int32_t rate;
    /*
     * `callr #create_timer / SLEEP TSEC*2` -- match_timer sets the
     * digits, makes the two on-screen digit objects, and then sleeps
     * for two seconds before its loop starts at all. So a match does
     * not begin losing time the instant the bell goes. Counted down
     * by wm_match_clock_tick; the between-round reset does NOT set
     * it again, because the process is one per MATCH and is long
     * past this sleep by round two.
     */
    int32_t start_delay;
} wm_match_clock_t;

/* DISPLAY.EQU:46 `TSEC equ 53`, and match_timer's own `SLEEP TSEC*2`. */
#define WM_MATCH_CLOCK_TSEC 53
#define WM_MATCH_CLOCK_START_DELAY (WM_MATCH_CLOCK_TSEC * 2)

/*
 * WRESTLE2.ASM:4339 #timer_table, `.asg 1500,BASETM` and five rows at
 * BASETM -30%, -15%, BASETM, +15%, +30%, indexed by the operator's
 * ADJSPEED (1 = slowest, 5 = fastest).
 *
 * The source's own comments on those rows say 76.6, 53.6 and 41.2
 * seconds a round, and the arithmetic here does not produce those
 * numbers: 99 units at 65536/1500 ticks each is 4325 ticks, and
 * DISPLAY.EQU:46's TSEC is 53, so the default row is about 82
 * seconds. The `.asg 1500,BASETM ;2100 ;16` line shows BASETM was
 * lowered from 2100 at some point and those comments were not
 * updated with it. The table is transcribed as it is, not as the
 * comments describe it.
 */
#define WM_MATCH_CLOCK_BASETM 1500
extern const int32_t wm_match_clock_timer_table[5];

/*
 * This port has no operator-settings system to read a live ADJSPEED
 * from, so it uses the arcade's own factory default rather than
 * inventing one: AUDIT.ASM's FACTORY_TABLE lists ADJSPEED (adjustment
 * 25) as 3, which is also match_timer's own `BADCHK a0,1,5,3`
 * fallback for an out-of-range read. Same reasoning, and the same
 * number, as wm_arcade_lifebar.h's WM_ARCADE_SPEED_ADJUSTMENT_16_16.
 */
#define WM_MATCH_CLOCK_ADJSPEED_DEFAULT 3

/*
 * The rate for one match. `adjspeed` is clamped to 1-5 and defaulted
 * to 3 exactly as BADCHK does; the three flags are the source's own
 * three slowdowns, applied in its order:
 *
 *   royal_rumble OR (a 1-on-3 with a player in the game) multiplies
 *     by 0xAAAA/65536 -- two thirds;
 *   then a final match, or the royal rumble again, halves it.
 *
 * The two can compound: a royal rumble gets both, ending at a third
 * of the normal rate, which is what the source's own comment ("slow
 * the clock to 1/3 speed if this is the royal rumble") describes.
 */
int32_t wm_match_clock_rate(int adjspeed, bool royal_rumble,
                            bool one_v_three, bool final_match);

/*
 * The top of match_timer: 99, no fraction, this match's rate, and the
 * two-second sleep before it starts counting. Once per MATCH.
 */
void wm_match_clock_start(wm_match_clock_t *c, int32_t rate);

/*
 * WRESTLE.ASM:2657 `;reset match_time` -- the same three stores and
 * nothing else, between rounds. The rate and the start delay belong
 * to the match's one timer process and are deliberately left alone.
 */
void wm_match_clock_reset(wm_match_clock_t *c);

/* True once both digits are zero: every reader's `jrz`. */
bool wm_match_clock_expired(const wm_match_clock_t *c);

/* The clock as a plain 0-99 count, which is what the digits are. */
int32_t wm_match_clock_value(const wm_match_clock_t *c);

/*
 * AWARD.ASM:996's own reading of it, which is not the same thing:
 * `andi 0fh,a0` takes the LOW half of the long -- the TENS digit --
 * multiplies it by ten, and adds the high half. So the arcade's
 * quick-win score is tens*10 + ones, the ordinary value; it is worth
 * spelling out because the halves look swapped and are not.
 */
int32_t wm_match_clock_award_score(const wm_match_clock_t *c);

/*
 * WRESTLE2.ASM:4297 #dec_timer, one tick of it. Returns true when a
 * digit actually changed, which is the only time the source does
 * anything else.
 *
 * `*warn` is set on a tick where the source plays sound 10, its time
 * warning: the test is on the two digits packed as BCD, `cmpi 10h,a0
 * / jrgt #no_change`, so it fires once a second from ten seconds
 * down. (The comment above it says "less than 15" and the code says
 * ten. The code is what shipped.)
 */
bool wm_match_clock_dec(wm_match_clock_t *c, bool *warn);

/*
 * WRESTLE2.ASM:4155 `#loop`, the whole gate around #dec_timer:
 *
 *   the two-second `SLEEP TSEC*2` the routine opens with, before
 *     `#loop` is reached at all;
 *   `move @HALT,a0 / jrnz #loop`      -- frozen, so do nothing;
 *   `move @match_time,a0,L / jrz #loop` -- already expired;
 *   `get_live_bits / cmpi 3,a3 / jrne #1tmded` -- and the clock does
 *     NOT run while either side is wiped out. That last one is easy
 *     to miss and changes the outcome: the five-second pin window
 *     after a knockout does not eat into the round.
 *
 * Returns true when a digit changed.
 */
bool wm_match_clock_tick(wm_match_clock_t *c, bool halt,
                         wm_arcade_actor_t *const *actors,
                         size_t actor_count, bool *warn);

/*
 * WRESTLE.ASM:2115 `#wraparound`: in attract mode (`@PSTATUS` zero --
 * nobody is playing) the clock does not end the round, it rolls back
 * to 99 and the demo keeps going. DEBUG's fight_debug does the same,
 * and is not modelled.
 */
void wm_match_clock_wrap(wm_match_clock_t *c);

/* ---- what happens when it runs out ------------------------------ */

/*
 * LIFEBAR.ASM:5149 set_winner's `#tmout` branch. The source's own
 * summary, and it is exactly what this does: "Award victory to the
 * team with the highest average life points remaining. In case of a
 * tie, winner is the last team to land a hit. If there have been no
 * hits, we'll wanna drop out and go straight to game over."
 *
 * Returns the winning PLYR_SIDE, or WM_MATCH_TIMEOUT_NO_WINNER when
 * nobody has landed a blow -- the source returns -1 there for the
 * same reason, and says so in a comment rather than picking someone.
 *
 * Not translated, because this port has neither: the is_8_on_1 and
 * royal_rumble override that hands the round to the CPU outright, and
 * the trailing "find the first live wrestler on the winning side"
 * search, which in a fixed two-man match can only ever find the one.
 */
#define WM_MATCH_TIMEOUT_NO_WINNER (-1)

int wm_match_timeout_winner(wm_arcade_actor_t *const *actors,
                            size_t actor_count);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_MATCH_CLOCK_H */
