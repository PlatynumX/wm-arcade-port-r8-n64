#include "wm_arcade_wrestle_target.h"

#include <limits.h>

static uint32_t isqrt64(uint64_t n)
{
    uint64_t bit = (uint64_t)1 << 62;
    uint64_t result = 0;

    while (bit > n) bit >>= 2;
    while (bit != 0) {
        if (n >= result + bit) {
            n -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result > UINT32_MAX ? UINT32_MAX : (uint32_t)result;
}

static uint32_t absdiff32(int32_t a, int32_t b)
{
    int64_t d = (int64_t)a - (int64_t)b;
    if (d < 0) d = -d;
    return d > UINT32_MAX ? UINT32_MAX : (uint32_t)d;
}

static wm_arcade_actor_t *find_player(const wm_arcade_closest_world_t *w,
                                      int32_t player_num)
{
    size_t i;
    if (!w) return 0;
    for (i = 0; i < w->actor_count; ++i) {
        wm_arcade_actor_t *a = w->actors ? w->actors[i] : 0;
        if (a && a->active && a->player_num == player_num) return a;
    }
    return 0;
}

/* WRESTLE2.ASM::is_a14_behind. */
static bool candidate_is_behind(const wm_arcade_actor_t *self,
                                const wm_arcade_actor_t *candidate)
{
    if (!self || !candidate) return false;
    if (candidate->x_int > self->x_int)
        return (self->facing_dir & WM_MOVE_RIGHT) == 0;
    return (self->facing_dir & WM_MOVE_LEFT) == 0;
}

/* WRESTLE.ASM does two integer square roots, not a floating-point distance. */
static uint32_t source_distance(uint32_t dx, uint32_t dy, uint32_t dz)
{
    uint32_t dxz = isqrt64((uint64_t)dx * dx + (uint64_t)dz * dz);
    return isqrt64((uint64_t)dxz * dxz + (uint64_t)dy * dy);
}

static int candidate_liveness(const wm_arcade_actor_t *a)
{
    if (!a) return -1;
    if (a->player_mode != WM_PMODE_DEAD) return 1;
    if (a->status_flags & WM_STATUS_ZOMBIE) return -1;
    return 0;
}

bool wm_arcade_calc_closest(wm_arcade_actor_t *self,
                            const wm_arcade_closest_world_t *world)
{
    size_t i;
    int current_class = -1;
    uint32_t best_biased = 0x7fffu;
    bool found = false;

    if (!self || !world || !world->actors) return false;

    for (i = 0; i < world->actor_count; ++i) {
        wm_arcade_actor_t *candidate = world->actors[i];
        wm_arcade_actor_t *current;
        uint32_t dx, dy, dz, raw, biased;
        int cls;
        bool compare_only = false;

        if (!candidate || !candidate->active) continue;
        if (candidate == self) continue;
        if (candidate->player_side == self->player_side) continue;

        /* WRESTLE.ASM running ahead/behind filter.  A behind candidate is
           ignored only when the current useful target is ahead and in-ring. */
        if (self->player_mode == WM_PMODE_RUNNING && current_class >= 0 &&
            candidate_is_behind(self, candidate)) {
            current = find_player(world, self->closest_num);
            if (current && !candidate_is_behind(self, current) &&
                current->in_ring == 0)
                continue;
        }

        dx = absdiff32(candidate->x_int, self->x_int);
        dz = absdiff32(candidate->z_int, self->z_int);
        dy = absdiff32(candidate->y_int, self->y_int);
        raw = source_distance(dx, dy, dz);
        biased = raw;

        /* Source bias order is significant. */
        if (candidate->player_mode == WM_PMODE_ONGROUND)
            biased <<= 1;
        if (self->who_i_hit == candidate)
            biased -= biased >> 2;
        if (candidate->in_ring != self->in_ring)
            biased *= 3u;
        if (candidate->player_num == self->closest_num)
            biased -= biased >> 2;
        if (self->combo_count != 0 && self->who_i_hit == candidate)
            biased = 0;
        biased += dz << 1;

        cls = candidate_liveness(candidate);
        if (cls < 0) {
            if (current_class >= 0) continue;
            compare_only = true;
        } else if (cls == 0) {
            if (current_class > 0) continue;
            if (current_class == 0) compare_only = true;
            else current_class = 0;
        } else {
            if (current_class > 0) compare_only = true;
            else current_class = 1;
        }

        if (compare_only && biased > best_biased) continue;

        best_biased = biased;
        self->closest_dist = (int32_t)raw;
        self->closest_xdist = (int32_t)dx;
        self->closest_zdist = (int32_t)dz;
        self->closest_ydist = (int32_t)dy;
        self->closest_num = candidate->player_num;
        self->smart_target = candidate;
        found = true;
    }

    return found;
}

bool wm_arcade_calc_closest2(wm_arcade_actor_t *self,
                             const wm_arcade_closest_world_t *world)
{
    wm_arcade_actor_t *current;
    if (!self || !world) return false;

    current = find_player(world, self->closest_num);
    if (!current || current->player_mode == WM_PMODE_DEAD)
        return wm_arcade_calc_closest(self, world);

    /* Source spreads the expensive scan over four PCNT phases. */
    if (((uint32_t)self->player_num & 3u) != (world->pcnt & 3u))
        return false;

    return wm_arcade_calc_closest(self, world);
}
