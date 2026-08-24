#ifndef WM_ARCADE_TAKER_H
#define WM_ARCADE_TAKER_H

#include "wm_arcade_roster.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dedicated direct-port module for TAKER.ASM.
 * Shared callback/environment types are only adapter contracts to global arcade services;
 * no wrestler behavior is implemented outside wm_arcade_taker.c. */
typedef wm_arcade_roster_env_t wm_arcade_taker_env_t;
typedef wm_arcade_roster_callbacks_t wm_arcade_taker_callbacks_t;

typedef enum wm_arcade_taker_secret_id {
    WM_TAKER_SECRET_BUTTON_HOLD,
    WM_TAKER_SECRET_GRAB_FLING,
    WM_TAKER_SECRET_HIP_TOSS,
    WM_TAKER_SECRET_GRAB_FLING2,
    WM_TAKER_SECRET_HIP_TOSS2,
    WM_TAKER_SECRET_NECK_GRAB,
    WM_TAKER_SECRET_TOMB_SMASH
} wm_arcade_taker_secret_id_t;

typedef enum wm_arcade_taker_monitor_id {
    WM_TAKER_MON_0,
    WM_TAKER_MON_1,
    WM_TAKER_MON_2,
    WM_TAKER_MON_3,
    WM_TAKER_MON_4,
    WM_TAKER_MON_5,
    WM_TAKER_MON_6,
    WM_TAKER_MON_7,
    WM_TAKER_MON_8,
    WM_TAKER_MON_9,
    WM_TAKER_MON_10,
    WM_TAKER_MON_11,
    WM_TAKER_MON_12
} wm_arcade_taker_monitor_id_t;

typedef enum wm_arcade_taker_step_result {
    WM_TAKER_STEP_IDLE=0, WM_TAKER_STEP_ACTION=1, WM_TAKER_STEP_EXTERNAL=2
} wm_arcade_taker_step_result_t;

wm_arcade_taker_step_result_t wm_arcade_move_taker(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, const wm_arcade_taker_env_t *env, const wm_arcade_taker_callbacks_t *cb);
int wm_arcade_taker_release_charge(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, uint16_t ticks, const wm_arcade_taker_callbacks_t *cb);
int wm_arcade_taker_fire_secret(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, wm_arcade_taker_secret_id_t id, uint32_t pcnt, const wm_arcade_taker_callbacks_t *cb);
int wm_arcade_taker_fire_monitor(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, wm_arcade_taker_monitor_id_t id, const wm_arcade_taker_env_t *env, int opponent_attack_is_leaping, const wm_arcade_taker_callbacks_t *cb);

void wm_arcade_taker_ani_init(wm_arcade_actor_t *wrestler, const wm_arcade_taker_callbacks_t *cb);

#ifdef __cplusplus
}
#endif
#endif
