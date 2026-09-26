/*
 * WRESTLE.ASM:663 plyr_strtb1 / plyr_strtb2 -- see
 * wm/arcade/wm_arcade_plyr_start.h.
 */
#include "wm/arcade/wm_arcade_plyr_start.h"

bool wm_plyr_start_allowed(int32_t gamstate, int32_t pstatus, int player) {
    /* `move @GAMSTATE,a0 / jrn #die` -- INDIAG is "any neg". */
    if (gamstate < 0) return false;
    /* `cmpi INPARTY,a0 / jreq #die`. */
    if (gamstate == WM_GAMSTATE_INPARTY) return false;
    /* `move @PSTATUS,a14 / btst a8,a14 / jrnz #die`. */
    if (player < 0 || player > 1) return false;
    if ((pstatus & (1 << player)) != 0) return false;
    return true;
}

/* The seven-way branch at the bottom. An unnamed GAMSTATE is the
   source's own LOCKUP. */
static wm_plyr_start_outcome_t dispatch(int32_t gamstate) {
    switch (gamstate) {
    case WM_GAMSTATE_INAMODE:    return WM_PLYR_START_AMODE;
    case WM_GAMSTATE_INGAMEOVER: return WM_PLYR_START_GAMEOVER;
    case WM_GAMSTATE_INSELECT:   return WM_PLYR_START_SELECT;
    case WM_GAMSTATE_INPREGAME:  return WM_PLYR_START_PREGAME;
    /* INPREGAME2 and INGAME share a target. */
    case WM_GAMSTATE_INPREGAME2:
    case WM_GAMSTATE_INGAME:     return WM_PLYR_START_MIDGAME;
    default:                     return WM_PLYR_START_IMPOSSIBLE;
    }
}

wm_plyr_start_result_t wm_plyr_start(int player, int32_t gamstate,
                                     int32_t pstatus, int32_t old_pstatus,
                                     bool credit_ok,
                                     wm_plyr_start_state_t *state) {
    wm_plyr_start_result_t r;

    r.outcome = WM_PLYR_START_DIE;
    r.player = player;
    r.awards_reset_due = false;
    r.dufus_msgs_reset_due = false;
    r.icon_total_clear_due = false;
    r.cleared_state = false;
    r.took_credit = false;

    if (!wm_plyr_start_allowed(gamstate, pstatus, player)) return r;

    /*
     * Everything from here to the OLD_PSTATUS test happens BEFORE
     * the credit check, so a player turned away for want of a coin
     * has still had all of this done to him.
     */
    r.awards_reset_due = true;      /* `calla rst_winstreak_awards` */
    r.dufus_msgs_reset_due = true;  /* `calla reset_dufus_msgs` */
    r.icon_total_clear_due = true;  /* `calla clear_icon_total` */

    if (state) {
        /* `clr a14 / move a14,*a0,W / MOVE A14,*A1,L / *A2,L / *A3,L`. */
        state->winstreak = 0;
        state->entered_inits = 0;
        state->process_ptr = 0;
        state->pin_time_total = 0;
        /* `move *a4,a0 / jrn #a4ok / move a14,*a4,W` -- a NEGATIVE
           winstreakd is left as it stands. */
        if (state->winstreakd >= 0) state->winstreakd = 0;
        r.cleared_state = true;
    }

    /* `move @OLD_PSTATUS,a14 / btst a8,a14 / jrz #reg`. */
    if ((old_pstatus & (1 << player)) != 0) {
        /*
         * He was playing a moment ago: this is a continue, and no
         * credit is taken here. Anywhere but the select screen the
         * source LOCKUPs rather than guessing what to do.
         */
        r.outcome = (gamstate == WM_GAMSTATE_INSELECT)
                        ? WM_PLYR_START_WAITCONT
                        : WM_PLYR_START_IMPOSSIBLE;
        return r;
    }

    /* `#reg: calla CR_STRTP / jalo #die`. */
    if (!credit_ok) return r;
    r.took_credit = true;
    r.outcome = dispatch(gamstate);
    return r;
}
