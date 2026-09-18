#ifndef WM_ARCADE_ROUND_RESET_H
#define WM_ARCADE_ROUND_RESET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wmania_rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE.ASM:2630 reset_for_round and :2797 reset_for_round2 -- what
 * puts the wrestlers back on their marks between rounds.
 *
 * "Reset world and both wrestlers for the start of a new round -
 * Called from lifebar", says the source's own comment, and that is
 * the gap this closes: wm/arcade/wm_arcade_round.h has been saying
 * "this port has no round-2 restart at all, so match_winner becoming
 * nonzero is currently a dead end". A round ended and nothing
 * happened. These two routines are the restart.
 *
 * They are two routines rather than one because they do different
 * kinds of work, not because anything happens between them:
 * LIFEBAR.ASM:3098-3099 calls them back to back, in that order, from
 * the `WRESTLERS_RESET` block whose own comment is "Cause wrestlers to
 * re-appear in the correct spot to start the next round". The first
 * puts the world back -- positions, facings, the round counter, life,
 * the special-move watchdogs; the second clears the per-round state on
 * each wrestler and wakes anyone who was KO'd. init_scroller is called
 * a few lines further down the same block, which is why it is here
 * too.
 */

/* ---- reset_wrestle (WRESTLE.ASM:2666) ---------------------------- */

/*
 * WRESTLE.ASM:2887's `#team1_starts` / `#team2_starts`: three rows per
 * side of X, Z, facing (and a fourth unused word). This port fixes
 * one wrestler per side, so only row 0 of each is reachable -- but
 * the rule that PICKS the row is translated rather than assumed,
 * because it is the interesting part: the index is "the number of
 * teammates with PLYRNUM's lower than ours", counted by walking
 * process_ptrs and stopping at yourself.
 */
typedef struct {
    int32_t x;
    int32_t z;
    int32_t facing;
} wm_round_start_t;

#define WM_ROUND_STARTS_PER_TEAM 3
extern const wm_round_start_t wm_round_team_starts[2][WM_ROUND_STARTS_PER_TEAM];

/*
 * The index into his side's table: how many ACTIVE wrestlers on the
 * same side appear before him in the process list. The source's loop
 * skips inactive slots and stops when it reaches him; a wrestler who
 * is not in the list at all falls off the end, and the source would
 * then read past the table, so this refuses with -1 instead.
 */
int wm_round_start_index(const wm_arcade_actor_t *a,
                         wm_arcade_actor_t *const *actors, size_t count);

/*
 * One wrestler's reset_wrestle. Position, facing, ground, velocities,
 * modes and the two timers, in the source's order.
 *
 * Two of its lines are worth stating outright:
 *
 *   `movi 14*60,a0 / move a0,*a13(DELAY_METER)` carries the source's
 *   own comment, "Don't allow meters for the first x seconds of
 *   round" -- fourteen seconds, in 60ths rather than TSEC ticks.
 *
 *   `clr a0 / move a0,*a13(INRING)`. INRING is ZERO inside the ring
 *   in this source, so clearing it puts him IN. This port keeps
 *   `in_ring` as an ordinary boolean, so it is SET here. The sense
 *   flips on purpose; getting it backwards would start every round
 *   with both wrestlers counted out.
 *
 * The source also calls drone_change_back and ani_init at this point
 * and clears ANIMODE2 and PLYR_DIZZY_CNT. The first two belong to the
 * caller's backend, and this port carries neither of the last two
 * fields -- there is one anim_mode on the actor, not two, and nothing
 * reads a separate dizzy counter. Returns the animation the caller
 * should (re)start him on: NULL, since ani_init's choice is the
 * backend's own per-wrestler table.
 */
void wm_round_reset_wrestler(wm_arcade_actor_t *a,
                             wm_arcade_actor_t *const *actors, size_t count,
                             uint32_t pcnt);

/* "Don't allow meters for the first x seconds of round" -- in 60ths. */
#define WM_ROUND_DELAY_METER (14 * 60)

/* ---- reset_wrestle2 (WRESTLE.ASM:2816) --------------------------- */

/*
 * PLYR.EQU:443 `SF_RESET_MASK equ M_TEMP_PAL|M_DID_BUCKOFF` -- the
 * only two STATUS_FLAGS bits that survive a round. Everything else,
 * including PINNED, DID_PIN, KOD, ZOMBIE and GUY_UP, is cleared.
 */
#define WM_SF_RESET_MASK (WM_STATUS_TEMP_PAL | WM_STATUS_DID_BUCKOFF)

/* `movk 30,a0 / move a0,*a13(IMMOBILIZE_TIME)` -- half a second of
   nobody moving while the round announcement clears. */
#define WM_ROUND_IMMOBILIZE 30

/*
 * The second pass over one wrestler: the per-round state, the flag
 * mask, and the PTIME of 1 the source sets "just in case they were
 * KO'd last round".
 *
 * Of the fourteen fields it clears, this port carries eleven. The
 * three it does not are PLYR_DIZZY_CNT, AUTO_PIN_CNTDOWN,
 * LAST_FLING_ATTEMPT and HIT_GATE_TIME -- no routine translated here
 * reads any of them, so there is nothing to clear rather than a field
 * added to be zeroed.
 */
void wm_round_reset_wrestler2(wm_arcade_actor_t *a);

