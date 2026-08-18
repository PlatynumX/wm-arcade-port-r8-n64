#ifndef WM_DEMO_H
#define WM_DEMO_H
#include <stdbool.h>
#include "wm/anim.h"
#include "wm/game.h"
#include "wm/input.h"
#include "wm/object.h"
#include "wm/visual.h"

typedef enum {
    WM_DEMO_FACING_2 = 2,
    WM_DEMO_FACING_4 = 4,
    WM_DEMO_FACING_6 = 6,
    WM_DEMO_FACING_8 = 8
} wm_demo_facing;

typedef enum {
    WM_DEMO_IDLE = 0,
    WM_DEMO_WALK,
    WM_DEMO_RUN,
    WM_DEMO_BLOCK,
    WM_DEMO_LIGHT_PUNCH,
    WM_DEMO_POWER_PUNCH,
    WM_DEMO_LIGHT_KICK,
    WM_DEMO_POWER_KICK
} wm_demo_action;

typedef struct {
    wm_visual_state visual;
    wm_visual_state torso_visual;
    wm_demo_facing facing;
    wm_demo_action action;
    int screen_x;
    int screen_y;
    bool flip_x;
    int health;
    unsigned stun_ticks;
    unsigned action_count;
    unsigned hit_count;
    unsigned blocked_count;
    bool attack_connected;
} wm_demo_fighter;

typedef struct {
    wm_game game;
    wm_object wrestler;
    wm_anim_state anim;
    wm_demo_fighter p1;
    wm_demo_fighter p2;
    bool ai_enabled;
    unsigned ai_cooldown;
    unsigned restarts;
    unsigned total_hits;
    unsigned total_blocks;
} wm_demo;

void wm_demo_init(wm_demo *d);
void wm_demo_reset_match(wm_demo *d);
void wm_demo_tick(wm_demo *d, const wm_input_state *input);
const char *wm_demo_action_name(wm_demo_action action);
const char *wm_demo_facing_name(wm_demo_facing facing);

#endif
