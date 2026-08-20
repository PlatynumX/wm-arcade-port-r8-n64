#include "wm/select_screen.h"

#include <string.h>

/* SELECT.ASM exact player-one raw DCS bindings recovered through DCSSOUND.ASM. */
#define WM_SEL_P1_MOVE_DCS       1460u
#define WM_SEL_RANDOM_MOVE_DCS    208u
#define WM_SEL_P1_CHOOSE_DCS      244u
#define WM_SEL_CLOCK_DCS          460u

#define WM_SEL_HOWARD_GOOD_EVENING 2560u
#define WM_SEL_HOWARD_MY_NAME      2564u
#define WM_SEL_HOWARD_WELCOME      2568u

#define WM_SEL_FINAL_WAIT_TICKS      30u
#define WM_SEL_NAME_WAIT_TICKS       20u
#define WM_SEL_MANUAL_DEBOUNCE        3u
#define WM_SEL_STICK_THRESHOLD        28
#define WM_SEL_TSEC                    53u
#define WM_SEL_SELECT_TIME_TICKS      (WM_SEL_TSEC * 15u) /* SELECT.ASM select_time */

static void send_dcs(wm_audio_state *audio, uint16_t command) {
    if (audio) (void)wm_audio_send_command(audio, command);
}

/*
 * RNDRNG0 is a shared Williams/Midway primitive referenced by SELECT.ASM but
 * its implementation is not present in the checked-in WWF source tree.
 * Keep that one missing primitive isolated here.  The random-select movement,
 * 5-tick cadence, 14-move wander, legal-direction fallback, and homing are the
 * already-translated SELECT.ASM routines in src/core/select.c.
 */
static uint32_t select_rng_next(wm_select_screen_state *state) {
    uint32_t x = state->rng_state ? state->rng_state : 0x57574653u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state->rng_state = x;
    return x;
}

static uint8_t rndrng0_bridge(wm_select_screen_state *state, uint8_t inclusive_max) {
    return (uint8_t)(select_rng_next(state) % ((uint32_t)inclusive_max + 1u));
}

static uint16_t wrestler_name_dcs(uint8_t source_id) {
    /* SELECT.ASM WHICH_SPEECH source wrestler ordering, rendered through the
       exact raw DCS tracks decoded from the WWF sound ROMs. */
    switch (source_id) {
        case 0: return 3664u; /* Bret Hart */
        case 1: return 3648u; /* Razor Ramon */
        case 2: return 3656u; /* Undertaker */
        case 3: return 3668u; /* Yokozuna */
        case 4: return 3644u; /* Shawn Michaels */
        case 5: return 3652u; /* Bam Bam Bigelow */
        case 6: return 3640u; /* Doink */
        case 8: return 3660u; /* Lex Luger */
        default: return 0u;
    }
}

void wm_select_screen_init(wm_select_screen_state *state) {
    if (!state) return;
    memset(state, 0, sizeof(*state));

    state->active = true;
    state->selected_source_wrestler = 0xffu;
    state->last_clock_digit = 4u; /* SELECT.ASM clock_digits initializes A11=4. */
    state->rng_state = 0x57574653u;
    state->buyin_blink_countdown = 30u; /* SELECT.ASM CNTR init */
    state->buyin_name_visible = true;

    wm_select_cursor_init(&state->p1, 0);
    /* SELECT.ASM select_clock: select_time equ TSEC*15.  Do not route this
       through the older generic clock shim: the display process reads the
       select_clock process PA9 directly and only becomes visible below 6. */
    state->select_ticks_remaining = WM_SEL_SELECT_TIME_TICKS;
    state->clock.ticks_remaining = WM_SEL_SELECT_TIME_TICKS;
    state->clock.time_out = false;
}


/*
 * SELECT.ASM #but_hit:
 *   no current stick/button -> player_pal_pref = 0
 *   PUNCH  -> 4
 *   SKICK  -> 7
 *   SPUNCH -> 5
 *   KICK   -> 6
 * The N64 attack arguments are edge events, but on the commit tick an edge
 * is also a current button, so this preserves the source decision.
 */
static uint8_t select_palette_preference(bool stick_active,
                                         bool light_punch_pressed,
                                         bool power_punch_pressed,
                                         bool light_kick_pressed,
                                         bool power_kick_pressed) {
    if (!stick_active)
        return 0u;
    if (light_punch_pressed) return 4u;
    if (power_kick_pressed)  return 7u;
    if (power_punch_pressed) return 5u;
    if (light_kick_pressed)  return 6u;
    return 0u;
}

