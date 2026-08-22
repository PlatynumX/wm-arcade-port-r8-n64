#include "wm_arcade_react1_core.h"

#include <stddef.h>

#define FX16(x) ((int32_t)((x) << 16))
#define FX_4_5  ((int32_t)0x00048000)
#define FX_3_75 ((int32_t)0x0003c000)

static const wm_arcade_react1_callbacks_t *cb_of(wm_arcade_react1_context_t *ctx)
{
    return ctx ? ctx->callbacks : NULL;
}

static void anim(wm_arcade_actor_t *victim, wm_arcade_react1_anim_group_t group,
                 wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->change_anim) cb->change_anim(victim, group, cb->user);
}

static void sound(wm_arcade_actor_t *victim, wm_arcade_react1_sound_t snd,
                  wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->play_sound) cb->play_sound(victim, snd, cb->user);
}

static void impact(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim,
                   wm_arcade_react1_impact_t kind,
                   wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->impact) cb->impact(attacker, victim, kind, cb->user);
}

static void collis_off(wm_arcade_actor_t *victim, wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    wm_arcade_wrestler_collisions_off(victim);
    if (cb && cb->collisions_off) cb->collisions_off(victim, cb->user);
}

static int32_t push_away_xvel(const wm_arcade_actor_t *attacker,
                              const wm_arcade_actor_t *victim,
                              int32_t magnitude)
{
    /* Source: positive if attacker is left of victim, otherwise negative. */
    return attacker->x_int < victim->x_int ? magnitude : -magnitude;
}

static void block_hit_common(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim,
                             int flail, wm_arcade_react1_context_t *ctx)
{
    victim->x_vel = push_away_xvel(attacker, victim, flail ? FX16(6) + 0x8000 : FX_4_5);
    sound(victim, WM_R1_SND_BLOCK, ctx);
    anim(victim, flail ? WM_R1_ANIM_HITBLOCK_FLAIL : WM_R1_ANIM_HITBLOCK, ctx);
    collis_off(victim, ctx);
}

void wm_arcade_react1_block_hit(wm_arcade_actor_t *attacker,
                                wm_arcade_actor_t *victim,
                                wm_arcade_react1_context_t *ctx)
{
    if (!attacker || !victim) return;
    block_hit_common(attacker, victim, 0, ctx);
}

void wm_arcade_react1_block_hit_flail(wm_arcade_actor_t *attacker,
                                      wm_arcade_actor_t *victim,
                                      wm_arcade_react1_context_t *ctx)
{
    if (!attacker || !victim) return;
    block_hit_common(attacker, victim, 1, ctx);
}

static void set_getup_time_r1(const wm_arcade_actor_t *attacker,
                              wm_arcade_actor_t *victim,
                              wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    wm_arcade_combat_callbacks_t ccb;
    ccb.victim_has_live_teammates = NULL;
    ccb.wrestler_hit = NULL;
    ccb.maybe_gidd_up = cb ? cb->maybe_gidd_up : NULL;
    ccb.user = cb ? cb->user : NULL;
    wm_arcade_set_getup_time(attacker, victim, &ccb);
}

static void react_punch(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim,
                        wm_arcade_react1_context_t *ctx)
{
    if (victim->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit(attacker, victim, ctx);
        return;
    }

    impact(attacker, victim, WM_R1_IMPACT_FACE, ctx);
    if (victim->life == 0) {
        collis_off(victim, ctx);
        return;
    }

    sound(victim, WM_R1_SND_PUNCH, ctx);
    victim->player_mode = WM_PMODE_NORMAL;

    if ((victim->y_int - victim->ground_y) >= 20) {
        anim(victim, WM_R1_ANIM_FALL_BACK, ctx);
        collis_off(victim, ctx);
        victim->x_vel = push_away_xvel(attacker, victim, FX16(3));
        return;
    }

    victim->consecutive_hits++;
    if (victim->consecutive_hits == 6) {
        victim->consecutive_hits = 0;
        if (!victim->who_hit_me || victim->who_hit_me->combo_count == 0) {
            anim(victim, WM_R1_ANIM_LOSE_BALANCE, ctx);
            collis_off(victim, ctx);
            return;
        }
    }

    anim(victim, WM_R1_ANIM_HEAD_HIT, ctx);
    collis_off(victim, ctx);
}

static void react_hdbutt(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim,
                         wm_arcade_react1_context_t *ctx)
{
    if (victim->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit(attacker, victim, ctx);
        return;
    }
    impact(attacker, victim, WM_R1_IMPACT_FACE, ctx);
    if (victim->life != 0) {
        sound(victim, WM_R1_SND_HDBUTT, ctx);
        victim->player_mode = WM_PMODE_NORMAL;
        anim(victim, WM_R1_ANIM_HEAD_HIT2, ctx);
    }
    collis_off(victim, ctx);
}

