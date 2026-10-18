/*
 * WRESTLE2.ASM's roster predicates.
 */
#include "wm/arcade/wm_arcade_teammates.h"

/*
 * The shared preamble of all four scans: `move *a1+,a3,L / jrz #nxt`
 * skips an empty slot, `cmp a3,a13 / jreq #nxt` skips yourself, and
 * comparing PLYR_SIDE skips the other team.
 */
static bool is_teammate(const wm_arcade_actor_t *self,
                        const wm_arcade_actor_t *other)
{
    if (!other || other == self) {
        return false;
    }
    return other->player_side == self->player_side;
}

bool wm_ck_teammate_pin(const wm_arcade_actor_t *self,
                        wm_arcade_actor_t *const *actors, size_t actor_count)
{
    size_t i;

    if (!self || !actors) {
        return false;
    }
    for (i = 0; i < actor_count && i < WM_NUM_WRES; ++i) {
        if (!is_teammate(self, actors[i])) {
            continue;
        }
        if (actors[i]->status_flags & WM_STATUS_DID_PIN) {
            return true;
        }
    }
    return false;
}

bool wm_ck_live_teammates(const wm_arcade_actor_t *self,
                          wm_arcade_actor_t *const *actors,
                          size_t actor_count)
{
    size_t i;

    if (!self || !actors) {
        return false;
    }
    for (i = 0; i < actor_count && i < WM_NUM_WRES; ++i) {
        if (!is_teammate(self, actors[i])) {
            continue;
        }
        if (actors[i]->player_mode != WM_PMODE_DEAD) {
            return true;
        }
    }
    return false;
}

bool wm_ck_any_teammates(const wm_arcade_actor_t *self,
                         wm_arcade_actor_t *const *actors,
                         size_t actor_count)
{
    size_t i;

    if (!self || !actors) {
        return false;
    }
    for (i = 0; i < actor_count && i < WM_NUM_WRES; ++i) {
        if (is_teammate(self, actors[i])) {
            return true;
        }
    }
    return false;
}

bool wm_is_behind(const wm_arcade_actor_t *self,
                  const wm_arcade_actor_t *other)
{
    int32_t delta;

    if (!self || !other) {
        return false;
    }
    /* `sub a1,a14 / jrn #onrt` -- my X minus his, and only a strictly
     * negative result puts him on my right. Equal X is "on my left". */
    delta = self->x_int - other->x_int;
    if (delta < 0) {
        /* He is on my right: facing right means I am facing him. */
        return (self->facing_dir & WM_MOVE_RIGHT) == 0;
    }
    /* He is on my left: facing left means I am facing him. */
    return (self->facing_dir & WM_MOVE_LEFT) == 0;
}

void wm_clr_climb(wm_arcade_actor_t *self)
{
    if (!self) {
        return;
    }
    self->climbing_thru = 0;
    /* `clr a0 / move a0,*a13(CLIMBING_THRU) / inc a0 / move a0,*a13(SAFE_TIME)`
     * -- SAFE_TIME lands on 1, not 0. */
    self->safe_time = 1;
}
