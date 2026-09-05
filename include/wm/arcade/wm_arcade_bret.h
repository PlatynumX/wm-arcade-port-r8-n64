#ifndef WM_ARCADE_BRET_H
#define WM_ARCADE_BRET_H

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Source-label tokens from BRET.ASM.  These are deliberately semantic IDs,
 * not guessed N64 addresses.  The merge adapter resolves them to the native
 * animation pointers/data already extracted from the arcade assets.
 */
typedef enum wm_arcade_bret_anim_id {
    WM_BRET_ANIM_NONE = 0,
    WM_BRET_ANIM_STAND2, WM_BRET_ANIM_STAND4,
    WM_BRET_ANIM_TORSO2, WM_BRET_ANIM_TORSO4,
    WM_BRET_ANIM_FALL_BACK,
    WM_BRET_ANIM_PIN2, WM_BRET_ANIM_PIN4,
    WM_BRET_ANIM_RAISE_ARM2, WM_BRET_ANIM_RAISE_ARM4,
    WM_BRET_ANIM_PUNCH2, WM_BRET_ANIM_PUNCH4,
    WM_BRET_ANIM_BUTT2, WM_BRET_ANIM_BUTT4,
    WM_BRET_ANIM_GROUND_PUNCH2, WM_BRET_ANIM_GROUND_PUNCH4,
    WM_BRET_ANIM_BLOCK4,
    WM_BRET_ANIM_BUTTS2, WM_BRET_ANIM_BUTTS4,
    WM_BRET_ANIM_UPPERCUT4,
    WM_BRET_ANIM_HAIR_PICKUP2, WM_BRET_ANIM_HAIR_PICKUP4,
    WM_BRET_ANIM_SHOOTER2, WM_BRET_ANIM_SHOOTER4,
    WM_BRET_ANIM_SUPER_PUNCH2_2, WM_BRET_ANIM_SUPER_PUNCH2_4,
    WM_BRET_ANIM_KICK2, WM_BRET_ANIM_KICK4,
    WM_BRET_ANIM_KNEE2, WM_BRET_ANIM_KNEE4,
    WM_BRET_ANIM_STOMP2, WM_BRET_ANIM_STOMP4,
    WM_BRET_ANIM_KICK_TB,
    WM_BRET_ANIM_START_RUN,
    WM_BRET_ANIM_SUPER_KICK2, WM_BRET_ANIM_SUPER_KICK4,
    WM_BRET_ANIM_KNEE_FALL4,
    WM_BRET_ANIM_RUNNING_DDT,
    WM_BRET_ANIM_RUNNING_GROUND_PUNCH,
    WM_BRET_ANIM_FLYING_KICK,
    WM_BRET_ANIM_RUN2,
    WM_BRET_ANIM_CLIMB_DOWN,
    WM_BRET_ANIM_TBUKL_LEAP,
    WM_BRET_ANIM_PUSH4,
    WM_BRET_ANIM_PILE_DRIVER3,
    WM_BRET_ANIM_HH_DDT2,
    WM_BRET_ANIM_HEAD_HELD_STAND3,
    WM_BRET_ANIM_UPPERCUTS_TO_HEAD,
    WM_BRET_ANIM_KNEES_TO_HEAD,
    WM_BRET_ANIM_KNEE_TO_HEAD4,
    /* No separate SUPER_PUNCH4 id: BRET.ASM's #scrt_cut (the supercut
       secret move) dispatches to hrt_4_super_punch_anim, the exact same
       real label WM_BRET_ANIM_SUPER_PUNCH2_4 already names -- see
       wm/bret_backend.h's own comment. wm_arcade_bret_fire_secret reuses
       that id directly instead of a second, permanently-unmapped one. */
    WM_BRET_ANIM_JUMP_KICK4,
    WM_BRET_ANIM_FAKE_HOLD3,
    WM_BRET_ANIM_HEAD_HOLD2_3,
    WM_BRET_ANIM_HEAD_HOLD3,
    WM_BRET_ANIM_HIPTOSS,
    WM_BRET_ANIM_GRABFLING_FACE24,
    WM_BRET_ANIM_RAKE_FACE,
    WM_BRET_ANIM_HIPTOSS2,
    WM_BRET_ANIM_COMBO_PUNCH,
    WM_BRET_ANIM_COMBO_KICK,
    WM_BRET_ANIM_FACE_DRIVER2_3,
    WM_BRET_ANIM_ROLL_UPPERCUT,
    WM_BRET_ANIM_FINISH1,
    WM_BRET_ANIM_FINISH2
} wm_arcade_bret_anim_id_t;

