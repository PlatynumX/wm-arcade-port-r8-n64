#ifndef WM_ARCADE_YOKO_H
#define WM_ARCADE_YOKO_H

#include "wm_arcade_roster.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dedicated direct-port module for YOKO.ASM.
 * Shared callback/environment types are only adapter contracts to global arcade services;
 * no wrestler behavior is implemented outside wm_arcade_yoko.c. */
typedef wm_arcade_roster_env_t wm_arcade_yoko_env_t;
typedef wm_arcade_roster_callbacks_t wm_arcade_yoko_callbacks_t;

typedef enum wm_arcade_yoko_secret_id {
    WM_YOKO_SECRET_CHARGE_SALT,
    WM_YOKO_SECRET_NECK_GRAB,
    WM_YOKO_SECRET_GRAB_FLING,
    WM_YOKO_SECRET_HIP_TOSS,
    WM_YOKO_SECRET_GRAB_FLING2,
    WM_YOKO_SECRET_HIP_TOSS2,
    WM_YOKO_SECRET_SCISSORS,
    WM_YOKO_SECRET_GUT_PUSH,
    WM_YOKO_SECRET_JABS
} wm_arcade_yoko_secret_id_t;

typedef enum wm_arcade_yoko_monitor_id {
    WM_YOKO_MON_0,
    WM_YOKO_MON_1,
    WM_YOKO_MON_2,
    WM_YOKO_MON_3,
    WM_YOKO_MON_4,
    WM_YOKO_MON_5,
    WM_YOKO_MON_6,
    WM_YOKO_MON_7,
    WM_YOKO_MON_8,
    WM_YOKO_MON_9
} wm_arcade_yoko_monitor_id_t;

typedef enum wm_arcade_yoko_step_result {
    WM_YOKO_STEP_IDLE=0, WM_YOKO_STEP_ACTION=1, WM_YOKO_STEP_EXTERNAL=2
} wm_arcade_yoko_step_result_t;

wm_arcade_yoko_step_result_t wm_arcade_move_yoko(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, const wm_arcade_yoko_env_t *env, const wm_arcade_yoko_callbacks_t *cb);
int wm_arcade_yoko_release_charge(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, uint16_t ticks, const wm_arcade_yoko_callbacks_t *cb);
int wm_arcade_yoko_fire_secret(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, wm_arcade_yoko_secret_id_t id, uint32_t pcnt, const wm_arcade_yoko_callbacks_t *cb);
int wm_arcade_yoko_fire_monitor(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, wm_arcade_yoko_monitor_id_t id, const wm_arcade_yoko_env_t *env, int opponent_attack_is_leaping, const wm_arcade_yoko_callbacks_t *cb);

#ifdef __cplusplus
}
#endif
#endif
