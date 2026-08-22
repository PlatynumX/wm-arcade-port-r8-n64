#ifndef WM_ARCADE_REACT_H
#define WM_ARCADE_REACT_H

#include <stdint.h>
#include "wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WM_ARCADE_ROSTER_COUNT 9
#define WM_ARCADE_MOD_ONE_8_8  0x100
#define WM_ARCADE_MOD_35PCT_8_8 89
#define WM_ARCADE_RISK_HIGH_BIT 0x8000u

typedef enum wm_arcade_reaction_id {
    WM_RXN_PUNCH = 0,
    WM_RXN_HDBUTT,
    WM_RXN_KICK,
    WM_RXN_FLYKICK,
    WM_RXN_GRABTHROW,
    WM_RXN_UPRCUT,
    WM_RXN_LBOWDROP,
    WM_RXN_GRABHOLD,
    WM_RXN_GRABFLING,
    WM_RXN_PUSH,
    WM_RXN_URN,
    WM_RXN_BIGBOOT,
    WM_RXN_KNEE,
    WM_RXN_HDBUTT2,
    WM_RXN_BOXPUNCH,
    WM_RXN_STOMP,
    WM_RXN_SPINKICK,
    WM_RXN_CLINE,
    WM_RXN_HEADHOLD,
    WM_RXN_JUMPKICK,
    WM_RXN_RUN,
    WM_RXN_PUPPET,
    WM_RXN_BACKHAND,
    WM_RXN_BUZZ,
    WM_RXN_HAYMAKER,
    WM_RXN_BLBOWDROP,
    WM_RXN_BSTOMP,
    WM_RXN_HEADKNEES,
    WM_RXN_EARSLAP,
    WM_RXN_HAMMER,
    WM_RXN_BUTTSTOMP,
    WM_RXN_PUPPET2,
    WM_RXN_PUPPET_HDGRAB,
    WM_RXN_TOMB,
    WM_RXN_BIGKNEE,
    WM_RXN_SHNBFKIK,
    WM_RXN_SHNSPDKIK,
    WM_RXN_SHNSPDKIK2,
    WM_RXN_HITCHECK,
    WM_RXN_COMBO_UPRCUT,
    WM_RXN_RSLASH,
    WM_RXN_HEADDSLASH,
    WM_RXN_HEADUSLASH,
    WM_RXN_HDBUTT_STAY,
    WM_RXN_FIRE_PUNCH,
    WM_RXN_BSTOMP2,
    WM_RXN_GUTPUSH,
    WM_RXN_SUPER_KICK,
    WM_RXN_PUPPET_NOFLAIL,
    WM_RXN_PUPPET_TOSS,
    WM_RXN_NAPALM,
    WM_RXN_ONTURNBUCKLE
} wm_arcade_reaction_id_t;

typedef enum wm_arcade_partner_breakout {
    WM_PARTNER_GOTO_STAND = 1,
    WM_PARTNER_ABORT_ATTACH_ANIM = 2
} wm_arcade_partner_breakout_t;

typedef struct wm_arcade_combat_runtime {
    uint32_t pcnt;
    uint16_t round_tickcount;
    int any_hits;
    int dam_mult;
} wm_arcade_combat_runtime_t;

