#ifndef WM_ARCADE_DRONE_H
#define WM_ARCADE_DRONE_H

#include <stddef.h>
#include <stdint.h>
#include "wm_arcade_combat.h"
#include "wm_arcade_damage.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Direct portable representation of DRONE.ASM process-local state.
 * missed_blocks[] is the logical per-drone slice of source atkcnt_t
 * (source indexes atkcnt_t as PLYRNUM*AT_NUM + ATTACK_TYPE).
 */
typedef struct wm_arcade_drone_state {
    int32_t mode;              /* DRN_MODE: -3..2 */
    int32_t skill;             /* logical source skill index 0..29 */
    int32_t delay;             /* DRN_DELAY */
    uint16_t but;              /* DRN_BUT */
    uint16_t joy;              /* DRN_JOY */
    uint16_t but_down;         /* DRN_BUTDT */
    uint16_t but_up;           /* DRN_BUTUT */
    uint16_t joy_down;         /* DRN_JOYDT */
    uint16_t joy_up;           /* DRN_JOYUT */
    uint16_t but_charge;       /* DRN_BUTCHRG */
    int32_t but_charge_delay;  /* DRN_BUTCHRGDLY */
    const char *charge_script; /* DRN_BUTCHRG_p exact source label */
    const char *script;        /* DRN_ACT_p exact source label */
    size_t script_pc;          /* portable offset inside decoded source script */
    int32_t script_mode;       /* DRN_SPMODE */
    int32_t seek_dir;          /* DRN_SEEKDIR */
    int32_t seek_dist;         /* DRN_SEEKDIST */
    uint16_t missed_blocks[WM_AT_NUM]; /* source atkcnt_t slice */
} wm_arcade_drone_state_t;

typedef struct wm_arcade_drone_world {
    wm_arcade_actor_t **actors;
    size_t actor_count;
    uint32_t pcnt;
    uint16_t round_tickcount;
    int first_ladder;          /* CURRENT_LADDER == LADDER */
} wm_arcade_drone_world_t;

/* DRONE.ASM mode-list entry ultimately points to a list whose first word is
 * the maximum random index. A negative source word marks a headhold list;
 * the absolute value is still the max index. On skill > 26 source forces 1.
 */
typedef struct wm_arcade_drone_script_list {
    int32_t source_max_index;
    const char *const *scripts;
    size_t script_count;
} wm_arcade_drone_script_list_t;

typedef enum wm_arcade_drone_script_opcode {
    WM_DRONE_SC_INPUT = 0,
    WM_DRONE_SC_SEEK = 1,              /* source command #1 */
    WM_DRONE_SC_SKILL_ABORT = 2,       /* source command #2 */
    WM_DRONE_SC_WAIT_INTERRUPTIBLE = 3,/* source command #3 */
    WM_DRONE_SC_ABORT_IF_BLOCKING = 4, /* source command #4 */
    WM_DRONE_SC_CALL_CODE = 5,         /* source command #5 */
    WM_DRONE_SC_RANDOM_JUMP = 6,       /* source command #6 */
    WM_DRONE_SC_JUMP = 7,              /* source command #7 */
    WM_DRONE_SC_CALL_FUNCTION = 8,     /* source EXGPC fallback */
    WM_DRONE_SC_DONE = 9               /* source command bit 14 */
} wm_arcade_drone_script_opcode_t;

typedef struct wm_arcade_drone_script_op {
    wm_arcade_drone_script_opcode_t opcode;
    uint16_t input_word;       /* low 5 buttons, high bits MOVE_* exactly like source */
    int32_t delay;             /* input delay */
    int32_t percent;           /* RANDOM_JUMP */
    size_t target_pc;          /* RANDOM_JUMP/JUMP */
    const char *source_label;  /* skill table or code/function label */
} wm_arcade_drone_script_op_t;

