#ifndef WM_ARCADE_SOURCE_ATTACK_FRAMES_H
#define WM_ARCADE_SOURCE_ATTACK_FRAMES_H
#include <stdbool.h>
#include <stdint.h>
#include "wm_arcade_anim_combat.h"
#ifdef __cplusplus
extern "C" {
#endif
bool wm_arcade_character_attack_for_source_frame(uint8_t roster_id, const char *source_frame,
                                                  bool *uses_z,
                                                  wm_arcade_attack_on_z_args_t *zargs,
                                                  wm_arcade_attack_on_args_t *args);
bool wm_arcade_bret_attack_for_source_frame(const char *source_frame, bool *uses_z,
                                             wm_arcade_attack_on_z_args_t *zargs,
                                             wm_arcade_attack_on_args_t *args);
#ifdef __cplusplus
}
#endif
#endif
