#ifndef WM_SOURCE_CLOCK_H
#define WM_SOURCE_CLOCK_H
#include <stdbool.h>
#include <stdint.h>

/* DISPLAY.EQU: TSEC equ 53. N64 video stays at 60 Hz; game state advances on
   the original source clock by accumulating 53 source ticks over 60 frames. */
#define WM_SOURCE_TICKS_PER_SEC 53u
#define WM_VIDEO_FRAMES_PER_SEC 60u

typedef struct {
    uint16_t accumulator;
    uint64_t video_frames;
    uint64_t source_ticks;
} wm_source_clock;

void wm_source_clock_init(wm_source_clock *clock);
/* Returns true when exactly one source tick is due on this video frame. */
bool wm_source_clock_video_frame(wm_source_clock *clock);

#endif
