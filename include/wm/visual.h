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
    /*
     * ANIM.ASM's RPT_COUNT loop (_ani_set_rptcount:3530,
     * _ani_dec_rptcount:3552, _ani_if_rptcount:3257): a span of the
     * animation that really plays several times before the stream
     * continues past it. loop_count is the ANI_SET_RPTCOUNT value and the
     * number of times frames[loop_first..loop_last] plays in total;
     * loop_count == 0 means the sequence has no such loop, which is every
     * sequence extracted before this existed.
     *
     * Only the deterministic form is representable here. ANI_SET_RPTCOUNT
     * with a NEGATIVE operand means RNDRNG0(-operand) -- a count drawn at
     * runtime -- which no static table can carry; tools/wlanim.py refuses
     * to emit those rather than baking in an invented number.
     */
    size_t loop_first;
    size_t loop_last;
    uint16_t loop_count;
} wm_visual_sequence;

typedef struct {
    const wm_visual_sequence *sequence;
    size_t frame_index;
    uint16_t ticks_left;
    bool just_started;
    bool ended;
    /* Live RPT_COUNT, seeded from the sequence's loop_count. */
    uint16_t rpt_count;
} wm_visual_state;

void wm_visual_start(wm_visual_state *state, const wm_visual_sequence *sequence);
void wm_visual_tick(wm_visual_state *state);
const wm_visual_frame *wm_visual_current(const wm_visual_state *state);

#endif
