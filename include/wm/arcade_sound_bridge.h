#ifndef WM_ARCADE_SOUND_BRIDGE_H
#define WM_ARCADE_SOUND_BRIDGE_H
#include <stdbool.h>
#include <stdint.h>
bool wm_arcade_sound_emit_to_audio(void *user, uint16_t command, int8_t source_channel);
#endif
