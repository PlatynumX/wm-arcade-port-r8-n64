#include "wm_arcade_react8_core.h"

#include <stddef.h>
#include <stdint.h>

#define FX16(x) ((int32_t)((x) << 16))

static const wm_arcade_react1_callbacks_t *cb_of(wm_arcade_react1_context_t *ctx)
{ return ctx ? ctx->callbacks : NULL; }

static void anim(wm_arcade_actor_t *v, wm_arcade_react1_anim_group_t g,
                 wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->change_anim) cb->change_anim(v, g, cb->user);
}

static void sound(wm_arcade_actor_t *v, wm_arcade_react1_sound_t s,
                  wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->play_sound) cb->play_sound(v, s, cb->user);
}

static void collis_off(wm_arcade_actor_t *v, wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    wm_arcade_wrestler_collisions_off(v);
    if (cb && cb->collisions_off) cb->collisions_off(v, cb->user);
}

static void setmode(wm_arcade_actor_t *v, uint16_t mode)
{
    if (v->player_mode != WM_PMODE_DEAD) v->player_mode = mode;
}

static void set_getup_time_r8(const wm_arcade_actor_t *a,
                              wm_arcade_actor_t *v,
                              wm_arcade_react1_context_t *ctx)
{
    wm_arcade_combat_callbacks_t cc = {0};
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb) {
        cc.maybe_gidd_up = cb->maybe_gidd_up;
        cc.user = cb->user;
    }
    wm_arcade_set_getup_time(a, v, &cc);
}

static int32_t push_away(const wm_arcade_actor_t *a,
                         const wm_arcade_actor_t *v,
                         int32_t mag)
{
    return a->x_int < v->x_int ? mag : -mag;
}

static void hit_shnbfkik(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                         wm_arcade_react1_context_t *ctx)
{
    /* Exact REACT8 tail-jump to hit_flykick. */
    (void)wm_arcade_react1_apply(a, v, WM_RXN_FLYKICK, NULL, NULL, ctx);
}

static void hit_shnspdkik(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                          wm_arcade_react1_context_t *ctx)
{
    if (v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(a, v, ctx);
        return;
    }

    if (v->life != 0) {
        sound(v, WM_R1_SND_KICK, ctx);
        setmode(v, WM_PMODE_NORMAL);
        anim(v, WM_R1_ANIM_HEAD_HIT, ctx);
    }
    collis_off(v, ctx);
}

static void hit_shnspdkik2(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                           wm_arcade_react1_context_t *ctx)
{
    if (v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(a, v, ctx);
        return;
    }

    sound(v, WM_R1_SND_FLYKICK, ctx);
    if (v->life != 0) setmode(v, WM_PMODE_NORMAL);

    v->z_vel = 0;
    v->roll_pos = 0;
    set_getup_time_r8(a, v, ctx);
    anim(v, WM_R1_ANIM_FALL_BACK, ctx);
    v->x_vel = push_away(a, v, FX16(4));
    collis_off(v, ctx);
}

static void hit_hitcheck(wm_arcade_actor_t *v, int16_t *pending,
                         wm_arcade_react1_context_t *ctx)
{
    if (pending) *pending = 0;
    collis_off(v, ctx);
}

void wm_arcade_react8_legacy_flyelbow(wm_arcade_actor_t *a,
                                      wm_arcade_actor_t *v,
                                      wm_arcade_react1_context_t *ctx)
{
    /*
     * Direct body from REACT8 hit_flyelbow: perform normal flykick; if it
     * wasn't blocked, negate the already-negated/halved attacker X velocity
     * so the elbow does not bounce away.
     */
    if (ctx) ctx->last_flykick_aborted = 0;
    (void)wm_arcade_react1_apply(a, v, WM_RXN_FLYKICK, NULL, NULL, ctx);
    if (ctx && ctx->last_flykick_aborted) return;
    a->x_vel = -a->x_vel;
}

int wm_arcade_react8_supports(wm_arcade_reaction_id_t r)
{
    switch (r) {
    case WM_RXN_SHNBFKIK:
    case WM_RXN_SHNSPDKIK:
    case WM_RXN_SHNSPDKIK2:
    case WM_RXN_HITCHECK:
        return 1;
    default:
        return 0;
    }
}

int wm_arcade_react8_apply(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                           wm_arcade_reaction_id_t r,
                           int16_t *pending, int16_t *newdir,
                           wm_arcade_react1_context_t *ctx)
{
    (void)newdir;
    if (!a || !v || !wm_arcade_react8_supports(r)) return 0;
    if (ctx) {
        ctx->last_reaction = r;
        ctx->last_supported = 1;
        ctx->last_flykick_aborted = 0;
    }

    switch (r) {
    case WM_RXN_SHNBFKIK: hit_shnbfkik(a, v, ctx); break;
    case WM_RXN_SHNSPDKIK: hit_shnspdkik(a, v, ctx); break;
    case WM_RXN_SHNSPDKIK2: hit_shnspdkik2(a, v, ctx); break;
    case WM_RXN_HITCHECK: hit_hitcheck(v, pending, ctx); break;
    default: return 0;
    }
    return 1;
}

void wm_arcade_react12345678_reaction_callback(wm_arcade_actor_t *a,
                                               wm_arcade_actor_t *v,
                                               wm_arcade_reaction_id_t r,
                                               int16_t *pending,
                                               int16_t *newdir,
                                               void *user)
{
    wm_arcade_react1_context_t *ctx = (wm_arcade_react1_context_t *)user;
    if (wm_arcade_react8_supports(r)) {
        (void)wm_arcade_react8_apply(a, v, r, pending, newdir, ctx);
        return;
    }
    wm_arcade_react1234567_reaction_callback(a, v, r, pending, newdir, user);
}
