#ifndef WMANIA_RING_OUT_H
#define WMANIA_RING_OUT_H

#include "wmania_ring_player_fields.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool active;

    uint8_t player_num;
    int16_t player_side;
    int16_t player_mode;
    uint16_t status_flags;

    int16_t inring;
    int32_t ring_time;
    uint16_t ptime;

    int16_t ground_y;
    int16_t object_y_int;

    uint8_t closest_num;
} WmRingOutPlayer;

/*
 * adjust_health returns the source carry result:
 * false = still alive, true = health adjustment killed player.
 */
typedef bool (*WmRingAdjustHealthFn)(
    void *user,
    uint8_t player_num,
    int16_t delta,
    int16_t source_a10_zero);

typedef struct {
    bool spawn_kill_when_hit_ground;
    bool show_dufus_message_3;

    bool health_adjusted;
    int16_t health_delta;

    bool death_by_ringout;
    bool create_disqual;
    bool announce_round_winner;
} WmRingOutEvents;

/*
 * Direct SPECIAL.ASM kill_when_hit_ground process body for one scheduler
 * tick. Returns true only at exact GROUND_Y == OBJ_YPOSINT; caller should
 * then apply -150 and terminate that helper process.
 */
bool wm_ring_kill_when_hit_ground_ready(
    const WmRingOutPlayer *player);

/*
 * Direct do_ringout_dufus predicate.
 * `ring_out_on != 0` suppresses this source message.
 */
bool wm_ring_do_ringout_dufus(
    const WmRingOutPlayer *self,
    const WmRingOutPlayer *players,
    size_t player_count,
    uint32_t tsec_ticks,
    bool ring_out_on);

/*
 * Direct ARE_WE_IN_RING tick.
 *
 * This routine mutates self->ring_time and self->player_mode and reports
 * source process/event creations through WmRingOutEvents.
 */
WmRingOutEvents wm_ring_are_we_in_ring_tick(
    WmRingOutPlayer *self,
    WmRingOutPlayer *players,
    size_t player_count,
    uint32_t pcnt,
    uint32_t tsec_ticks,
    bool halt,
    bool ring_out_on,
    WmRingAdjustHealthFn adjust_health,
    void *adjust_health_user);

/*
 * Apply the -150 source helper hit when kill_when_hit_ground reaches ground.
 */
bool wm_ring_kill_when_hit_ground_apply(
    const WmRingOutPlayer *player,
    WmRingAdjustHealthFn adjust_health,
    void *adjust_health_user);

#ifdef __cplusplus
}
#endif

#endif
