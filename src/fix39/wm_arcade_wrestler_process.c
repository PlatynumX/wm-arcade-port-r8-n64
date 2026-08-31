#include "wm_arcade_wrestler_process.h"

#include <stdint.h>

void wm_arcade_wrestler_process_init(wm_arcade_wrestler_process_t *proc,
                                     wm_arcade_actor_t *actor)
{
    if (proc)
        proc->resume = WM_WRESTLER_RESUME_CALC_CLOSEST;
    if (actor)
        actor->ptime = 1;
}

bool wm_arcade_wrestler_process_dispatch_ready(wm_arcade_actor_t *actor)
{
    uint16_t ptime;
    int16_t signed_after_dec;

    if (!actor || !actor->active)
        return false;

    ptime = (uint16_t)actor->ptime;
    ptime = (uint16_t)(ptime - 1u);
    actor->ptime = (int32_t)ptime;
    signed_after_dec = (int16_t)ptime;
    return signed_after_dec <= 0;
}

wm_arcade_wrestler_resume_t
wm_arcade_wrestler_process_resume(const wm_arcade_wrestler_process_t *proc)
{
    return proc ? proc->resume : WM_WRESTLER_RESUME_POST_SLEEP;
}

void wm_arcade_wrestler_process_sleep(wm_arcade_wrestler_process_t *proc,
                                      wm_arcade_actor_t *actor)
{
    if (proc)
        proc->resume = WM_WRESTLER_RESUME_POST_SLEEP;
    if (!actor)
        return;

    actor->ptime =
        (actor->status_flags & WM_STATUS_KOD) ? 0x7fff : 1;
}

void wm_arcade_wrestler_process_wake(wm_arcade_actor_t *actor)
{
    if (actor)
        actor->ptime = 1;
}
