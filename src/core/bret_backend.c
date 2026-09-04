#include "wm/bret_backend.h"
#include "wm/bret_visuals.h"
#include <string.h>

void wm_bret_backend_init(wm_bret_backend_actor *bva) {
    if (!bva) return;
    memset(bva, 0, sizeof(*bva));
}

/* BRET.ASM:2897 hrt_leg_anims_table, transcribed value-for-value.
   leg_table[move_compass][facing_compass], both wm_convert_facing() 0-7
   results, matching WRESTLE.ASM::change_walk_anim's move_compass*8+
   facing_compass addressing (WRESTLE.ASM:5000-5014). */
static const wm_visual_sequence *const leg_table[8][8] = {
    /* MOVE=UP */
    { &wm_bret_walk1_f2_anim, &wm_bret_walk1_f2_anim, &wm_bret_walk1_f4_anim, &wm_bret_walk1_f4_anim,
      &wm_bret_walk1_f4_anim, &wm_bret_walk1_f4_anim, &wm_bret_walk1_f2_anim, &wm_bret_walk1_f2_anim },
    /* MOVE=UP-RIGHT */
    { &wm_bret_walk2_f2_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk2_f4_anim,
      &wm_bret_walk8_f4_anim, &wm_bret_walk8_f4_anim, &wm_bret_walk4_f2_anim, &wm_bret_walk4_f2_anim },
    /* MOVE=RIGHT */
    { &wm_bret_walk2_f2_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk4_f4_anim,
      &wm_bret_walk4_f4_anim, &wm_bret_walk8_f4_anim, &wm_bret_walk6_f2_anim, &wm_bret_walk6_f2_anim },
    /* MOVE=DOWN-RIGHT */
    { &wm_bret_walk2_f2_anim, &wm_bret_walk8_f2_anim, &wm_bret_walk4_f4_anim, &wm_bret_walk4_f4_anim,
      &wm_bret_walk2_f4_anim, &wm_bret_walk6_f4_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk6_f2_anim },
    /* MOVE=DOWN */
    { &wm_bret_walk5_f2_anim, &wm_bret_walk5_f2_anim, &wm_bret_walk5_f4_anim, &wm_bret_walk5_f4_anim,
      &wm_bret_walk5_f4_anim, &wm_bret_walk5_f4_anim, &wm_bret_walk5_f2_anim, &wm_bret_walk5_f2_anim },
    /* MOVE=DOWN-LEFT */
    { &wm_bret_walk2_f2_anim, &wm_bret_walk6_f2_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk6_f4_anim,
      &wm_bret_walk2_f4_anim, &wm_bret_walk4_f4_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk8_f2_anim },
    /* MOVE=LEFT */
    { &wm_bret_walk2_f2_anim, &wm_bret_walk6_f2_anim, &wm_bret_walk6_f2_anim, &wm_bret_walk8_f4_anim,
      &wm_bret_walk4_f4_anim, &wm_bret_walk4_f4_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk2_f2_anim },
    /* MOVE=UP-LEFT */
    { &wm_bret_walk2_f2_anim, &wm_bret_walk4_f2_anim, &wm_bret_walk6_f2_anim, &wm_bret_walk8_f4_anim,
      &wm_bret_walk6_f4_anim, &wm_bret_walk2_f4_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk2_f2_anim },
};

const wm_visual_sequence *wm_bret_leg_anim(int move_compass, int facing_compass) {
    if (move_compass < 0 || move_compass > 7 || facing_compass < 0 || facing_compass > 7)
        return NULL;
    return leg_table[move_compass][facing_compass];
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

    /* WRESTLE.ASM::change_walk_anim's leg half (WRESTLE.ASM:5000-5014):
       reselects hrt_leg_anims_table[MOVE_DIR][FACING_DIR] every tick. Real
       FACING_DIR tracking needs NEW_FACING_DIR (set by an as-yet-unlocated
       shared routine) plus set_rotate_anim's FACING_DIR=NEW_FACING_DIR copy,
       neither of which is ported. While actually moving, this substitutes
       FACING_DIR=MOVE_DIR -- correct for straight walking, which is the
       only case reachable today (nothing here supports facing one way while
       walking another). #zip already cleared MOVE_DIR to 0, so
       wm_convert_facing returns -1 and no leg animation is (re)selected. */
    if (actor->move_dir != 0) {
        int move_compass, facing_compass;
        actor->facing_dir = actor->move_dir;
        move_compass = wm_convert_facing(actor->move_dir);
        facing_compass = wm_convert_facing(actor->facing_dir);
        start_if_new(&bva->visual, wm_bret_leg_anim(move_compass, facing_compass));
    }
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
