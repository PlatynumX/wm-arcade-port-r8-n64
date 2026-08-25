#include "wm/audio.h"
#include "wm/dcs_port_bindings.h"
#include <string.h>

void wm_audio_init(wm_audio_state *audio) {
    if (!audio) return;
    memset(audio, 0, sizeof(*audio));
}

void wm_audio_source_tick(wm_audio_state *audio) {
    if (audio) ++audio->source_tick;
}

bool wm_audio_send_routed_command(wm_audio_state *audio, uint16_t command, int8_t source_channel) {
    if (!audio) return false;
    audio->last_command = command;

    if (command == 0u) {
        ++audio->stop_events;
        audio->last_command_has_decoded_binding = false;
        audio->last_stream_address = 0u;
        audio->last_decoded_path = 0;
    } else {
        const wm_dcs_port_binding *binding = wm_dcs_find_first_binding_for_command(command);
        audio->last_command_has_decoded_binding = binding != 0;
        if (binding) {
            ++audio->bound_events;
            audio->last_stream_address = binding->stream_address;
            audio->last_decoded_path = binding->dragonfs_path;
        } else {
            ++audio->unbound_events;
            audio->last_stream_address = 0u;
            audio->last_decoded_path = 0;
        }
    }

    if (audio->count >= WM_AUDIO_EVENT_CAPACITY) { ++audio->dropped_events; return false; }
    wm_audio_event *event = &audio->events[audio->write_index];
    event->command = command; event->source_tick = audio->source_tick; event->source_channel = source_channel;
    audio->write_index = (uint8_t)((audio->write_index + 1u) % WM_AUDIO_EVENT_CAPACITY); ++audio->count; return true;
}

bool wm_audio_command_has_decoded_asset(uint16_t command) {
    return command != 0u && wm_dcs_command_has_decoded_binding(command);
}

const char *wm_audio_command_decoded_path(uint16_t command) {
    const wm_dcs_port_binding *binding = wm_dcs_find_first_binding_for_command(command);
    return binding ? binding->dragonfs_path : 0;
}

uint32_t wm_audio_command_decoded_stream_address(uint16_t command) {
    const wm_dcs_port_binding *binding = wm_dcs_find_first_binding_for_command(command);
    return binding ? binding->stream_address : 0u;
}

bool wm_audio_send_command(wm_audio_state *audio, uint16_t command) {
    return wm_audio_send_routed_command(audio, command, -2);
}

bool wm_audio_pop_event(wm_audio_state *audio, wm_audio_event *event) {
    if (!audio || !event || audio->count == 0) return false;
    *event = audio->events[audio->read_index];
    audio->read_index = (uint8_t)((audio->read_index + 1u) % WM_AUDIO_EVENT_CAPACITY);
    --audio->count;
    return true;
}