typedef enum wm_arcade_bret_sound_id {
    WM_BRET_SND_NONE = 0,
    WM_BRET_SND_PUNCH,
    WM_BRET_SND_HDBUTT,
    WM_BRET_SND_LBOWDROP,
    WM_BRET_SND_BLOCK_WOOSH,
    WM_BRET_SND_UPRCUT,
    WM_BRET_SND_KICK,
    WM_BRET_SND_FLYKICK,
    WM_BRET_SND_GRABFLING,
    WM_BRET_SND_HIPTOSS,
    WM_BRET_SND_PUSH,
    WM_BRET_SND_TURNDIVE
} wm_arcade_bret_sound_id_t;

typedef enum wm_arcade_bret_action_id {
    WM_BRET_ACT_NONE = 0,
    WM_BRET_ACT_PUNCH,
    WM_BRET_ACT_BLOCK,
    WM_BRET_ACT_SUPER_PUNCH,
    WM_BRET_ACT_KICK,
    WM_BRET_ACT_PUNCHKICK,
    WM_BRET_ACT_SUPER_KICK,
    WM_BRET_ACT_GRABOH
} wm_arcade_bret_action_id_t;

typedef enum wm_arcade_bret_secret_id {
    WM_BRET_SECRET_CHARGE_DDT = 0,
    WM_BRET_SECRET_NECK_GRAB,
    WM_BRET_SECRET_GRAB_FLING,
    WM_BRET_SECRET_HIP_TOSS,
    WM_BRET_SECRET_GRAB_FLING2,
    WM_BRET_SECRET_HIP_TOSS2,
    WM_BRET_SECRET_FACE_RAKE,
    WM_BRET_SECRET_JUMP_KICK,
    WM_BRET_SECRET_SUPERCUT
} wm_arcade_bret_secret_id_t;

typedef struct wm_arcade_bret_sequence_step {
    uint16_t value;
    uint16_t ignore_mask;
} wm_arcade_bret_sequence_step_t;

typedef struct wm_arcade_bret_secret_pattern {
    wm_arcade_bret_secret_id_t id;
    const wm_arcade_bret_sequence_step_t *steps;
    uint16_t step_count;
    uint16_t max_ticks;
} wm_arcade_bret_secret_pattern_t;

extern const wm_arcade_bret_secret_pattern_t wm_arcade_bret_secret_patterns[8];

typedef enum wm_arcade_bret_monitor_id {
    WM_BRET_MON_ROLL_UPPERCUT = 0,
    WM_BRET_MON_HEADHOLD_COMBO1,
    WM_BRET_MON_HEADHOLD_COMBO2,
    WM_BRET_MON_HEADHOLD_PILE,
    WM_BRET_MON_HEADHOLD_DDT,
    WM_BRET_MON_HEADHOLD_FACESLAM,
    WM_BRET_MON_GRAB_TOSS_AIR,
    WM_BRET_MON_FINISH1,
    WM_BRET_MON_FINISH2
} wm_arcade_bret_monitor_id_t;

typedef struct wm_arcade_bret_monitor_pattern {
    wm_arcade_bret_monitor_id_t id;
    const wm_arcade_bret_sequence_step_t *steps;
    uint16_t step_count;
    uint16_t max_ticks;
} wm_arcade_bret_monitor_pattern_t;

extern const wm_arcade_bret_monitor_pattern_t wm_arcade_bret_monitor_patterns[9];

