#ifndef WM_ARCADE_WRESTLER_PROCESS_H
#define WM_ARCADE_WRESTLER_PROCESS_H

#include <stdbool.h>
#include "wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MPROC.ASM::process_dispatch semantics for a wrestler process: PTIME is
 * decremented by the dispatcher; the process resumes when the result is <= 0. */
bool wm_arcade_wrestler_process_dispatch_ready(wm_arcade_actor_t *actor);

/* WRESTLE.ASM::wrestler_main #slp. Ordinary wrestlers SLEEPR 1. A wrestler
 * carrying B_KOD sleeps at >7fff until another source path writes PTIME=1. */
void wm_arcade_wrestler_process_sleep_loop(wm_arcade_actor_t *actor);

/* Source wake operations (for example can_pin/reset paths) write PTIME=1 so
 * process_dispatch will resume the wrestler on its next visit. */
void wm_arcade_wrestler_process_wake(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif
#endif
