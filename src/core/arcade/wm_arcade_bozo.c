/*
 * DOINK.ASM:3316 bozo_check. See wm/arcade/wm_arcade_bozo.h.
 */
#include "wm/arcade/wm_arcade_bozo.h"

#include "wm/arcade/wm_arcade_combat_defs.h"

bool wm_arcade_bozo_check(wm_arcade_actor_t *actor, const wm_bozo_env_t *env) {
    int32_t total;

    if (!actor) return false;

    /* `move SPUNCHB_COUNT,a0 / move SKICKB_COUNT,a1 /
        move BLOCKB_COUNT,a2 / add a0,a1 / add a1,a2` */
    total = actor->spunchb_count + actor->skickb_count + actor->blockb_count;
    if (total < WM_BOZO_BUTTON_TOTAL) return false;      /* `#no_bozo` */

    /* "Do reversal unless I have been immobilized!" The count is not
       cleared here, so it still stands when he thaws. */
    if (actor->immobilize_time != 0) return false;

    /* SMRTTGT a13,WHOHITME (JJXM.H:37) -- the flag AND the target, in
       that order, and both halves matter. */
    actor->status_flags |= (uint32_t)WM_STATUS_SMART_ATTACK;
    actor->smart_target = actor->who_hit_me;

    /* "immobilize WHOHITME". The null check is this port's; see the
       header for why the source does not need one. */
    if (actor->who_hit_me)
        actor->who_hit_me->immobilize_time = WM_BOZO_IMMOBILIZE;

    if (env && env->find_and_kill_endless)
        env->find_and_kill_endless(actor, env->user);

    return true;                                          /* `setc` */
}
