#include "wm_arcade_react.h"
#include "wm_arcade_damage.h"

#include <stddef.h>

/* REACT1.ASM damage_values, entries 0..55, in original order. */
static const int16_t k_damage_values[56][2] = {
    { WM_D_PUNCH,        WM_RD_PUNCH        }, /* 0  */
    { WM_D_HDBUTT,       WM_RD_HDBUTT       }, /* 1  */
    { WM_D_KICK,         WM_RD_KICK         }, /* 2  */
    { WM_D_FLYKICK,      WM_RD_FLYKICK      }, /* 3  */
    { WM_D_GRABTHROW,    WM_RD_GRABTHROW    }, /* 4  */
    { WM_D_UPRCUT,       WM_RD_UPRCUT       }, /* 5  */
    { WM_D_LBDROP,       WM_RD_LBDROP       }, /* 6  */
    { WM_D_GRBHOLD,      WM_RD_GRBHOLD      }, /* 7  */
    { WM_D_GRBFLNG,      WM_RD_GRBFLNG      }, /* 8  */
    { WM_D_PUSH,         WM_RD_PUSH         }, /* 9  */
    { WM_D_BCKHAND,      WM_RD_BCKHAND      }, /* 10 */
    { WM_D_BIGBOOT,      WM_RD_BIGBOOT      }, /* 11 */
    { WM_D_KNEE,         WM_RD_KNEE         }, /* 12 */
    { WM_D_HDKNEES,      WM_RD_HDKNEES      }, /* 13 */
    { WM_D_BOXPUNCH,     WM_RD_BOXPUNCH     }, /* 14 */
    { WM_D_STOMP,        WM_RD_STOMP        }, /* 15 */
    { WM_D_SPINKIK,      WM_RD_SPINKIK      }, /* 16 */
    { WM_D_CLINE,        WM_RD_CLINE        }, /* 17 */
    { WM_D_HEDHOLD,      WM_RD_HEDHOLD      }, /* 18 */
    { WM_D_JUMPKICK,     WM_RD_JUMPKICK     }, /* 19 */
    { WM_D_RUN,          WM_RD_RUN          }, /* 20 */
    { WM_D_PUPPET,       WM_RD_PUPPET       }, /* 21 */
    { WM_D_BCKHAND,      WM_RD_BCKHAND      }, /* 22 */
    { WM_D_BUZZ,         WM_RD_BUZZ         }, /* 23 */
    { WM_D_HAYMAKER,     WM_RD_HAYMAKER     }, /* 24 */
    { WM_D_BLBDROP,      WM_RD_BLBDROP      }, /* 25 */
    { WM_D_BSTOMP,       WM_RD_BSTOMP       }, /* 26 */
    { WM_D_HDKNEES,      WM_RD_HDKNEES      }, /* 27 */
    { WM_D_EARSLAP2,     WM_RD_EARSLAP2     }, /* 28 */
    { WM_D_HAMMER2,      WM_RD_HAMMER2      }, /* 29 */
    { WM_D_BUTTSTOMP,    WM_RD_BUTTSTOMP    }, /* 30 */
    { WM_D_ATT31,        WM_RD_ATT31        }, /* 31 */
    { WM_D_ATT32,        WM_RD_ATT32        }, /* 32 */
    { WM_D_TOMB,         WM_RD_TOMB         }, /* 33 */
    { WM_D_BIGKNEE,      WM_RD_BIGKNEE      }, /* 34 */
    { WM_D_FLPKIK,       WM_RD_FLPKIK       }, /* 35 */
    { WM_D_SPDKIK,       WM_RD_SPDKIK       }, /* 36 */
    { WM_D_SPDKIK2,      WM_RD_SPDKIK2      }, /* 37 */
    { WM_D_HITCK,        WM_RD_HITCK        }, /* 38 */
    { WM_D_ARMBRK,       WM_RD_ARMBRK       }, /* 39 */
    { WM_D_RSLASH,       WM_RD_RSLASH       }, /* 40 */
    { WM_D_HEADDSLASH,   WM_RD_HEADDSLASH   }, /* 41 */
    { WM_D_HEADUSLASH,   WM_RD_HEADUSLASH   }, /* 42 */
    { WM_D_RSLASH2,      WM_RD_RSLASH2      }, /* 43 */
    { WM_D_HDBUTT_STAY,  WM_RD_HDBUTT_STAY  }, /* 44 */
    { WM_D_FIRE_PUNCH,   WM_RD_FIRE_PUNCH   }, /* 45 */
    { WM_D_BSTOMP2,      WM_RD_BSTOMP2      }, /* 46 */
    { WM_D_GUTPUSH,      WM_RD_GUTPUSH      }, /* 47 */
    { WM_D_SPINKIK,      WM_RD_SPINKIK      }, /* 48 SUPER_KICK */
    { WM_D_PUNCH2,       WM_RD_PUNCH2       }, /* 49 */
    { WM_D_BCKHAND,      WM_RD_BCKHAND      }, /* 50 */
    { WM_D_LBDROP2,      WM_RD_LBDROP2      }, /* 51 */
    { WM_D_STOMP2,       WM_RD_STOMP2       }, /* 52 */
    { 0,                 0                  }, /* 53 */
    { 0,                 0                  }, /* 54 */
    { WM_D_NAPALM,       WM_RD_NAPALM       }  /* 55 */
};

