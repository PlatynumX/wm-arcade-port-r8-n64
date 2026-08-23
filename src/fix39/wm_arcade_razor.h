#ifndef WM_ARCADE_RAZOR_H
#define WM_ARCADE_RAZOR_H

#include "wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum wm_arcade_razor_anim_id {
    WM_RZR_ANIM_NONE = 0,
    WM_RZR_ANIM_STAND2, WM_RZR_ANIM_STAND4,
    WM_RZR_ANIM_TORSO2, WM_RZR_ANIM_TORSO4,
    WM_RZR_ANIM_FALL_BACK,
    WM_RZR_ANIM_PIN2, WM_RZR_ANIM_PIN4,
    WM_RZR_ANIM_RAISE_ARM2, WM_RZR_ANIM_RAISE_ARM4,
    WM_RZR_ANIM_PUNCH2, WM_RZR_ANIM_PUNCH4,
    WM_RZR_ANIM_BUTT2, WM_RZR_ANIM_BUTT4,
    WM_RZR_ANIM_GROUND_PUNCH2, WM_RZR_ANIM_GROUND_PUNCH4,
    WM_RZR_ANIM_BLOCK2, WM_RZR_ANIM_BLOCK4,
    WM_RZR_ANIM_UPPERCUT4,
    WM_RZR_ANIM_PUMMEL2, WM_RZR_ANIM_PUMMEL4,
    WM_RZR_ANIM_USLASH3, WM_RZR_ANIM_DSLASH3,
    WM_RZR_ANIM_HAIR_PICKUP2, WM_RZR_ANIM_HAIR_PICKUP4,
    WM_RZR_ANIM_RUGSHAKE,
    WM_RZR_ANIM_KICK2, WM_RZR_ANIM_KICK4,
    WM_RZR_ANIM_KNEE2, WM_RZR_ANIM_KNEE4,
    WM_RZR_ANIM_STOMP2, WM_RZR_ANIM_STOMP4,
    WM_RZR_ANIM_KICK_TB,
    WM_RZR_ANIM_SUPER_KICK2, WM_RZR_ANIM_SUPER_KICK4,
    WM_RZR_ANIM_KNEE_FALL4,
    WM_RZR_ANIM_BIGBOOT4,
    WM_RZR_ANIM_START_RUN,
    WM_RZR_ANIM_FLYING_KICK,
    WM_RZR_ANIM_FLYING_ELBOW,
    WM_RZR_ANIM_RUN2,
    WM_RZR_ANIM_CLIMB_DOWN,
    WM_RZR_ANIM_TBUKL_ELBOW,
    WM_RZR_ANIM_PUSH4,
    WM_RZR_ANIM_HEAD_HOLD2_3, WM_RZR_ANIM_HEAD_HOLD3, WM_RZR_ANIM_FAKE_HOLD3,
    WM_RZR_ANIM_GRABFLING2, WM_RZR_ANIM_GRABFLING4,
    WM_RZR_ANIM_HIPTOSS2, WM_RZR_ANIM_HIPTOSS4,
    WM_RZR_ANIM_HIPTOSS2_2, WM_RZR_ANIM_HIPTOSS2_4,
    WM_RZR_ANIM_REPEAT_SLASH,
    WM_RZR_ANIM_SLIDING_RUG,
    WM_RZR_ANIM_RAZORS_EDGE,
    WM_RZR_ANIM_PILE_DRIVER3,
    WM_RZR_ANIM_RUGSHAKE2,
    WM_RZR_ANIM_COMBO_PUNCH, WM_RZR_ANIM_COMBO_KICK,
    WM_RZR_ANIM_USLASHES_TO_HEAD, WM_RZR_ANIM_DSLASHES_TO_HEAD,
    WM_RZR_ANIM_KICK2_4,
    WM_RZR_ANIM_HEAD_HELD_STAND3,
    WM_RZR_ANIM_FINISH1, WM_RZR_ANIM_FINISH2
} wm_arcade_razor_anim_id_t;

