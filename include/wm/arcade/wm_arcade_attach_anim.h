#ifndef WM_ARCADE_ATTACH_ANIM_H
#define WM_ARCADE_ATTACH_ANIM_H

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum wm_arcade_attach_status {
    WM_ATTACH_OK = 0,
    WM_ATTACH_NO_TARGET = 1,
    WM_ATTACH_NOT_RECIPROCAL = 2
} wm_arcade_attach_status_t;

/* ANIM.ASM wres_slave_anim semantic state. */
void wm_arcade_anim_enter_slave_idle(wm_arcade_actor_t *actor);

/* ANIM commands 10, 80, 81, 105, 106, 110, 113. */
wm_arcade_attach_status_t wm_arcade_anim_detach(wm_arcade_actor_t *actor);
wm_arcade_attach_status_t wm_arcade_anim_set_attach_from_whoihit(wm_arcade_actor_t *actor);
wm_arcade_attach_status_t wm_arcade_anim_set_opp_mode_bits(wm_arcade_actor_t *actor, uint16_t bits);
wm_arcade_attach_status_t wm_arcade_anim_clear_opp_mode_bits(wm_arcade_actor_t *actor, uint16_t bits);
wm_arcade_attach_status_t wm_arcade_anim_set_opp_player_mode(wm_arcade_actor_t *actor, uint16_t mode);
wm_arcade_attach_status_t wm_arcade_anim_xflip_opp(wm_arcade_actor_t *actor);
wm_arcade_attach_status_t wm_arcade_anim_set_opp_vels(wm_arcade_actor_t *actor,
                                                       int32_t xvel,
                                                       int32_t yvel,
                                                       int32_t zvel);

/* WRESTLE.ASM keep_attached: update this actor from its reciprocal master. */
wm_arcade_attach_status_t wm_arcade_master_keep_attached(wm_arcade_actor_t *actor);
wm_arcade_attach_status_t wm_arcade_keep_attached(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif
#endif
