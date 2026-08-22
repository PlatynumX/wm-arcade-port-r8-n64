#include "wm_arcade_react5_core.h"
#include "wm_arcade_damage.h"

#include <stddef.h>
#include <stdint.h>

#define FX16(x) ((int32_t)((x) << 16))
#define WM_W_SHAWN 4
#define WM_W_YOKO  3

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

static void triple(wm_arcade_actor_t *v, uint16_t id,
                   wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->triple_sound) cb->triple_sound(v, id, cb->user);
}

static void grade(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                  wm_arcade_move_grade_t g, wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->move_grade) cb->move_grade(a, v, g, cb->user);
}

static void collis_off(wm_arcade_actor_t *v, wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    wm_arcade_wrestler_collisions_off(v);
    if (cb && cb->collisions_off) cb->collisions_off(v, cb->user);
}

static void setmode(wm_arcade_actor_t *v, uint16_t mode)
{
    /* MACROS.H SETMODE: MODE_DEAD is immutable. */
    if (v->player_mode != WM_PMODE_DEAD) v->player_mode = mode;
}

static int32_t victim_push_away(const wm_arcade_actor_t *a,
                                const wm_arcade_actor_t *v,
                                int32_t mag)
{
    return a->x_int < v->x_int ? mag : -mag;
}

static int32_t attacker_push_away(const wm_arcade_actor_t *a,
                                  const wm_arcade_actor_t *v,
                                  int32_t mag)
{
    return a->x_int > v->x_int ? mag : -mag;
}

int wm_arcade_react5_good_run_hit(const wm_arcade_actor_t *a,
                                  const wm_arcade_actor_t *v)
{
    int32_t dz;
    if (!a || !v) return 0;
    dz = a->z_fixed - v->z_fixed;
    if (dz < 0) dz = -dz;
    dz >>= 16;

    if (a->wrestler_num == WM_W_YOKO) {
        if (a->getup_time != 0) return 0;
        return dz < 5;
    }
    return dz <= 2;
}

int wm_arcade_react5_good_run_hit_callback(wm_arcade_actor_t *a,
                                           wm_arcade_actor_t *v,
                                           void *user)
{
    (void)user;
    return wm_arcade_react5_good_run_hit(a, v);
}

static void hit_run(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                    int16_t *pending, int16_t *newdir,
                    wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);

    /* REACT5 calls good_run_hit a second time inside hit_run. */
    if (!wm_arcade_react5_good_run_hit(a, v)) {
        if (newdir) *newdir = (int16_t)v->move_dir;
        collis_off(v, ctx);
        return;
    }

    if (v->player_mode == WM_PMODE_INAIR ||
        v->player_mode == WM_PMODE_INAIR2) {
        collis_off(v, ctx);
        return;
    }

    if (v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(a, v, ctx);
        v->y_vel = FX16(3);
    } else {
        if (v->player_mode == WM_PMODE_ONGROUND || v->life == 0) {
            collis_off(v, ctx);
            return;
        }

        sound(v, WM_R1_SND_LBOWDROP, ctx);
        setmode(v, WM_PMODE_NORMAL);

        if (a->wrestler_num == WM_W_YOKO) {
            anim(v, WM_R1_ANIM_FALL_BACK, ctx);
            if (pending) *pending = (int16_t)-WM_D_GUTPUSH;
            v->x_vel = victim_push_away(a, v, FX16(3));
        } else {
            anim(v, WM_R1_ANIM_LOSE_BALANCE, ctx);
        }
    }

    collis_off(v, ctx);

    /* What happens to the running wrestler after a genuine collision. */
    a->y_vel = FX16(3);
    a->player_mode = WM_PMODE_NORMAL; /* source uses direct MOVE, not SETMODE */

    if (a->dizzy != 0) {
        a->run_time = 0;
        anim(a, WM_R1_ANIM_BOUNCE_OFF_DIZZY, ctx);
    } else {
        if (a->getup_time != 0 && a->meter_proc != NULL &&
            cb && cb->slide_getup_meter) {
            cb->slide_getup_meter(a, cb->user);
        }
        a->run_time = 0;
        v->run_time = 0;
        a->getup_time = 0;
        anim(a, WM_R1_ANIM_BOUNCE_OFF, ctx);
    }

    a->x_vel = attacker_push_away(a, v, FX16(3));
    collis_off(v, ctx);
}

