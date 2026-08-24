#ifndef WM_ARCADE_COMBAT_H
#define WM_ARCADE_COMBAT_H

#include <stddef.h>
#include <stdint.h>
#include "wm_arcade_combat_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wm_arcade_box3 {
    int32_t x1, x2;
    int32_t y1, y2;
    int32_t z1, z2;
} wm_arcade_box3_t;

typedef struct wm_arcade_frame_box {
    int32_t iani3x;
    int32_t iani3y;
    int32_t iani3z;
    int32_t iani3id;
} wm_arcade_frame_box_t;

typedef struct wm_arcade_actor wm_arcade_actor_t;

/*
 * Merge adapter only.  It names original arcade process fields in portable C;
 * it is not intended to replace the N64 port's wrestler structure.
 */
struct wm_arcade_actor {
    int active;

    int32_t x_int;
    int32_t y_int;
    int32_t z_int;
    /* Full 16.16 positions used by attachment animation commands. */
    int32_t x_fixed;
    int32_t y_fixed;
    int32_t z_fixed;

    uint16_t obj_control;
    uint16_t player_mode;
    uint16_t attack_mode;
    uint16_t anim_mode;
    uint32_t status_flags;

    int32_t move_dir;
    /* WRESTLE.ASM confine_wrestler/fix1/fix2 state. */
    uint16_t can_move_dir;
    uint16_t can_move_temp;
    /* Stage 7 / REACT5: directional and input state used by running/hiptoss reactions. */
    int32_t facing_dir;
    int32_t new_facing_dir;
    uint16_t stick_val_cur;
    int32_t immobilize_time;
    int32_t combo_count;
    /* Stage 4: REACT2 combo-uppercut reads WHOHITME->RPT_COUNT. */
    int32_t rpt_count;
    int32_t in_ring;

    wm_arcade_actor_t *attach_proc;
    wm_arcade_actor_t *smart_target;
    wm_arcade_actor_t *who_i_hit;
    wm_arcade_actor_t *who_hit_me;

    wm_arcade_box3_t hurt_box;

    int32_t attack_xoff;
    int32_t attack_yoff;
    int32_t attack_zoff;
    int32_t attack_width;
    int32_t attack_height;
    int32_t attack_depth;
    wm_arcade_box3_t attack_box;

    int32_t hit_blocker;
    int32_t hit_side;
    int32_t ani_count;
    int32_t ani_count2;

    int32_t getup_time;
    int32_t delay_meter;
    int32_t safe_time;
    int32_t dizzy;
    uint32_t head_grab_time;
    void *meter_proc;
    int32_t wrestler_num;
    int32_t player_num;

    /* Stage 2: REACT1.ASM / ANIM.ASM combat state. */
    uint32_t last_hit_time;
    uint16_t last_damage;          /* source stores a WORD PCNT stamp */
    int16_t next_damage;
    uint32_t special_damage_time;
    uint16_t risk;

    int32_t ptime;
    int32_t stars_flag;
    int32_t debris_x;
    int32_t run_time;
    void *shadtrail_proc;
    void *attimg_cur_frame;
    int32_t my_pal;
    int32_t obj_pal;

    /* Stage 3: concrete REACT1.ASM reaction state. Velocities are source 16.16. */
    int32_t x_vel;
    int32_t y_vel;
    int32_t z_vel;
    /* WRESTLE/WRESTLE2 object motion fields (source 16.16 where noted). */
    int32_t gravity;             /* OBJ_GRAVITY, 16.16 */
    int32_t obj_friction;        /* OBJ_FRICTION, 16.16 */
    int32_t ground_y;            /* integer source-space Y */
    int32_t priority;            /* source object priority selected by calc_ground_y */
    int32_t climbing_thru;       /* CLIMBING_THRU transition flag */
    int32_t roll_pos;
    int32_t usr_var1;
    int32_t usr_var2;              /* PLYR.EQU USR_VAR2; Yoko salt failure flag. */
    int32_t player_side;           /* PLYR.EQU PLYR_SIDE: 0, 1, or -1. */
    int32_t consecutive_hits;
    int32_t life;

    int32_t attach_xoff;
    int32_t attach_yoff;
    int32_t attach_zoff;
    uint16_t attack_time;          /* round_tickcount WORD */

    /* Final combat-core integration: animation/move-dispatch fields. */
    uint16_t ani_speed;
    uintptr_t special_move_addr;   /* source SPECIAL_MOVE_ADDR pointer/token */

    /* Stage 14 / BRET.ASM character-control adapter fields. */
    uint16_t but_val_cur;
    uint16_t but_val_down;
    uint16_t but_val_up;
    uint16_t stick_val_down;
    uint16_t stick_val_up;
    int32_t closest_num;
    int32_t closest_dist;
    int32_t closest_xdist;
    int32_t closest_ydist;
    int32_t closest_zdist;
    int32_t walk_fast;
    int32_t i_will_die;
    uint32_t block_time;
    uint32_t last_headhold;
    uintptr_t code_addr;         /* source CODE_ADDR pointer/token; resolver owns meaning */
    int32_t delay_butns;
    int32_t attack_type;

    /* Combat2CE: fields read/written directly by the original ANIM.ASM VM. */
    int32_t tgt_xoff;
    int32_t tgt_yoff;
    int32_t tgt_zoff;
    int32_t scroll_y;
    uint16_t foot_pcnt;
    int32_t punchb_count;
    int32_t blockb_count;
    int32_t spunchb_count;
    int32_t kickb_count;
    int32_t skickb_count;
    uint32_t anti_combo_time;
    int32_t attachimg_xoff;
    int32_t attachimg_yoff;
    int32_t attachimg_zoff;
    const char *attachimg_frame;
    int32_t source_vm_fault;
};

typedef struct wm_arcade_combat_callbacks {
    int (*victim_has_live_teammates)(const wm_arcade_actor_t *victim, void *user);
    void (*wrestler_hit)(wm_arcade_actor_t *attacker,
                         wm_arcade_actor_t *victim,
                         void *user);
    void (*maybe_gidd_up)(wm_arcade_actor_t *victim, void *user);
    void *user;
} wm_arcade_combat_callbacks_t;

typedef enum wm_arcade_hit_result {
    WM_HIT_REJECTED = 0,
    WM_HIT_ACCEPTED = 1
} wm_arcade_hit_result_t;

void wm_arcade_set_hurt_box(wm_arcade_actor_t *actor,
                            const wm_arcade_frame_box_t *frame);
void wm_arcade_set_attack_box(wm_arcade_actor_t *actor);
int wm_arcade_resolve_overlap(wm_arcade_actor_t *mover,
                              const wm_arcade_actor_t *other);
wm_arcade_hit_result_t wm_arcade_try_attack_hit(
    wm_arcade_actor_t *attacker,
    wm_arcade_actor_t *victim,
    const wm_arcade_combat_callbacks_t *callbacks);
int wm_arcade_check_wrestler_collisions(
    wm_arcade_actor_t **actors,
    size_t actor_count,
    uint32_t round_tick,
    const wm_arcade_combat_callbacks_t *callbacks);
void wm_arcade_wrestler_collisions_off(wm_arcade_actor_t *actor);
void wm_arcade_set_getup_time(
    const wm_arcade_actor_t *attacker,
    wm_arcade_actor_t *victim,
    const wm_arcade_combat_callbacks_t *callbacks);

#ifdef __cplusplus
}
#endif

#endif