typedef struct wm_arcade_react_callbacks {
    /* REACT1 good_run_hit: nonzero means the run hit is genuine. */
    int (*good_run_hit)(wm_arcade_actor_t *attacker,
                        wm_arcade_actor_t *victim,
                        void *user);

    /*
     * Called at the exact REACT1 hit_table point, before health is applied.
     * Reaction code is allowed to rewrite both globals, matching the source.
     */
    void (*reaction)(wm_arcade_actor_t *attacker,
                     wm_arcade_actor_t *victim,
                     wm_arcade_reaction_id_t reaction,
                     int16_t *hit_damage_pending,
                     int16_t *new_victim_movedir,
                     void *user);

    /* Existing port implementation of the arcade adjust_health routine. */
    void (*adjust_health)(wm_arcade_actor_t *victim,
                          int16_t signed_delta,
                          wm_arcade_actor_t *damage_source,
                          void *user);

    void (*round_first_hit_award)(wm_arcade_actor_t *attacker, void *user);
    void (*first_hit_message)(wm_arcade_actor_t *attacker, void *user);
    void (*bonus_message)(wm_arcade_actor_t *attacker, void *user);

    /* Renderer-specific cleanup for the source's DMAWNZ/control reset. */
    void (*restore_hit_render_state)(wm_arcade_actor_t *actor, void *user);

    /* Exact animation hooks named by REACT1 hit_stuff. */
    void (*partner_breakout)(wm_arcade_actor_t *partner,
                             wm_arcade_partner_breakout_t kind,
                             void *user);
    void (*ditch_getup_meter)(wm_arcade_actor_t *victim, void *user);

    void *user;
} wm_arcade_react_callbacks_t;

typedef enum wm_arcade_wrestler_hit_status {
    WM_WRESTLER_HIT_OK = 0,
    WM_WRESTLER_HIT_IGNORED_RUN = 1,
    WM_WRESTLER_HIT_BAD_ARGUMENT = -1,
    WM_WRESTLER_HIT_BAD_ATTACK_MODE = -2,
    WM_WRESTLER_HIT_BAD_WRESTLER_INDEX = -3,
    WM_WRESTLER_HIT_NEEDS_RUN_HOOK = -4
} wm_arcade_wrestler_hit_status_t;

typedef struct wm_arcade_wrestler_hit_result {
    wm_arcade_wrestler_hit_status_t status;
    int16_t damage_before_reaction;
    int16_t damage_after_reaction;
    int16_t new_victim_movedir;
    wm_arcade_reaction_id_t reaction;
    int reaction_hook_called;
    int health_hook_called;
} wm_arcade_wrestler_hit_result_t;

/* Ready-made adapter for Stage 1 wm_arcade_combat_callbacks.wrestler_hit. */
typedef struct wm_arcade_react_bridge {
    wm_arcade_combat_runtime_t *runtime;
    const wm_arcade_react_callbacks_t *callbacks;
    wm_arcade_wrestler_hit_result_t last_result;
} wm_arcade_react_bridge_t;

void wm_arcade_combat_runtime_init(wm_arcade_combat_runtime_t *runtime);

/* Direct table translation from REACT1.ASM damage_values. */
int wm_arcade_attack_damage_pair(uint16_t attack_mode,
                                 int16_t *full_damage,
                                 int16_t *reduced_damage);
wm_arcade_reaction_id_t wm_arcade_attack_reaction(uint16_t attack_mode);

/* Direct safe portion of REACT1 hit_stuff (through RUN_TIME cleanup). */
void wm_arcade_hit_stuff(wm_arcade_actor_t *attacker,
                         wm_arcade_actor_t *victim,
                         const wm_arcade_react_callbacks_t *callbacks);

/*
 * Same REACT1 hit_stuff body, but with an opaque attacker-process identity.
 * SPECIAL.ASM/COLLIS.ASM pass special-process pointers through the same source
 * routine, so Stage 24 needs identity comparison without pretending a special
 * process is a wrestler actor.
 */
void wm_arcade_hit_stuff_identity(const void *attacker_identity,
                                  int attacker_is_hitcheck,
                                  wm_arcade_actor_t *victim,
                                  const wm_arcade_react_callbacks_t *callbacks);

/* REACT1.ASM wrestler_hit dispatcher. */
wm_arcade_wrestler_hit_result_t wm_arcade_wrestler_hit(
    wm_arcade_actor_t *attacker,
    wm_arcade_actor_t *victim,
    wm_arcade_combat_runtime_t *runtime,
    const wm_arcade_react_callbacks_t *callbacks);

void wm_arcade_wrestler_hit_collision_callback(
    wm_arcade_actor_t *attacker,
    wm_arcade_actor_t *victim,
    void *bridge_user);

#ifdef __cplusplus
}
#endif

#endif
