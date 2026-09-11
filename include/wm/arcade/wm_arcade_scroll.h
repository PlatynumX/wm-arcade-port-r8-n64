/*
 * WRESTLE2.ASM:1681 scroll_world -- the camera, and WRESTLE.ASM:2238
 * update_positions, which is what feeds it.
 *
 * wm/match_display.h has been saying a screen coordinate there is "in
 * world-pixel space with the camera at the origin", because this port
 * had no translated scroller. This is the scroller. Subtracting
 * WORLDTL from a draw item's screen_x/screen_y is what the DMA does
 * when it plots, and what DISPLAY.ASM's SCRTST does when it decides
 * whether a thing is on screen at all.
 *
 * The source's own comment lists eight tracking rules, covering royal
 * rumble and up to four live wrestlers. This port has a fixed pair, so
 * only two of those rules can be reached -- "two and only two active
 * wrestlers, track on them", and the attract-mode case with no humans,
 * which tracks on the first live drone and his closest opponent. Both
 * reduce to the same thing here and both are translated;
 * wm_scroll_pick_pair says which it took. The multi-wrestler averaging
 * is not translated, and it needs a roster this port cannot build.
 *
 * The MOTION is fully general and all of it is here: the X dead-band
 * and its eighth-of-the-error easing, the Y that comes from depth
 * rather than height, the SCROLL_CTRL override, the quarter easing on
 * Y and the front-fence stop.
 */
#ifndef WM_ARCADE_SCROLL_H
#define WM_ARCADE_SCROLL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * `#BUFFER equ [20,0]` -- twenty pixels of slack either side of the
 * midpoint before the camera moves at all. Without it the camera would
 * chase every step.
 */
#define WM_SCROLL_BUFFER 0x00140000

/* The camera stops rather than clamping; see wm_scroll_world. */
#define WM_SCROLL_X_MIN 0x012f0000
#define WM_SCROLL_X_MAX 0x06480000
/* "Don't allow scroller to go past front fence" -- LIMITYB. */
#define WM_SCROLL_Y_MAX 0x00970000

/* Half of the 400x256 screen, which is what the midpoint is centred on. */
#define WM_SCROLL_HALF_W 200
#define WM_SCROLL_HALF_H 0xd8

/*
 * WRESTLE.ASM:2238 update_positions -- what a wrestler publishes for
 * the scroller to read, once per tick.
 *
 * X and Z are OBJ_XPOS and OBJ_ZPOS, straight through. Y is the one to
 * look at: the source's `move *a13(OBJ_YPOS),a14,L` is commented out
 * and what it actually publishes is GROUND_Y shifted up 16 and floored
 * at zero. So the camera follows the mat a wrestler is standing on,
 * not the wrestler -- which is why jumping does not make the view
 * lurch.
 */
typedef struct wm_scroll_point {
    int32_t x;      /* 16.16 */
    int32_t y;      /* 16.16 */
    int32_t z;      /* 16.16 */
} wm_scroll_point;

void wm_scroll_update_positions(const wm_arcade_actor_t *actor,
                                wm_scroll_point *out);

typedef struct wm_scroll_state {
    int32_t worldtlx;   /* 16.16 */
    int32_t worldtly;
} wm_scroll_state;

/*
 * Which two wrestlers the camera tracks. Returns false when it found
 * nobody, which the source reaches only in its "bizarre. everyone's
 * dead in the attract mode" case, where it falls back on the first two
 * drone slots.
 *
 * `humans_present` is PSTATUS != 0. With one pair the only live rules
 * are "track on the two of them" and the no-humans sweep for the first
 * live actor and his closest; both are here.
 */
bool wm_scroll_pick_pair(const wm_arcade_actor_t *const *actors,
                         size_t count, bool humans_present,
                         size_t *out_a, size_t *out_b);

/*
 * One tick of the camera.
 *
 * X: the midpoint of the two tracked wrestlers, less half a screen, is
 * where the camera wants to be. The difference is reduced by
 * WM_SCROLL_BUFFER first and the camera does not move at all if that
 * consumes it; otherwise it moves an eighth of what is left. If the
 * result would leave [WM_SCROLL_X_MIN, WM_SCROLL_X_MAX] it is not
 * written -- the source STOPS rather than clamping, so the camera
 * holds its last legal value.
 *
 * Y: not the wrestlers' height. It is `mean_z_int * Y_SCALE_MULTIPLIER
 * - mean_y`, the same depth-to-screen mapping #set_image uses, less
 * half a screen. Then every actor carrying WM_STATUS_SCROLL_CTRL and
 * standing within the horizontal window gets a say: its own high point
 * (`z * Y_SCALE_MULTIPLIER - y - scroll_y`) pulls the camera up if it
 * is above what the pair asked for. The result eases a quarter of the
 * way each tick, and is dropped entirely if it would pass the front
 * fence.
 *
 * The caller owns the two gates at the top of the routine, `@HALT` and
 * `@in_finish_move`, because both belong to states this module cannot
 * see.
 */
void wm_scroll_world(wm_scroll_state *s,
                     const wm_scroll_point *p1, const wm_scroll_point *p2,
                     const wm_arcade_actor_t *const *actors, size_t count);

#ifdef __cplusplus
}
#endif
#endif
