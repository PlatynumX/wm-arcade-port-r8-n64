#include "wm/match.h"
#include <string.h>

/* LIFEBAR.ASM:135 LIFE_MAX equ 163 (green pixels in life bar). */
#define WM_MATCH_LIFE_MAX 163

/* PLYR.EQU: PSIDE_PLYR1 equ 0, PSIDE_PLYR2 equ 1. */
#define WM_MATCH_PSIDE_PLYR1 0
#define WM_MATCH_PSIDE_PLYR2 1

void wm_match_init(wm_match_state *m) {
    if (!m) return;
    memset(m, 0, sizeof(*m));
}

unsigned wm_match_draw_wrestler_index(WmRng *rng) {
    uint32_t v = wm_rng_rndrng0(rng, 7);
    if (v == 7) v = 8;
    return v;
}

static void init_actor_life(wm_arcade_actor_t *a) {
    memset(a, 0, sizeof(*a));
    a->active = 1;
    a->in_ring = 1;
    a->life = WM_MATCH_LIFE_MAX;
}

static void init_bret_backends(wm_match_state *m) {
    unsigned i;
    for (i = 0; i < WM_MATCH_MAX_ACTORS; ++i) {
        wm_bret_backend_init(&m->bret_visual[i]);
        if (m->actors[i].wrestler_num == WM_ROSTER_BRET) {
            wm_arcade_bret_callbacks_t cb = wm_bret_backend_callbacks(&m->bret_visual[i]);
            wm_arcade_bret_ani_init(&m->actors[i], &cb);
        }
    }
}

void wm_match_start_attract(wm_match_state *m, WmRng *rng) {
    wm_arcade_actor_t *p1, *opp;
    if (!m) return;

    m->index1 = wm_match_draw_wrestler_index(rng);
    /* Placeholder opponent draw -- see wm/match.h. Not @index2. */
    m->opponent_wrestler = wm_match_draw_wrestler_index(rng);

    p1 = &m->actors[0];
    opp = &m->actors[1];
    init_actor_life(p1);
    init_actor_life(opp);

    /* WRESTLE.ASM:1775-1782 #0plyr: P1 drone, PLYRNUM=2, PSIDE_PLYR1. */
    p1->player_num = 2;
    p1->player_side = WM_MATCH_PSIDE_PLYR1;
    p1->wrestler_num = (int32_t)m->index1;

    /* WRESTLE.ASM:1786-1794: opponent drone(s), PLYRNUM starts at 3,
       PSIDE_PLYR2. The source loop repeats this NUM_OPPS times from the
       ladder table; only one opponent is created here (see wm/match.h). */
    opp->player_num = 3;
    opp->player_side = WM_MATCH_PSIDE_PLYR2;
    opp->wrestler_num = (int32_t)m->opponent_wrestler;

    p1->smart_target = opp;
    opp->smart_target = p1;

    wm_arcade_drone_init(&m->drones[0], 0);
    wm_arcade_drone_init(&m->drones[1], 0);

    init_bret_backends(m);

    m->actor_count = WM_MATCH_MAX_ACTORS;
    m->active = true;
    m->tick_count = 0;
    wm_arcade_combat_runtime_init(&m->combat_runtime);
}

void wm_match_start_selected(wm_match_state *m, WmRng *rng,
                             uint8_t p1_source_wrestler) {
    wm_arcade_actor_t *p1, *opp;
    if (!m) return;

    p1 = &m->actors[0];
    opp = &m->actors[1];
    init_actor_life(p1);
    init_actor_life(opp);

    /* WRESTLE.ASM:1713-1727 #1plyr: human, PLYRNUM=0, PSIDE_PLYR1, wrestler
       from @index1 (whatever the select screen actually chose). */
    p1->player_num = 0;
    p1->player_side = WM_MATCH_PSIDE_PLYR1;
    p1->wrestler_num = (int32_t)p1_source_wrestler;

    /* WRESTLE.ASM:1733-1741 #ndrone: placeholder opponent draw -- see
       wm/match.h. Not @index2/ladder-derived; PLYRNUM starts at 2. */
    m->opponent_wrestler = wm_match_draw_wrestler_index(rng);
    opp->player_num = 2;
    opp->player_side = WM_MATCH_PSIDE_PLYR2;
    opp->wrestler_num = (int32_t)m->opponent_wrestler;

    p1->smart_target = opp;
    opp->smart_target = p1;

    wm_arcade_drone_init(&m->drones[0], 0);
    wm_arcade_drone_init(&m->drones[1], 0);

    init_bret_backends(m);

    m->has_human = true;
    m->human_actor_index = 0;
    wm_human_input_init(&m->human_input_state);

    m->actor_count = WM_MATCH_MAX_ACTORS;
    m->active = true;
    m->tick_count = 0;
    wm_arcade_combat_runtime_init(&m->combat_runtime);
}

