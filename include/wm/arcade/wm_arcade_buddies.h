#ifndef WM_ARCADE_BUDDIES_H
#define WM_ARCADE_BUDDIES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wmania_rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * PROGRESS.ASM:4273 get_rnd_wrestler and WRESTLE2.ASM:4548
 * choose_buddies -- picking the two drone partners buddy mode puts
 * in the ring alongside the human players.
 *
 * get_rnd_wrestler is shared: the ladder builder draws from it too
 * (src/core/pregame.c, which used to carry its own copy of the same
 * seven instructions). It lives here so there is one translation of
 * it rather than two that can drift.
 */

/*
 * "Get a random wrestler among those available.
 *  >a7 = excluded wrestler mask  >a8 = excluded wrestler count
 *  <a0 = choice among those available"
 *
 *     movk 7,a0 / sub a8,a0 / calla RNDRNG0 / inc a0
 *     movi -1,a14
 *   #lp2
 *     inc  a14
 *     btst a14,a7
 *     jrnz #lp2
 *     dsj  a0,#lp2
 *
 * It counts the EXCLUSION MASK and nothing else, so the eight slots
 * it draws from include slot 7 -- the cut Adam Bomb. Everywhere else
 * in the game that picks a wrestler at random skips 7 explicitly
 * (RNDRNG0(7) then a re-draw); this does not, so a buddy really can
 * be the wrestler with no logo and Doink's hand-me-down stories.
 *
 * `rng` is RNDRNG0, whose maximum is INCLUSIVE. With every slot
 * excluded the source would draw RNDRNG0(-1) and walk off the end;
 * this returns 0 rather than reproducing that, and says so.
 */
#define WM_BUDDY_WRESTLER_SLOTS 8
unsigned wm_get_rnd_wrestler(uint8_t excluded, WmRng *rng);

/* `clr a8 / movk 8,a0 / #lp1: srl 1,a14 / jrnc #nxt1 / inc a8` --
   the popcount choose_buddies feeds get_rnd_wrestler, over 8 bits. */
unsigned wm_buddy_excluded_count(uint8_t excluded);

typedef struct {
    /*
     * The two draws, in the order choose_buddies MAKES them: `first`
     * is the one it pushes and pulls back into a1, `second` is the
     * one left in a0.
     */
    unsigned first;
    unsigned second;
    /* The mask after both, for a caller that wants to keep drawing. */
    uint8_t excluded;
} wm_buddies_t;

/*
 * choose_buddies: build the mask from @index1 and @index2, count it,
 * draw, add that draw to the mask, draw again.
 *
 * Note the second draw sees a mask containing the first, so the two
 * are always different -- unlike the ladder's own scramble, which
 * re-draws once on a collision and tolerates one anyway.
 */
wm_buddies_t wm_choose_buddies(unsigned index1, unsigned index2,
                               WmRng *rng);

/*
 * Which buddy goes to which side, which is NOT the order they were
 * drawn in. #2plyr does `PUSH a0,a1` then two `PULL a11`, and MMTM
 * pushes the higher-numbered register first, so a0 -- the SECOND
 * draw -- comes back off the stack first and becomes player one's
 * partner. Player two gets the first draw.
 */
unsigned wm_buddy_for_player1(const wm_buddies_t *b);
unsigned wm_buddy_for_player2(const wm_buddies_t *b);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_BUDDIES_H */
