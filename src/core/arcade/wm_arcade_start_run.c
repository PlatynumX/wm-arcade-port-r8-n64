#include "wm/arcade/wm_arcade_start_run.h"
#include "wm/arcade/wm_arcade_combat_defs.h"

/*
 * WRESTLE2.ASM:3443 start_run_anim, and the #setup_run/#dorun code its own
 * ANI_CODE runs. The animation itself has no WL frames at all -- it is a
 * state-setup routine that ends by selecting the running wrestler's real
 * run animation out of #run_anims[WRESTLERNUM] and setting MODE RUNNING.
 *
 * Shared rather than per-wrestler because the source's is: the only
 * wrestler-specific part is that one table lookup, which the caller
 * supplies.
 *
 * The source's rotate-first path (#contx: halve XVEL, zero ZVEL, defer
 * through CODE_ADDR + MODE WAITANIM) is dead in the shipped code --
 * #ok1 does an unconditional `jruc #dorun` and the `jruc #contx` after it
 * can never be reached -- so it is deliberately not translated.
 */
void wm_arcade_start_run(wm_arcade_actor_t *actor) {
    uint16_t lr;

    if (!actor) return;

    /* Direction: the stick's own left/right if the player is holding one,
       otherwise whichever way he is already facing. */
    lr = (uint16_t)(actor->stick_val_cur & (WM_MOVE_LEFT | WM_MOVE_RIGHT));
    if (lr == 0)
        lr = (uint16_t)(actor->facing_dir & (WM_MOVE_LEFT | WM_MOVE_RIGHT));

    if (lr != (uint16_t)(actor->facing_dir & (WM_MOVE_LEFT | WM_MOVE_RIGHT))) {
        /* "He wants to run in the opposite direction than he is facing --
           rotate him around first": both facings take the new left/right
           with the old up/down kept. */
        uint16_t ud = (uint16_t)(actor->facing_dir & (WM_MOVE_UP | WM_MOVE_DOWN));
        actor->new_facing_dir = (int32_t)(lr | ud);
        actor->facing_dir = (int32_t)(lr | ud);
    }

    /* #dorun / #dorun_flung */
    actor->getup_time = 0;
    actor->usr_var1 = 0;
    actor->run_time = 0;

    actor->move_dir = actor->facing_dir;
    actor->facing_dir = (int32_t)(
        ((uint16_t)actor->new_facing_dir & (WM_MOVE_UP | WM_MOVE_DOWN)) |
        (uint16_t)actor->move_dir);

    actor->player_mode = (uint16_t)WM_PMODE_RUNNING;
    actor->delay_butns = 1;
}
