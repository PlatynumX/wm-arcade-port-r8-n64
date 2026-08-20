#include "wm/arcade/wm_arcade_anim_combat.h"

static uint16_t elapsed_word(uint32_t now, uint16_t then)
{
    return (uint16_t)((uint16_t)now - then);
}

void wm_arcade_ani_attack_on(wm_arcade_actor_t *actor,
                             const wm_arcade_attack_on_args_t *args)
{
    if (!actor || !args)
        return;

    actor->hit_blocker = 0;
    actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
    actor->anim_mode |= WM_MODE_CHECKHIT;
    actor->attack_mode = args->attack_mode;
    actor->attack_xoff = args->xoff;
    actor->attack_yoff = args->yoff;
    actor->attack_width = args->width;
    actor->attack_height = args->height;
    actor->attack_zoff = -40;
    actor->attack_depth = 80;
}

void wm_arcade_ani_attack_on_z(wm_arcade_actor_t *actor,
                               const wm_arcade_attack_on_z_args_t *args)
{
    if (!actor || !args)
        return;

    actor->hit_blocker = 0;
    actor->attach_zoff = 0;
    actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
    actor->anim_mode |= WM_MODE_CHECKHIT;
    actor->attack_mode = args->attack_mode;
    actor->attack_xoff = args->xoff;
    actor->attack_yoff = args->yoff;
    actor->attack_zoff = args->zoff;
    actor->attack_width = args->width;
    actor->attack_height = args->height;
    actor->attack_depth = args->depth;
}

void wm_arcade_ani_attack_off(wm_arcade_actor_t *actor,
                              uint16_t round_tickcount)
{
    if (!actor)
        return;

    actor->anim_mode &= (uint16_t)~(WM_MODE_CHECKHIT | WM_MODE_WAITHITOPP);
    actor->status_flags &= ~WM_STATUS_SMART_ATTACK;
    actor->smart_target = 0;
    actor->attack_time = round_tickcount;
}

void wm_arcade_ani_clr_damage(wm_arcade_actor_t *actor)
{
    (void)actor;
    /* clear_damage_log is commented out in ANIM.ASM opcode 47. */
}

void wm_arcade_ani_damage(wm_arcade_actor_t *actor,
                          int16_t script_damage,
                          const wm_arcade_react_callbacks_t *callbacks)
{
    if (!actor)
        return;
    if (callbacks && callbacks->adjust_health)
        callbacks->adjust_health(actor, (int16_t)-script_damage, actor,
                                 callbacks->user);
}

void wm_arcade_ani_clr_status(wm_arcade_actor_t *actor)
{
    if (actor)
        actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
}

wm_arcade_anim_damageopp_result_t wm_arcade_ani_damageopp(
    wm_arcade_actor_t *actor,
    int16_t full_damage,
    int16_t reduced_damage,
    wm_arcade_combat_runtime_t *runtime,
    const wm_arcade_react_callbacks_t *callbacks)
{
    wm_arcade_anim_damageopp_result_t out;
    wm_arcade_actor_t *target;
    int16_t damage;

    out.status = WM_ANI_DAMAGEOPP_BAD_ARGUMENT;
    out.target = 0;
    out.signed_damage = 0;
    out.used_reduced_damage = 0;
    out.used_next_damage = 0;
    out.health_hook_called = 0;

    if (!actor || !runtime)
        return out;

    target = actor->attach_proc ? actor->attach_proc : actor->who_i_hit;
    out.target = target;
    if (!target) {
        /* Arcade source assumes this pointer is valid; portable guard only. */
        out.status = WM_ANI_DAMAGEOPP_NO_TARGET;
        return out;
    }

    damage = full_damage;
    if (target->last_damage != 0 && elapsed_word(runtime->pcnt, target->last_damage) <= 30) {
        damage = reduced_damage;
        out.used_reduced_damage = 1;
    }

    /* Unlike wrestler_hit, opcode 66 does not compare or clear NEXT_DAMAGE. */
    if (actor->next_damage != 0 && runtime->pcnt <= actor->special_damage_time) {
        damage = actor->next_damage;
        out.used_next_damage = 1;
    }

    damage = (int16_t)-damage;
    out.signed_damage = damage;

    if (damage <= -2) {
        if (actor->risk != 0) {
            if (actor->risk & WM_ARCADE_RISK_HIGH_BIT) {
                runtime->dam_mult = 4;
                if (callbacks && callbacks->bonus_message)
                    callbacks->bonus_message(actor, callbacks->user);
            }
            actor->risk = 0;
            runtime->any_hits = 1;
        } else if (runtime->any_hits == 0 && target->player_mode != WM_PMODE_BLOCK) {
            if (callbacks && callbacks->round_first_hit_award)
                callbacks->round_first_hit_award(actor, callbacks->user);
            if (callbacks && callbacks->first_hit_message)
                callbacks->first_hit_message(actor, callbacks->user);
            runtime->dam_mult = 2;
            runtime->any_hits = 1;
        }
    }

    /* Source invokes adjust_health even for zero damage in opcode 66. */
    if (callbacks && callbacks->adjust_health) {
        callbacks->adjust_health(target, damage, actor, callbacks->user);
        out.health_hook_called = 1;
    }

    out.status = WM_ANI_DAMAGEOPP_OK;
    return out;
}

void wm_arcade_ani_waithitopp(wm_arcade_actor_t *actor)
{
    if (actor)
        actor->anim_mode |= WM_MODE_WAITHITOPP;
}
