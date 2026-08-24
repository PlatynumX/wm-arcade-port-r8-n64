#ifndef WM_ARCADE_DRONE_SOURCE_BODIES_H
#define WM_ARCADE_DRONE_SOURCE_BODIES_H

#include "wm_arcade_drone.h"

int wm_arcade_drone_source_install_generated_bodies(void);
int wm_arcade_drone_source_generated_body_count(void);

/*
 * DRONE.ASM::drone_chrg stores a continuation pointer in DRN_BUTCHRG_p.
 * Combat2EG executes those exact continuations natively once the source
 * charge delay expires.
 */
wm_arcade_drone_step_result_t wm_arcade_drone_native_charge_step(
    wm_arcade_actor_t *self,
    wm_arcade_actor_t *opp,
    wm_arcade_drone_state_t *drone,
    const wm_arcade_drone_callbacks_t *cb);

#endif
