#ifndef WM_ARCADE_START_RUN_H
#define WM_ARCADE_START_RUN_H

#include "wm/arcade/wm_arcade_combat.h"

/*
 * WRESTLE2.ASM:3443 start_run_anim's own state setup: pick the run
 * direction, clear the getup/run timers, face the way he is running, and
 * enter MODE RUNNING. The caller selects the wrestler's own run animation,
 * which is the only wrestler-specific part of the source routine
 * (#run_anims[WRESTLERNUM]).
 */
void wm_arcade_start_run(wm_arcade_actor_t *actor);

#endif
