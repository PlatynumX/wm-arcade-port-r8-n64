#ifndef WM_ARCADE_WRESTLER_PORT_H
#define WM_ARCADE_WRESTLER_PORT_H

#include "wm/arcade/wm_arcade_roster.h"
#include "wm/arcade/wm_arcade_bret.h"
#include "wm/arcade/wm_arcade_razor.h"
#include "wm/arcade/wm_arcade_taker.h"
#include "wm/arcade/wm_arcade_yoko.h"
#include "wm/arcade/wm_arcade_shawn.h"
#include "wm/arcade/wm_arcade_bam.h"
#include "wm/arcade/wm_arcade_doink.h"
#include "wm/arcade/wm_arcade_lex.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Stage 23 unified roster surface.
 *
 * Dispatch only: all eight wrestlers execute dedicated direct-port modules.
 * No character behavior is implemented in this wrapper.
 */
typedef struct wm_arcade_wrestler_port_bindings {
    const wm_arcade_bret_callbacks_t *bret;
    const wm_arcade_razor_callbacks_t *razor;
    const wm_arcade_taker_callbacks_t *taker;
    const wm_arcade_yoko_callbacks_t *yoko;
    const wm_arcade_shawn_callbacks_t *shawn;
    const wm_arcade_bam_callbacks_t *bam;
    const wm_arcade_doink_callbacks_t *doink;
    const wm_arcade_lex_callbacks_t *lex;
} wm_arcade_wrestler_port_bindings_t;

wm_arcade_roster_step_result_t wm_arcade_move_ported_wrestler(
    const wm_arcade_wrestler_profile_t *profile,
    wm_arcade_actor_t *wrestler,
    wm_arcade_actor_t *opponent,
    const wm_arcade_roster_env_t *env,
    const wm_arcade_wrestler_port_bindings_t *bindings);

/* Execute a source secret-handler by its original source-table label. */
int wm_arcade_port_fire_secret(
    const wm_arcade_wrestler_profile_t *profile,
    wm_arcade_actor_t *wrestler,
    wm_arcade_actor_t *opponent,
    const char *source_label,
    uint32_t pcnt,
    const wm_arcade_wrestler_port_bindings_t *bindings);

/*
 * Execute the release body for source charge processes/probes.
 * source_label is the literal source symbol (for example charge_ddt,
 * hrt_charge_flying_kick, charge_flying_kick, rzr_charge_slashes).
 */
int wm_arcade_port_release_charge(
    const wm_arcade_wrestler_profile_t *profile,
    wm_arcade_actor_t *wrestler,
    wm_arcade_actor_t *opponent,
    const char *source_label,
    uint16_t charge_ticks,
    const wm_arcade_wrestler_port_bindings_t *bindings);

/* Execute an already-matched persistent special-move monitor by source label. */
int wm_arcade_port_fire_monitor(
    const wm_arcade_wrestler_profile_t *profile,
    wm_arcade_actor_t *wrestler,
    wm_arcade_actor_t *opponent,
    const char *source_label,
    const wm_arcade_roster_env_t *env,
    int opponent_attack_is_leaping,
    const wm_arcade_wrestler_port_bindings_t *bindings);

#ifdef __cplusplus
}
#endif
#endif
