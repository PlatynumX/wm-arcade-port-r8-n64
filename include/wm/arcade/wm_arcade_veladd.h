#ifndef WM_ARCADE_VELADD_H
#define WM_ARCADE_VELADD_H

#include "wm/arcade/wm_arcade_combat.h"
#include "wm/anim_program.h"
#include "wm/arcade/wmania_ring_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE2.ASM:2282 SUBR wrestler_veladd -- the per-tick physics
 * integrator, and WRESTLE.ASM:2456 SUBRP wrestler_friction beside it.
 *
 * Until this existed, every velocity this port wrote went nowhere. The
 * reaction code (REACT1-4) has always set real launch velocities straight
 * out of the source -- `victim->y_vel = FX16(6)` for a body slam,
 * `attacker->y_vel = FX16(4)` for a leap -- and the animation VM has
 * always run ANI_SET_YVEL/ANI_ZEROVELS. Nothing moved: no code added a
 * velocity to a position. Every knockdown, throw and slam in this port
 * landed instantly, because the wrestler was never in the air to begin
 * with.
 *
 * Sign convention, read off the source rather than assumed: LARGER Y IS
 * HIGHER. ANIM.ASM:890 _ani_waithitgnd tests `OBJ_YPOSINT > GROUND_Y` for
 * "still in the air", and its comment on `OBJ_YVEL ... jrp #no_gnd` reads
 * "must have down velocity" -- so a NEGATIVE OBJ_YVEL falls. Gravity
 * therefore SUBTRACTS from OBJ_YVEL (:2370), and WRESTLE2.ASM:2280's
 * MAX_YVEL of -1000000h is a floor on how fast a wrestler may fall, not a
 * ceiling.
 *
 * The source's main loop (WRESTLE.ASM:2457) runs these in a fixed order:
 *
 *     update_joystat -> count_button_presses -> keep_onscreen
 *       -> wrestler_veladd -> wrestler_friction
 *       -> animate_wrestler -> set_collision_boxes -> confine_wrestler
 *
 * so this runs BEFORE the animation ticks, and the confinement pass after
 * it sees the position this produced.
 *
 * `exec` is the wrestler's own running animation, or NULL. The integrator
 * genuinely reaches into it: WRESTLE2.ASM:2316's `#set_yvel` stuffs a 1
 * into ANICNT when MODE_WAITHITOPP is set, which is how landing cuts an
 * ANI_WAITHITOPP hold short. Passing NULL simply skips that.
 *
 * `in_pregame2` is GAMSTATE == INPREGAME2 (WRESTLE2.ASM:2424), which
 * suppresses calc_ground_y's "he is outside the ring now" write during the
 * entrance walk. This port has no pre-game phase, so wm_match_tick passes
 * 0; the parameter keeps the branch visible rather than silently gone.
 */
void wm_wrestler_veladd(wm_arcade_actor_t *a, wm_anim_exec *exec,
                        int in_pregame2);

/*
 * WRESTLE2.ASM:2385 SUBRP calc_ground_y. Sets OBJ_PRIORITY from where the
 * wrestler is, then decides GROUND_Y and INRING from whether he is between
 * the two mat-edge boundary lines at his Z.
 *
 * NOTE the INRING polarity: the source's field is documented at
 * PLYR.EQU:103 as "0 = in ring, 1 = outside" and this port stores the
 * ordinary boolean instead (see wm_arcade_actor::in_ring), so every test
 * and every write here is flipped from the source's text on purpose.
 */
void wm_wrestler_calc_ground_y(wm_arcade_actor_t *a, int in_pregame2);

/* WRESTLE.ASM:2456 SUBRP wrestler_friction: with MODE_FRICTION set, decay
   OBJ_XVEL toward zero by OBJ_FRICTION without crossing it. */
void wm_wrestler_friction(wm_arcade_actor_t *a);

/* GAME.EQU:436 GRAVITY -- the value every animation change resets
   OBJ_GRAVITY to (ANIM.ASM:4520, :4553). */
#define WM_GRAVITY 0x8000

/* WRESTLE2.ASM:2280 MAX_YVEL: the fastest a wrestler may fall. */
#define WM_MAX_YVEL (-0x1000000)

#ifdef __cplusplus
}
#endif
#endif
