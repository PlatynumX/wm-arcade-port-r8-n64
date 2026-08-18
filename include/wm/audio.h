#ifndef WM_AUDIO_H
#define WM_AUDIO_H
#include <stdbool.h>
#include <stdint.h>

#define WM_AUDIO_EVENT_CAPACITY 16u

typedef struct {
    uint16_t command;
    uint32_t source_tick;
} wm_audio_event;

typedef struct {
    wm_audio_event events[WM_AUDIO_EVENT_CAPACITY];
    uint8_t read_index;
    uint8_t write_index;
    uint8_t count;
    uint32_t source_tick;
    uint32_t dropped_events;
    uint16_t last_command;
} wm_audio_state;

void wm_audio_init(wm_audio_state *audio);
void wm_audio_source_tick(wm_audio_state *audio);
bool wm_audio_send_command(wm_audio_state *audio, uint16_t command);
bool wm_audio_pop_event(wm_audio_state *audio, wm_audio_event *event);
#endif
