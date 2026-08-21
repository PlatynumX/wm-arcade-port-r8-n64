#ifndef WM_ARCADE_REACT1_CORE_H
#define WM_ARCADE_REACT1_CORE_H

#include <stdint.h>
#include "wm_arcade_react.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Semantic names of the exact REACT1.ASM FACETBL/FACE24TBL groups used by
 * this chunk.  The N64 merge layer resolves these names to the already-ported
 * per-wrestler arcade animation sequences; Stage 3 does not invent substitutes.
 */
typedef enum wm_arcade_react1_anim_group {
    WM_R1_ANIM_HITBLOCK = 1,
    WM_R1_ANIM_HITBLOCK_FLAIL,
    WM_R1_ANIM_HEAD_HIT,
    WM_R1_ANIM_HEAD_HIT2,
    WM_R1_ANIM_BODY_HIT,
    WM_R1_ANIM_FALL_BACK,
    WM_R1_ANIM_HIT_ON_GROUND,
    WM_R1_ANIM_FALL_BACK_TBUKL,
    WM_R1_ANIM_LOSE_BALANCE,
    /* REACT2.ASM adds a per-wrestler knockdown FACETBL. */
    WM_R1_ANIM_KNOCKDOWN,
    /* REACT3.ASM / REACT4.ASM tables kept distinct where source differs. */
    WM_R1_ANIM_KNEE_HIT,
    WM_R1_ANIM_QUICK_KNEE_HIT,
    WM_R1_ANIM_SPINKICK_HEAD_HIT,
    WM_R1_ANIM_FALL_BACK2,
    WM_R1_ANIM_JUMPKICK_HEAD_HIT,
    /* REACT5.ASM tables / generic slave animation entry. */
    WM_R1_ANIM_BOUNCE_OFF,
    WM_R1_ANIM_BOUNCE_OFF_DIZZY,
    WM_R1_ANIM_BACKHAND_HEAD_HIT,
    WM_R1_ANIM_EARSLAP_HEAD_HIT,
    WM_R1_ANIM_GET_BUZZ,
    WM_R1_ANIM_WRES_SLAVE,
    /* REACT9.ASM */
    WM_R1_ANIM_BURN,
    /* REACT1 wrestler_hit_special FACETBLs. */
    WM_R1_ANIM_SPECIAL_HEAD_HIT2_SAND,
    WM_R1_ANIM_SPECIAL_BODY_HIT2
} wm_arcade_react1_anim_group_t;

typedef enum wm_arcade_react1_sound {
    WM_R1_SND_BLOCK = 1,
    WM_R1_SND_PUNCH,
    WM_R1_SND_HDBUTT,
    WM_R1_SND_KICK,
    WM_R1_SND_FLYKICK,
    WM_R1_SND_SCREAM,
    WM_R1_SND_UPRCUT,
    WM_R1_SND_LBOWDROP,
    WM_R1_SND_PUSH,
    /* REACT9.ASM Razor slash family */
    WM_R1_SND_RSLASH
} wm_arcade_react1_sound_t;

typedef enum wm_arcade_react1_impact {
    WM_R1_IMPACT_FACE = 1,
    WM_R1_IMPACT_MID,
    WM_R1_IMPACT_DROP_KICK
} wm_arcade_react1_impact_t;

/* Exact named ANIBASE comparisons made by REACT4.ASM hit_stomp. */
typedef enum wm_arcade_react_anim_tag {
    WM_R_ANIMTAG_OTHER = 0,
    WM_R_ANIMTAG_SHN_COMBO_RUN_STOMP,
    WM_R_ANIMTAG_SHN_RUN_STOMP,
    WM_R_ANIMTAG_DNK_BELLY,
    WM_R_ANIMTAG_UND_FLYING_BUTT_DROP,
    WM_R_ANIMTAG_LEX_FLYING_GROUND_PUNCH
} wm_arcade_react_anim_tag_t;

typedef enum wm_arcade_move_grade {
    WM_R_MOVE_AVERAGE = 1,
    WM_R_MOVE_NASTY
} wm_arcade_move_grade_t;

