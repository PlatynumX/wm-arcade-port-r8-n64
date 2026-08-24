#include "wm_arcade_react9_core.h"

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

static void impact(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                   wm_arcade_react1_impact_t i,
                   wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->impact) cb->impact(a, v, i, cb->user);
}

static void triple(wm_arcade_actor_t *v, uint16_t id,
                   wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->triple_sound) cb->triple_sound(v, id, cb->user);
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

static int32_t push_away(const wm_arcade_actor_t *a,
                         const wm_arcade_actor_t *v,
                         int32_t mag)
{
    return a->x_int < v->x_int ? mag : -mag;
}

static void hit_rslash(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                       wm_arcade_react1_context_t *ctx)
{
    if (v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(a, v, ctx);
        return;
    }

    impact(a, v, WM_R1_IMPACT_FACE, ctx);
    if (v->life == 0) {
        collis_off(v, ctx);
        return;
    }

    sound(v, WM_R1_SND_RSLASH, ctx);
    setmode(v, WM_PMODE_NORMAL);

    if ((v->y_int - v->ground_y) >= 20) {
        anim(v, WM_R1_ANIM_FALL_BACK, ctx);
        collis_off(v, ctx);
        v->x_vel = push_away(a, v, FX16(3));
        return;
    }

    anim(v, WM_R1_ANIM_HEAD_HIT2, ctx);
    collis_off(v, ctx);
}

static void hit_headslash_common(wm_arcade_actor_t *a,
                                 wm_arcade_actor_t *v,
                                 int downslash,
                                 wm_arcade_react1_context_t *ctx)
{
    sound(v, WM_R1_SND_RSLASH, ctx);

    if (v->life != 0) {
        anim(v, WM_R1_ANIM_KNEE_HIT, ctx);
        v->y_vel = downslash ? 0x0002c000 : 0x00040000;
        v->x_vel = 0;

        if (downslash) {
            int32_t step = 5;
            if (a->x_int > v->x_int) step = -step;
            v->x_int += step;
        }

        collis_off(v, ctx);
        return;
    }

    anim(v, WM_R1_ANIM_FALL_BACK, ctx);
    v->x_vel = push_away(a, v, FX16(4));
    collis_off(v, ctx);
}

static void hit_napalm(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                       int16_t *pending,
                       wm_arcade_react1_context_t *ctx)
{
    (void)a;

    /* Standing or blocking victims explicitly take no damage. */
    if (v->player_mode == WM_PMODE_NORMAL || v->player_mode == WM_PMODE_BLOCK) {
        if (pending) *pending = 0;
        collis_off(v, ctx);
        return;
    }

    collis_off(v, ctx);

    if (v->player_mode != WM_PMODE_ONGROUND && v->player_mode != WM_PMODE_DEAD)
        return;

    anim(v, WM_R1_ANIM_BURN, ctx);
    v->status_flags |= WM_STATUS_DEAD_ANIM;
    sound(v, WM_R1_SND_LBOWDROP, ctx);
    triple(v, 0x43u, ctx);
    collis_off(v, ctx);
}

int wm_arcade_react9_supports(wm_arcade_reaction_id_t r)
{
    switch (r) {
    case WM_RXN_RSLASH:
    case WM_RXN_HEADDSLASH:
    case WM_RXN_HEADUSLASH:
    case WM_RXN_NAPALM:
        return 1;
    default:
        return 0;
    }
}

int wm_arcade_react9_apply(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                           wm_arcade_reaction_id_t r,
                           int16_t *pending, int16_t *newdir,
                           wm_arcade_react1_context_t *ctx)
{
    (void)newdir;
    if (!a || !v || !wm_arcade_react9_supports(r)) return 0;
    if (ctx) {
        ctx->last_reaction = r;
        ctx->last_supported = 1;
        ctx->last_flykick_aborted = 0;
    }

    switch (r) {
    case WM_RXN_RSLASH: hit_rslash(a, v, ctx); break;
    case WM_RXN_HEADDSLASH: hit_headslash_common(a, v, 1, ctx); break;
    case WM_RXN_HEADUSLASH: hit_headslash_common(a, v, 0, ctx); break;
    case WM_RXN_NAPALM: hit_napalm(a, v, pending, ctx); break;
    default: return 0;
    }
    return 1;
}

void wm_arcade_react123456789_reaction_callback(wm_arcade_actor_t *a,
                                                wm_arcade_actor_t *v,
                                                wm_arcade_reaction_id_t r,
                                                int16_t *pending,
                                                int16_t *newdir,
                                                void *user)
{
    wm_arcade_react1_context_t *ctx = (wm_arcade_react1_context_t *)user;
    if (wm_arcade_react9_supports(r)) {
        (void)wm_arcade_react9_apply(a, v, r, pending, newdir, ctx);
        return;
    }
    wm_arcade_react12345678_reaction_callback(a, v, r, pending, newdir, user);
}
