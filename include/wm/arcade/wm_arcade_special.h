#ifndef WM_ARCADE_SPECIAL_H
#define WM_ARCADE_SPECIAL_H

#include <stddef.h>
#include <stdint.h>
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_react.h"
#include "wm/arcade/wm_arcade_react1_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SPECIAL.EQU / SPECIAL.ASM side values. */
enum wm_arcade_special_side {
    WM_SP_SIDE_NEUTRAL = -1,
    WM_SP_SIDE_P1 = 0,
    WM_SP_SIDE_P2 = 1
};

/* SPECIAL.ASM splat_tbl IDs. */
enum wm_arcade_special_id {
    WM_SP_ID_SPIRIT = 0,
    WM_SP_ID_REAPER = 1,
    WM_SP_ID_SALT = 2
};

typedef enum wm_arcade_special_kind {
    WM_SP_KIND_DOINK_PIE = 0,
    WM_SP_KIND_BAM_FIREBALL,
    WM_SP_KIND_TAKER_SPIRIT,
    WM_SP_KIND_TAKER_REAPER,
    WM_SP_KIND_YOKO_SALT
} wm_arcade_special_kind_t;

typedef enum wm_arcade_special_anim {
    WM_SP_ANIM_NONE = 0,
    WM_SP_ANIM_PIE,
    WM_SP_ANIM_FIREBALL,
    WM_SP_ANIM_SPIRIT,
    WM_SP_ANIM_REAPER_GROW,
    WM_SP_ANIM_REAPER,
    WM_SP_ANIM_SALT_GROW,
    WM_SP_ANIM_SALT,
    WM_SP_ANIM_SPIRIT_SPLAT,
    WM_SP_ANIM_REAPER_SPLAT,
    WM_SP_ANIM_SALT_SPLAT,
    WM_SP_ANIM_FIRE_SPLAT,
    WM_SP_ANIM_PIE_SPLAT
} wm_arcade_special_anim_t;

typedef struct wm_arcade_special_obj wm_arcade_special_obj_t;
struct wm_arcade_special_obj {
    /* Relevant SPECIAL.EQU process fields. Positions/velocities are 16.16. */
    int32_t x_fixed;
    int32_t y_fixed;
    int32_t z_fixed;
    wm_arcade_special_obj_t *next;
    uint16_t obj_control;
    uint16_t anim_count;
    uint16_t lifespan;
    uint16_t die;
    wm_arcade_actor_t *owner;
    int16_t player_side;

    int16_t xoff, width;
    int16_t yoff, height;
    int16_t zoff, depth;
    wm_arcade_box3_t collision_box;

    int16_t in_ring;
    int32_t ground_y_fixed;
    int32_t gravity;
    int32_t x_vel;
    int32_t y_vel;
    int32_t z_vel;
    int16_t id;

    wm_arcade_special_kind_t kind;
    wm_arcade_special_anim_t anim;
    int in_list;

    /* Portable interpreter bookkeeping for source-timed combat opcodes. */
    uint16_t source_phase;
    uint16_t source_phase_ticks;
    int source_unchecked_splat_id;
};

typedef struct wm_arcade_special_lists {
    wm_arcade_special_obj_t *p1;
    wm_arcade_special_obj_t *p2;
    wm_arcade_special_obj_t *neutral;
} wm_arcade_special_lists_t;

typedef struct wm_arcade_special_callbacks {
    /* Existing direct-port health / hit_stuff services. */
    const wm_arcade_react_callbacks_t *react;
    /* Existing REACT1 animation/sound adapter, extended by Stage 24 groups. */
    wm_arcade_react1_context_t *react1;
} wm_arcade_special_callbacks_t;

typedef struct wm_arcade_special_collision_result {
    int object_object_hit;
    int unresolved_unchecked_splat_id;
    int wrestler_hits;
    wm_arcade_special_obj_t *last_object;
    wm_arcade_actor_t *last_victim;
} wm_arcade_special_collision_result_t;

