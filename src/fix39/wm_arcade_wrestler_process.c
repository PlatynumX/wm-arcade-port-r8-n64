#include "wm_arcade_wrestler_process.h"

bool wm_arcade_wrestler_process_dispatch_ready(wm_arcade_actor_t *actor)
{
    uint16_t ptime;
    int16_t signed_after_dec;

    if (!actor || !actor->active) return false;

    /* MPROC.ASM::process_dispatch stores PTIME as a 16-bit word:
         move *a13(PTIME),a0
         subk 1,a0
         move a0,*a13(PTIME)
         jrgt #next
       Preserve the 16-bit stored value (including 0000 -> ffff), while the
       branch decision uses the signed result of the decrement. */
    ptime = (uint16_t)actor->ptime;
    ptime = (uint16_t)(ptime - 1u);
    actor->ptime = (int32_t)ptime;
    signed_after_dec = (int16_t)ptime;
    return signed_after_dec <= 0;
}

void wm_arcade_wrestler_process_sleep_loop(wm_arcade_actor_t *actor)
{
    if (!actor) return;

    /* WRESTLE.ASM wrestler_main:
         movk 1,a0
         move *a13(STATUS_FLAGS),a14
         btst B_KOD,a14
         jrz  #slp
         movi >7fff,a0
       #slp SLEEPR a0
       MPROC.EQU::SLEEPR routes that value into PTIME. */
    actor->ptime = (actor->status_flags & WM_STATUS_KOD) ? 0x7fff : 1;
}

void wm_arcade_wrestler_process_wake(wm_arcade_actor_t *actor)
{
    /* Source wake sites write PTIME=1; process_dispatch decrements it to zero
       when that process-list entry is reached. */
    if (actor) actor->ptime = 1;
}
