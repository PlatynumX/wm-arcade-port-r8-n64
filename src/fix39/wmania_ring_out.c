#include "wmania_ring_out.h"

#include <string.h>

static WmRingOutEvents no_events(void)
{
    WmRingOutEvents e;
    memset(&e, 0, sizeof(e));
    return e;
}

bool wm_ring_kill_when_hit_ground_ready(
    const WmRingOutPlayer *player)
{
    return player != 0 &&
           player->ground_y == player->object_y_int;
}

bool wm_ring_kill_when_hit_ground_apply(
    const WmRingOutPlayer *player,
    WmRingAdjustHealthFn adjust_health,
    void *adjust_health_user)
{
    if (!wm_ring_kill_when_hit_ground_ready(player) ||
        adjust_health == 0) {
        return false;
    }

    (void)adjust_health(
        adjust_health_user,
        player->player_num,
        -150,
        0);

    return true;
}

bool wm_ring_do_ringout_dufus(
    const WmRingOutPlayer *self,
    const WmRingOutPlayer *players,
    size_t player_count,
    uint32_t tsec_ticks,
    bool ring_out_on)
{
    size_t i;
    int32_t four_seconds;

    if (self == 0 || players == 0) return false;

    /*
     * SPECIAL.ASM:
     * if ring_out_on != 0 -> exit
     * self must be below -(7*TSEC - 3*TSEC) == -4*TSEC.
     */
    if (ring_out_on) return false;

    four_seconds = (int32_t)(tsec_ticks * 4u);
    if (self->ring_time >= -four_seconds) return false;

    for (i = 0u; i < player_count; ++i) {
        const WmRingOutPlayer *p = &players[i];

        if (!p->active) continue;
        if (p->player_side == self->player_side) continue;

        /*
         * Source does NOT skip dead opponents in do_ringout_dufus.
         * Any opponent with RING_TIME <= +4*TSEC suppresses the message.
         */
        if (p->ring_time <= four_seconds) return false;
    }

    return true;
}

WmRingOutEvents wm_ring_are_we_in_ring_tick(
    WmRingOutPlayer *self,
    WmRingOutPlayer *players,
    size_t player_count,
    uint32_t pcnt,
    uint32_t tsec_ticks,
    bool halt,
    bool ring_out_on,
    WmRingAdjustHealthFn adjust_health,
    void *adjust_health_user)
{
    WmRingOutEvents e = no_events();
    size_t i;
    int32_t out_time;
    bool killed;

    if (self == 0 || players == 0) return e;

    /* don't do anything if HALT is set */
    if (halt) return e;

    /*
     * DEAD special path comes before closest-opponent logic.
     * JRP means strictly positive: positive increments; zero/negative -> 1.
     */
    if (self->player_mode == WM_RING_MODE_DEAD) {
        if (self->ring_time > 0) {
            ++self->ring_time;
        } else {
            self->ring_time = 1;
        }
        return e;
    }

    /*
     * Source short-circuits if closest opponent is DEAD.
     * It assumes CLOSEST_NUM indexes a valid process.
     */
    if ((size_t)self->closest_num >= player_count) return e;
    if (!players[self->closest_num].active) return e;
    if (players[self->closest_num].player_mode == WM_RING_MODE_DEAD) return e;

    /* signed RING_TIME transition state */
    if (self->ring_time >= 0) {
        ++self->ring_time;

        if (self->inring != 0) {
            self->ring_time = -1;
            if (ring_out_on) {
                e.spawn_kill_when_hit_ground = true;
            }
        }
    } else {
        --self->ring_time;

        if (self->inring == 0) {
            self->ring_time = 1;
        }
    }

    e.show_dufus_message_3 = wm_ring_do_ringout_dufus(
        self, players, player_count, tsec_ticks, ring_out_on);

    out_time = (int32_t)(tsec_ticks * 7u);

    /*
     * Greater/equal -out_time means inside or not outside long enough.
     */
    if (self->ring_time >= -out_time) return e;

    /* Source low-three-bit test: only every eighth PCNT. */
    if ((pcnt & 7u) != 0u) return e;

    /*
     * Potential pain: every active opponent from another side must have an
     * adjusted RING_TIME > +out_time.
     */
    for (i = 0u; i < player_count; ++i) {
        const WmRingOutPlayer *p = &players[i];
        int32_t adjusted;

        if (!p->active) continue;
        if (p->player_side == self->player_side) continue;

        adjusted = p->ring_time;

        /*
         * Sleeping-drone hack:
         * if PTIME != 1, add (0x7fff - PTIME).
         */
        if (p->ptime != 1u) {
            adjusted += (int32_t)0x7fff - (int32_t)p->ptime;
        }

        if (adjusted <= out_time) return e;
    }

    e.health_adjusted = true;
    e.health_delta = -1;

    if (adjust_health == 0) {
        /*
         * We can report the exact requested damage, but without the source
         * health system's carry result we must not invent death.
         */
        return e;
    }

    killed = adjust_health(
        adjust_health_user,
        self->player_num,
        -1,
        0);

    if (!killed) return e;

    /* DEATH BY RING-OUT */
    self->player_mode = WM_RING_MODE_DEAD;
    e.death_by_ringout = true;

    /*
     * Skip disqual if any active same-side teammate is alive, or dead but
     * carrying the zombie status flag.
     */
    for (i = 0u; i < player_count; ++i) {
        const WmRingOutPlayer *p = &players[i];

        if (!p->active) continue;
        if (p == self) continue;
        if (p->player_side != self->player_side) continue;

        if (p->player_mode != WM_RING_MODE_DEAD) {
            return e;
        }

        if ((p->status_flags & WM_RING_STATUS_ZOMBIE) != 0u) {
            return e;
        }
    }

    e.create_disqual = true;
    e.announce_round_winner = true;
    return e;
}