static void choose_current(wm_select_screen_state *state,
                           wm_audio_state *audio,
                           wm_wrestler_id *p1_choice,
                           uint8_t palette_preference) {
    uint8_t source_id = 0xffu;
    if (!wm_select_choose(&state->p1, &source_id))
        return;

    state->selected_source_wrestler = source_id;
    state->player_pal_pref = palette_preference;
    state->name_pending = true;
    state->name_wait = WM_SEL_NAME_WAIT_TICKS;
    state->final_wait = WM_SEL_FINAL_WAIT_TICKS;

    if (p1_choice) {
        wm_wrestler_id roster;
        if (wm_select_source_to_roster(source_id, &roster))
            *p1_choice = roster;
    }

    /* p1info PI_SELSOUND = 0CBh -> exact raw DCS track 244. */
    send_dcs(audio, WM_SEL_P1_CHOOSE_DCS);
}

static void tick_clock(wm_select_screen_state *state, wm_audio_state *audio) {
    /* Direct port of SELECT.ASM select_clock.  In the one-player N64 path
       PSTATUS does not change while choosing, so the source reset-on-buyin
       branch has no additional state to service here. */
    if (state->clock.time_out)
        return;

    if (state->select_ticks_remaining > 0u)
        --state->select_ticks_remaining;

    state->clock.ticks_remaining = state->select_ticks_remaining;
    if (state->select_ticks_remaining == 0u) {
        state->clock.time_out = true;
        return;
    }

    /* clock_digits reads select_clock.PA9 >> 6.  It keeps the digit object
       OFF for values >= 6, then swaps FNT9_n and turns it ON. */
    uint8_t digit = (uint8_t)(state->select_ticks_remaining >> 6);
    if (digit < 6u && digit != state->last_clock_digit) {
        state->last_clock_digit = digit;
        send_dcs(audio, WM_SEL_CLOCK_DCS);
    }
}

