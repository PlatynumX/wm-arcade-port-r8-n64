#ifndef WM_ARCADE_SOUND_LOOKUP_H
#define WM_ARCADE_SOUND_LOOKUP_H
#include <stdbool.h>
#include <stdint.h>
#include "wm/arcade_sound.h"
struct wm_arcade_actor;
bool wm_arcade_sound_wrsnd(wm_arcade_sound *s, uint8_t wrestler, uint16_t sound_index);
bool wm_arcade_sound_wrsnd_pair(wm_arcade_sound *s, uint8_t wrestler, uint16_t a, int b);
bool wm_arcade_sound_play_label(wm_arcade_sound *s, const struct wm_arcade_actor *actor, const char *label);
#endif
