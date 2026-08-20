#ifndef WM_ARCADE_ROSTER_H
#define WM_ARCADE_ROSTER_H

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum wm_arcade_roster_id {
    WM_ROSTER_BRET = 0,
    WM_ROSTER_RAZOR = 1,
    WM_ROSTER_TAKER = 2,
    WM_ROSTER_YOKO = 3,
    WM_ROSTER_SHAWN = 4,
    WM_ROSTER_BAM = 5,
    WM_ROSTER_DOINK = 6,
    WM_ROSTER_LEX = 8
} wm_arcade_roster_id_t;

typedef struct wm_arcade_input_step {
    uint16_t value;
    uint16_t ignore_mask;
} wm_arcade_input_step_t;

typedef struct wm_arcade_input_pattern {
    const char *source_label;
    const wm_arcade_input_step_t *steps;
    uint16_t step_count;
    uint16_t max_ticks;
} wm_arcade_input_pattern_t;

typedef struct wm_arcade_wrestler_profile {
    wm_arcade_roster_id_t id;
    const char *name;
    const char *source_file;
    unsigned source_lines;
    const char *prefix;
    uint16_t charge_button;
    uint16_t charge_ticks;
    const wm_arcade_input_pattern_t *secrets;
    size_t secret_count;
    const char *const *special_processes;
    size_t special_process_count;
} wm_arcade_wrestler_profile_t;

typedef struct wm_arcade_roster_env {
    uint32_t pcnt;
    int hyper_speed_on;
    int blocking_off;
    int p1rounds;
    int p2rounds;
} wm_arcade_roster_env_t;

typedef struct wm_arcade_roster_callbacks {
    void (*change_anim_label)(wm_arcade_actor_t *, const char *source_label, void *);
    void (*change_torso_label)(wm_arcade_actor_t *, const char *source_label, void *);
    void (*sound_label)(wm_arcade_actor_t *, const char *source_label, void *);
    void (*check_secret_moves)(wm_arcade_actor_t *, const wm_arcade_input_pattern_t *, size_t, void *);
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
    void (*jump_rope_audio)(wm_arcade_actor_t *, void *);
    void (*master_keep_attached)(wm_arcade_actor_t *, void *);
    void (*keep_attached)(wm_arcade_actor_t *, void *);
    void (*mode_dead)(wm_arcade_actor_t *, void *);
    void (*mode_puppet)(wm_arcade_actor_t *, void *);
    void (*mode_inair2)(wm_arcade_actor_t *, void *);
    void (*mode_choking)(wm_arcade_actor_t *, void *);
    void (*code_addr)(wm_arcade_actor_t *, uint32_t token, void *);
    uintptr_t (*resolve_label_token)(const char *source_label, void *);
    void (*start_special_label)(wm_arcade_actor_t *, const char *source_label, void *);
    void *user;
} wm_arcade_roster_callbacks_t;

typedef enum wm_arcade_roster_step_result {
    WM_ROSTER_STEP_IDLE = 0,
    WM_ROSTER_STEP_ACTION = 1,
    WM_ROSTER_STEP_EXTERNAL = 2
} wm_arcade_roster_step_result_t;

extern const wm_arcade_wrestler_profile_t wm_arcade_profile_bret;
extern const wm_arcade_wrestler_profile_t wm_arcade_profile_razor;
extern const wm_arcade_wrestler_profile_t wm_arcade_profile_taker;
extern const wm_arcade_wrestler_profile_t wm_arcade_profile_yoko;
extern const wm_arcade_wrestler_profile_t wm_arcade_profile_shawn;
extern const wm_arcade_wrestler_profile_t wm_arcade_profile_bam;
extern const wm_arcade_wrestler_profile_t wm_arcade_profile_doink;
extern const wm_arcade_wrestler_profile_t wm_arcade_profile_lex;

const wm_arcade_wrestler_profile_t *wm_arcade_roster_profile(wm_arcade_roster_id_t id);

/* Character behavior is intentionally not implemented here.
 * Each wrestler has a dedicated direct-port module (wm_arcade_<name>.c). */

/* Exact shared 32-entry action selector used by the six source files. */
uint8_t wm_arcade_roster_action_for_buttons(uint16_t but_val_down);

#ifdef __cplusplus
}
#endif
#endif
