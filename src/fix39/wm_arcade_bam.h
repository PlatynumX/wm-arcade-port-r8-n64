#ifndef WM_ARCADE_BAM_H
#define WM_ARCADE_BAM_H

#include "wm_arcade_roster.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dedicated direct-port module for BAM.ASM.
 * Shared callback/environment types are only adapter contracts to global arcade services;
 * no wrestler behavior is implemented outside wm_arcade_bam.c. */
typedef wm_arcade_roster_env_t wm_arcade_bam_env_t;
typedef wm_arcade_roster_callbacks_t wm_arcade_bam_callbacks_t;

typedef enum wm_arcade_bam_secret_id {
    WM_BAM_SECRET_FIREPNCH,
    WM_BAM_SECRET_NECK_GRAB,
    WM_BAM_SECRET_GRAB_FLING,
    WM_BAM_SECRET_HIP_TOSS,
    WM_BAM_SECRET_GRAB_FLING2,
    WM_BAM_SECRET_HIP_TOSS2,
    WM_BAM_SECRET_JUMPKICK,
    WM_BAM_SECRET_GRAB_FLING2_DUP,
    WM_BAM_SECRET_HIP_TOSS2_DUP,
    WM_BAM_SECRET_NAPALM
} wm_arcade_bam_secret_id_t;

typedef enum wm_arcade_bam_monitor_id {
    WM_BAM_MON_0,
    WM_BAM_MON_1,
    WM_BAM_MON_2,
    WM_BAM_MON_3,
    WM_BAM_MON_4,
    WM_BAM_MON_5,
    WM_BAM_MON_6,
    WM_BAM_MON_7,
    WM_BAM_MON_8,
    WM_BAM_MON_9
} wm_arcade_bam_monitor_id_t;

typedef enum wm_arcade_bam_step_result {
    WM_BAM_STEP_IDLE=0, WM_BAM_STEP_ACTION=1, WM_BAM_STEP_EXTERNAL=2
} wm_arcade_bam_step_result_t;

wm_arcade_bam_step_result_t wm_arcade_move_bam(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, const wm_arcade_bam_env_t *env, const wm_arcade_bam_callbacks_t *cb);
int wm_arcade_bam_release_charge(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, uint16_t ticks, const wm_arcade_bam_callbacks_t *cb);
int wm_arcade_bam_fire_secret(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, wm_arcade_bam_secret_id_t id, uint32_t pcnt, const wm_arcade_bam_callbacks_t *cb);
int wm_arcade_bam_fire_monitor(wm_arcade_actor_t *wrestler, wm_arcade_actor_t *opponent, wm_arcade_bam_monitor_id_t id, const wm_arcade_bam_env_t *env, int opponent_attack_is_leaping, const wm_arcade_bam_callbacks_t *cb);

#ifdef __cplusplus
}
#endif
#endif