typedef struct wm_arcade_react1_callbacks {
    void (*change_anim)(wm_arcade_actor_t *victim,
                        wm_arcade_react1_anim_group_t group,
                        void *user);
    void (*play_sound)(wm_arcade_actor_t *victim,
                       wm_arcade_react1_sound_t sound,
                       void *user);
    void (*impact)(wm_arcade_actor_t *attacker,
                   wm_arcade_actor_t *victim,
                   wm_arcade_react1_impact_t impact,
                   void *user);

    /* Exact REACT1 Lex hack: true only for lex_flying_kick_anim/super_kick_anim. */
    int (*attacker_uses_lex_flykick_anim)(const wm_arcade_actor_t *attacker,
                                          void *user);

    /* Passed through to Stage 1 set_getup_time's maybe_gidd_up hook. */
    void (*maybe_gidd_up)(wm_arcade_actor_t *victim, void *user);

    /* REACT2 source dependencies. */
    int32_t (*get_health)(const wm_arcade_actor_t *victim, void *user);
    int (*victim_has_live_teammates)(const wm_arcade_actor_t *victim, void *user);
    void (*flash_white)(wm_arcade_actor_t *victim, void *user);
    void (*triple_sound)(wm_arcade_actor_t *victim, uint16_t sound_id, void *user);

    /*
     * REACT3 hit_bigboot executes RNDPER with A0=100 and branches on JRHI.
     * Return nonzero iff the target implementation's RNDPER leaves HI true.
     * A NULL hook makes only that running-victim branch unavailable; the
     * deterministic INAIR and face-hit paths remain directly translatable.
     */
    int (*rndper_hi)(uint16_t argument, void *user);

    /* REACT4 source hooks used for move grading / statistics. */
    void (*move_grade)(wm_arcade_actor_t *attacker,
                       wm_arcade_actor_t *victim,
                       wm_arcade_move_grade_t grade,
                       void *user);

    /* Resolve the exact ANIBASE labels compared by hit_stomp. */
    wm_arcade_react_anim_tag_t (*attacker_anim_tag)(
        const wm_arcade_actor_t *attacker, void *user);

    /* REACT4 bounce helper side effects. */
    void (*shake_all_ropes)(void *user);
    void (*shaker2)(int amount, void *user);

    /* REACT5: transfer the attacker's active GETUP meter to slide_offscr. */
    void (*slide_getup_meter)(wm_arcade_actor_t *attacker, void *user);

    /* Optional visibility for integration/testing after wres_collis_off. */
    void (*collisions_off)(wm_arcade_actor_t *victim, void *user);

    /* Called only when Stage 2 dispatches a reaction not covered by REACT1. */
    void (*unhandled_reaction)(wm_arcade_actor_t *attacker,
                               wm_arcade_actor_t *victim,
                               wm_arcade_reaction_id_t reaction,
                               void *user);
    void *user;
} wm_arcade_react1_callbacks_t;

typedef struct wm_arcade_react1_context {
    const wm_arcade_react1_callbacks_t *callbacks;
    wm_arcade_reaction_id_t last_reaction;
    int last_supported;
    int last_flykick_aborted;
} wm_arcade_react1_context_t;

void wm_arcade_react1_context_init(wm_arcade_react1_context_t *ctx,
                                   const wm_arcade_react1_callbacks_t *callbacks);

/* External REACT1 helpers called directly by REACT2.ASM. */
void wm_arcade_react1_block_hit(wm_arcade_actor_t *attacker,
                                wm_arcade_actor_t *victim,
                                wm_arcade_react1_context_t *ctx);
void wm_arcade_react1_block_hit_flail(wm_arcade_actor_t *attacker,
                                      wm_arcade_actor_t *victim,
                                      wm_arcade_react1_context_t *ctx);

/*
 * Concrete REACT1 reaction dispatcher. Returns nonzero when the supplied
 * reaction is implemented by this chunk.  hit_damage_pending/new_movedir are
 * accepted to match the Stage 2 hook; these REACT1 routines do not rewrite
 * either value.
 */
int wm_arcade_react1_apply(wm_arcade_actor_t *attacker,
                           wm_arcade_actor_t *victim,
                           wm_arcade_reaction_id_t reaction,
                           int16_t *hit_damage_pending,
                           int16_t *new_victim_movedir,
                           wm_arcade_react1_context_t *ctx);

/* Signature-compatible bridge for wm_arcade_react_callbacks.reaction. */
void wm_arcade_react1_reaction_callback(wm_arcade_actor_t *attacker,
                                        wm_arcade_actor_t *victim,
                                        wm_arcade_reaction_id_t reaction,
                                        int16_t *hit_damage_pending,
                                        int16_t *new_victim_movedir,
                                        void *user);

#ifdef __cplusplus
}
#endif

#endif
