#include "wm/arcade/wmania_hiscore_entry.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

const uint8_t wm_hs_entry_grid_display[WM_HS_ENTRY_GRID_COUNT] = {
    'A','B','C','D','E','F','G','H','I','J',
    'K','L','M','N','O','P','Q','R','S','T',
    'U','V','W','X','Y','Z','!',0x10u,0x11u,0x12u
};

const uint8_t wm_hs_entry_grid_values[WM_HS_ENTRY_GRID_COUNT] = {
    'A','B','C','D','E','F','G','H','I','J',
    'K','L','M','N','O','P','Q','R','S','T',
    'U','V','W','X','Y','Z','!',0x20u,0x11u,0x12u
};

const WmHsEntryLayout wm_hs_entry_layout[2] = {
    {
        37, 59,   /* grid: source 32+crap_off, 59 */
        82, 204,  /* initials */
        81, 179,  /* ENTER INITIALS prompt */
        82, 38,   /* WIN STREAK / FAST VICTORY / BEATEN GAME title */
        50, 16,   /* WINS : */
        "BLUE", "FNT9WHT2P"
    },
    {
        278, 59,  /* grid: source 273+crap_off, 59 */
        320, 204,
        320, 179,
        322, 38,
        290, 16,
        "RUBYPAL", "FNT9RED_P"
    }
};

static const int8_t joytab[16] = {
     0, -5,  5,  0,
    -1, -6,  4, -1,
     1, -4,  6,  1,
     0, -5,  5,  0
};

static const uint8_t random_initials[7][4] = {
    {'M','J','T',0},
    {'J','Y','T',0},
    {'S','A','L',0},
    {'J','M','S',0},
    {'J','A','Z',0},
    {'R','J','R',0},
    {'M','J','L',0}
};

/* Comments alongside the source's encoded dw_table. */
static const char *const dirty_words[] = {
    "FUQ", "FUC", "FUK", "PHUQ", "PHUK", "PHUC",
    "PUSSY", "PENIS", "BITCH", "SLUT", "SHIT",
    "CUNT", "CLIT", "COCK", "SEGA"
};


static void terminate_preview(WmHsEntryState *state)
{
    unsigned limit = (unsigned)state->length;

    if (state->committed < limit) {
        state->initials[state->committed] = 0u;
    }
}

static void preview_cursor_character(WmHsEntryState *state)
{
    unsigned limit = (unsigned)state->length;
    uint8_t cell = state->cursor_index;

    if (state->committed >= limit ||
        cell == WM_HS_ENTRY_DELETE_CELL ||
        cell == WM_HS_ENTRY_END_CELL) {
        terminate_preview(state);
        return;
    }

    state->initials[state->committed] = wm_hs_entry_grid_values[cell];
    if ((unsigned)state->committed + 1u < WM_HS_NUM_INITIALS) {
        state->initials[state->committed + 1u] = 0u;
    }
}

bool wm_hs_entry_is_empty(const uint8_t initials[WM_HS_NUM_INITIALS])
{
    unsigned i;

    for (i = 0; i < WM_HS_NUM_INITIALS; ++i) {
        if (initials[i] != 0u && initials[i] != (uint8_t)' ') {
            return false;
        }
    }
    return true;
}

bool wm_hs_entry_is_dirty(const uint8_t initials[WM_HS_NUM_INITIALS])
{
    char normalized[WM_HS_NUM_INITIALS + 1u];
    size_t n = 0u;
    unsigned i;
    size_t w;

    /*
     * The assembly constructs a base-32 value while scanning from every
     * starting character. Spaces/'!' and control 0x19 do not contribute.
     * Comparing all accumulated prefixes is equivalent to searching the
     * normalized letter stream for each word listed in dw_table.
     */
    for (i = 0; i < WM_HS_NUM_INITIALS; ++i) {
        uint8_t c = initials[i];
        if (c == 0u) {
            break;
        }
        if (c == 0x20u || c == 0x21u || c == 0x19u) {
            continue;
        }
        if (n < WM_HS_NUM_INITIALS) {
            normalized[n++] = (char)toupper((int)c);
        }
    }
    normalized[n] = '\0';

    for (w = 0u; w < sizeof(dirty_words) / sizeof(dirty_words[0]); ++w) {
        if (strstr(normalized, dirty_words[w]) != NULL) {
            return true;
        }
    }

    return false;
}

static uint32_t finalize_entry(WmHsEntryState *state)
{
    uint32_t events = WM_HS_ENTRY_EVENT_SELECT | WM_HS_ENTRY_EVENT_FINISHED;
    uint32_t r;
    unsigned i;

    if (state->finalized) {
        return 0u;
    }

    /*
     * Do NOT erase the live preview here. The source only clears the
     * editable slot when cursor motion lands on DELETE/END. If the timer
     * expires while a normal letter is highlighted, that preview remains
     * part of the submitted initials.
     */
    if (wm_hs_entry_is_dirty(state->initials) ||
        wm_hs_entry_is_empty(state->initials)) {
        /*
         * HSTD.ASM:
         *   movk 6,a0
         *   calla RNDRNG0
         * The result is already exactly 0..6 inclusive.
         */
        r = state->random_range != NULL
            ? state->random_range(state->random_user, 6u)
            : 0u;

        /* A malformed adapter must not index past the source table. */
        if (r > 6u) {
            r = 6u;
        }

        memset(state->initials, 0, sizeof(state->initials));
        memcpy(state->initials, random_initials[r], 3u);
        state->replacement_used = true;
        events |= WM_HS_ENTRY_EVENT_REPLACED;
    }

    if (state->length == WM_HS_ENTRY_THREE_PLUS_WRESTLER) {
        /*
         * A three-initial entry stores wrestler number as 'A'+which_player
         * wrestler in byte four; the fifth byte is blank/zero.
         */
        state->initials[3] = (uint8_t)('A' + state->wrestler_index);
        state->initials[4] = 0u;
    }

    for (i = 0; i < 3u; ++i) {
        if (state->initials[i] == 0u) {
            state->initials[i] = (uint8_t)' ';
        }
    }

    if (state->initials[0] == 'T' &&
        state->initials[1] == 'J' &&
        state->initials[2] == 'M') {
        events |= WM_HS_ENTRY_EVENT_TJM_VOICE;
    }
    if (state->initials[0] == 'S' &&
        state->initials[1] == 'M' &&
        state->initials[2] == 'J') {
        events |= WM_HS_ENTRY_EVENT_SMJ_VOICE;
    }

    state->finished = true;
    state->finalized = true;
    return events;
}

