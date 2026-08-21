#ifndef WM_ARCADE_DRONE_SOURCE_SCRIPTS_H
#define WM_ARCADE_DRONE_SOURCE_SCRIPTS_H

#include <stdbool.h>
#include "wm_arcade_drone.h"

#ifdef __cplusplus
extern "C" {
#endif

/* C3: decoded DRONE.ASM script bodies and literal command-2 skill tables. */
bool wm_arcade_drone_source_scripts_ready(void);
const wm_arcade_drone_script_t *wm_arcade_drone_source_resolve_script(const char *source_label, void *user);
int32_t wm_arcade_drone_source_script_skill_pct(const char *source_table_label, int skill, void *user);
int wm_arcade_drone_source_c4_seam_count(void);

#ifdef __cplusplus
}
#endif
#endif
