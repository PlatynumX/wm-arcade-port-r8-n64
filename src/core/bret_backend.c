#include "wm/bret_backend.h"
#include "wm/arcade/wm_arcade_anim_combat.h"
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

/*
 * Real ANIM.ASM ATTACK_ON(_Z) args for the 6 attack ids wm_bret_anim_sequence
 * maps, hand-traced against HRTSEQ2.ASM (see wm_bret_backend_tick's comment):
 * each attack's ANI_ATTACK_ON(_Z) command falls right before the WL frame
 * line at active_frame_index (0-based) in that id's wm_visual_sequence.
 */
typedef struct {
    wm_arcade_bret_anim_id_t id;
    size_t active_frame_index;
    bool use_z;
    wm_arcade_attack_on_args_t args;
    wm_arcade_attack_on_z_args_t z_args;
} wm_bret_attack_window_t;

static const wm_bret_attack_window_t attack_windows[] = {
    /* HRTSEQ2.ASM:204 ANI_ATTACK_ON_Z, AMODE_PUNCH,30,91,-45,50,15,45 */
    { WM_BRET_ANIM_PUNCH2, 5, true, {0,0,0,0,0},
      { WM_AMODE_PUNCH, 30, 91, -45, 50, 15, 45 } },
    /* HRTSEQ2.ASM:322 ANI_ATTACK_ON_Z, AMODE_PUNCH,30,91,0,50,15,45 */
    { WM_BRET_ANIM_PUNCH4, 5, true, {0,0,0,0,0},
      { WM_AMODE_PUNCH, 30, 91, 0, 50, 15, 45 } },
    /* HRTSEQ2.ASM:247 ANI_ATTACK_ON,AMODE_UPRCUT,-6,40,64,90 */
    { WM_BRET_ANIM_SUPER_PUNCH2_4, 5, false,
      { WM_AMODE_UPRCUT, -6, 40, 64, 90 }, {0,0,0,0,0,0,0} },
    /* HRTSEQ2.ASM:1074 ANI_ATTACK_ON,AMODE_KICK,23,73,50,17 */
    { WM_BRET_ANIM_KICK2, 5, false,
      { WM_AMODE_KICK, 23, 73, 50, 17 }, {0,0,0,0,0,0,0} },
    /* HRTSEQ2.ASM:1240 ANI_ATTACK_ON,AMODE_KICK,23,73,50,17 */
    { WM_BRET_ANIM_KICK4, 5, false,
      { WM_AMODE_KICK, 23, 73, 50, 17 }, {0,0,0,0,0,0,0} },
    /* HRTSEQ2.ASM:1357 ANI_ATTACK_ON,AMODE_SUPER_KICK,5,54,70,34 */
    { WM_BRET_ANIM_SUPER_KICK2, 4, false,
      { WM_AMODE_SUPER_KICK, 5, 54, 70, 34 }, {0,0,0,0,0,0,0} },
};
#define WM_BRET_ATTACK_WINDOW_COUNT \
    (sizeof(attack_windows) / sizeof(attack_windows[0]))

static const wm_bret_attack_window_t *find_attack_window(wm_arcade_bret_anim_id_t id) {
    size_t i;
    for (i = 0; i < WM_BRET_ATTACK_WINDOW_COUNT; ++i)
        if (attack_windows[i].id == id) return &attack_windows[i];
    return NULL;
}

wm_arcade_frame_box_t wm_bret_hurt_box_for_frame(const char *source_frame) {
    wm_arcade_frame_box_t box;
    const wm_bret_frame_geometry_t *geo;

    box.iani3x = 0;
    box.iani3y = 0;
    box.iani3z = 0;
    box.iani3id = 0;

    geo = source_frame ? wm_bret_frame_geometry_find(source_frame) : NULL;
    if (!geo) return box;

    box.iani3x = -(int32_t)geo->xani;
    box.iani3y = -(int32_t)geo->yani;
    box.iani3z = (int32_t)geo->width;
    box.iani3id = (int32_t)geo->height;
    return box;
}

void wm_bret_backend_change_anim(wm_arcade_actor_t *actor,
                                 wm_arcade_bret_anim_id_t id, void *user) {
    wm_bret_backend_actor *bva = (wm_bret_backend_actor *)user;
    (void)actor;
    if (!bva) return;
    start_if_new(&bva->visual, wm_bret_anim_sequence(id));
    bva->current_id = id;
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

void wm_bret_backend_tick(wm_bret_backend_actor *bva, wm_arcade_actor_t *actor,
                          uint16_t round_tickcount) {
    const wm_bret_attack_window_t *w;
    bool at_active_frame;

    if (!bva) return;
    wm_visual_tick(&bva->visual);
    wm_visual_tick(&bva->torso_visual);

    if (!actor) return;

    {
        const wm_visual_frame *cur = wm_visual_current(&bva->visual);
        wm_arcade_frame_box_t box =
            wm_bret_hurt_box_for_frame(cur ? cur->source_frame : NULL);
        wm_arcade_set_hurt_box(actor, &box);
    }

    w = find_attack_window(bva->current_id);
    at_active_frame = w && bva->visual.sequence == wm_bret_anim_sequence(bva->current_id) &&
                      bva->visual.frame_index == w->active_frame_index;

    if (at_active_frame) {
        if (!bva->attack_active) {
            if (w->use_z) wm_arcade_ani_attack_on_z(actor, &w->z_args);
            else wm_arcade_ani_attack_on(actor, &w->args);
            bva->attack_active = true;
        }
    } else if (bva->attack_active) {
        wm_arcade_ani_attack_off(actor, round_tickcount);
        bva->attack_active = false;
    }
}

void wm_bret_backend_tick_position(wm_arcade_actor_t *actor) {
    wm_integrate_position(actor);
}
