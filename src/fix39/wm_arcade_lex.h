#ifndef WM_ARCADE_LEX_H
#define WM_ARCADE_LEX_H

#include "wm_arcade_roster.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dedicated direct-port module for LEX.ASM.
 * Shared callback/environment types are only adapter contracts to global arcade services;
 * no wrestler behavior is implemented outside wm_arcade_lex.c. */
typedef wm_arcade_roster_env_t wm_arcade_lex_env_t;
typedef wm_arcade_roster_callbacks_t wm_arcade_lex_callbacks_t;

typedef enum wm_arcade_lex_secret_id {
    WM_LEX_SECRET_CHARGE_CLOBBER,
    WM_LEX_SECRET_NECK_GRAB,
    WM_LEX_SECRET_GRAB_FLING,
    WM_LEX_SECRET_HIP_TOSS,
    WM_LEX_SECRET_GRAB_FLING2,
    WM_LEX_SECRET_HIP_TOSS2,
    WM_LEX_SECRET_SLIDING_ELBOW,
    WM_LEX_SECRET_HAMMER
} wm_arcade_lex_secret_id_t;

typedef enum wm_arcade_lex_monitor_id {
    WM_LEX_MON_0,
    WM_LEX_MON_1,
    WM_LEX_MON_2,
    WM_LEX_MON_3,
    WM_LEX_MON_4,
    WM_LEX_MON_5,
    WM_LEX_MON_6,
    WM_LEX_MON_7,
    WM_LEX_MON_8,
    WM_LEX_MON_9
} wm_arcade_lex_monitor_id_t;

typedef enum wm_arcade_lex_step_result {
    WM_LEX_STEP_IDLE=0, WM_LEX_STEP_ACTION=1, WM_LEX_STEP_EXTERNAL=2
} wm_arcade_lex_step_result_t;

wm_arcade_lex_step_result_t wm_arcade_move_lex(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, const wm_arcade_lex_env_t *env, const wm_arcade_lex_callbacks_t *cb);
int wm_arcade_lex_release_charge(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, uint16_t ticks, const wm_arcade_lex_callbacks_t *cb);
int wm_arcade_lex_fire_secret(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, wm_arcade_lex_secret_id_t id, uint32_t pcnt, const wm_arcade_lex_callbacks_t *cb);
int wm_arcade_lex_fire_monitor(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, wm_arcade_lex_monitor_id_t id, const wm_arcade_lex_env_t *env, int opponent_attack_is_leaping, const wm_arcade_lex_callbacks_t *cb);

void wm_arcade_lex_ani_init(wm_arcade_actor_t *wrestler, const wm_arcade_lex_callbacks_t *cb);

#ifdef __cplusplus
}
#endif
#endif