/*
 * LIFEBAR.ASM:1547-1591 (adjust_health's damage-application tail), for the
 * one case wm_match ever creates: two actors, no royal rumble, no buddy
 * mode, no combo finisher.
 *
 * Translated:
 *   a3 = life + delta; clamp to [0, LIFE_MAX], except:
 *     - LIFEBAR.ASM:1561-1569 "fudge": a killing hit of 20+ points that
 *       doesn't overkill by more than 9 points bumps life to 5 instead of 0.
 *     - LIFEBAR.ASM:1578-1581 "if we're in attract mode, don't die!":
 *       PSTATUS==0 (m->has_human false here) snaps life back to LIFE_MAX
 *       instead of letting it hit 0.
 *
 * NOT translated (needs global state/tables this port doesn't have, and is
 * out of hurt-box-work scope): the DAM_MULT/COMBO_COUNT combo multiplier,
 * damage_mod_table drone-count scaling, speed_adjustment scaling, the
 * lifebar-flash warning process, ACTUAL_PLYRNUM/royal-rumble redirection,
 * and everything LIFEBAR.ASM does once life actually reaches 0 (MODE_DEAD,
 * teammate/getup handling) -- victim->player_mode is left untouched here.
 */
static void wm_match_adjust_health(wm_arcade_actor_t *victim, int16_t signed_delta,
                                   wm_arcade_actor_t *damage_source, void *user) {
    wm_match_state *m = (wm_match_state *)user;
    int32_t life = (int32_t)victim->life + signed_delta;
    (void)damage_source;

    if (life <= 0) {
        if (life > -10 && signed_delta <= -20)
            life = 5;
        else
            life = (m && m->has_human) ? 0 : WM_MATCH_LIFE_MAX;
    } else if (life > WM_MATCH_LIFE_MAX) {
        life = WM_MATCH_LIFE_MAX;
    }

    victim->life = life;
}

void wm_match_tick(wm_match_state *m, const wm_arcade_drone_callbacks_t *cb,
                   const wm_input_state *human_input) {
    wm_arcade_drone_world_t world;
    wm_arcade_actor_t *actor_ptrs[WM_MATCH_MAX_ACTORS];
    unsigned i;

    if (!m || !m->active) return;

    for (i = 0; i < m->actor_count; ++i)
        actor_ptrs[i] = &m->actors[i];

    memset(&world, 0, sizeof(world));
    world.actors = actor_ptrs;
    world.actor_count = m->actor_count;
    world.round_tickcount = (uint16_t)m->tick_count;
    world.first_ladder = 1;

    for (i = 0; i < m->actor_count; ++i) {
        if (m->has_human && i == m->human_actor_index) {
            wm_human_input_commit(&m->actors[i], &m->human_input_state, human_input);
        } else {
            uint16_t old_but = m->drones[i].but;
            uint16_t old_joy = m->drones[i].joy;
            (void)wm_arcade_drone_main(&m->actors[i], &m->drones[i], &world, cb);
            wm_arcade_drone_commit_inputs(&m->actors[i], &m->drones[i], old_but, old_joy);
        }

        if (m->actors[i].wrestler_num == WM_ROSTER_BRET) {
            /* WM_MATCH_MAX_ACTORS==2: the other slot is always the opponent. */
            wm_arcade_actor_t *opp = &m->actors[1u - i];
            wm_arcade_bret_env_t env;
            wm_arcade_bret_callbacks_t bret_cb;
            m->bret_visual[i].opponent = opp;
            bret_cb = wm_bret_backend_callbacks(&m->bret_visual[i]);
            memset(&env, 0, sizeof(env));
            env.pcnt = m->tick_count;
            wm_arcade_calc_closest(&m->actors[i], opp);
            (void)wm_arcade_move_bret(&m->actors[i], opp, &env, &bret_cb);
            wm_bret_backend_tick(&m->bret_visual[i], &m->actors[i], (uint16_t)m->tick_count);
            wm_bret_backend_tick_position(&m->actors[i]);
        }
    }

    {
        wm_arcade_react_callbacks_t react_cb;
        wm_arcade_react_bridge_t bridge;
        wm_arcade_combat_callbacks_t combat_cb;

        memset(&react_cb, 0, sizeof(react_cb));
        react_cb.adjust_health = wm_match_adjust_health;
        react_cb.user = m;

        m->combat_runtime.pcnt = m->tick_count;
        m->combat_runtime.round_tickcount = (uint16_t)m->tick_count;

        bridge.runtime = &m->combat_runtime;
        bridge.callbacks = &react_cb;
        memset(&bridge.last_result, 0, sizeof(bridge.last_result));

        memset(&combat_cb, 0, sizeof(combat_cb));
        combat_cb.wrestler_hit = wm_arcade_wrestler_hit_collision_callback;
        combat_cb.user = &bridge;

        (void)wm_arcade_check_wrestler_collisions(actor_ptrs, m->actor_count,
                                                  m->tick_count, &combat_cb);
    }

    ++m->tick_count;
}
