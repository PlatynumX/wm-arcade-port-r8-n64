#include "wm/arcade_sound_bridge.h"
#include "wm/audio.h"

bool wm_arcade_sound_emit_to_audio(void *user, uint16_t command, int8_t source_channel) {
    return wm_audio_send_routed_command((wm_audio_state *)user, command, source_channel);
}