/* Exact BRET.ASM run constants (16.16). */
#define WM_BRET_XRUN   0x00064000
#define WM_BRET_XRUN2  0x00088000
#define WM_BRET_ZDRIFT 0x00028000
#define WM_BRET_WALK_VEL  0x0003a000
#define WM_BRET_WALK_DVEL 0x00031000

typedef struct wm_arcade_bret_env {
    uint32_t pcnt;
    int hyper_speed_on;
    int blocking_off;
    int p1rounds;
    int p2rounds;
} wm_arcade_bret_env_t;

typedef struct wm_arcade_bret_callbacks {
    void (*change_anim)(wm_arcade_actor_t *, wm_arcade_bret_anim_id_t, void *);
    void (*change_torso_anim)(wm_arcade_actor_t *, wm_arcade_bret_anim_id_t, void *);
    void (*sound)(wm_arcade_actor_t *, wm_arcade_bret_sound_id_t, void *);
    void (*check_secret_moves)(wm_arcade_actor_t *, const wm_arcade_bret_secret_pattern_t *, size_t, void *);
    void (*execute_walk)(wm_arcade_actor_t *, void *);
    int  (*climb_turnbuckle)(wm_arcade_actor_t *, void *);
    void (*bounce_off_ropes)(wm_arcade_actor_t *, void *);
    int  (*ck_ignore)(wm_arcade_actor_t *, void *);
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
} wm_arcade_bret_callbacks_t;

typedef enum wm_arcade_bret_step_result {
    WM_BRET_STEP_IDLE = 0,
    WM_BRET_STEP_ACTION = 1,
    WM_BRET_STEP_EXTERNAL = 2
} wm_arcade_bret_step_result_t;

/* BRET.ASM animation init and live mode dispatcher. */
void wm_arcade_bret_ani_init(wm_arcade_actor_t *bret,
                             const wm_arcade_bret_callbacks_t *cb);
wm_arcade_bret_step_result_t wm_arcade_move_bret(
    wm_arcade_actor_t *bret,
    wm_arcade_actor_t *opp,
    const wm_arcade_bret_env_t *env,
    const wm_arcade_bret_callbacks_t *cb);

/* Source secret-move handler bodies, invoked after the common recognizer matches. */
int wm_arcade_bret_try_charge_ddt(wm_arcade_actor_t *bret,
                                   wm_arcade_actor_t *opp,
                                   uint16_t powerp_dtime,
                                   const wm_arcade_bret_callbacks_t *cb);

int wm_arcade_bret_fire_secret(wm_arcade_actor_t *bret,
                               wm_arcade_actor_t *opp,
                               wm_arcade_bret_secret_id_t id,
                               uint32_t pcnt,
                               const wm_arcade_bret_callbacks_t *cb);

/* Handler bodies for the persistent hrt_smove_table input-monitor processes.
 * The common N64 input-history scheduler matches wm_arcade_bret_monitor_patterns,
 * then calls this routine for the exact source-side state/reversal/queue behavior. */
int wm_arcade_bret_fire_monitor(wm_arcade_actor_t *bret,
                                wm_arcade_actor_t *opp,
                                wm_arcade_bret_monitor_id_t id,
                                const wm_arcade_bret_env_t *env,
                                int opponent_attack_is_leaping,
                                const wm_arcade_bret_callbacks_t *cb);

/* Charge-process release bodies from hrt_charge_flying_kick/face_rake. */
int wm_arcade_bret_release_charge_flying_kick(wm_arcade_actor_t *bret,
                                               wm_arcade_actor_t *opp,
                                               uint16_t charge_ticks,
                                               const wm_arcade_bret_callbacks_t *cb);
int wm_arcade_bret_release_charge_face_rake(wm_arcade_actor_t *bret,
                                             uint16_t charge_ticks,
                                             const wm_arcade_bret_callbacks_t *cb);

/* Exact 8-way movement velocity table from the end of BRET.ASM. */
void wm_arcade_bret_velocity_for_dir(unsigned dir, int32_t *xvel, int32_t *zvel);

#ifdef __cplusplus
}
#endif
#endif