static void react_hdbutt2(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim,
                          wm_arcade_react1_context_t *ctx)
{
    impact(attacker, victim, WM_R1_IMPACT_FACE, ctx);
    sound(victim, WM_R1_SND_HDBUTT, ctx);
    anim(victim, WM_R1_ANIM_HEAD_HIT2, ctx);
    victim->y_vel = FX_3_75;
    victim->x_vel = 0;
    collis_off(victim, ctx);
}

static void react_urn(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim,
                      wm_arcade_react1_context_t *ctx)
{
    if (victim->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(attacker, victim, ctx);
        return;
    }
    impact(attacker, victim, WM_R1_IMPACT_FACE, ctx);
    if (victim->life == 0) {
        collis_off(victim, ctx);
        return;
    }
    sound(victim, WM_R1_SND_HDBUTT, ctx);
    victim->player_mode = WM_PMODE_NORMAL;
    if ((victim->y_int - victim->ground_y) >= 20) {
        anim(victim, WM_R1_ANIM_FALL_BACK, ctx);
        collis_off(victim, ctx);
        victim->x_vel = push_away_xvel(attacker, victim, FX16(3));
        return;
    }
    anim(victim, WM_R1_ANIM_HEAD_HIT2, ctx);
    collis_off(victim, ctx);
}

static void react_hdbutt_stay(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim,
                              wm_arcade_react1_context_t *ctx)
{
    if (victim->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(attacker, victim, ctx);
        return;
    }
    impact(attacker, victim, WM_R1_IMPACT_FACE, ctx);
    victim->player_mode = WM_PMODE_NORMAL;
    sound(victim, WM_R1_SND_HDBUTT, ctx);
    anim(victim, WM_R1_ANIM_HEAD_HIT2, ctx);
    collis_off(victim, ctx);
    victim->delay_meter = 6 * 60;
    victim->x_vel = 0;
}

static void react_tomb(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim,
                       wm_arcade_react1_context_t *ctx)
{
    uint16_t old_mode;
    if (victim->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(attacker, victim, ctx);
        return;
    }
    impact(attacker, victim, WM_R1_IMPACT_FACE, ctx);
    if (victim->life == 0) {
        collis_off(victim, ctx);
        return;
    }
    sound(victim, WM_R1_SND_SCREAM, ctx);
    old_mode = victim->player_mode;
    if (old_mode == WM_PMODE_ONGROUND || old_mode == WM_PMODE_DEAD) {
        anim(victim, WM_R1_ANIM_HIT_ON_GROUND, ctx);
        collis_off(victim, ctx);
        return;
    }
    victim->player_mode = WM_PMODE_NORMAL;
    if ((victim->y_int - victim->ground_y) >= 20) {
        anim(victim, WM_R1_ANIM_FALL_BACK, ctx);
        collis_off(victim, ctx);
        victim->x_vel = push_away_xvel(attacker, victim, FX16(3));
        return;
    }
    anim(victim, WM_R1_ANIM_HEAD_HIT2, ctx);
    collis_off(victim, ctx);
}

static void react_kick(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim,
                       int super_kick, wm_arcade_react1_context_t *ctx)
{
    if (victim->player_mode == WM_PMODE_BLOCK) {
        if (super_kick) wm_arcade_react1_block_hit_flail(attacker, victim, ctx);
        else wm_arcade_react1_block_hit(attacker, victim, ctx);
        return;
    }
    impact(attacker, victim, WM_R1_IMPACT_MID, ctx);
    if (victim->life != 0) {
        sound(victim, WM_R1_SND_KICK, ctx);
        victim->player_mode = WM_PMODE_NORMAL;
        victim->usr_var1 = 0;
        anim(victim, WM_R1_ANIM_BODY_HIT, ctx);
    }
    collis_off(victim, ctx);
}

static void react_flykick(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim,
                          wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    int32_t half = attacker->x_vel >> 1;
    int32_t mag = half < 0 ? -half : half;

    if (mag >= 0x00020000) mag = FX16(4);
    if (attacker->x_vel >= 0) mag = -mag;
    attacker->x_vel = mag;
    attacker->y_vel = FX16(4);

    if (victim->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(attacker, victim, ctx);
        if (ctx) ctx->last_flykick_aborted = 1;
        return;
    }

    if (cb && cb->attacker_uses_lex_flykick_anim &&
        cb->attacker_uses_lex_flykick_anim(attacker, cb->user))
        impact(attacker, victim, WM_R1_IMPACT_MID, ctx);
    else
        impact(attacker, victim, WM_R1_IMPACT_DROP_KICK, ctx);

    sound(victim, WM_R1_SND_FLYKICK, ctx);
    if (victim->life != 0) victim->player_mode = WM_PMODE_NORMAL;
    victim->roll_pos = 0;
    set_getup_time_r1(attacker, victim, ctx);
    anim(victim, WM_R1_ANIM_FALL_BACK, ctx);
    victim->x_vel = push_away_xvel(attacker, victim, FX16(2));
    collis_off(victim, ctx);
}

