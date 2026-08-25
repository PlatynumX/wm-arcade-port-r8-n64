#ifndef WM_AUDIO_H
#define WM_AUDIO_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define WM_AUDIO_EVENT_CAPACITY 16u

typedef struct {
    uint16_t command;
    uint32_t source_tick;
    int8_t source_channel; /* -2 raw/control, 0..3 translated DCSSOUND voice */
} wm_audio_event;

typedef struct {
    wm_audio_event events[WM_AUDIO_EVENT_CAPACITY];
    uint8_t read_index;
    uint8_t write_index;
    uint8_t count;
    uint32_t source_tick;
    uint32_t dropped_events;
    uint16_t last_command;

    /*
     * R35 decoded DCS binding state. The platform audio backend can consume
     * normal wm_audio_event entries and inspect these fields/helpers to route
     * source DCS command ids to DragonFS WAV64 assets. Command 0 stays a
     * stop/reset control command and deliberately has no decoded asset path.
     */
    uint32_t bound_events;
    uint32_t unbound_events;
    uint32_t stop_events;
    bool last_command_has_decoded_binding;
    uint32_t last_stream_address;
    const char *last_decoded_path;
} wm_audio_state;

void wm_audio_init(wm_audio_state *audio);
void wm_audio_source_tick(wm_audio_state *audio);
bool wm_audio_send_command(wm_audio_state *audio, uint16_t command);
bool wm_audio_send_routed_command(wm_audio_state *audio, uint16_t command, int8_t source_channel);
bool wm_audio_pop_event(wm_audio_state *audio, wm_audio_event *event);
bool wm_audio_command_has_decoded_asset(uint16_t command);
const char *wm_audio_command_decoded_path(uint16_t command);
uint32_t wm_audio_command_decoded_stream_address(uint16_t command);
#endif
