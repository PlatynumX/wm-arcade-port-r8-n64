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

    for (unsigned i = 0; i < WM_MATCH_MAX_ACTORS; ++i) {
        wm_bret_backend_init(&m->bret_visual[i]);
        if (m->actors[i].wrestler_num == WM_ROSTER_BRET) {
            wm_arcade_bret_callbacks_t cb = wm_bret_backend_callbacks(&m->bret_visual[i]);
            wm_arcade_bret_ani_init(&m->actors[i], &cb);
        }
    }

    m->actor_count = WM_MATCH_MAX_ACTORS;
    m->active = true;
    m->tick_count = 0;
}

void wm_match_tick(wm_match_state *m, const wm_arcade_drone_callbacks_t *cb) {
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
        uint16_t old_but = m->drones[i].but;
        uint16_t old_joy = m->drones[i].joy;
        (void)wm_arcade_drone_main(&m->actors[i], &m->drones[i], &world, cb);
        wm_arcade_drone_commit_inputs(&m->actors[i], &m->drones[i], old_but, old_joy);

        if (m->actors[i].wrestler_num == WM_ROSTER_BRET) {
            /* WM_MATCH_MAX_ACTORS==2: the other slot is always the opponent. */
            wm_arcade_actor_t *opp = &m->actors[1u - i];
            wm_arcade_bret_env_t env;
            wm_arcade_bret_callbacks_t bret_cb;
            m->bret_visual[i].opponent = opp;
            bret_cb = wm_bret_backend_callbacks(&m->bret_visual[i]);
            memset(&env, 0, sizeof(env));
            env.pcnt = m->tick_count;
            (void)wm_arcade_move_bret(&m->actors[i], opp, &env, &bret_cb);
            wm_bret_backend_tick(&m->bret_visual[i]);
            wm_bret_backend_tick_position(&m->actors[i]);
        }
    }

    ++m->tick_count;
}