static void attach_as_puppet(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                             wm_arcade_react1_context_t *ctx)
{
    setmode(v, WM_PMODE_PUPPET);
    v->attach_proc = a;
    a->attach_proc = v;
    anim(v, WM_R1_ANIM_WRES_SLAVE, ctx);
    v->getup_time = 0;
}

static void hit_puppet(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                       wm_arcade_react1_context_t *ctx)
{
    if (v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit(a, v, ctx);
        return;
    }
    /* Source's hit_puppet_even_if_dead: attach even when SETMODE preserves DEAD. */
    attach_as_puppet(a, v, ctx);
    collis_off(v, ctx);
}

static void hit_puppet_noflail(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                               wm_arcade_react1_context_t *ctx)
{
    if (v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit(a, v, ctx);
        return;
    }
    if (v->player_mode == WM_PMODE_DEAD) {
        collis_off(v, ctx);
        return;
    }
    attach_as_puppet(a, v, ctx);
    collis_off(v, ctx);
}

static void hit_puppet2(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                        wm_arcade_react1_context_t *ctx)
{
    if (v->getup_time == 0) {
        a->anim_mode &= (uint16_t)~WM_ARCADE_MODE_STATUS;
        collis_off(v, ctx);
        return;
    }
    if (v->player_mode == WM_PMODE_DEAD) {
        collis_off(v, ctx);
        return;
    }
    attach_as_puppet(a, v, ctx);
    collis_off(v, ctx);
}

static void hit_puppet_hdgrab(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                              wm_arcade_react1_context_t *ctx)
{
    if (v->safe_time != 0 && v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(a, v, ctx);
        return;
    }
    if (v->player_mode == WM_PMODE_DEAD) {
        collis_off(v, ctx);
        return;
    }
    a->hit_blocker = 0;
    /* wrestler_hit stores current 32-bit PCNT in attacker LAST_HIT_TIME before reaction. */
    v->head_grab_time = a->last_hit_time;
    attach_as_puppet(a, v, ctx);
    collis_off(v, ctx);
}

static int toss_is_blocked_down_away(const wm_arcade_actor_t *v)
{
    uint16_t horiz = (uint16_t)(v->stick_val_cur & 0x000cu);
    uint16_t facing_horiz = (uint16_t)(v->new_facing_dir & 0x000cu);
    if (horiz == 0) return 0;
    if (horiz == facing_horiz) return 0;
    if ((v->stick_val_cur & 0x0002u) == 0) return 0;
    return 1;
}

static void hit_puppet_toss(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                            wm_arcade_react1_context_t *ctx)
{
    if (v->safe_time != 0) {
        if (v->player_mode == WM_PMODE_BLOCK) {
            wm_arcade_react1_block_hit(a, v, ctx);
            return;
        }
    } else if (v->player_mode == WM_PMODE_BLOCK && toss_is_blocked_down_away(v)) {
        wm_arcade_react1_block_hit(a, v, ctx);
        return;
    }

    /* Source intentionally has the dead-player rejection commented out here. */
    a->hit_blocker = 0;
    attach_as_puppet(a, v, ctx);
    collis_off(v, ctx);
}

static void hit_backhand(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                         wm_arcade_react1_context_t *ctx)
{
    if (v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(a, v, ctx);
        return;
    }

    triple(v, a->wrestler_num == WM_W_SHAWN ? 0x33u : 0x43u, ctx);
    grade(a, v, WM_R_MOVE_AVERAGE, ctx);
    if (v->life == 0) {
        collis_off(v, ctx);
        return;
    }

    sound(v, WM_R1_SND_UPRCUT, ctx);
    setmode(v, WM_PMODE_NORMAL);
    if ((v->y_int - v->ground_y) >= 20) {
        anim(v, WM_R1_ANIM_FALL_BACK, ctx);
        collis_off(v, ctx);
        v->x_vel = victim_push_away(a, v, FX16(3));
        return;
    }
    anim(v, WM_R1_ANIM_BACKHAND_HEAD_HIT, ctx);
    collis_off(v, ctx);
}