typedef struct wm_arcade_drone_script {
    const char *source_label;
    const wm_arcade_drone_script_op_t *ops;
    size_t op_count;
} wm_arcade_drone_script_t;

typedef struct wm_arcade_drone_callbacks {
    /* DRONE.ASM uses both rnd and rndrng0; keep the services distinct. */
    uint32_t (*rnd_upto)(uint32_t max_inclusive, void *user);
    uint32_t (*rndrng0_upto)(uint32_t max_inclusive, void *user);
    wm_arcade_actor_t *(*closest_actor)(wm_arcade_actor_t *actor, void *user);
    wm_arcade_actor_t *(*closest_actor_for)(wm_arcade_actor_t *actor, void *user);
    int32_t (*closest_dist_for)(const wm_arcade_actor_t *actor, void *user);

    /* Literal source-table seams. No fallback gameplay values are invented. */
    int32_t (*block_base_pct)(int skill, void *user);          /* blkbase_t */
    int32_t (*block_attack_pct)(int missed_count, void *user); /* blkatk_t */
    int32_t (*headhold_delay_max)(int skill, void *user);      /* sklhhdly_t */
    int32_t (*headheld_delay_max)(int skill, void *user);      /* sklhrdly_t */

    int (*check_combo_go)(wm_arcade_actor_t *actor, void *user);
    void (*seek_dir_dist)(wm_arcade_actor_t *actor, wm_arcade_drone_state_t *drone, void *user);

    /* Direct table resolver for wnshort_t / wnmed_t / wnlong_t mode lists. */
    const wm_arcade_drone_script_list_t *(*range_script_list)(
        const wm_arcade_actor_t *self, const wm_arcade_actor_t *opp,
        int range_band, int my_mode, int opp_mode, void *user);

    /* Decoded script VM seams for the source pointers/calls. */
    const wm_arcade_drone_script_t *(*resolve_script)(const char *source_label, void *user);
    int32_t (*script_skill_pct)(const char *source_table_label, int skill, void *user);
    int (*script_seek)(wm_arcade_actor_t *self, wm_arcade_drone_state_t *drone, void *user);
    void (*script_call)(wm_arcade_actor_t *self, const char *source_label, void *user);

    void (*script_selected)(wm_arcade_actor_t *self, const char *source_label, void *user);
    void *user;
} wm_arcade_drone_callbacks_t;

typedef enum wm_arcade_drone_step_result {
    WM_DRONE_STEP_IDLE = 0,
    WM_DRONE_STEP_INPUT = 1,
    WM_DRONE_STEP_SCRIPT = 2,
    WM_DRONE_STEP_BLOCK = 3,
    WM_DRONE_STEP_ABORT_SCRIPT = 4
} wm_arcade_drone_step_result_t;

void wm_arcade_drone_init(wm_arcade_drone_state_t *drone, int skill);
int wm_arcade_drone_getup_pct(int skill);

/* Direct decoded-port of DRONE.ASM's script command interpreter. */
wm_arcade_drone_step_result_t wm_arcade_drone_script_step(
    wm_arcade_actor_t *self,
    wm_arcade_actor_t *opp,
    wm_arcade_drone_state_t *drone,
    const wm_arcade_drone_script_t *script,
    const wm_arcade_drone_callbacks_t *cb);

/* Direct-port decision core from DRONE.ASM::drone_main. */
wm_arcade_drone_step_result_t wm_arcade_drone_main(
    wm_arcade_actor_t *self,
    wm_arcade_drone_state_t *drone,
    const wm_arcade_drone_world_t *world,
    const wm_arcade_drone_callbacks_t *cb);

/* DRONE.ASM final transition calculation; also mirrors values into actor input fields. */
void wm_arcade_drone_commit_inputs(wm_arcade_actor_t *actor,
                                   wm_arcade_drone_state_t *drone,
                                   uint16_t old_but,
                                   uint16_t old_joy);

#ifdef __cplusplus
}
#endif
#endif
