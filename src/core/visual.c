#include "wm/visual.h"
#include <string.h>

void wm_visual_start(wm_visual_state *state, const wm_visual_sequence *sequence) {
    memset(state, 0, sizeof(*state));
    state->sequence = sequence;
    if (!sequence || !sequence->frames || sequence->frame_count == 0) {
        state->ended = true;
        return;
    }
    state->ticks_left = sequence->frames[0].ticks;
    if (state->ticks_left == 0)
        state->ticks_left = 1;
    /* The game loop advances simulation before rendering. Preserve the first
       source frame for its full duration instead of consuming one tick before
       it has ever been visible. */
    state->just_started = true;
}

const wm_visual_frame *wm_visual_current(const wm_visual_state *state) {
    if (!state || state->ended || !state->sequence ||
        state->frame_index >= state->sequence->frame_count)
        return NULL;
    return &state->sequence->frames[state->frame_index];
}

void wm_visual_tick(wm_visual_state *state) {
    if (!state || state->ended || !state->sequence || state->sequence->frame_count == 0)
        return;

    if (state->just_started) {
        state->just_started = false;
        return;
    }

    if (state->ticks_left > 1) {
        --state->ticks_left;
        return;
    }

    ++state->frame_index;
    if (state->frame_index >= state->sequence->frame_count) {
        if (!state->sequence->repeat) {
            state->frame_index = state->sequence->frame_count - 1;
            state->ticks_left = 0;
            state->ended = true;
            return;
        }
        state->frame_index = 0;
    }

    state->ticks_left = state->sequence->frames[state->frame_index].ticks;
    if (state->ticks_left == 0)
        state->ticks_left = 1;
}
