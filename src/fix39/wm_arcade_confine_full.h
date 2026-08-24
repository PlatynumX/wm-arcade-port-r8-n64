#ifndef WM_ARCADE_CONFINE_FULL_H
#define WM_ARCADE_CONFINE_FULL_H

#include <stdint.h>
#include "wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum wm_arcade_confine_climb_request {
    WM_CONFINE_CLIMB_OUT_TOP = 0,
    WM_CONFINE_CLIMB_OUT_BOTTOM,
    WM_CONFINE_CLIMB_OUT_SIDE,
    WM_CONFINE_CLIMB_IN_TOP,
    WM_CONFINE_CLIMB_IN_BOTTOM,
    WM_CONFINE_CLIMB_IN_SIDE
} wm_arcade_confine_climb_request_t;

typedef enum wm_arcade_confine_gate_anim {
    WM_CONFINE_GATE_FALL_BACK = 0,
    WM_CONFINE_GATE_BOUNCE
} wm_arcade_confine_gate_anim_t;

enum {
    WM_CONFINE_ROPE_LEFT = 2,
    WM_CONFINE_ROPE_RIGHT = 3
};

typedef struct wm_arcade_confine_callbacks {
    void (*climb)(wm_arcade_actor_t *actor,
                  wm_arcade_confine_climb_request_t request,
                  int16_t line_x,
                  void *user);
    void (*rope_bounce_io)(wm_arcade_actor_t *actor,
                           unsigned rope_bank,
                           uint8_t selector,
                           void *user);
    void (*sound)(wm_arcade_actor_t *actor, uint16_t source_sound, void *user);
    void (*gate_anim)(wm_arcade_actor_t *actor,
                      wm_arcade_confine_gate_anim_t which,
                      void *user);
    void (*adjust_health)(wm_arcade_actor_t *actor, int delta, void *user);
    void (*ditch_getup_meter)(wm_arcade_actor_t *actor, void *user);
    void (*zombie_transform)(wm_arcade_actor_t *actor, void *user);
    void *user;
} wm_arcade_confine_callbacks_t;

void wm_arcade_confine_wrestler(wm_arcade_actor_t *actor,
                                uint32_t pcnt,
                                const wm_arcade_confine_callbacks_t *callbacks);
void wm_arcade_confine_fix1(wm_arcade_actor_t *actor);
void wm_arcade_confine_fix2(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif
#endif
