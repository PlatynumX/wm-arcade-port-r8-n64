#ifndef WM_ARCADE_ANIM_COMBAT_H
#define WM_ARCADE_ANIM_COMBAT_H

#include <stdint.h>
#include "wm/arcade/wm_arcade_react.h"

#ifdef __cplusplus
extern "C" {
#endif

enum wm_arcade_anim_combat_opcode {
    WM_ANI_ATTACK_ON      = 6,
    WM_ANI_ATTACK_OFF     = 7,
    WM_ANI_ATTACK_ON_Z    = 14,
    WM_ANI_CLR_DAMAGE     = 47,
    WM_ANI_DAMAGE         = 56,
    WM_ANI_CLR_STATUS     = 58,
    WM_ANI_DAMAGEOPP      = 66,
    WM_ANI_WAITHITOPP     = 68
};

typedef struct wm_arcade_attack_on_args {
    uint16_t attack_mode;
    int16_t xoff;
    int16_t yoff;
    int16_t width;
    int16_t height;
} wm_arcade_attack_on_args_t;

typedef struct wm_arcade_attack_on_z_args {
    uint16_t attack_mode;
    int16_t xoff;
    int16_t yoff;
    int16_t zoff;
    int16_t width;
    int16_t height;
    int16_t depth;
} wm_arcade_attack_on_z_args_t;

typedef enum wm_arcade_anim_damageopp_status {
    WM_ANI_DAMAGEOPP_OK = 0,
    WM_ANI_DAMAGEOPP_NO_TARGET = 1,
    WM_ANI_DAMAGEOPP_BAD_ARGUMENT = -1
} wm_arcade_anim_damageopp_status_t;

typedef struct wm_arcade_anim_damageopp_result {
    wm_arcade_anim_damageopp_status_t status;
    wm_arcade_actor_t *target;
    int16_t signed_damage;
    int used_reduced_damage;
    int used_next_damage;
    int health_hook_called;
} wm_arcade_anim_damageopp_result_t;

/* ANIM.ASM opcode 6.  Logical fields unpack the source's paired LONG writes. */
void wm_arcade_ani_attack_on(wm_arcade_actor_t *actor,
                             const wm_arcade_attack_on_args_t *args);
/* ANIM.ASM opcode 14. */
void wm_arcade_ani_attack_on_z(wm_arcade_actor_t *actor,
                               const wm_arcade_attack_on_z_args_t *args);
/* ANIM.ASM opcode 7. */
void wm_arcade_ani_attack_off(wm_arcade_actor_t *actor,
                              uint16_t round_tickcount);
/* ANIM.ASM opcode 47: currently a deliberate no-op in the arcade source. */
void wm_arcade_ani_clr_damage(wm_arcade_actor_t *actor);
/* ANIM.ASM opcode 56. */
void wm_arcade_ani_damage(wm_arcade_actor_t *actor,
                          int16_t script_damage,
                          const wm_arcade_react_callbacks_t *callbacks);
/* ANIM.ASM opcode 58. */
void wm_arcade_ani_clr_status(wm_arcade_actor_t *actor);
/* ANIM.ASM opcode 66. */
wm_arcade_anim_damageopp_result_t wm_arcade_ani_damageopp(
    wm_arcade_actor_t *actor,
    int16_t full_damage,
    int16_t reduced_damage,
    wm_arcade_combat_runtime_t *runtime,
    const wm_arcade_react_callbacks_t *callbacks);
/* ANIM.ASM opcode 68. */
void wm_arcade_ani_waithitopp(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif

#endif
