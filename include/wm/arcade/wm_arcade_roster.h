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
    /*
     * ANIM.ASM has TWO primary-animation entry points and the eight
     * wrestler files call both, deliberately:
     *
     *   :4532 change_anim1   -- if MODE_END_BIT is set restart anyway,
     *                           otherwise compare ANIBASE to the request
     *                           and RETURN WITHOUT RESTARTING when they
     *                           are the same animation.
     *   :4542 change_anim1a  -- the tail of change_anim1, entered
     *                           directly: always reset ANIBASE, ANIPC,
     *                           ANIMODE, ANICNT and OBJ_GRAVITY and run
     *                           animate_wrestler1 from the top.
     *
     * change_anim_label is change_anim1 (guarded); change_anim_restart
     * is change_anim1a (unguarded). Which one a move uses is visible
     * behaviour, not a detail: a held block must NOT retrigger each
     * tick, while a mashed stomp MUST replay from frame 0. 44 of the
     * roughly 450 call sites in the eight wrestler files are the
     * guarded form; everything else is change_anim1a.
     */
    void (*change_anim_label)(wm_arcade_actor_t *, const char *source_label, void *);
    void (*change_anim_restart)(wm_arcade_actor_t *, const char *source_label, void *);
    /*
     * JJXM.H:44 `RND_AWARD a13,BLOCKS_AWD`, which expands to
     * round_award(a13, BLOCKS_AWD). Every wrestler file has EXACTLY ONE,
     * in std_block right after the @blocking_off gate and right before
     * the block animation -- BRET.ASM:1571, RAZOR.ASM:1415,
     * TAKER.ASM:1768, YOKO.ASM:1390, SHAWN.ASM:2000, BAM.ASM:1531,
     * DOINK.ASM:1946, LEX.ASM:1381.
     *
     * So the answer to "which block events does the source score" is:
     * ENTERING a block, once, and nothing else. Not a successful block,
     * not a blocked hit, not holding one. That was the open question
     * behind this seam, and reading the eight call sites settles it.
     *
     * The seam existed only in the Bret and Razor structs, so six of the
     * eight wrestlers could not score a block award at all even with it
     * filled.
     */
    void (*round_award_block)(wm_arcade_actor_t *, void *);
    /* :4563 change_anim2 / :4573 change_anim2a, the same split on the
       second channel. The guarded form is the walk torso and the three
       mode_oppoverhead stands; every *_ani_init is the unguarded one. */
    void (*change_torso_label)(wm_arcade_actor_t *, const char *source_label, void *);
    void (*change_torso_restart)(wm_arcade_actor_t *, const char *source_label, void *);
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
    /*
     * DCSSOUND.ASM:2914 ADD_IF_SILENT, called with a SPEECH TABLE in a2
     * and an RNDPER percentage in a0 -- the announcer's random-phrase
     * picker, not a WRSND.
     *
     * The wrestler files call it at TWO places around the turnbuckle,
     * with TWO DIFFERENT TABLES, and the port had one seam for both:
     *
     *   climb_rope_audio  mode_normal's climb branch, after
     *                     climb_turnbuckle sets carry -- BRET.ASM:1456
     *                     `MOVI CLIMB_ROPES,A2 / MOVI 1000,A0` and its
     *                     eight siblings. Nine call sites.
     *   jump_rope_audio   mode_turn, the dive off the top --
     *                     BRET.ASM:2296 `movi JUMP_ROPES,a2` and its
     *                     eight. Nine call sites.
     *
     * They are different tables with different headers: CLIMB_ROPES
     * draws one word from twelve rows and carries CRESCENDO_TABLE as its
     * crowd reaction, JUMP_ROPES draws two words from six and carries
     * ROPES_CHEER. Climbing up and leaping off do not sound the same.
     *
     * The registers differ too: the climb passes WRESTLERNUM in a5 and
     * the dive passes PLYRNUM. That selects whose voice a personal call
     * uses, and neither of these two tables holds one, so it does not
     * change what is said today -- recorded because it is a real
     * difference and the next table to reach one of these sites might.
     */
    void (*climb_rope_audio)(wm_arcade_actor_t *, void *);
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
