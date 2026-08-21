#ifndef WM_ARCADE_DRONE_SOURCE_SERVICES_H
#define WM_ARCADE_DRONE_SOURCE_SERVICES_H
#include <stdbool.h>
#include <stdint.h>
#include "wm_arcade_combat.h"
#ifdef __cplusplus
extern "C" {
typedef int (*wm_arcade_drone_source_service_handler_t)(wm_arcade_actor_t *self, void *user);

/* Chunk 5e: explicit translated-body attachment. A source seam is executable
   only when a real translated handler has been attached; otherwise dispatch
   remains fail-closed. */
int wm_arcade_drone_source_service_attach(const char *label,
    wm_arcade_drone_source_service_handler_t handler);
int wm_arcade_drone_source_service_body_ready(const char *label);
void wm_arcade_drone_source_service_reset_handlers(void);

#endif
bool wm_arcade_drone_source_services_ready(void);
int wm_arcade_drone_source_service_id(const char *label);
uint32_t wm_arcade_drone_source_service_addr(const char *label);
int wm_arcade_drone_source_service_kind(const char *label);
int wm_arcade_drone_source_service_dispatch(wm_arcade_actor_t *self,const char *label,void *user);
#ifdef __cplusplus
}
typedef int (*wm_arcade_drone_source_service_handler_t)(wm_arcade_actor_t *self, void *user);

/* Chunk 5e: explicit translated-body attachment. A source seam is executable
   only when a real translated handler has been attached; otherwise dispatch
   remains fail-closed. */
int wm_arcade_drone_source_service_attach(const char *label,
    wm_arcade_drone_source_service_handler_t handler);
int wm_arcade_drone_source_service_body_ready(const char *label);
void wm_arcade_drone_source_service_reset_handlers(void);

#endif
typedef int (*wm_arcade_drone_source_service_handler_t)(wm_arcade_actor_t *self, void *user);

/* Chunk 5e: explicit translated-body attachment. A source seam is executable
   only when a real translated handler has been attached; otherwise dispatch
   remains fail-closed. */
int wm_arcade_drone_source_service_attach(const char *label,
    wm_arcade_drone_source_service_handler_t handler);
int wm_arcade_drone_source_service_body_ready(const char *label);
void wm_arcade_drone_source_service_reset_handlers(void);

#endif
