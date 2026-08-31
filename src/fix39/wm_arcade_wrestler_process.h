#ifndef WM_ARCADE_WRESTLER_PROCESS_H
#define WM_ARCADE_WRESTLER_PROCESS_H

#include <stdbool.h>
#include "wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum wm_arcade_wrestler_resume {
    WM_WRESTLER_RESUME_CALC_CLOSEST = 0,
    WM_WRESTLER_RESUME_POST_SLEEP = 1
} wm_arcade_wrestler_resume_t;

typedef struct wm_arcade_wrestler_process {
    wm_arcade_wrestler_resume_t resume;
} wm_arcade_wrestler_process_t;

void wm_arcade_wrestler_process_init(wm_arcade_wrestler_process_t *proc,
                                     wm_arcade_actor_t *actor);
bool wm_arcade_wrestler_process_dispatch_ready(wm_arcade_actor_t *actor);
wm_arcade_wrestler_resume_t
wm_arcade_wrestler_process_resume(const wm_arcade_wrestler_process_t *proc);
void wm_arcade_wrestler_process_sleep(wm_arcade_wrestler_process_t *proc,
                                      wm_arcade_actor_t *actor);
void wm_arcade_wrestler_process_wake(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif
#endif
