#ifndef WM_ARCADE_SHAWN_H
#define WM_ARCADE_SHAWN_H

#include "wm_arcade_roster.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dedicated direct-port module for SHAWN.ASM.
 * Shared callback/environment types are only adapter contracts to global arcade services;
 * no wrestler behavior is implemented outside wm_arcade_shawn.c. */
typedef wm_arcade_roster_env_t wm_arcade_shawn_env_t;
typedef wm_arcade_roster_callbacks_t wm_arcade_shawn_callbacks_t;

typedef enum wm_arcade_shawn_secret_id {
    WM_SHAWN_SECRET_CHARGE_FLYING_KICK,
    WM_SHAWN_SECRET_GRAB_FLING,
    WM_SHAWN_SECRET_HIP_TOSS,
    WM_SHAWN_SECRET_GRAB_FLING2,
    WM_SHAWN_SECRET_HIP_TOSS2,
    WM_SHAWN_SECRET_NECK_GRAB,
    WM_SHAWN_SECRET_FRANKENSTEINER,
    WM_SHAWN_SECRET_JUMP_KICK
} wm_arcade_shawn_secret_id_t;

typedef enum wm_arcade_shawn_monitor_id {
    WM_SHAWN_MON_0,
    WM_SHAWN_MON_1,
    WM_SHAWN_MON_2,
    WM_SHAWN_MON_3,
    WM_SHAWN_MON_4,
    WM_SHAWN_MON_5,
    WM_SHAWN_MON_6,
    WM_SHAWN_MON_7,
    WM_SHAWN_MON_8,
    WM_SHAWN_MON_9,
    WM_SHAWN_MON_10,
    WM_SHAWN_MON_11,
    WM_SHAWN_MON_12,
    WM_SHAWN_MON_13,
    WM_SHAWN_MON_14
} wm_arcade_shawn_monitor_id_t;

typedef enum wm_arcade_shawn_step_result {
    WM_SHAWN_STEP_IDLE=0, WM_SHAWN_STEP_ACTION=1, WM_SHAWN_STEP_EXTERNAL=2
} wm_arcade_shawn_step_result_t;

wm_arcade_shawn_step_result_t wm_arcade_move_shawn(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, const wm_arcade_shawn_env_t *env, const wm_arcade_shawn_callbacks_t *cb);
int wm_arcade_shawn_release_charge(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, uint16_t ticks, const wm_arcade_shawn_callbacks_t *cb);
int wm_arcade_shawn_fire_secret(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, wm_arcade_shawn_secret_id_t id, uint32_t pcnt, const wm_arcade_shawn_callbacks_t *cb);
int wm_arcade_shawn_fire_monitor(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, wm_arcade_shawn_monitor_id_t id, const wm_arcade_shawn_env_t *env, int opponent_attack_is_leaping, const wm_arcade_shawn_callbacks_t *cb);

void wm_arcade_shawn_ani_init(wm_arcade_actor_t *wrestler, const wm_arcade_shawn_callbacks_t *cb);

#ifdef __cplusplus
}
#endif
#endif
