#ifndef WM_ARCADE_MOVEMENT_H
#define WM_ARCADE_MOVEMENT_H

#include <stdbool.h>
#include <stdint.h>
#include "wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WM_ARCADE_GRAVITY 0x00008000
#define WM_ARCADE_MAX_YVEL ((int32_t)-0x01000000)

/* Direct WRESTLE2.ASM services adapted to wm_arcade_actor_t. */
void wm_arcade_calc_ground_y(wm_arcade_actor_t *actor, bool in_pregame2);
void wm_arcade_wrestler_veladd(wm_arcade_actor_t *actor,
                               bool halt, bool in_pregame2);
/* Direct WRESTLE.ASM wrestler_friction. */
void wm_arcade_wrestler_friction(wm_arcade_actor_t *actor);

/* WRESTLE2 facing-relative joystick helpers. */
uint16_t wm_arcade_xflip_joy(uint16_t joy);
uint16_t wm_arcade_stick_relative(uint16_t joy, bool facing_right);
uint16_t wm_arcade_stick_relative_new(uint16_t joy_cur,
                                      uint16_t joy_down,
                                      uint16_t joy_up,
                                      bool facing_right);

#ifdef __cplusplus
}
#endif
#endif
