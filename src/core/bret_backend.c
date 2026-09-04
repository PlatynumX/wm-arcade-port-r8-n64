#include "wm/bret_backend.h"
#include "wm/bret_visuals.h"
#include <string.h>

void wm_bret_backend_init(wm_bret_backend_actor *bva) {
    if (!bva) return;
    memset(bva, 0, sizeof(*bva));
}

const wm_visual_sequence *wm_bret_anim_sequence(wm_arcade_bret_anim_id_t id) {
    switch (id) {
        case WM_BRET_ANIM_STAND2: return &wm_bret_stand2_anim;
        case WM_BRET_ANIM_STAND4: return &wm_bret_stand4_anim;
        case WM_BRET_ANIM_TORSO2: return &wm_bret_torso2_anim;
        case WM_BRET_ANIM_TORSO4: return &wm_bret_torso4_anim;
        case WM_BRET_ANIM_PUNCH2: return &wm_bret_light_punch2_anim;
        case WM_BRET_ANIM_PUNCH4: return &wm_bret_light_punch4_anim;
        case WM_BRET_ANIM_SUPER_PUNCH2_4: return &wm_bret_power_punch_anim;
        case WM_BRET_ANIM_KICK2: return &wm_bret_light_kick2_anim;
        case WM_BRET_ANIM_KICK4: return &wm_bret_light_kick4_anim;
        case WM_BRET_ANIM_SUPER_KICK2: return &wm_bret_power_kick_anim;
        default: return NULL;
    }
}

static void start_if_new(wm_visual_state *state, const wm_visual_sequence *seq) {
    if (!state || !seq) return;
    if (state->sequence != seq || state->ended)
        wm_visual_start(state, seq);
}

void wm_bret_backend_change_anim(wm_arcade_actor_t *actor,
                                 wm_arcade_bret_anim_id_t id, void *user) {
    wm_bret_backend_actor *bva = (wm_bret_backend_actor *)user;
    (void)actor;
    if (!bva) return;
    start_if_new(&bva->visual, wm_bret_anim_sequence(id));
}

void wm_bret_backend_change_torso_anim(wm_arcade_actor_t *actor,
                                       wm_arcade_bret_anim_id_t id, void *user) {
    wm_bret_backend_actor *bva = (wm_bret_backend_actor *)user;
    (void)actor;
    if (!bva) return;
    start_if_new(&bva->torso_visual, wm_bret_anim_sequence(id));
}

void wm_bret_backend_execute_walk(wm_arcade_actor_t *actor, void *user) {
    wm_bret_backend_actor *bva = (wm_bret_backend_actor *)user;
    if (!actor || !bva) return;
    wm_execute_walk(actor, bva->opponent, wm_bret_velocity_table);
}

wm_arcade_bret_callbacks_t wm_bret_backend_callbacks(wm_bret_backend_actor *bva) {
    wm_arcade_bret_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.change_anim = wm_bret_backend_change_anim;
    cb.change_torso_anim = wm_bret_backend_change_torso_anim;
    cb.execute_walk = wm_bret_backend_execute_walk;
    cb.user = bva;
    return cb;
}

void wm_bret_backend_tick(wm_bret_backend_actor *bva) {
    if (!bva) return;
    wm_visual_tick(&bva->visual);
    wm_visual_tick(&bva->torso_visual);
}

void wm_bret_backend_tick_position(wm_arcade_actor_t *actor) {
    wm_integrate_position(actor);
}
