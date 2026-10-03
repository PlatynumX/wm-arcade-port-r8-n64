#ifndef WM_ARCADE_PLYR_START_H
#define WM_ARCADE_PLYR_START_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE.ASM:663 plyr_strtb1 / plyr_strtb2 -- somebody pressed
 * start.
 *
 * Two entry points with one body: plyr_strtb1 loads a8 = 0 and
 * plyr_strtb2 loads a8 = 1, and every difference between the two
 * players after that is a pointer picked by that one bit. It is the
 * routine that lets a second player join a game already in progress,
 * and it is also how the first one starts, which is why the exit at
 * the bottom is a seven-way branch on GAMSTATE rather than a call.
 *
 * The order matters: the guards come first, then the per-player
 * state is wiped WHETHER OR NOT the credit check later succeeds, and
 * only then is the credit taken. A player who presses start with no
 * credit has still had his win streak and his pin times cleared by
 * the time he is turned away.
 */

/* GAME.EQU:309-319. INDIAG is "any negative". */
#define WM_GAMSTATE_INDIAG (-1)
#define WM_GAMSTATE_INAMODE 1
#define WM_GAMSTATE_INSELECT 2
#define WM_GAMSTATE_INPREGAME 3
#define WM_GAMSTATE_INGAME 4
#define WM_GAMSTATE_INPARTY 6
#define WM_GAMSTATE_INGAMEOVER 7
#define WM_GAMSTATE_INPREGAME2 9

typedef enum {
    /* `#die` -- the process ends and nothing happens. */
    WM_PLYR_START_DIE = 0,
    /* `#start_from_waitcont`: this player was in the game a moment
       ago (OLD_PSTATUS) and is continuing, not joining. */
    WM_PLYR_START_WAITCONT,
    WM_PLYR_START_AMODE,
    WM_PLYR_START_GAMEOVER,
    WM_PLYR_START_SELECT,
    WM_PLYR_START_PREGAME,
    /* INPREGAME2 and INGAME both land here. */
    WM_PLYR_START_MIDGAME,
    /*
     * `LOCKUP` -- the source's own "this cannot happen" trap, which
     * on hardware halts the machine. Two reach it: a GAMSTATE the
     * dispatch does not name, and OLD_PSTATUS set while GAMSTATE is
     * not INSELECT. Reported rather than halting.
     */
    WM_PLYR_START_IMPOSSIBLE
} wm_plyr_start_outcome_t;

/*
 * The per-player state the routine clears before it decides
 * anything. Every field is one of the pointers chosen by a8.
 */
typedef struct {
    /* p1winstreak / p2winstreak, a WORD. */
    int16_t winstreak;
    /*
     * p1winstreakd / p2winstreakd, also a word, and the one
     * exception: `move *a4,a0 / jrn #a4ok` leaves it alone when it
     * is NEGATIVE. A negative winstreakd survives a start press.
     */
    int16_t winstreakd;
    /* entered_inits, a long. */
    int32_t entered_inits;
    /* process_ptrs[player], a long -- the player's process is
       unhooked here, not killed. */
    int32_t process_ptr;
    /*
     * MATCH_TIMERS[player], a long: the accumulated pin time. Named
     * for what it holds rather than for the array, because
     * WRESTLE2.ASM:4098 has a routine called match_timer and it is
     * a different thing entirely (the round clock).
     */
    int32_t pin_time_total;
} wm_plyr_start_state_t;

/*
 * What the routine did, so a caller can see the calls it would have
 * made without this having to make them.
 */
typedef struct {
    wm_plyr_start_outcome_t outcome;
    int player;                 /* a8: 0 or 1 */
    /* `calla rst_winstreak_awards` with p1ws_award / p2ws_award. */
    bool awards_reset_due;
    /* `calla reset_dufus_msgs` with a0 = the player. */
    bool dufus_msgs_reset_due;
    /* `calla clear_icon_total` with a0 = the player. */
    bool icon_total_clear_due;
    /* Whether the state above was wiped at all. */
    bool cleared_state;
    /* `calla CR_STRTP / jalo #die` -- was a credit taken? Only the
       joining path reaches it; the continue path does not. */
    bool took_credit;
} wm_plyr_start_result_t;

/*
 * The three guards, in the source's order:
 *
 *   `move @GAMSTATE,a0 / jrn #die`   -- any negative GAMSTATE is
 *                                        the diagnostic menu.
 *   `cmpi INPARTY,a0 / jreq #die`    -- the winner's party.
 *   `move @PSTATUS,a14 / btst a8,a14 / jrnz #die` -- already in.
 */
bool wm_plyr_start_allowed(int32_t gamstate, int32_t pstatus, int player);

/*
 * `player` is 0 for plyr_strtb1 and 1 for plyr_strtb2. `credit_ok`
 * is what CR_STRTP would answer, consulted only on the joining path.
 * `state` is cleared in place when the guards pass.
 */
wm_plyr_start_result_t wm_plyr_start(int player, int32_t gamstate,
                                     int32_t pstatus, int32_t old_pstatus,
                                     bool credit_ok,
                                     wm_plyr_start_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_PLYR_START_H */
