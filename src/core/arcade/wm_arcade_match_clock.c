/*
 * WRESTLE2.ASM:4098 match_timer's clock, and LIFEBAR.ASM:5149
 * set_winner's `#tmout` branch -- see wm/arcade/wm_arcade_match_clock.h.
 */
#include "wm/arcade/wm_arcade_match_clock.h"

#include <string.h>

#include "wm/arcade/wm_arcade_round.h"

/*
 * WRESTLE2.ASM:4339. `.asg 1500,BASETM` then five rows, and the two
 * outer pairs are computed by the assembler as BASETM*30/100 and
 * BASETM*15/100 -- integer arithmetic on 1500, so 450 and 225 exactly
 * with nothing to round.
 */
const int32_t wm_match_clock_timer_table[5] = {
    WM_MATCH_CLOCK_BASETM - WM_MATCH_CLOCK_BASETM * 30 / 100,   /* 1050 */
    WM_MATCH_CLOCK_BASETM - WM_MATCH_CLOCK_BASETM * 15 / 100,   /* 1275 */
    WM_MATCH_CLOCK_BASETM,                                      /* 1500 */
    WM_MATCH_CLOCK_BASETM + WM_MATCH_CLOCK_BASETM * 15 / 100,   /* 1725 */
    WM_MATCH_CLOCK_BASETM + WM_MATCH_CLOCK_BASETM * 30 / 100    /* 1950 */
};

int32_t wm_match_clock_rate(int adjspeed, bool royal_rumble,
                            bool one_v_three, bool final_match) {
    int32_t rate;

    /* `BADCHK a0,1,5,3` -- out of range is not clamped to the nearest
       end, it is replaced by the default. */
    if (adjspeed < 1 || adjspeed > 5)
        adjspeed = WM_MATCH_CLOCK_ADJSPEED_DEFAULT;
    rate = wm_match_clock_timer_table[adjspeed - 1];

    /*
     * `movi >AAAA,a14 / mpyu a14,a1 / srl 16,a1` -- an unsigned
     * multiply by 0xAAAA then a 16-bit shift down, which is two
     * thirds to within the truncation the source accepts. Reached by
     * the royal rumble, and by a 1-on-3 where somebody is playing.
     */
    if (royal_rumble || one_v_three)
        rate = (int32_t)(((uint32_t)rate * 0xAAAAu) >> 16);

    /* `sra 1,a1` -- and then halved again for a final match, or for
       the royal rumble a second time, which is how the rumble ends up
       at the third the source's comment claims. */
    if (final_match || royal_rumble)
        rate >>= 1;

    return rate;
}

void wm_match_clock_start(wm_match_clock_t *c, int32_t rate) {
    if (!c) return;
    wm_match_clock_reset(c);
    c->rate = rate;
    /* `callr #create_timer / SLEEP TSEC*2`. */
    c->start_delay = WM_MATCH_CLOCK_START_DELAY;
}

void wm_match_clock_reset(wm_match_clock_t *c) {
    if (!c) return;
    /* `movk 9,a0 / move a0,@match_time / movk 9,a0 / move
       a0,@match_time+10h / clr a0 / move a0,@match_time+20h`. */
    c->tens = 9;
    c->ones = 9;
    c->frac = 0;
}

bool wm_match_clock_expired(const wm_match_clock_t *c) {
    /* `move @match_time,a0,L / jrz` -- BOTH digits, as one long. The
       fraction is not part of it. */
    return !c || (c->tens == 0 && c->ones == 0);
}

int32_t wm_match_clock_value(const wm_match_clock_t *c) {
    return c ? c->tens * 10 + c->ones : 0;
}

int32_t wm_match_clock_award_score(const wm_match_clock_t *c) {
    /* AWARD.ASM:996: the low half of the long is the TENS, and it is
       the one multiplied by ten. Same number as the value above; the
       source just gets there by a route that looks inverted. */
    if (!c) return 0;
    return (c->tens & 0x0f) * 10 + c->ones;
}

