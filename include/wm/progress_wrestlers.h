#ifndef WM_PROGRESS_WRESTLERS_H
#define WM_PROGRESS_WRESTLERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wm/bret_sprites.h"

typedef enum {
    WM_PROGRESS_ACT_STAND = 0,
    WM_PROGRESS_ACT_RUN,
    WM_PROGRESS_ACT_WAIT,
    WM_PROGRESS_ACT_CLEVER,
    WM_PROGRESS_ACT_TAUNT,
    WM_PROGRESS_ACT_DEAD,
    WM_PROGRESS_ACT_COUNT
} wm_progress_action;

typedef struct {
    const char *source_frame;
    uint16_t ticks;
    bool xflip;
    /* Cumulative ANI_OFFSET state in source world coordinates at this frame.
       PROGRESS.ASM uses this for Bam Bam's offscreen wait/rise choreography. */
    int16_t offset_x;
    int16_t offset_y;
    int16_t offset_z;
} wm_progress_anim_frame;

typedef struct {
    const char *source_label;
    const wm_progress_anim_frame *frames;
    size_t frame_count;
    size_t loop_start;
    bool repeat;
} wm_progress_anim;

typedef struct {
    const char *source_name;
    uint16_t *rgba5551;
    uint16_t color_count;
} wm_progress_palette;

const wm_source_sprite *wm_progress_sprite_find(const char *source_frame);
const wm_progress_anim *wm_progress_anim_get(uint8_t source_wrestler,
                                             wm_progress_action action,
                                             bool torso);
const wm_progress_anim_frame *wm_progress_anim_frame_at(const wm_progress_anim *anim,
                                                        unsigned source_ticks);
const wm_progress_palette *wm_progress_palette_for_wrestler(uint8_t source_wrestler);
size_t wm_progress_sprite_count(void);

#endif