static void react_bigknee(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim,
                          wm_arcade_react1_context_t *ctx)
{
    if (victim->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(attacker, victim, ctx);
        return;
    }
    impact(attacker, victim, WM_R1_IMPACT_DROP_KICK, ctx);
    sound(victim, WM_R1_SND_FLYKICK, ctx);
    if (victim->life != 0) victim->player_mode = WM_PMODE_NORMAL;
    victim->roll_pos = 0;
    set_getup_time_r1(attacker, victim, ctx);
    anim(victim, WM_R1_ANIM_FALL_BACK, ctx);
    victim->x_vel = push_away_xvel(attacker, victim, FX16(4));
    collis_off(victim, ctx);
}

static void react_ontbukl(wm_arcade_actor_t *attacker, wm_arcade_actor_t *victim,
                          wm_arcade_react1_context_t *ctx)
{
    sound(victim, WM_R1_SND_FLYKICK, ctx);
    anim(victim, WM_R1_ANIM_FALL_BACK_TBUKL, ctx);
    victim->player_mode = WM_PMODE_INAIR;
    victim->status_flags |= WM_STATUS_DEAD_ANIM;
    victim->x_vel = push_away_xvel(attacker, victim, FX16(4));
    victim->y_vel = FX16(6);
    collis_off(victim, ctx);
}

void wm_arcade_react1_context_init(wm_arcade_react1_context_t *ctx,
                                   const wm_arcade_react1_callbacks_t *callbacks)
{
    if (!ctx) return;
    ctx->callbacks = callbacks;
    ctx->last_reaction = WM_RXN_HITCHECK;
    ctx->last_supported = 0;
    ctx->last_flykick_aborted = 0;
}

int wm_arcade_react1_apply(wm_arcade_actor_t *attacker,
                           wm_arcade_actor_t *victim,
                           wm_arcade_reaction_id_t reaction,
                           int16_t *hit_damage_pending,
                           int16_t *new_victim_movedir,
                           wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb;
    (void)hit_damage_pending;
    (void)new_victim_movedir;
    if (!attacker || !victim) return 0;
    if (ctx) {
        ctx->last_reaction = reaction;
        ctx->last_supported = 1;
        ctx->last_flykick_aborted = 0;
    }

    switch (reaction) {
    case WM_RXN_PUNCH:
    case WM_RXN_FIRE_PUNCH:
        react_punch(attacker, victim, ctx); break;
    case WM_RXN_HDBUTT:
        react_hdbutt(attacker, victim, ctx); break;
    case WM_RXN_HDBUTT2:
        react_hdbutt2(attacker, victim, ctx); break;
    case WM_RXN_URN:
        react_urn(attacker, victim, ctx); break;
    case WM_RXN_HDBUTT_STAY:
        react_hdbutt_stay(attacker, victim, ctx); break;
    case WM_RXN_TOMB:
        react_tomb(attacker, victim, ctx); break;
    case WM_RXN_KICK:
        react_kick(attacker, victim, 0, ctx); break;
    case WM_RXN_SUPER_KICK:
        react_kick(attacker, victim, 1, ctx); break;
    case WM_RXN_FLYKICK:
        react_flykick(attacker, victim, ctx); break;
    case WM_RXN_BIGKNEE:
        react_bigknee(attacker, victim, ctx); break;
    case WM_RXN_GRABTHROW:
        /* REACT1 hit_grabthrow is exactly a bare rets. */
        break;
    case WM_RXN_ONTURNBUCKLE:
        react_ontbukl(attacker, victim, ctx); break;
    default:
        if (ctx) ctx->last_supported = 0;
        cb = cb_of(ctx);
        if (cb && cb->unhandled_reaction)
            cb->unhandled_reaction(attacker, victim, reaction, cb->user);
        return 0;
    }
    return 1;
}

void wm_arcade_react1_reaction_callback(wm_arcade_actor_t *attacker,
                                        wm_arcade_actor_t *victim,
                                        wm_arcade_reaction_id_t reaction,
                                        int16_t *hit_damage_pending,
                                        int16_t *new_victim_movedir,
                                        void *user)
{
    wm_arcade_react1_context_t *ctx = (wm_arcade_react1_context_t *)user;
    (void)wm_arcade_react1_apply(attacker, victim, reaction,
                                 hit_damage_pending, new_victim_movedir, ctx);
}
