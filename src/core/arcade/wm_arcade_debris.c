/*
 * SPECIAL.ASM:3415 react_debris. See wm/arcade/wm_arcade_debris.h for
 * what this is and is not.
 */
#include "wm/arcade/wm_arcade_debris.h"

#include "wm/arcade/wm_arcade_roster.h"

#include <string.h>

void wm_debris_init(wm_debris_state *st) {
    if (st) memset(st, 0, sizeof(*st));
}

bool wm_debris_react(wm_debris_state *st, WmRng *rng,
                     const wm_arcade_actor_t *victim,
                     int percent, int shape, bool taker_pin_anim,
                     wm_debris_burst *out) {
    const wm_debris_shape *sh;
    const wm_debris_sounds *snd;
    int pieces;

    if (out) memset(out, 0, sizeof(*out));
    if (!st || !victim) return false;
    if (shape < 0 || shape >= WM_DEBRIS_SHAPES) return false;

    /* `move *a8(0),a0 / calla RNDPER / jrls #die` */
    if (!rng) return false;
    if (wm_rng_rndrng0(rng, 999u) >= (uint32_t)percent) return false;

    /* "if there's too much debris already, don't do anything." */
    if (st->count >= WM_DEBRIS_MAX) return false;
    ++st->count;                       /* "increment global debris count" */

    sh = &wm_debris_shapes[shape];
    pieces = sh->loop * sh->count;

    if (out) {
        out->shape = sh;
        out->pieces = pieces;
        out->taker_pin = taker_pin_anim &&
            victim->wrestler_num == WM_ROSTER_TAKER;
    }

    /*
     * The impact sound, drawn from this wrestler's own row: the first word
     * of the row is RNDRNG0's inclusive maximum, so a hit on Bam Bam picks
     * one of five and a hit on Bret one of two. The cut wrestler has no
     * row, and a zero entry is silence -- both of the source's own
     * `JRZ NO_SOUND_AT_ALL` exits.
     */
    if (victim->wrestler_num >= 0 &&
        victim->wrestler_num < WM_DEBRIS_WRESTLERS) {
        snd = &wm_debris_sound_table[victim->wrestler_num];
        if (snd->sounds && snd->count) {
            uint32_t i = wm_rng_rndrng0(rng, (uint32_t)(snd->count - 1));
            if (i < snd->count && out) out->sound = snd->sounds[i];
        }
    }
    /* The Taker's pin overrides it with the bat, every piece. */
    if (out && out->taker_pin) out->sound = WM_DEBRIS_TAKER_BAT_SOUND;
    return true;
}

void wm_debris_finish(wm_debris_state *st) {
    /* `#exit: decrement global debris count` */
    if (st && st->count > 0) --st->count;
}