bool wm_match_clock_dec(wm_match_clock_t *c, bool *warn) {
    uint32_t frac;
    int32_t bcd;

    if (warn) *warn = false;
    if (!c) return false;

    /*
     * `move @match_time+20h,a0 / sub a10,a0 / move a0,@match_time+20h
     * / jrnc #no_change`. The fraction is a WORD, so this wraps at 16
     * bits, and a borrow out of it is what moves the clock. On the
     * TMS34010 `sub` sets carry ON a borrow, which is what makes the
     * source's own `#no_borrow` label read the right way round.
     */
    frac = (uint32_t)c->frac - (uint32_t)c->rate;
    c->frac = (uint16_t)frac;
    if (!(frac & 0x10000u))
        return false;                    /* no borrow: nothing moved */

    /* `move @match_time+10h,a0 / dec a0 / jrnc #no_borrow`. */
    c->ones -= 1;
    if (c->ones < 0) {
        c->ones = 9;
        c->tens -= 1;
        if (c->tens < 0) {
            /*
             * The source cannot reach this: the whole clock is gated
             * on `match_time != 0`, so 00 never decrements. Pinning
             * it here rather than letting the digits go negative
             * keeps every reader's `jrz` honest if a caller does tick
             * an expired clock.
             */
            c->tens = 0;
            c->ones = 0;
        }
        /* `cmpi 0,a0 / jrne #no_borrow` then a palette change: the
           clock digits turn red at ten. Display, not translated. */
    }

    /*
     * `move @match_time,a0,L / ... / cmpi 10h,a0 / jrgt #no_change /
     * movk 10,a0 / calla triple_sound`. The two digits are packed as
     * BCD and compared against 0x10, so the warning starts at ten
     * seconds and sounds once for each one after that. The comment
     * above it in the source says fifteen; the code says ten.
     */
    bcd = ((c->tens & 0x0f) << 4) | (c->ones & 0x0f);
    if (warn && bcd <= 0x10) *warn = true;
    return true;
}

bool wm_match_clock_tick(wm_match_clock_t *c, bool halt,
                         wm_arcade_actor_t *const *actors,
                         size_t actor_count, bool *warn) {
    if (warn) *warn = false;
    if (!c) return false;

    /* `SLEEP TSEC*2` before the loop is entered even once. */
    if (c->start_delay > 0) { c->start_delay -= 1; return false; }
    /* `move @HALT,a0 / jrnz #loop`. */
    if (halt) return false;
    /* `move @match_time,a0,L / jrz #loop`. */
    if (wm_match_clock_expired(c)) return false;
    /*
     * `callr get_live_bits / cmpi 3,a3 / jrne #1tmded`. Both sides
     * have to have somebody alive. This is not a detail: once one
     * side is wiped out the routine goes to its five-second pin
     * window instead, and the clock stops dead for the whole of it.
     */
    if (wm_arcade_get_live_bits(actors, actor_count) != 3) return false;

    return wm_match_clock_dec(c, warn);
}

void wm_match_clock_wrap(wm_match_clock_t *c) {
    /* `movi 00090009h,a14 / move a14,@match_time,L` -- both digits at
       once, and the fraction is deliberately left where it is. */
    if (!c) return;
    c->tens = 9;
    c->ones = 9;
}

/* ------------------------------------------------------------------ */
/* LIFEBAR.ASM:5149 set_winner #tmout                                 */

int wm_match_timeout_winner(wm_arcade_actor_t *const *actors,
                            size_t actor_count) {
    int32_t count[2] = { 0, 0 };
    int32_t total[2] = { 0, 0 };
    int32_t avg[2] = { 0, 0 };
    uint32_t best_hit = 0;
    int best_side = 0;
    size_t i;
    int s;

    if (!actors) return WM_MATCH_TIMEOUT_NO_WINNER;

    /*
     * `#loop2`: every process_ptrs slot, skipping the empty ones.
     * Note what it does NOT skip -- a dead wrestler is still counted,
     * with the zero health he has, which drags his side's average
     * down rather than being left out of it.
     */
    for (i = 0; i < actor_count; ++i) {
        const wm_arcade_actor_t *a = actors[i];
        if (!a || !a->active) continue;
        s = (a->player_side != 0) ? 1 : 0;
        count[s] += 1;
        /* `calla get_health`, which this port keeps on the actor. */
        total[s] += a->life;
    }

    for (s = 0; s < 2; ++s) {
        /*
         * `divu a4,a5`. The source does not guard the divisor and
         * says so -- "we could check for the very likely case of the
         * divisor being one, but there's no real need" -- because a
         * match always has somebody on both sides. A side with
         * nobody on it averages zero here rather than dividing by it.
         */
        avg[s] = count[s] ? total[s] / count[s] : 0;
    }

    /* `cmp a5,a7 / jrlt #t1w / jrgt #t2w`. */
    if (avg[1] < avg[0]) return 0;
    if (avg[1] > avg[0]) return 1;

    /*
     * `#tie`: the last team to land a hit. LAST_HIT_TIME is stamped
     * on the ATTACKER (REACT1.ASM:159 "update LAST_HIT_TIME on
     * attacker"), and holds all 32 bits of PCNT, so the source notes
     * it does not have to worry about wraparound. Strictly greater
     * wins, so the first wrestler found keeps it on a tie.
     */
    for (i = 0; i < actor_count; ++i) {
        const wm_arcade_actor_t *a = actors[i];
        if (!a || !a->active) continue;
        if (a->last_hit_time > best_hit) {
            best_hit = a->last_hit_time;
            best_side = (a->player_side != 0) ? 1 : 0;
        }
    }

    /* `TEST a0 / jrz #no_hits` -- "battle ended in a tie with neither
       side landing a blow. drop out to game over." */
    if (best_hit == 0) return WM_MATCH_TIMEOUT_NO_WINNER;
    return best_side;
}
