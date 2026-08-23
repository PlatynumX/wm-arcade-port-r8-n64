#ifndef WM_ARCADE_SOURCE_ANIMATION_RUNTIME_H
#define WM_ARCADE_SOURCE_ANIMATION_RUNTIME_H
#include <stdbool.h>
#include <stdint.h>
#include "wm_arcade_combat.h"
#include "wm_arcade_source_animation_catalog.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wm_source_anim_runtime {
    const wm_source_anim_def_t *def;
    uint16_t frame_index;
    uint16_t ticks_left;
    const char *current_frame;
} wm_source_anim_runtime_t;

void wm_source_anim_runtime_init(wm_source_anim_runtime_t *s);
bool wm_source_anim_runtime_change(wm_source_anim_runtime_t *s, wm_arcade_actor_t *a,
                                   uint8_t roster_id, const char *label);
void wm_source_anim_runtime_tick(wm_source_anim_runtime_t *s, wm_arcade_actor_t *a);
const char *wm_source_anim_runtime_frame(const wm_source_anim_runtime_t *s);
const char *wm_source_anim_runtime_label(const wm_source_anim_runtime_t *s);

#ifdef __cplusplus
}
#endif
#endif
