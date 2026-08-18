#include "wm/select.h"

uint8_t wm_select_move(uint8_t index, wm_select_direction direction) {
    if (index >= WM_SELECT_VISIBLE_SLOTS) return index;
    switch (direction) {
        case WM_SELECT_DIR_DOWN:
            return index < 6 ? (uint8_t)(index + 2) : index;
        case WM_SELECT_DIR_UP:
            return index >= 2 ? (uint8_t)(index - 2) : index;
        case WM_SELECT_DIR_LEFT:
            return (index & 1u) ? (uint8_t)(index - 1) : index;
        case WM_SELECT_DIR_RIGHT:
            return !(index & 1u) ? (uint8_t)(index + 1) : index;
        case WM_SELECT_DIR_NONE:
            return index;
    }
    return index;
}

uint8_t wm_select_random_move(uint8_t index, uint8_t source_direction_roll,
                              bool source_fallback_roll) {
    if (index >= WM_SELECT_VISIBLE_SLOTS) return index;

    /* Source RNDRNG0 result dispatch: 2=up, 1=down, otherwise sideways. */
    if (source_direction_roll == 2u) {
        if (index >= 2u) return wm_select_move(index, WM_SELECT_DIR_UP);
        /* Illegal up: source RNDRNG0(1): 0 sideways, nonzero down. */
        return source_fallback_roll
            ? wm_select_move(index, WM_SELECT_DIR_DOWN)
            : (uint8_t)(index ^ 1u);
    }
    if (source_direction_roll == 1u) {
        if (index <= 5u) return wm_select_move(index, WM_SELECT_DIR_DOWN);
        /* Illegal down: source RNDRNG0(1): 0 sideways, nonzero up. */
        return source_fallback_roll
            ? wm_select_move(index, WM_SELECT_DIR_UP)
            : (uint8_t)(index ^ 1u);
    }
    return (uint8_t)(index ^ 1u);
}

uint8_t wm_select_home_move(uint8_t index, uint8_t destination,
                            uint8_t source_one_in_three_roll,
                            uint8_t source_direction_roll,
                            bool source_fallback_roll) {
    if (index >= WM_SELECT_VISIBLE_SLOTS || destination >= WM_SELECT_VISIBLE_SLOTS)
        return index;
    if (index == destination) return index;

    /* Source: one time in three (RNDRNG0(3) returns zero), wander anyway. */
    if (source_one_in_three_roll == 0u)
        return wm_select_random_move(index, source_direction_roll, source_fallback_roll);

    /* Same row if ((dest XOR index) >> 1) == 0. */
    if (((destination ^ index) >> 1) == 0u)
        return (uint8_t)(index ^ 1u);

    return destination < index
        ? wm_select_move(index, WM_SELECT_DIR_UP)
        : wm_select_move(index, WM_SELECT_DIR_DOWN);
}

uint8_t wm_select_slot_to_source_wrestler(uint8_t slot) {
    if (slot >= WM_SELECT_VISIBLE_SLOTS) return 0xffu;
    return wm_select_slot_source_wrestlers[slot];
}

bool wm_select_source_to_roster(uint8_t source_id, wm_wrestler_id *out) {
    if (!out) return false;
    switch (source_id) {
        case WM_SOURCE_WRESTLER_BRET: *out = WM_WRESTLER_BRET; return true;
        case WM_SOURCE_WRESTLER_RAZOR: *out = WM_WRESTLER_RAZOR; return true;
        case WM_SOURCE_WRESTLER_UNDERTAKER: *out = WM_WRESTLER_UNDERTAKER; return true;
        case WM_SOURCE_WRESTLER_YOKOZUNA: *out = WM_WRESTLER_YOKOZUNA; return true;
        case WM_SOURCE_WRESTLER_SHAWN: *out = WM_WRESTLER_SHAWN; return true;
        case WM_SOURCE_WRESTLER_BAM_BAM: *out = WM_WRESTLER_BAM_BAM; return true;
        case WM_SOURCE_WRESTLER_DOINK: *out = WM_WRESTLER_DOINK; return true;
        case WM_SOURCE_WRESTLER_LEX: *out = WM_WRESTLER_LEX; return true;
        case WM_SOURCE_WRESTLER_ADAM_BOMB: return false;
    }
    return false;
}

void wm_select_cursor_init(wm_select_cursor *cursor, unsigned player) {
    if (!cursor) return;
    unsigned p = player ? 1u : 0u;
    cursor->index = wm_select_players[p].start_index;
    cursor->start_index = wm_select_players[p].start_index;
    cursor->random_dest = -1;
    cursor->random_delay = 0;
    cursor->random_wander = 0;
    cursor->selected = false;
    cursor->selected_source_wrestler = -1;
}

bool wm_select_random_can_begin(const wm_select_cursor *cursor,
                                bool start_held, bool up_held) {
    return cursor && !cursor->selected && cursor->random_dest < 0 &&
           start_held && up_held && cursor->index == cursor->start_index;
}

bool wm_select_begin_random(wm_select_cursor *cursor, uint8_t source_destination) {
    if (!cursor || cursor->selected || source_destination >= WM_SELECT_VISIBLE_SLOTS)
        return false;
    cursor->random_dest = (int8_t)source_destination;
    cursor->random_delay = WM_SELECT_RND_MOVE_SPEED;
    cursor->random_wander = WM_SELECT_RND_WANDER;
    return true;
}

bool wm_select_random_event(wm_select_cursor *cursor,
                            uint8_t source_one_in_three_roll,
                            uint8_t source_direction_roll,
                            bool source_fallback_roll) {
    if (!cursor || cursor->selected || cursor->random_dest < 0) return false;
    uint8_t dest = (uint8_t)cursor->random_dest;
    if (cursor->index == dest) return true;

    if (cursor->random_wander > 0) {
        --cursor->random_wander;
        cursor->index = wm_select_random_move(cursor->index,
                                              source_direction_roll,
                                              source_fallback_roll);
    } else {
        cursor->index = wm_select_home_move(cursor->index, dest,
                                            source_one_in_three_roll,
                                            source_direction_roll,
                                            source_fallback_roll);
    }
    cursor->random_delay = WM_SELECT_RND_MOVE_SPEED;
    return cursor->index == dest && cursor->random_wander == 0;
}

bool wm_select_choose(wm_select_cursor *cursor, uint8_t *source_wrestler_out) {
    if (!cursor || cursor->selected || cursor->index >= WM_SELECT_VISIBLE_SLOTS)
        return false;
    uint8_t id = wm_select_slot_to_source_wrestler(cursor->index);
    cursor->selected = true;
    cursor->selected_source_wrestler = (int8_t)id;
    cursor->random_dest = -1;
    if (source_wrestler_out) *source_wrestler_out = id;
    return true;
}


void wm_select_clock_init(wm_select_clock *clock, uint8_t pstatus) {
    if (!clock) return;
    clock->pstatus_snapshot = pstatus;
    clock->ticks_remaining = WM_SELECT_TIME_TICKS;
    clock->time_out = false;
}

void wm_select_clock_tick(wm_select_clock *clock, uint8_t pstatus, uint8_t old_pstatus) {
    if (!clock || clock->time_out) return;

    if (pstatus != clock->pstatus_snapshot) {
        wm_select_clock_init(clock, pstatus);
        return;
    }

    if (clock->ticks_remaining > 0)
        --clock->ticks_remaining;
    if (clock->ticks_remaining != 0) return;

    if (old_pstatus != 0) {
        wm_select_clock_init(clock, pstatus);
        return;
    }
    clock->time_out = true;
}
