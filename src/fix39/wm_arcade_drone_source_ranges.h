#ifndef WM_ARCADE_DRONE_SOURCE_RANGES_H
#define WM_ARCADE_DRONE_SOURCE_RANGES_H

#include <stdbool.h>
#include "wm_arcade_drone.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exact wnshort_t / wnmed_t / wnlong_t mode-list resolver.  The generated
 * payload contains source script-list LABELS only; script bodies are a later
 * chunk and remain intentionally unresolved here. */
bool wm_arcade_drone_source_ranges_ready(void);
const wm_arcade_drone_script_list_t *wm_arcade_drone_source_range_script_list(
    const wm_arcade_actor_t *self,
    const wm_arcade_actor_t *opp,
    int range_band,
    int my_mode,
    int opp_mode,
    void *user);

#ifdef __cplusplus
}
#endif
#endif