static void hit_earslap(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                        wm_arcade_react1_context_t *ctx)
{
    if (v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit(a, v, ctx);
        return;
    }
    triple(v, 0x43u, ctx);
    if (v->life == 0) {
        collis_off(v, ctx);
        return;
    }
    sound(v, WM_R1_SND_HDBUTT, ctx);
    setmode(v, WM_PMODE_NORMAL);
    anim(v, WM_R1_ANIM_EARSLAP_HEAD_HIT, ctx);
    collis_off(v, ctx);
}

static void hit_buzz(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                     wm_arcade_react1_context_t *ctx)
{
    if (v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit(a, v, ctx);
        return;
    }
    if (v->life != 0) setmode(v, WM_PMODE_NORMAL);

    /* REACT5 attaches both processes even on the zero-life branch. */
    v->attach_proc = a;
    a->attach_proc = v;
    sound(v, WM_R1_SND_PUNCH, ctx);
    anim(v, WM_R1_ANIM_GET_BUZZ, ctx);
    collis_off(v, ctx);
}

static void hit_haymaker(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                         wm_arcade_react1_context_t *ctx)
{
    if (v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(a, v, ctx);
        return;
    }
    sound(v, WM_R1_SND_FLYKICK, ctx);
    if (v->life != 0) setmode(v, WM_PMODE_NORMAL);
    anim(v, WM_R1_ANIM_FALL_BACK, ctx);
    v->x_vel = victim_push_away(a, v, FX16(4));
    collis_off(v, ctx);
}

int wm_arcade_react5_supports(wm_arcade_reaction_id_t r)
{
    switch (r) {
    case WM_RXN_RUN:
    case WM_RXN_PUPPET:
    case WM_RXN_PUPPET_NOFLAIL:
    case WM_RXN_PUPPET2:
    case WM_RXN_PUPPET_HDGRAB:
    case WM_RXN_PUPPET_TOSS:
    case WM_RXN_BACKHAND:
    case WM_RXN_EARSLAP:
    case WM_RXN_BUZZ:
    case WM_RXN_HAYMAKER:
        return 1;
    default:
        return 0;
    }
}

int wm_arcade_react5_apply(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                           wm_arcade_reaction_id_t r,
                           int16_t *pending, int16_t *newdir,
                           wm_arcade_react1_context_t *ctx)
{
    if (!a || !v || !wm_arcade_react5_supports(r)) return 0;
    if (ctx) {
        ctx->last_reaction = r;
        ctx->last_supported = 1;
        ctx->last_flykick_aborted = 0;
    }

    switch (r) {
    case WM_RXN_RUN: hit_run(a, v, pending, newdir, ctx); break;
    case WM_RXN_PUPPET: hit_puppet(a, v, ctx); break;
    case WM_RXN_PUPPET_NOFLAIL: hit_puppet_noflail(a, v, ctx); break;
    case WM_RXN_PUPPET2: hit_puppet2(a, v, ctx); break;
    case WM_RXN_PUPPET_HDGRAB: hit_puppet_hdgrab(a, v, ctx); break;
    case WM_RXN_PUPPET_TOSS: hit_puppet_toss(a, v, ctx); break;
    case WM_RXN_BACKHAND: hit_backhand(a, v, ctx); break;
    case WM_RXN_EARSLAP: hit_earslap(a, v, ctx); break;
    case WM_RXN_BUZZ: hit_buzz(a, v, ctx); break;
    case WM_RXN_HAYMAKER: hit_haymaker(a, v, ctx); break;
    default: return 0;
    }
    return 1;
}

void wm_arcade_react12345_reaction_callback(wm_arcade_actor_t *a,
                                            wm_arcade_actor_t *v,
                                            wm_arcade_reaction_id_t r,
                                            int16_t *pending,
                                            int16_t *newdir,
                                            void *user)
{
    wm_arcade_react1_context_t *ctx = (wm_arcade_react1_context_t *)user;
    if (wm_arcade_react5_supports(r)) {
        (void)wm_arcade_react5_apply(a, v, r, pending, newdir, ctx);
        return;
    }
    wm_arcade_react1234_reaction_callback(a, v, r, pending, newdir, user);
}
