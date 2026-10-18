#ifndef WM_ARCADE_BONUS_MESS_H
#define WM_ARCADE_BONUS_MESS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * LIFEBAR.ASM:3302 SUBR BONUS_MESS, under its own header comment:
 * "Process which displays high risk - 2x damage & possibly move name.
 *  This comes in from Secret button moves. A8 = * plyr proc of who fired
 *  off the move. A10 = # of move message. Messages display the first
 *  time only."
 *
 * The seam this replaces was ledgered as display: "The bonus text a
 * special move puts on screen, plus the award behind it. The message is
 * display; whether an award rides along -- as it does in
 * DO_REVERSAL_MESS, which is why that one got wired -- has to be read
 * off the callers."
 *
 * It does not have to be read off the callers, because it is not in
 * them. BONUS_MESS itself does three non-display things on every path
 * that gets past its first test:
 *
 *   - `movk 2,a1 / move a1,@DAM_MULT` (:3385) -- the damage multiplier.
 *   - `RND_AWARD a8,HIGH_RISK_AWD` (:3316 and :3387) -- on BOTH paths.
 *   - `MOVI 0BBH,A0 / CALLA triple_sound` -- the guitar.
 *
 * So every secret move in the game is meant to hit harder and score a
 * high-risk award, and in this port it did neither.
 *
 * TWO PATHS, DECIDED BY THE SIGN OF A10.
 *
 * `move a10,a10 / jrz #already / jrnn #reg` -- zero does nothing at all,
 * NEGATIVE falls into #tag, positive goes to #reg.
 *
 *   negative (#tag): ANIM.ASM:2230's taunt-style high risk. Its caller
 *       has ALREADY set DAM_MULT to 4 and passes -1, and #tag's own
 *       DAM_MULT write is commented out at :3320 -- so the caller's
 *       value stands and this path must not overwrite it.
 *   positive (#reg): the wrestler files' secret moves, passing a real
 *       move-message number. This path sets DAM_MULT itself, to 2.
 *
 * #reg also re-tests `*a8(RISK)` bit 15 and diverts to #tag if it is
 * set, so a high-risk secret move takes the taunt path whatever number
 * it passed.
 *
 * THE FIRST-TIME-ONLY FLAG GATES THE TEXT AND NOTHING ELSE. `jrnz
 * #already` on an already-set bit jumps to a label that is ABOVE the
 * DAM_MULT write and the award, so a repeat still hits harder and still
 * scores -- only the words are suppressed. Reading it the other way
 * would silently halve the damage of every secret move after its first
 * use.
 *
 * THE MULTIPLIER VALUES DISAGREE WITH BOTH COMMENTS, and the code is
 * what shipped. LIFEBAR.ASM:1451's own table is "MULT 2: damage *= 3/2,
 * MULT 3: *= 4/2, MULT 4+: *= 5/2", so BONUS_MESS's 2 is x1.5 where its
 * header says "2x damage", and ANIM.ASM's 4 is x2.5 where its comment
 * says "give 3x dmg".
 */

/* LIFEBAR.ASM:108 `BSSX message_flag,32*2`. */
#define WM_BONUS_MESS_FLAG_BITS 64

/*
 * WRESTLE.ASM:1621, in start_match: `clr a0 / move a0,@message_flag,L`.
 *
 * A LONG is 32 bits and the field is 64, so START_MATCH CLEARS ONLY THE
 * LOW HALF. A move whose message number is 32 or above keeps its
 * already-shown bit for the rest of the credit. That is what the
 * instruction does; it is preserved rather than tidied, and
 * wm_bonus_mess_reset_match is named for the half it really resets.
 */
#define WM_BONUS_MESS_FLAG_CLEARED_BITS 32

typedef struct {
    uint32_t shown[2];   /* the two longs message_flag spans */
} wm_bonus_mess_state;

/* What one call did, for a caller that owns the award, sound and text. */
typedef struct {
    bool ran;              /* false for the `jrz #already` no-op */
    bool high_risk_award;  /* RND_AWARD a8,HIGH_RISK_AWD */
    bool guitar;           /* triple_sound 0BBh */
    bool show_text;        /* the message_flag bit was clear */
    int32_t dam_mult;      /* 0 = leave the caller's value alone */
} wm_bonus_mess_result;

void wm_bonus_mess_init(wm_bonus_mess_state *s);

/* WRESTLE.ASM:1621's own clear -- the low 32 bits only. */
void wm_bonus_mess_reset_match(wm_bonus_mess_state *s);

/*
 * BONUS_MESS. `bonus` is A10 and its SIGN picks the path; `risk` is
 * *a8(RISK), whose bit 15 sends #reg to #tag.
 */
wm_bonus_mess_result wm_bonus_mess(wm_bonus_mess_state *s, int bonus,
                                   uint16_t risk);

#ifdef __cplusplus
}
#endif
#endif
