/*
 * LIFEBAR.ASM:3302 BONUS_MESS -- see wm/arcade/wm_arcade_bonus_mess.h
 * for why this is not the display routine the seam ledger called it.
 */
#include "wm/arcade/wm_arcade_bonus_mess.h"
#include "wm/arcade/wm_arcade_react.h"

#include <string.h>

void wm_bonus_mess_init(wm_bonus_mess_state *s) {
    if (s) memset(s, 0, sizeof *s);
}

void wm_bonus_mess_reset_match(wm_bonus_mess_state *s) {
    if (!s) return;
    /* `move a0,@message_flag,L` -- one long, so shown[1] survives. */
    s->shown[0] = 0;
}

wm_bonus_mess_result wm_bonus_mess(wm_bonus_mess_state *s, int bonus,
                                   uint16_t risk) {
    wm_bonus_mess_result r;
    int high_risk;

    memset(&r, 0, sizeof r);

    /* `move a10,a10 / jrz #already` -- but #already is reached with a14
       still clear from the top, and the real no-op is a10 == 0 never
       arriving here at all. The source falls THROUGH to the award and
       the multiplier; what a zero loses is only the text. Reading the
       jump as an early return would drop both, so it is spelled out. */
    if (bonus == 0) {
        /*
         * `jrz #already`, and #already is BELOW nothing -- it is the
         * label immediately above the DAM_MULT write and the award. So
         * a zero still hits harder and still scores; its `;No message`
         * comment is about the words only. a14 is still clear, so the
         * closing `jaz SUCIDE` skips the text.
         */
        r.ran = true;
        r.high_risk_award = true;
        r.guitar = true;
        r.show_text = false;
        r.dam_mult = 2;
        return r;
    }

    r.ran = true;
    /* `jrnn #reg`: negative is the taunt-style path. And #reg's own
       `move *a8(RISK),a1 / btst 15,a1 / jrnz #tag` sends a high-risk
       move there too. */
    high_risk = (bonus < 0) || ((risk & WM_ARCADE_RISK_HIGH_BIT) != 0);

    if (high_risk) {
        /* #tag. Its DAM_MULT write is commented out at :3320, so the
           caller's value -- 4, from ANIM.ASM:2234 -- stands. */
        r.high_risk_award = true;
        r.guitar = true;
        /*
         * And NO text. #tag does `PUSHP a14` with a14 still clear from
         * the top of the routine and never loads #message_tbl, so the
         * closing `PULLP a8 / jaz SUCIDE` dies before DO_THIS_MESS. The
         * taunt-style path is award, guitar and damage -- no words --
         * which is why it does not consult message_flag either.
         */
        r.show_text = false;
        r.dam_mult = 0;
        return r;
    }

    /* #reg. The flag gates the TEXT; everything below #already does not
       depend on it. */
    if (s) {
        int idx = bonus;
        unsigned half = 0;
        if (idx >= 32) { half = 1; idx -= 32; }
        if (idx < 32) {
            uint32_t bit = (uint32_t)1u << idx;
            if (half < 2) {
                r.show_text = (s->shown[half] & bit) == 0;
                s->shown[half] |= bit;
            }
        }
    }
    r.high_risk_award = true;
    r.guitar = true;
    r.dam_mult = 2;
    return r;
}