typedef enum wm_arcade_razor_sound_id {
    WM_RZR_SND_NONE = 0,
    WM_RZR_SND_PUNCH,
    WM_RZR_SND_HDBUTT,
    WM_RZR_SND_LBOWDROP,
    WM_RZR_SND_BLOCK_WOOSH,
    WM_RZR_SND_UPRCUT,
    WM_RZR_SND_UPRCUT_T2,
    WM_RZR_SND_KICK,
    WM_RZR_SND_KICK_T2,
    WM_RZR_SND_FLYKICK,
    WM_RZR_SND_GRABHOLD,
    WM_RZR_SND_GRABFLING_PUNCH,
    WM_RZR_SND_PUSH,
    WM_RZR_SND_TURNDIVE
} wm_arcade_razor_sound_id_t;

typedef enum wm_arcade_razor_action_id {
    WM_RZR_ACT_NONE = 0,
    WM_RZR_ACT_PUNCH,
    WM_RZR_ACT_BLOCK,
    WM_RZR_ACT_SUPER_PUNCH,
    WM_RZR_ACT_KICK,
    WM_RZR_ACT_PUNCHKICK,
    WM_RZR_ACT_SUPER_KICK,
    WM_RZR_ACT_GRABOH
} wm_arcade_razor_action_id_t;

typedef enum wm_arcade_razor_secret_id {
    WM_RZR_SECRET_CHARGE_FLYING_KICK = 0,
    WM_RZR_SECRET_NECK_GRAB,
    WM_RZR_SECRET_GRAB_FLING,
    WM_RZR_SECRET_HIP_TOSS,
    WM_RZR_SECRET_GRAB_FLING2,
    WM_RZR_SECRET_HIP_TOSS2,
    WM_RZR_SECRET_DOWN_SLASH
} wm_arcade_razor_secret_id_t;

typedef struct wm_arcade_razor_sequence_step {
    uint16_t value;
    uint16_t ignore_mask;
} wm_arcade_razor_sequence_step_t;

typedef struct wm_arcade_razor_secret_pattern {
    wm_arcade_razor_secret_id_t id;
    const wm_arcade_razor_sequence_step_t *steps;
    uint16_t step_count;
    uint16_t max_ticks;
} wm_arcade_razor_secret_pattern_t;

/* charge_flying_kick is an executable release check, not a table sequence. */
extern const wm_arcade_razor_secret_pattern_t wm_arcade_razor_secret_patterns[6];

typedef enum wm_arcade_razor_monitor_id {
    WM_RZR_MON_CHARGE_SLASHES = 0,
    WM_RZR_MON_HEADHOLD_PILE,
    WM_RZR_MON_HEADHOLD_COMBO1,
    WM_RZR_MON_HEADHOLD_EDGE,
    WM_RZR_MON_HEADHOLD_RUG,
    WM_RZR_MON_GRAB_TOSS_AIR,
    WM_RZR_MON_HEADHOLD_COMBO2,
    WM_RZR_MON_SLIDING_RUG,
    WM_RZR_MON_FINISH1,
    WM_RZR_MON_FINISH2
} wm_arcade_razor_monitor_id_t;

typedef struct wm_arcade_razor_monitor_pattern {
    wm_arcade_razor_monitor_id_t id;
    const wm_arcade_razor_sequence_step_t *steps;
    uint16_t step_count;
    uint16_t max_ticks;
} wm_arcade_razor_monitor_pattern_t;

/* charge_slashes is a hold/release process, so the sequence table has 9 entries. */
extern const wm_arcade_razor_monitor_pattern_t wm_arcade_razor_monitor_patterns[9];

#define WM_RZR_XRUN      0x00060000
#define WM_RZR_ZDRIFT    0x00020000
#define WM_RZR_WALK_VEL  0x0003a000
#define WM_RZR_WALK_DVEL 0x00031000

typedef struct wm_arcade_razor_env {
    uint32_t pcnt;
    int hyper_speed_on;
    int blocking_off;
    int p1rounds;
    int p2rounds;
} wm_arcade_razor_env_t;

