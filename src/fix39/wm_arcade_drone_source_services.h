#ifndef WM_ARCADE_DRONE_SOURCE_SERVICES_H
#define WM_ARCADE_DRONE_SOURCE_SERVICES_H
#include <stdbool.h>
#include "wm_arcade_drone.h"
#ifdef __cplusplus
extern "C" {
#endif
bool wm_arcade_drone_source_services_ready(void);
int wm_arcade_drone_source_service_id(const char *source_label);
int wm_arcade_drone_source_service_dispatch(wm_arcade_actor_t *self, const char *source_label, void *user);
#ifdef __cplusplus
}
#endif
#endif
