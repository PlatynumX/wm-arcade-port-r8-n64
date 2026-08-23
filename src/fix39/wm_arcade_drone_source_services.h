#ifndef WM_ARCADE_DRONE_SOURCE_SERVICES_H
#define WM_ARCADE_DRONE_SOURCE_SERVICES_H

#include <stdbool.h>
#include <stdint.h>
#include "wm_arcade_combat.h"
#include "wm_arcade_drone.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*wm_arcade_drone_source_service_handler_t)(
    wm_arcade_actor_t *self,
    wm_arcade_actor_t *opp,
    wm_arcade_drone_state_t *drone,
    void *user);

bool wm_arcade_drone_source_services_ready(void);
int wm_arcade_drone_source_service_id(const char *label);
uint32_t wm_arcade_drone_source_service_addr(const char *label);
int wm_arcade_drone_source_service_kind(const char *label);

/* A historical executable seam becomes runnable only after a translated
   source body is attached.  Unattached/unknown seams remain fail-closed. */
int wm_arcade_drone_source_service_attach(
    const char *label,
    wm_arcade_drone_source_service_handler_t handler);
int wm_arcade_drone_source_service_body_ready(const char *label);
void wm_arcade_drone_source_service_reset_handlers(void);
int wm_arcade_drone_source_service_dispatch(
    wm_arcade_actor_t *self,
    wm_arcade_actor_t *opp,
    wm_arcade_drone_state_t *drone,
    const char *label,
    void *user);

#ifdef __cplusplus
}
#endif

#endif