/* ---- update_links (WRESTLE.ASM:3003) ----------------------------- */

/*
 * Five instructions, and a rule the attachment system depends on: if
 * the man I am attached to is not attached back to ME, the link is
 * stale and mine is dropped. It runs once a tick per wrestler, and it
 * is what stops a puppet being dragged by somebody who has already
 * let go.
 *
 * Note it only breaks ITS OWN side of the link. The other wrestler
 * keeps whatever he has, and clears it on his own tick if it is his
 * turn to be wrong.
 */
void wm_round_update_links(wm_arcade_actor_t *a);

/* ---- init_scroller (WRESTLE.ASM:6688) ---------------------------- */

/*
 * Where the camera starts, which this port left at the origin.
 *
 * X is RING_X_CENTER-200 -- half a screen left of the ring's middle,
 * so the ring is centred on the first frame rather than sliding in.
 * Y is a NEGATIVE 16.16 value, and which one depends on the match:
 * `[0ffe5h,0]` normally, `[0ffe9h,0]` when NUM_OPPS is exactly 2.
 * The source writes the two as raw hex words, so they are -27 and
 * -23 pixels.
 */
void wm_round_init_scroller(int32_t *worldtlx, int32_t *worldtly,
                            int32_t num_opps);

#define WM_SCROLL_START_Y_DEFAULT ((int32_t)0xffe50000)
#define WM_SCROLL_START_Y_1V2     ((int32_t)0xffe90000)

/* ---- show_most_damage (WRESTLE.ASM:1359) ------------------------- */

/*
 * "Player N did X% of the damage", the postgame line. The text and
 * its two message objects are display; the two numbers behind it are
 * not, and they are the whole reason the routine walks the process
 * list.
 *
 * DRONES ARE SKIPPED -- `move *a9(PLYR_TYPE),a10 / jrnz` -- so the
 * total it divides by is the HUMANS' damage only, not the match's.
 * With one human on the winning side that makes the answer 100% every
 * time, which is the source's own behaviour and not a rounding
 * artefact.
 *
 * The tie goes to player 2: `cmp a8,a9 / jrlt #p1_most` takes player 1
 * only when his total is strictly greater.
 *
 * Returns the 1-based player number, and writes the percentage --
 * `mpyu 100 / divu total`, an unsigned multiply then divide, so it
 * truncates. Returns 0 when no human did any damage at all, which the
 * source does not guard against (it would divide by zero).
 */
int wm_round_most_damage(const int32_t *damage_given, size_t count,
                         const int32_t *plyr_type, int32_t *out_percent);

/* ---- loser_snd (WRESTLE2.ASM:3987) ------------------------------- */

/*
 * "Call this when a match has just ended. Does an appropriate sound if
 * someone's winning streak has just ended." Three things gate it, and
 * each one narrows it a lot:
 *
 *   TWO HUMANS ONLY. `cmpi 3,a14` on PSTATUS -- a streak broken by a
 *   drone gets nothing said about it.
 *
 *   The LOSER's streak, not the winner's. `move @match_winner,a1 /
 *   NOT A1 / ANDI 3,A1 / DEC A1` turns the winner (1 or 2) into the
 *   other player's index: NOT 1 is ...fffe, masked to 2, minus 1 is
 *   1; NOT 2 is ...fffd, masked to 1, minus 1 is 0.
 *
 *   And he must HAVE had one: p1oldwinstreak / p2oldwinstreak, the
 *   value from before this match, non-zero.
 *
 * Then RNDRNG0(2) picks one of three lines. The source keeps four
 * more commented out above and below the live three, which is why the
 * table is exactly this length and not longer.
 *
 * Returns the sound id to queue through ADD_VOICE, or 0.
 */
uint16_t wm_round_loser_snd(int32_t pstatus, int32_t match_winner,
                            const int32_t *old_winstreak, WmRng *rng);

/* SOUND.EQU's three, in the source's own order. */
#define WM_SND_SOMEHOW_I_DONT_THINK  0x1BAu
#define WM_SND_L_BACK_TO_SANDBOX     0x301u
#define WM_SND_ARE_YOU_TOUGH_ENOUGH  0x191u

/* ---- maybe_do_flashes (WRESTLE2.ASM:4014) ------------------------ */

/*
 * A five-tick poll that fires the high flashes while the camera is
 * looking high enough, and dies the moment reduce_bog goes up. Three
 * lines of logic and one call:
 *
 *   reduce_bog non-zero -> DIE, permanently. The machine is already
 *   struggling and this is the first thing to go.
 *
 *   `CMPI [>ff97,0],A0 / JRGT #top` on WORLDTLY -- a SIGNED compare
 *   against a negative 16.16 value, so the flashes only run while the
 *   camera is scrolled ABOVE that line. Below it, it just polls.
 *
 *   Otherwise START_HI_FLASHES and sleep 30 rather than 5, so a burst
 *   is followed by a longer pause.
 *
 * Returns true on a tick that should fire. The caller owns the two
 * sleeps and the flashes themselves.
 */
bool wm_round_maybe_do_flashes(int32_t worldtly, int32_t reduce_bog);

#define WM_FLASHES_Y_LIMIT ((int32_t)0xff970000)
#define WM_FLASHES_POLL_TICKS 5
#define WM_FLASHES_REST_TICKS 30

#ifdef __cplusplus
}
#endif
#endif
