#ifndef WM_VISUAL_H
#define WM_VISUAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *source_frame;
    uint16_t ticks;
} wm_visual_frame;

typedef struct {
    const char *source_file;
    const char *source_label;
    const wm_visual_frame *frames;
    size_t frame_count;
    bool repeat;
} wm_visual_sequence;

typedef struct {
    const wm_visual_sequence *sequence;
    size_t frame_index;
    uint16_t ticks_left;
    bool just_started;
    bool ended;
} wm_visual_state;

void wm_visual_start(wm_visual_state *state, const wm_visual_sequence *sequence);
void wm_visual_tick(wm_visual_state *state);
const wm_visual_frame *wm_visual_current(const wm_visual_state *state);

#endif
