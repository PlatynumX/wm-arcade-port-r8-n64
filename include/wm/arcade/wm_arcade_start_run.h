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

/*
 * The CODE_ADDR token for WRESTLE2.ASM's #dorun_flung. CODE_ADDR's other
 * tokens are WmRingClimbContinuation values (1..3); this one sits well
 * above them so a resolver can never read one family as the other.
 */
#define WM_CODE_ADDR_DORUN_FLUNG 0x100u

/*
 * WRESTLE2.ASM:3488 #ok2, the second ANI_CODE in start_run_flung (:3433).
 * Clear DELAY_METER, park #dorun_flung in CODE_ADDR, SETMODE WAITANIM,
 * halve OBJ_XVEL (`sra 1`) and zero OBJ_ZVEL. The flung wrestler then
 * slides at half speed until the animation's ANI_END lets mode_waitanim
 * call CODE_ADDR.
 */
void wm_arcade_start_run_flung_ok2(wm_arcade_actor_t *actor);

/*
 * WRESTLE2.ASM:3515 #dorun_flung, in the two halves either side of its
 * `calla change_anim1a`. _begin clears USR_VAR1 and RUN_TIME and faces
 * him the way he will run; the caller then changes to #run_anims
 * [WRESTLERNUM]; _end does SETMODE RUNNING and DELAY_BUTNS = 1. #dorun
 * (:3511) is the same code with GETUP_TIME cleared first; #dorun_flung
 * does not clear it, so a flung wrestler keeps FLUNG_TIME.
 */
void wm_arcade_dorun_flung_begin(wm_arcade_actor_t *actor);
void wm_arcade_dorun_flung_end(wm_arcade_actor_t *actor);

#endif
