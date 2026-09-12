#include "wm/arcade/wm_arcade_mode_dead.h"

/* DOINK.ASM:2920-2926, in this port's always-reached subset only -- see
   wm_arcade_mode_dead.h for the full derivation of why every branch
   collapses to this. */
void wm_arcade_mode_dead(wm_arcade_actor_t *actor) {
    if (!actor) return;

    if (actor->status_flags & WM_STATUS_ZOMBIE) return;
    if (actor->status_flags & (WM_STATUS_DID_BUCKOFF |
                               WM_STATUS_NO_BUCKOFF |
                               WM_STATUS_DO_BUCKOFF)) {
        return;
    }

    actor->status_flags |= WM_STATUS_NO_BUCKOFF;
}
