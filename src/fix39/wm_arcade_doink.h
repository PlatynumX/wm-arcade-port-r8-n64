#ifndef WM_ARCADE_DOINK_H
#define WM_ARCADE_DOINK_H

#include "wm_arcade_roster.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dedicated direct-port module for DOINK.ASM.
 * Shared callback/environment types are only adapter contracts to global arcade services;
 * no wrestler behavior is implemented outside wm_arcade_doink.c. */
typedef wm_arcade_roster_env_t wm_arcade_doink_env_t;
typedef wm_arcade_roster_callbacks_t wm_arcade_doink_callbacks_t;

typedef enum wm_arcade_doink_secret_id {
    WM_DOINK_SECRET_CHARGE_BUZZ,
    WM_DOINK_SECRET_GRAB_FLING,
    WM_DOINK_SECRET_HIP_TOSS,
    WM_DOINK_SECRET_GRAB_FLING2,
    WM_DOINK_SECRET_HIP_TOSS2,
    WM_DOINK_SECRET_EARSLAP,
    WM_DOINK_SECRET_HAMMER,
    WM_DOINK_SECRET_NECK_GRAB,
    WM_DOINK_SECRET_BOXING_PNCH
} wm_arcade_doink_secret_id_t;

typedef enum wm_arcade_doink_monitor_id {
    WM_DOINK_MON_0,
    WM_DOINK_MON_1,
    WM_DOINK_MON_2,
    WM_DOINK_MON_3,
    WM_DOINK_MON_4,
    WM_DOINK_MON_5,
    WM_DOINK_MON_6,
    WM_DOINK_MON_7,
    WM_DOINK_MON_8,
    WM_DOINK_MON_9,
    WM_DOINK_MON_10
} wm_arcade_doink_monitor_id_t;

typedef enum wm_arcade_doink_step_result {
    WM_DOINK_STEP_IDLE=0, WM_DOINK_STEP_ACTION=1, WM_DOINK_STEP_EXTERNAL=2
} wm_arcade_doink_step_result_t;

wm_arcade_doink_step_result_t wm_arcade_move_doink(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, const wm_arcade_doink_env_t *env, const wm_arcade_doink_callbacks_t *cb);
int wm_arcade_doink_release_charge(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, uint16_t ticks, const wm_arcade_doink_callbacks_t *cb);
int wm_arcade_doink_fire_secret(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, wm_arcade_doink_secret_id_t id, uint32_t pcnt, const wm_arcade_doink_callbacks_t *cb);
int wm_arcade_doink_fire_monitor(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, wm_arcade_doink_monitor_id_t id, const wm_arcade_doink_env_t *env, int opponent_attack_is_leaping, const wm_arcade_doink_callbacks_t *cb);

#ifdef __cplusplus
}
#endif
#endif