/* REACT1.ASM offense_mods / defense_mods: all nine roster slots. */
static const int16_t k_offense_mods[WM_ARCADE_ROSTER_COUNT] = {
    89,89,89,89,89,89,89,89,89
};
static const int16_t k_defense_mods[WM_ARCADE_ROSTER_COUNT] = {
    0,0,0,0,0,0,0,0,0
};

/* hit_table translated to a compact reaction id. */
static const wm_arcade_reaction_id_t k_reaction[56] = {
    WM_RXN_PUNCH, WM_RXN_HDBUTT, WM_RXN_KICK, WM_RXN_FLYKICK,
    WM_RXN_GRABTHROW, WM_RXN_UPRCUT, WM_RXN_LBOWDROP, WM_RXN_GRABHOLD,
    WM_RXN_GRABFLING, WM_RXN_PUSH, WM_RXN_URN, WM_RXN_BIGBOOT,
    WM_RXN_KNEE, WM_RXN_HDBUTT2, WM_RXN_BOXPUNCH, WM_RXN_STOMP,
    WM_RXN_SPINKICK, WM_RXN_CLINE, WM_RXN_HEADHOLD, WM_RXN_JUMPKICK,
    WM_RXN_RUN, WM_RXN_PUPPET, WM_RXN_BACKHAND, WM_RXN_BUZZ,
    WM_RXN_HAYMAKER, WM_RXN_BLBOWDROP, WM_RXN_BSTOMP, WM_RXN_HEADKNEES,
    WM_RXN_EARSLAP, WM_RXN_HAMMER, WM_RXN_BUTTSTOMP, WM_RXN_PUPPET2,
    WM_RXN_PUPPET_HDGRAB, WM_RXN_TOMB, WM_RXN_BIGKNEE, WM_RXN_SHNBFKIK,
    WM_RXN_SHNSPDKIK, WM_RXN_SHNSPDKIK2, WM_RXN_HITCHECK,
    WM_RXN_COMBO_UPRCUT, WM_RXN_RSLASH, WM_RXN_HEADDSLASH,
    WM_RXN_HEADUSLASH, WM_RXN_RSLASH, WM_RXN_HDBUTT_STAY,
    WM_RXN_FIRE_PUNCH, WM_RXN_BSTOMP2, WM_RXN_GUTPUSH,
    WM_RXN_SUPER_KICK, WM_RXN_PUNCH, WM_RXN_URN, WM_RXN_LBOWDROP,
    WM_RXN_STOMP, WM_RXN_PUPPET_NOFLAIL, WM_RXN_PUPPET_TOSS,
    WM_RXN_NAPALM
};

static uint16_t elapsed_word(uint32_t now, uint16_t then)
{
    return (uint16_t)((uint16_t)now - then);
}

static int is_turnbuckle_override(uint16_t attack_mode)
{
    return attack_mode != WM_AMODE_PUPPET &&
           attack_mode != WM_AMODE_PUPPET2 &&
           attack_mode != WM_AMODE_PUPPET_HDGRAB &&
           attack_mode != WM_AMODE_HITCHECK;
}

static void clear_visual_state(wm_arcade_actor_t *actor,
                               const wm_arcade_react_callbacks_t *callbacks)
{
    actor->stars_flag = 0;
    actor->debris_x = 0;
    actor->combo_count = 0;
    actor->shadtrail_proc = NULL;
    actor->attimg_cur_frame = NULL;
    actor->obj_pal = actor->my_pal;

    /*
     * REACT1 also clears OBJ_CONTROL low four bits and ORs DMAWNZ.  DMAWNZ is
     * renderer/hardware-specific, so Stage 2 deliberately routes that exact
     * operation through the merge hook rather than inventing an N64 value.
     */
    if (callbacks && callbacks->restore_hit_render_state)
        callbacks->restore_hit_render_state(actor, callbacks->user);
}