typedef struct wm_arcade_razor_callbacks {
    void (*change_anim)(wm_arcade_actor_t *, wm_arcade_razor_anim_id_t, void *);
    void (*change_torso_anim)(wm_arcade_actor_t *, wm_arcade_razor_anim_id_t, void *);
    void (*sound)(wm_arcade_actor_t *, wm_arcade_razor_sound_id_t, void *);
    void (*check_secret_moves)(wm_arcade_actor_t *, const wm_arcade_razor_secret_pattern_t *, size_t, void *);
    void (*execute_walk)(wm_arcade_actor_t *, void *);
    int  (*climb_turnbuckle)(wm_arcade_actor_t *, void *);
    void (*bounce_off_ropes)(wm_arcade_actor_t *, void *);
    int  (*ck_ignore)(wm_arcade_actor_t *, void *);
    int  (*ck_ignore_reversed)(wm_arcade_actor_t *, wm_arcade_actor_t *, void *);
    int  (*bozo_check)(wm_arcade_actor_t *, void *);
    int  (*check_combo_go)(wm_arcade_actor_t *, void *);
    void (*find_and_kill_endless)(wm_arcade_actor_t *, void *);
    void (*do_reversal)(wm_arcade_actor_t *, void *);
    void (*do_reversal_message)(wm_arcade_actor_t *, void *);
    void (*adjust_health)(wm_arcade_actor_t *, int delta, void *);
    int  (*teammate_pin)(wm_arcade_actor_t *, void *);
    int  (*raisearm_check)(wm_arcade_actor_t *, void *);
    int  (*can_pin)(wm_arcade_actor_t *, const wm_arcade_actor_t *, void *);
    void (*drone_change_back)(wm_arcade_actor_t *, void *);
    void (*set_raisearm_bit)(wm_arcade_actor_t *, void *);
    void (*round_award_block)(wm_arcade_actor_t *, void *);
    void (*bonus_message)(wm_arcade_actor_t *, int bonus, void *);
    void (*jump_rope_audio)(wm_arcade_actor_t *, void *);
    void (*master_keep_attached)(wm_arcade_actor_t *, void *);
    void (*keep_attached)(wm_arcade_actor_t *, void *);
    void (*mode_dead)(wm_arcade_actor_t *, void *);
    void (*mode_puppet)(wm_arcade_actor_t *, void *);
    void (*mode_inair2)(wm_arcade_actor_t *, void *);
    void (*mode_choking)(wm_arcade_actor_t *, void *);
    void (*code_addr)(wm_arcade_actor_t *, uint32_t token, void *);
    void *user;
} wm_arcade_razor_callbacks_t;

typedef enum wm_arcade_razor_step_result {
    WM_RZR_STEP_IDLE = 0,
    WM_RZR_STEP_ACTION = 1,
    WM_RZR_STEP_EXTERNAL = 2
} wm_arcade_razor_step_result_t;

void wm_arcade_razor_ani_init(wm_arcade_actor_t *razor,
                              const wm_arcade_razor_callbacks_t *cb);
wm_arcade_razor_step_result_t wm_arcade_move_razor(
    wm_arcade_actor_t *razor,
    wm_arcade_actor_t *opp,
    const wm_arcade_razor_env_t *env,
    const wm_arcade_razor_callbacks_t *cb);

int wm_arcade_razor_release_charge_flying_kick(
    wm_arcade_actor_t *razor, wm_arcade_actor_t *opp, uint16_t charge_ticks,
    const wm_arcade_razor_callbacks_t *cb);
int wm_arcade_razor_release_charge_slashes(
    wm_arcade_actor_t *razor, uint16_t charge_ticks,
    const wm_arcade_razor_callbacks_t *cb);
int wm_arcade_razor_fire_secret(
    wm_arcade_actor_t *razor, wm_arcade_actor_t *opp,
    wm_arcade_razor_secret_id_t id, uint32_t pcnt,
    const wm_arcade_razor_callbacks_t *cb);
int wm_arcade_razor_fire_monitor(
    wm_arcade_actor_t *razor, wm_arcade_actor_t *opp,
    wm_arcade_razor_monitor_id_t id,
    const wm_arcade_razor_env_t *env,
    int opponent_attack_is_leaping,
    const wm_arcade_razor_callbacks_t *cb);

void wm_arcade_razor_velocity_for_dir(unsigned dir, int32_t *xvel, int32_t *zvel);

#ifdef __cplusplus
}
#endif
#endif