void wm_hs_entry_begin(
    WmHsEntryState *state,
    WmHsEntryLength length,
    uint8_t player_index,
    uint8_t wrestler_index,
    WmHsRandomRangeFn random_range,
    void *random_user)
{
    memset(state, 0, sizeof(*state));
    state->length = length;
    state->player_index = player_index;
    state->wrestler_index = wrestler_index;
    state->cursor_index = 0u;
    state->timer_ticks = WM_HS_ENTRY_TIMER_TICKS;
    state->repeat_ticks = 0;
    state->debounce_ticks = 0;
    state->countdown_digit = -1;
    state->random_range = random_range;
    state->random_user = random_user;

    /* The source preloads 'A' at the first editable slot. */
    state->initials[0] = (uint8_t)'A';
}

static uint32_t move_cursor(WmHsEntryState *state, uint8_t stick)
{
    int pos;
    int delta = joytab[stick & 0x0fu];

    if (delta == 0) {
        return 0u;
    }

    pos = (int)state->cursor_index + delta;
    while (pos < 0) {
        pos += (int)WM_HS_ENTRY_GRID_COUNT;
    }
    while (pos >= (int)WM_HS_ENTRY_GRID_COUNT) {
        pos -= (int)WM_HS_ENTRY_GRID_COUNT;
    }

    state->cursor_index = (uint8_t)pos;
    preview_cursor_character(state);
    return WM_HS_ENTRY_EVENT_MOVE;
}

uint32_t wm_hs_entry_tick(
    WmHsEntryState *state,
    const WmHsEntryInput *input)
{
    uint32_t events = 0u;
    uint8_t limit;
    int8_t old_digit;

    if (state == NULL || input == NULL || state->finished) {
        return 0u;
    }

    old_digit = state->countdown_digit;

    /*
     * The assembly updates the countdown image from the current B5 value,
     * then DEC B5 and tests for timeout.
     */
    if (state->timer_ticks <= WM_HS_ENTRY_COUNTDOWN_START) {
        state->countdown_digit = (int8_t)(
            state->timer_ticks / WM_HS_ENTRY_COUNTDOWN_DIVISOR);
        if (state->countdown_digit != old_digit) {
            events |= WM_HS_ENTRY_EVENT_COUNTDOWN;
        }
    }

    if (state->timer_ticks > 0u) {
        --state->timer_ticks;
    }

    if (state->timer_ticks == 0u) {
        return events | finalize_entry(state);
    }

    limit = (uint8_t)state->length;

    if (input->accept_down) {
        uint8_t cell = state->cursor_index;

        if (cell == WM_HS_ENTRY_DELETE_CELL) {
            if (state->committed > 0u) {
                --state->committed;
                state->initials[state->committed] = 0u;
            }
        } else if (cell == WM_HS_ENTRY_END_CELL) {
            return events | finalize_entry(state);
        } else if (state->committed < limit) {
            state->initials[state->committed] =
                wm_hs_entry_grid_values[cell];
            ++state->committed;
            if (state->committed < WM_HS_NUM_INITIALS) {
                state->initials[state->committed] = 0u;
            }
            events |= WM_HS_ENTRY_EVENT_ADD;

            if (state->committed == limit) {
                state->cursor_index = WM_HS_ENTRY_END_CELL;
            }
        }
    }

    if (state->debounce_ticks > 0) {
        --state->debounce_ticks;
        return events;
    }

    if ((input->stick_down & 0x0fu) != 0u) {
        state->repeat_ticks =
            (int16_t)(WM_HS_ENTRY_REPEAT_STATIC - WM_HS_ENTRY_DEBOUNCE);
        state->debounce_ticks = (int16_t)WM_HS_ENTRY_DEBOUNCE;
        events |= move_cursor(state, input->stick_down);
    } else {
        if (state->repeat_ticks > 0) {
            --state->repeat_ticks;
        } else if ((input->stick_current & 0x0fu) != 0u) {
            state->repeat_ticks = (int16_t)WM_HS_ENTRY_REPEAT_MOVING;
            events |= move_cursor(state, input->stick_current);
        }
    }

    return events;
}

void wm_hs_entry_get_initials(
    const WmHsEntryState *state,
    uint8_t out[WM_HS_NUM_INITIALS])
{
    memcpy(out, state->initials, WM_HS_NUM_INITIALS);
}

uint8_t wm_hs_entry_cursor_col(const WmHsEntryState *state)
{
    return (uint8_t)(state->cursor_index % WM_HS_ENTRY_GRID_COLUMNS);
}

uint8_t wm_hs_entry_cursor_row(const WmHsEntryState *state)
{
    return (uint8_t)(state->cursor_index / WM_HS_ENTRY_GRID_COLUMNS);
}