void wm_select_screen_tick(wm_select_screen_state *state,
                           int stick_x, int stick_y,
                           bool start_pressed,
                           bool light_punch_pressed,
                           bool power_punch_pressed,
                           bool light_kick_pressed,
                           bool power_kick_pressed,
                           wm_audio_state *audio,
                           wm_wrestler_id *p1_choice) {
    if (!state || !state->active)
        return;

    /* select_screen stays blank for SLEEPK 1 before display_unblank. */
    if (state->setup_ticks == 0u) {
        state->setup_ticks = 1u;
        return;
    }

    if (!state->howard_queued) {
        /*
         * SELECT.ASM GOOD_EVENING queues 1FBh,1FCh,1FDh once on the first
         * selection screen.  The N64 DCS bank serializes these announcer tracks.
         */
        send_dcs(audio, WM_SEL_HOWARD_GOOD_EVENING);
        send_dcs(audio, WM_SEL_HOWARD_MY_NAME);
        send_dcs(audio, WM_SEL_HOWARD_WELCOME);
        state->howard_queued = true;
    }

    tick_clock(state, audio);

    /* SELECT.ASM #blink: CNTR starts at 30, then toggles the inactive
       name-band object every 22 ticks while waiting for a buy-in. */
    if (state->buyin_blink_countdown > 0u)
        --state->buyin_blink_countdown;
    if (state->buyin_blink_countdown == 0u) {
        state->buyin_blink_countdown = 22u;
        state->buyin_name_visible = !state->buyin_name_visible;
    }

    /*
     * SELECT.ASM #waitloop:
     * HIPLATE OZPOS alternates 2 <-> 3 and HILITE alternates 5 <-> 4
     * once per source tick.
     *
     * The same loop invokes external FLASHME every eight ticks. FLASHME is
     * not defined in the available WWF source drop, so Fix35 does not invent
     * a substitute implementation.
     */
    if (!state->p1.selected)
        state->cursor_z_flip = !state->cursor_z_flip;

    if (state->p1.selected) {
        if (state->name_pending && state->name_wait > 0u) {
            --state->name_wait;
            if (state->name_wait == 0u) {
                uint16_t cmd = wrestler_name_dcs(state->selected_source_wrestler);
                if (cmd) send_dcs(audio, cmd);
                state->name_pending = false;
            }
        }

        if (state->final_wait > 0u) {
            --state->final_wait;
            if (state->final_wait == 0u)
                state->finished = true;
        }
        return;
    }

    const bool up = stick_y > WM_SEL_STICK_THRESHOLD;
    const bool down = stick_y < -WM_SEL_STICK_THRESHOLD;
    const bool left = stick_x < -WM_SEL_STICK_THRESHOLD;
    const bool right = stick_x > WM_SEL_STICK_THRESHOLD;

    const bool palette_stick_active = up || down || left || right;
    const uint8_t palette_preference =
        select_palette_preference(palette_stick_active,
                                  light_punch_pressed,
                                  power_punch_pressed,
                                  light_kick_pressed,
                                  power_kick_pressed);

    if (state->clock.time_out) {
        choose_current(state, audio, p1_choice, palette_preference);
        return;
    }

    const bool up_down = up && !state->prev_up;
    const bool down_down = down && !state->prev_down;
    const bool left_down = left && !state->prev_left;
    const bool right_down = right && !state->prev_right;

    state->prev_up = up;
    state->prev_down = down;
    state->prev_left = left;
    state->prev_right = right;

    /* Random mode has its own five-source-tick cadence. */
    if (state->p1.random_dest >= 0) {
        if (state->p1.random_delay > 0u) {
            --state->p1.random_delay;
            return;
        }

        /* Source checks destination before executing the next random/homing move. */
        if (state->p1.random_wander == 0u &&
            state->p1.index == (uint8_t)state->p1.random_dest) {
            choose_current(state, audio, p1_choice, palette_preference);
            return;
        }

        uint8_t old_index = state->p1.index;
        (void)wm_select_random_event(
            &state->p1,
            rndrng0_bridge(state, 3u),
            rndrng0_bridge(state, 2u),
            rndrng0_bridge(state, 1u) != 0u);

        if (state->p1.index != old_index)
            send_dcs(audio, WM_SEL_RANDOM_MOVE_DCS);
        return;
    }

    /* SELECT.ASM: Start held + Up held + cursor on its start position. */
    if (start_pressed && up &&
        wm_select_random_can_begin(&state->p1, true, true)) {
        (void)wm_select_begin_random(&state->p1, rndrng0_bridge(state, 7u));
        return;
    }

    /* get_but_val_down: any of the four attack buttons commits the wrestler. */
    if (light_punch_pressed || power_punch_pressed ||
        light_kick_pressed || power_kick_pressed) {
        choose_current(state, audio, p1_choice, palette_preference);
        return;
    }

    if (state->manual_debounce > 0u) {
        --state->manual_debounce;
        return;
    }

    wm_select_direction dir = WM_SELECT_DIR_NONE;
    if (down_down) dir = WM_SELECT_DIR_DOWN;
    else if (up_down) dir = WM_SELECT_DIR_UP;
    else if (left_down) dir = WM_SELECT_DIR_LEFT;
    else if (right_down) dir = WM_SELECT_DIR_RIGHT;

    if (dir != WM_SELECT_DIR_NONE) {
        uint8_t old_index = state->p1.index;
        state->p1.index = wm_select_move(state->p1.index, dir);
        if (state->p1.index != old_index) {
            /* p1info PI_MOVESOUND = C8h -> exact raw DCS track 1460. */
            send_dcs(audio, WM_SEL_P1_MOVE_DCS);
            state->manual_debounce = WM_SEL_MANUAL_DEBOUNCE;
        }
    }
}

uint8_t wm_select_screen_current_source(const wm_select_screen_state *state) {
    if (!state) return 0xffu;
    if (state->p1.selected)
        return state->selected_source_wrestler;
    return wm_select_slot_to_source_wrestler(state->p1.index);
}

int wm_select_screen_clock_digit(const wm_select_screen_state *state) {
    if (!state || state->clock.time_out)
        return -1;
    uint8_t digit = (uint8_t)(state->select_ticks_remaining >> 6);
    return digit < 6u ? (int)digit : -1;
}

bool wm_select_screen_highlight_visible(const wm_select_screen_state *state) {
    if (!state) return false;
    if (!state->p1.selected) return true;

    /*
     * #flashloop toggles M_CONZER every SLEEPK 2 for TSEC/4 iterations.
     * final_wait begins at 30, so this reproduces that two-tick visible toggle
     * through the source's final selection hold without inventing a new effect.
     */
    if (state->final_wait == 0u) return true;
    return ((state->final_wait / 2u) & 1u) != 0u;
}

bool wm_select_screen_cursor_z_flipped(const wm_select_screen_state *state) {
    return state && !state->p1.selected && state->cursor_z_flip;
}

uint8_t wm_select_screen_player_palette_preference(const wm_select_screen_state *state) {
    return state ? state->player_pal_pref : 0u;
}
