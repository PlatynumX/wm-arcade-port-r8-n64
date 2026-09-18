#ifndef WM_ARCADE_DEBRIS_H
#define WM_ARCADE_DEBRIS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wmania_rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SPECIAL.ASM:3415 react_debris -- what ANI_DEBRIS and ANI_DEBRISAT start.
 *
 * The pieces themselves are sprite objects this port has no renderer for.
 * Everything in FRONT of them is data and arithmetic, and all of it is
 * here: the percentage gate, the global cap on how much debris can exist
 * at once, the per-wrestler impact SOUND (which is audible, not visual),
 * the Undertaker's own extra bat sound during a pin, and the burst shape
 * -- how many rounds, how many pieces each, how long between them.
 */

#define WM_DEBRIS_SHAPES 8
#define WM_DEBRIS_WRESTLERS 9
#define WM_DEBRIS_ANIMS 8

/* GAME.EQU:578 `DEBRIS_MAX .equ 30`, "max active debris processes". */
#define WM_DEBRIS_MAX 30

/*
 * SPECIAL.ASM:3467's Undertaker special case: when the wrestler being hit
 * is the Taker AND his current animation is und_4_pin2_anim, every piece
 * plays a bat sound and the sprite index is pushed 8 rows up the table.
 */
#define WM_DEBRIS_TAKER_BAT_SOUND 0xCF
#define WM_DEBRIS_TAKER_PIN_OFFSET 8

/* SPECIAL.ASM:3542 #debris_table. Velocities and gravity are 16.16; the
   offsets, lifespan and counts are whole units. */
typedef struct {
    int16_t loop;        /* DB_LOOP: rounds of pieces */
    int16_t count;       /* DB_COUNT: pieces a round */
    int16_t sleep;       /* DB_SLEEP: ticks between rounds */
    int16_t rxoff, ryoff, rzoff;    /* random +/- position spread */
    int32_t xvel, yvel, zvel;       /* initial velocity */
    int32_t rxvel, ryvel, rzvel;    /* random +/- velocity spread */
    int32_t gravity;
    int16_t lifespan, rlifespan;
} wm_debris_shape;

extern const wm_debris_shape wm_debris_shapes[WM_DEBRIS_SHAPES];

/* SPECIAL.ASM:3505 WHICH_DEBRIS_SOUND, one row per wrestler. Slot 7 is
   the cut wrestler and has no row at all. */
typedef struct {
    const int16_t *sounds;
    size_t count;
} wm_debris_sounds;

extern const wm_debris_sounds wm_debris_sound_table[WM_DEBRIS_WRESTLERS];

/* SPECIAL.ASM:3789 debris_anims -- eight debris sprites per wrestler. */
extern const char *const
    wm_debris_anims[WM_DEBRIS_WRESTLERS][WM_DEBRIS_ANIMS];

/* @debris_count, the global react_debris and react_debris2 both cap. */
typedef struct {
    int32_t count;
} wm_debris_state;

void wm_debris_init(wm_debris_state *st);

/*
 * One react_debris run, as a decision rather than a process.
 *
 * `percent` is the command's own RNDPER chance; `shape` its #debris_table
 * index; `victim` the wrestler the pieces come off. Returns how many
 * pieces the burst WOULD create -- 0 when the percentage refused or the
 * global cap is already reached -- and reports the sound it picked and
 * whether the Undertaker's pin case applied.
 *
 * The count is loop * count, which is what the source's two nested loops
 * produce; the sleep between rounds is in the shape for a caller that
 * wants to pace them.
 */
typedef struct {
    int pieces;          /* loop * count, or 0 */
    int sound;           /* the drawn impact sound, or 0 for silence */
    bool taker_pin;      /* the bat sound and the +8 sprite offset */
    const wm_debris_shape *shape;
} wm_debris_burst;

bool wm_debris_react(wm_debris_state *st, WmRng *rng,
                     const wm_arcade_actor_t *victim,
                     int percent, int shape, bool taker_pin_anim,
                     wm_debris_burst *out);

/*
 * react_debris's own `#exit`: the burst is over, give the slot back.
 *
 * It takes only the state, as the source does -- `move @debris_count,a14 /
 * dec a14 / move a14,@debris_count` -- so pairing it with a react that
 * actually STARTED is the caller's job, exactly as it is the process's in
 * the original (the `#die` exits never reach `#exit`).
 */
void wm_debris_finish(wm_debris_state *st);

#ifdef __cplusplus
}
#endif
#endif