void wm_arcade_special_lists_init(wm_arcade_special_lists_t *lists);
/* Cold-initialize a process slot once. Constructors then preserve fields the
 * arcade source does not overwrite (notably SP_ID for pie/fireball). */
void wm_arcade_special_obj_init(wm_arcade_special_obj_t *obj);
void wm_arcade_special_insert(wm_arcade_special_lists_t *lists,
                              wm_arcade_special_obj_t *obj);
void wm_arcade_special_delete(wm_arcade_special_lists_t *lists,
                              wm_arcade_special_obj_t *obj);

const char *wm_arcade_special_anim_source_name(wm_arcade_special_anim_t anim);
void wm_arcade_special_change_anim(wm_arcade_special_obj_t *obj,
                                   wm_arcade_special_anim_t anim);
void wm_arcade_special_set_boxes(wm_arcade_special_obj_t *obj);
void wm_arcade_special_set_all_boxes(wm_arcade_special_lists_t *lists);
void wm_arcade_special_velocity_add(wm_arcade_special_obj_t *obj);
void wm_arcade_special_standard_bounce(wm_arcade_special_obj_t *obj);

/* Exact constructor state from SPECIAL.ASM. */
void wm_arcade_spawn_doink_pie(wm_arcade_special_lists_t *lists,
                               wm_arcade_special_obj_t *obj,
                               wm_arcade_actor_t *owner);
void wm_arcade_spawn_bam_fireball(wm_arcade_special_lists_t *lists,
                                  wm_arcade_special_obj_t *obj,
                                  wm_arcade_actor_t *owner);
void wm_arcade_spawn_taker_spirit(wm_arcade_special_lists_t *lists,
                                  wm_arcade_special_obj_t *obj,
                                  wm_arcade_actor_t *owner);
void wm_arcade_spawn_taker_reaper(wm_arcade_special_lists_t *lists,
                                  wm_arcade_special_obj_t *obj,
                                  wm_arcade_actor_t *owner);
void wm_arcade_spawn_yoko_salt(wm_arcade_special_lists_t *lists,
                               wm_arcade_special_obj_t *obj,
                               wm_arcade_actor_t *owner);

/*
 * Exact source animation-state transitions needed by collision behavior.
 * `wm_arcade_special_tick_source_state` is called once per projectile process
 * iteration AFTER wm_arcade_special_velocity_add(), matching SPECIAL.ASM.
 */
void wm_arcade_special_reaper_finish_grow(wm_arcade_special_obj_t *obj);
void wm_arcade_special_reaper_enable_collision(wm_arcade_special_obj_t *obj);
void wm_arcade_special_salt_become_live(wm_arcade_special_obj_t *obj);
void wm_arcade_special_salt_disable_collision(wm_arcade_special_obj_t *obj);
void wm_arcade_special_tick_source_state(wm_arcade_special_obj_t *obj);

/* REACT1.ASM wrestler_hit_special and SPECIAL.ASM special_hit. */
int wm_arcade_wrestler_hit_special(wm_arcade_special_lists_t *lists,
                                   wm_arcade_special_obj_t *obj,
                                   wm_arcade_actor_t *victim,
                                   wm_arcade_combat_runtime_t *runtime,
                                   const wm_arcade_special_callbacks_t *callbacks);
/* Returns 1 when both source splat-table IDs are 0..2. Returns 0 when the
 * arcade would perform an unchecked out-of-range table read; no substitute
 * animation is invented in that case. */
int wm_arcade_special_hit(wm_arcade_special_lists_t *lists,
                          wm_arcade_special_obj_t *a,
                          wm_arcade_special_obj_t *b);

/* COLLIS.ASM object_collisions order: boxes, P1-v-P2, then P1/P2/neutral v wrestlers. */
wm_arcade_special_collision_result_t wm_arcade_object_collisions(
    wm_arcade_special_lists_t *lists,
    wm_arcade_actor_t **actors,
    size_t actor_count,
    wm_arcade_combat_runtime_t *runtime,
    const wm_arcade_special_callbacks_t *callbacks);

#ifdef __cplusplus
}
#endif

#endif
