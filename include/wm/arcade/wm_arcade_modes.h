/*
 * The three PLYRMODE handlers every wrestler shares and nobody owns.
 *
 * Each wrestler's mode_table has its own entry for most modes, but
 * three of them are one routine the whole roster jumps to, and the
 * source says so where it defines them:
 *
 *   mode_puppet   ;20  DOINK.ASM:3550  "(used by everyone)"
 *   mode_inair2   ;21  DOINK.ASM:3611  "All wrestlers use this
 *                                       mode_inair2 code, and only for
 *                                       turnbuckle stuff."
 *   mode_choking  ;25  TAKER.ASM:3110
 *
 * All eight of this port's dispatchers already had a callback seam for
 * each -- and unlike keep_attached/master_keep_attached, which fall
 * back to the shared translation when the seam is NULL, these three had
 * no fallback and no implementation anywhere. So a wrestler who entered
 * any of the three modes stayed in it: the dispatcher called nothing,
 * and nothing else in the game takes him out.
 *
 * Two of the three exist BECAUSE of that failure mode. mode_puppet is a
 * watchdog -- the source's own comment is "bark! Been here too long" --
 * and mode_choking is a bail-out. They are the arcade's answer to a
 * wrestler stranded by a grapple that went away, and the port had the
 * stranding without the answer.
 */
#ifndef WM_ARCADE_MODES_H
#define WM_ARCADE_MODES_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* `#timeout_val .equ TSEC*2` (DOINK.ASM:3552). TSEC is the source's own
   second -- 53 ticks, the arcade clock this port runs on. */
#define WM_PUPPET_TIMEOUT (53 * 2)

/*
 * mode_inair2's drift, DOINK.ASM:3617-3618, and both are per-tick
 * POSITION deltas rather than velocities -- see wm_arcade_mode_inair2.
 */
#define WM_INAIR2_ZDRIFT 0x58000
#define WM_INAIR2_XDRIFT 0x30000

/*
 * mode_puppet (DOINK.ASM:3550), the watchdog.
 *
 * A puppet is a wrestler being posed by somebody else's animation, and
 * the arrangement is only sound while the two point at each other. The
 * routine checks exactly that: if ATTACH_PROC is set AND that process
 * points back, it returns immediately and the master keeps driving him.
 * Otherwise it counts.
 *
 * The count is a run-length, not a total. PUPPET_TIME holds the PCNT of
 * the last tick this ran, and only a difference of EXACTLY 1 -- that
 * is, a tick where he was a puppet and the tick before it too --
 * continues the run; anything else restarts it at 1. So a wrestler who
 * is puppeted on and off never accumulates, and two seconds of
 * uninterrupted stranding trips it.
 *
 * When it trips, the source glitches him out: `SETMODE NORMAL` and
 * change_anim1a on his own standing animation, which the FACE24TBL over
 * DOINK.ASM:3594's `#stand_tbl` picks by FACING_DIR. PLYRMODE is
 * applied here; the animation is returned for the caller to start,
 * since this module owns no animation system.
 */
typedef struct {
    /* The watchdog fired. PLYRMODE is already MODE_NORMAL. */
    bool glitched_to_stand;
    /* His standing animation, or NULL for a wrestler with no row (Adam
       Bomb's is `.long 0,0`). The source would have called
       change_anim1a with that 0. */
    const char *stand_anim;
} wm_mode_puppet_result_t;

wm_mode_puppet_result_t wm_arcade_mode_puppet(wm_arcade_actor_t *actor,
                                              uint32_t pcnt);

/*
 * mode_inair2 (DOINK.ASM:3611): steer a wrestler in mid-air off a
 * turnbuckle. "Read joystick for floating off from turnbuckle moves."
 *
 * It reads STICK_VAL_CUR and adds a fixed drift per axis -- up is
 * negative Z, down positive, left negative X, right positive -- and
 * nothing else. No gate on being airborne, no clamp, no facing: the
 * only thing that decides whether it runs is being in the mode.
 *
 * It adds to POSITION, not velocity. Both velocity forms are written
 * out in the source and both are commented out, the position stores
 * left live underneath them, so the drift does not accumulate or
 * persist -- release the stick and he stops moving that tick. Faithful,
 * and not what the field names would lead you to write.
 *
 * Diagonals apply both axes, since the two tests are independent, and
 * up beats down and left beats right because each pair is tested in
 * that order with the second only reached when the first is clear.
 */
void wm_arcade_mode_inair2(wm_arcade_actor_t *actor);

/*
 * mode_choking (TAKER.ASM:3110), the Undertaker's chokehold seen from
 * the man being choked -- and, like mode_puppet, a mutual-attachment
 * check with a bail-out.
 *
 * While ATTACH_PROC points at somebody who points back, it does
 * nothing at all and the choker's animation owns him. The moment that
 * stops being true it lets go: ATTACH_PROC cleared, the looping choke
 * sound killed, PLYRMODE and ANIMODE both back to MODE_NORMAL.
 *
 * ANIMODE is set to the literal MODE_NORMAL (0) rather than having bits
 * cleared -- `movi MODE_NORMAL,a0 / move a0,*a13(ANIMODE)` -- so every
 * animation flag he was carrying goes with it.
 *
 * There is a dead precondition worth recording: the two lines testing
 * GETUP_TIME immediately above `rets` are commented out in the shipped
 * source, so a choke victim is held regardless of his getup timer.
 */
typedef struct {
    /* The #fall_out path ran: he is loose and MODE_NORMAL. */
    bool fell_out;
    /* `CALLA FIND_AND_KILL_ENDLESS` -- a DCSSOUND.ASM routine that
       stops a looping sound, so it is reported like every other sound
       here rather than performed. True exactly when fell_out is. */
    bool kill_endless_sound;
} wm_mode_choking_result_t;

wm_mode_choking_result_t wm_arcade_mode_choking(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_MODES_H */
