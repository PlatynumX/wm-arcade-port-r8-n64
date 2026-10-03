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
 * start_run_anim's rotate-first path (`jruc #contx` after #ok1) is dead
 * in the shipped code -- #ok1 does an unconditional `jruc #dorun` -- but
 * #contx itself is NOT dead: start_run_flung's #ok2 falls into it. That
 * half is wm_arcade_start_run_flung_ok2 below.
 */

/* MACROS.H:131 SETMODE: every PLYRMODE write except onto MODE_DEAD. */
static void setmode(wm_arcade_actor_t *actor, unsigned mode) {
    if (actor->player_mode == WM_PMODE_DEAD) return;
    actor->player_mode = (uint16_t)mode;
}

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

    /* #dorun, then its fall-through into #dorun_flung. */
    actor->getup_time = 0;
    wm_arcade_dorun_flung_begin(actor);
    wm_arcade_dorun_flung_end(actor);
}

void wm_arcade_start_run_flung_ok2(wm_arcade_actor_t *actor) {
    if (!actor) return;
    /* "Whenever you fling someone, a meter can & will appear" */
    actor->delay_meter = 0;
    /* #contx */
    actor->code_addr = (uintptr_t)WM_CODE_ADDR_DORUN_FLUNG;
    setmode(actor, WM_PMODE_WAITANIM);
    actor->x_vel >>= 1;    /* `sra 1`: arithmetic, so a leftward fling stays leftward */
    actor->z_vel = 0;
}

void wm_arcade_dorun_flung_begin(wm_arcade_actor_t *actor) {
    if (!actor) return;
    actor->usr_var1 = 0;   /* "with x-xel" */
    actor->run_time = 0;

    /* ";Bogosity.." -- the whole FACING_DIR, not just its left/right. */
    actor->move_dir = actor->facing_dir;
    actor->facing_dir = (int32_t)(
        ((uint16_t)actor->new_facing_dir & (WM_MOVE_UP | WM_MOVE_DOWN)) |
        (uint16_t)actor->move_dir);
}

void wm_arcade_dorun_flung_end(wm_arcade_actor_t *actor) {
    if (!actor) return;
    setmode(actor, WM_PMODE_RUNNING);
    actor->delay_butns = 1;
}