static wm_arcade_partner_breakout_t breakout_for_mode(uint16_t mode)
{
    switch (mode) {
    case WM_PMODE_PUPPET:
    case WM_PMODE_PUPPET2:
    case WM_PMODE_NORMAL:
    case WM_PMODE_RUNNING:
    case WM_PMODE_HEADHELD:
    case WM_PMODE_HEADHOLD:
    case WM_PMODE_CHOKEHOLD:
    case WM_PMODE_OPPOVERHEAD:
        return WM_PARTNER_GOTO_STAND;

    case WM_PMODE_ONGROUND:
    case WM_PMODE_ATTACHED:
    case WM_PMODE_INAIR:
    case WM_PMODE_INAIR2:
    case WM_PMODE_DEAD:
        return WM_PARTNER_ABORT_ATTACH_ANIM;

    default:
        return (wm_arcade_partner_breakout_t)0;
    }
}

void wm_arcade_combat_runtime_init(wm_arcade_combat_runtime_t *runtime)
{
    if (!runtime)
        return;
    runtime->pcnt = 0;
    runtime->round_tickcount = 0;
    runtime->any_hits = 0;
    runtime->dam_mult = 1;
}

int wm_arcade_attack_damage_pair(uint16_t attack_mode,
                                 int16_t *full_damage,
                                 int16_t *reduced_damage)
{
    if (attack_mode >= 56)
        return 0;
    if (full_damage)
        *full_damage = k_damage_values[attack_mode][0];
    if (reduced_damage)
        *reduced_damage = k_damage_values[attack_mode][1];
    return 1;
}

wm_arcade_reaction_id_t wm_arcade_attack_reaction(uint16_t attack_mode)
{
    if (attack_mode >= 56)
        return WM_RXN_HITCHECK;
    return k_reaction[attack_mode];
}

void wm_arcade_hit_stuff_identity(const void *attacker_identity,
                                  int attacker_is_hitcheck,
                                  wm_arcade_actor_t *victim,
                                  const wm_arcade_react_callbacks_t *callbacks)
{
    wm_arcade_actor_t *partner;
    wm_arcade_partner_breakout_t breakout;

    if (!attacker_identity || !victim || attacker_is_hitcheck)
        return;

    if (victim->status_flags & WM_STATUS_KOD) {
        victim->status_flags |= WM_STATUS_NO_KO;
        victim->status_flags &= ~WM_STATUS_KOD;
        victim->ptime = 1;
    }

    clear_visual_state(victim, callbacks);

    partner = victim->attach_proc;
    if (partner) {
        victim->attach_proc = NULL;

        /* Source compares ATTACH_PROC against the actual attacker process. */
        if (partner->attach_proc == victim &&
            (const void *)partner != attacker_identity) {
            partner->attach_proc = NULL;
            clear_visual_state(partner, callbacks);
            breakout = breakout_for_mode(partner->player_mode);
            if (breakout && callbacks && callbacks->partner_breakout)
                callbacks->partner_breakout(partner, breakout, callbacks->user);
        }
    }

    victim->smart_target = NULL;
    victim->status_flags &= ~WM_STATUS_SMART_ATTACK;

    if ((victim->player_mode == WM_PMODE_BOUNCING ||
         victim->player_mode == WM_PMODE_RUNNING) &&
        callbacks && callbacks->ditch_getup_meter) {
        callbacks->ditch_getup_meter(victim, callbacks->user);
    }

    victim->run_time = 0;

    /*
     * REACT1 lines 809-817 remain intentionally unresolved exactly as in the
     * Stage 2 audit; Stage 24 does not invent semantics for that contradictory
     * attachment-tail branch.
     */
}

void wm_arcade_hit_stuff(wm_arcade_actor_t *attacker,
                         wm_arcade_actor_t *victim,
                         const wm_arcade_react_callbacks_t *callbacks)
{
    if (!attacker)
        return;
    wm_arcade_hit_stuff_identity(attacker,
                                 attacker->attack_mode == WM_AMODE_HITCHECK,
                                 victim,
                                 callbacks);
}

