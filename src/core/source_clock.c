#include "wm/source_clock.h"
#include <string.h>

void wm_source_clock_init(wm_source_clock *clock) {
    memset(clock, 0, sizeof(*clock));
}

bool wm_source_clock_video_frame(wm_source_clock *clock) {
    if (!clock) return false;
    ++clock->video_frames;
    clock->accumulator = (uint16_t)(clock->accumulator + WM_SOURCE_TICKS_PER_SEC);
    if (clock->accumulator < WM_VIDEO_FRAMES_PER_SEC)
        return false;
    clock->accumulator = (uint16_t)(clock->accumulator - WM_VIDEO_FRAMES_PER_SEC);
    ++clock->source_ticks;
    return true;
}
