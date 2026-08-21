#include "wm_arcade_react2_core.h"

#include <stddef.h>

#define FX16(x) ((int32_t)((x) << 16))
#define FX_1_5 ((int32_t)0x00018000)

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

static void triple(wm_arcade_actor_t *victim, uint16_t sound_id,
                   wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->triple_sound) cb->triple_sound(victim, sound_id, cb->user);
}

static void flash_white(wm_arcade_actor_t *victim, wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->flash_white) cb->flash_white(victim, cb->user);
}

static void collis_off(wm_arcade_actor_t *victim, wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    wm_arcade_wrestler_collisions_off(victim);
    if (cb && cb->collisions_off) cb->collisions_off(victim, cb->user);
}

static int32_t health_of(const wm_arcade_actor_t *victim,
                         wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->get_health) return cb->get_health(victim, cb->user);
    return victim->life;
}

static int has_live_teammates(const wm_arcade_actor_t *victim,
                              wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb = cb_of(ctx);
    if (cb && cb->victim_has_live_teammates)
        return cb->victim_has_live_teammates(victim, cb->user) != 0;
    return 0;
}

static void set_getup_time_r2(const wm_arcade_actor_t *attacker,
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

static void setmode_normal(wm_arcade_actor_t *victim)
{
    /* Exact SETMODE macro rule: MODE_DEAD is never changed here. */
    if (victim->player_mode != WM_PMODE_DEAD)
        victim->player_mode = WM_PMODE_NORMAL;
}

static int32_t push_away_xvel(const wm_arcade_actor_t *attacker,
                              const wm_arcade_actor_t *victim,
                              int32_t magnitude)
{
    return attacker->x_int < victim->x_int ? magnitude : -magnitude;
}

static void hit_uprcut(wm_arcade_actor_t *attacker,
                       wm_arcade_actor_t *victim,
                       wm_arcade_react1_context_t *ctx)
{
    int32_t yvel;

    if (victim->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(attacker, victim, ctx);
        return;
    }

    if (health_of(victim, ctx) != 0)
        setmode_normal(victim);

    sound(victim, WM_R1_SND_UPRCUT, ctx);
    anim(victim, WM_R1_ANIM_FALL_BACK, ctx);
    flash_white(victim, ctx);
    victim->roll_pos = 0;
    set_getup_time_r2(attacker, victim, ctx);

    /* REACT2.ASM "silly temp" launch-height wrestler special cases. */
    if (attacker->wrestler_num == 3) {
        yvel = FX16(15);
    } else if (attacker->wrestler_num == 2) {
        yvel = has_live_teammates(victim, ctx) ? FX16(11) : FX16(18);
    } else {
        yvel = FX16(13);
    }
    victim->y_vel = yvel;
    victim->x_vel = push_away_xvel(attacker, victim, FX16(2));
    collis_off(victim, ctx);
}

static void hit_combo_uprcut(wm_arcade_actor_t *attacker,
                             wm_arcade_actor_t *victim,
                             wm_arcade_react1_context_t *ctx)
{
    wm_arcade_actor_t *who;

    if (victim->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit(attacker, victim, ctx);
        return;
    }

    if (health_of(victim, ctx) != 0)
        setmode_normal(victim);

    sound(victim, WM_R1_SND_UPRCUT, ctx);
    anim(victim, WM_R1_ANIM_FALL_BACK, ctx);
    flash_white(victim, ctx);
    victim->roll_pos = 0;
    set_getup_time_r2(attacker, victim, ctx);

    victim->anim_mode = WM_ARCADE_MODE_UNINT | WM_ARCADE_MODE_NOAUTOFLIP | WM_ARCADE_MODE_OVERLAP;

    /* Stage 2 establishes WHOHITME before reaction dispatch, as the source does. */
    who = victim->who_hit_me;
    victim->y_vel = (who && who->rpt_count == 1) ? FX16(7) : FX16(3);
    victim->x_vel = push_away_xvel(attacker, victim, FX_1_5);
    collis_off(victim, ctx);
}

static void hit_lbowdrop(wm_arcade_actor_t *attacker,
                         wm_arcade_actor_t *victim,
                         int16_t *hit_damage_pending,
                         wm_arcade_react1_context_t *ctx)
{
    (void)attacker;

    /* Standing/blocking opponents explicitly ignore this attack and take 0. */
    if (victim->player_mode == WM_PMODE_NORMAL ||
        victim->player_mode == WM_PMODE_BLOCK) {
        if (hit_damage_pending) *hit_damage_pending = 0;
        collis_off(victim, ctx);
        return;
    }

    sound(victim, WM_R1_SND_LBOWDROP, ctx);
    triple(victim, 0x33u, ctx);
    collis_off(victim, ctx);
    anim(victim, WM_R1_ANIM_HIT_ON_GROUND, ctx);
    /* Source calls wres_collis_off a second time after change_anim1a. */
    collis_off(victim, ctx);
}

static void hit_blbowdrop(wm_arcade_actor_t *attacker,
                          wm_arcade_actor_t *victim,
                          wm_arcade_react1_context_t *ctx)
{
    int32_t height;

    sound(victim, WM_R1_SND_SCREAM, ctx);   /* DO_SCREAM */
    sound(victim, WM_R1_SND_FLYKICK, ctx);
    sound(victim, WM_R1_SND_LBOWDROP, ctx);
    collis_off(victim, ctx);

    if (victim->player_mode == WM_PMODE_ONGROUND ||
        victim->player_mode == WM_PMODE_DEAD) {
        anim(victim, WM_R1_ANIM_HIT_ON_GROUND, ctx);
        collis_off(victim, ctx);
        return;
    }

    setmode_normal(victim);
    height = victim->y_int - victim->ground_y;
    if (height < 20) {
        set_getup_time_r2(attacker, victim, ctx);
        anim(victim, WM_R1_ANIM_KNOCKDOWN, ctx);
        collis_off(victim, ctx);
        return;
    }

    triple(victim, 0x43u, ctx);
    anim(victim, WM_R1_ANIM_FALL_BACK, ctx);
    collis_off(victim, ctx);
    victim->x_vel = push_away_xvel(attacker, victim, FX16(3));
    victim->y_vel = -0x00030000;
}

static void hit_push_common(wm_arcade_actor_t *attacker,
                            wm_arcade_actor_t *victim,
                            int16_t *hit_damage_pending,
                            wm_arcade_react1_context_t *ctx)
{
    setmode_normal(victim);
    attacker->x_vel = 0;
    anim(victim, WM_R1_ANIM_LOSE_BALANCE, ctx);
    victim->x_vel = push_away_xvel(attacker, victim, FX16(8));
    sound(victim, WM_R1_SND_PUSH, ctx);

    /* Exact REACT2 check: if get_health() returns 1, suppress push damage. */
    if (health_of(victim, ctx) == 1 && hit_damage_pending)
        *hit_damage_pending = 0;

    collis_off(victim, ctx);
}

static void hit_gutpush(wm_arcade_actor_t *attacker,
                        wm_arcade_actor_t *victim,
                        int16_t *hit_damage_pending,
                        wm_arcade_react1_context_t *ctx)
{
    if (victim->player_mode == WM_PMODE_BLOCK) {
        /* Source clears XVEL immediately before calling block_hit_flail. */
        victim->x_vel = 0;
        wm_arcade_react1_block_hit_flail(attacker, victim, ctx);
        return;
    }
    hit_push_common(attacker, victim, hit_damage_pending, ctx);
}

int wm_arcade_react2_supports(wm_arcade_reaction_id_t reaction)
{
    switch (reaction) {
    case WM_RXN_UPRCUT:
    case WM_RXN_COMBO_UPRCUT:
    case WM_RXN_LBOWDROP:
    case WM_RXN_BLBOWDROP:
    case WM_RXN_GRABHOLD:
    case WM_RXN_GRABFLING:
    case WM_RXN_PUSH:
    case WM_RXN_GUTPUSH:
        return 1;
    default:
        return 0;
    }
}

int wm_arcade_react2_apply(wm_arcade_actor_t *attacker,
                           wm_arcade_actor_t *victim,
                           wm_arcade_reaction_id_t reaction,
                           int16_t *hit_damage_pending,
                           int16_t *new_victim_movedir,
                           wm_arcade_react1_context_t *ctx)
{
    (void)new_victim_movedir;
    if (!attacker || !victim || !wm_arcade_react2_supports(reaction))
        return 0;

    if (ctx) {
        ctx->last_reaction = reaction;
        ctx->last_supported = 1;
        ctx->last_flykick_aborted = 0;
    }

    switch (reaction) {
    case WM_RXN_UPRCUT:
        hit_uprcut(attacker, victim, ctx);
        break;
    case WM_RXN_COMBO_UPRCUT:
        hit_combo_uprcut(attacker, victim, ctx);
        break;
    case WM_RXN_LBOWDROP:
        hit_lbowdrop(attacker, victim, hit_damage_pending, ctx);
        break;
    case WM_RXN_BLBOWDROP:
        hit_blbowdrop(attacker, victim, ctx);
        break;
    case WM_RXN_GRABHOLD:
        /* REACT2 body is commented out and falls through to hit_grabfling. */
        break;
    case WM_RXN_GRABFLING:
        /* REACT2 hit_grabfling is a bare rets. */
        break;
    case WM_RXN_PUSH:
        hit_push_common(attacker, victim, hit_damage_pending, ctx);
        break;
    case WM_RXN_GUTPUSH:
        hit_gutpush(attacker, victim, hit_damage_pending, ctx);
        break;
    default:
        return 0;
    }
    return 1;
}

void wm_arcade_react12_reaction_callback(wm_arcade_actor_t *attacker,
                                         wm_arcade_actor_t *victim,
                                         wm_arcade_reaction_id_t reaction,
                                         int16_t *hit_damage_pending,
                                         int16_t *new_victim_movedir,
                                         void *user)
{
    wm_arcade_react1_context_t *ctx = (wm_arcade_react1_context_t *)user;

    if (wm_arcade_react2_supports(reaction)) {
        (void)wm_arcade_react2_apply(attacker, victim, reaction,
                                     hit_damage_pending, new_victim_movedir, ctx);
        return;
    }

    (void)wm_arcade_react1_apply(attacker, victim, reaction,
                                 hit_damage_pending, new_victim_movedir, ctx);
}