wm_arcade_wrestler_hit_result_t wm_arcade_wrestler_hit(
    wm_arcade_actor_t *attacker,
    wm_arcade_actor_t *victim,
    wm_arcade_combat_runtime_t *runtime,
    const wm_arcade_react_callbacks_t *callbacks)
{
    wm_arcade_wrestler_hit_result_t out;
    int16_t full_damage = 0, reduced_damage = 0;
    int32_t damage;
    int16_t pending;
    int16_t new_movedir = 0;
    wm_arcade_reaction_id_t reaction;

    out.status = WM_WRESTLER_HIT_BAD_ARGUMENT;
    out.damage_before_reaction = 0;
    out.damage_after_reaction = 0;
    out.new_victim_movedir = 0;
    out.reaction = WM_RXN_HITCHECK;
    out.reaction_hook_called = 0;
    out.health_hook_called = 0;

    if (!attacker || !victim || !runtime)
        return out;
    if (attacker->attack_mode >= 56) {
        out.status = WM_WRESTLER_HIT_BAD_ATTACK_MODE;
        return out;
    }
    if (attacker->wrestler_num < 0 || attacker->wrestler_num >= WM_ARCADE_ROSTER_COUNT ||
        victim->wrestler_num < 0 || victim->wrestler_num >= WM_ARCADE_ROSTER_COUNT) {
        out.status = WM_WRESTLER_HIT_BAD_WRESTLER_INDEX;
        return out;
    }

    if (attacker->attack_mode == WM_AMODE_RUN) {
        if (!callbacks || !callbacks->good_run_hit) {
            out.status = WM_WRESTLER_HIT_NEEDS_RUN_HOOK;
            return out;
        }
        if (!callbacks->good_run_hit(attacker, victim, callbacks->user)) {
            out.status = WM_WRESTLER_HIT_IGNORED_RUN;
            return out;
        }
    }

    victim->who_hit_me = attacker;
    attacker->who_i_hit = victim;

    wm_arcade_hit_stuff(attacker, victim, callbacks);
    attacker->last_hit_time = runtime->pcnt;

    (void)wm_arcade_attack_damage_pair(attacker->attack_mode,
                                       &full_damage, &reduced_damage);
    damage = full_damage;
    if (victim->last_damage != 0 && elapsed_word(runtime->pcnt, victim->last_damage) <= 50)
        damage = reduced_damage;

    /* NEXT_DAMAGE only participates when ordinary damage is nonzero. */
    if (damage != 0 && attacker->next_damage != 0) {
        if (runtime->pcnt > attacker->special_damage_time ||
            attacker->next_damage > damage) {
            attacker->next_damage = 0;
        } else {
            damage = attacker->next_damage;
            attacker->next_damage = 0;
        }
    }

    /* Two source 8.8 modifiers, then >>16, then negate. */
    damage *= WM_ARCADE_MOD_ONE_8_8 + k_offense_mods[attacker->wrestler_num];
    damage *= WM_ARCADE_MOD_ONE_8_8 + k_defense_mods[victim->wrestler_num];
    damage >>= 16;
    damage = -damage;

    if (victim->player_mode == WM_PMODE_BLOCK &&
        attacker->attack_mode != WM_AMODE_BSTOMP &&
        attacker->attack_mode != WM_AMODE_BLBOWDROP) {
        if (damage != 0)
            damage = -1;
        attacker->risk = 0;
    } else if (damage != 0) {
        if (attacker->risk != 0) {
            if (attacker->risk & WM_ARCADE_RISK_HIGH_BIT) {
                runtime->dam_mult = 4;
                if (callbacks && callbacks->bonus_message)
                    callbacks->bonus_message(attacker, callbacks->user);
            }
            runtime->any_hits = 1;
        }
        attacker->risk = 0;
    }
    victim->risk = 0;

    pending = (int16_t)damage;
    out.damage_before_reaction = pending;

    if (victim->player_mode == WM_PMODE_ONTURNBKL &&
        is_turnbuckle_override(attacker->attack_mode)) {
        reaction = WM_RXN_ONTURNBUCKLE;
    } else {
        reaction = wm_arcade_attack_reaction(attacker->attack_mode);
    }
    out.reaction = reaction;

    if (callbacks && callbacks->reaction) {
        callbacks->reaction(attacker, victim, reaction, &pending, &new_movedir,
                            callbacks->user);
        out.reaction_hook_called = 1;
    }

    out.damage_after_reaction = pending;

    if (pending != 0) {
        if (pending <= -2 && runtime->any_hits == 0 &&
            victim->player_mode != WM_PMODE_BLOCK) {
            if (callbacks && callbacks->round_first_hit_award)
                callbacks->round_first_hit_award(attacker, callbacks->user);
            if (callbacks && callbacks->first_hit_message)
                callbacks->first_hit_message(attacker, callbacks->user);
            runtime->dam_mult = 2;
            runtime->any_hits = 1;
        }

        if (callbacks && callbacks->adjust_health) {
            callbacks->adjust_health(victim, pending, attacker, callbacks->user);
            out.health_hook_called = 1;
        }
    }

    victim->move_dir = new_movedir;
    out.new_victim_movedir = new_movedir;
    out.status = WM_WRESTLER_HIT_OK;
    return out;
}

void wm_arcade_wrestler_hit_collision_callback(
    wm_arcade_actor_t *attacker,
    wm_arcade_actor_t *victim,
    void *bridge_user)
{
    wm_arcade_react_bridge_t *bridge = (wm_arcade_react_bridge_t *)bridge_user;
    if (!bridge)
        return;
    bridge->last_result = wm_arcade_wrestler_hit(attacker, victim,
                                                  bridge->runtime,
                                                  bridge->callbacks);
}
