#include "wm/select_continue.h"

#include <string.h>

/* SELECT.ASM setup_clock_sound: triple_sound 0Ah.
   This is the same decoded raw DCS binding already used by select_clock. */
#define WM_SELECT_CONTINUE_TICK_DCS 460u

static void restart_counter(wm_select_continue_state *state) {
    state->digit = WM_SELECT_CONTINUE_INITIAL_DIGIT;
    state->subcounter = WM_SELECT_CONTINUE_TICKS_PER_DIGIT;
}

static void enter_offer(wm_select_continue_state *state) {
    state->phase = WM_SELECT_CONTINUE_OFFER;
    state->resets_remaining = WM_SELECT_CONTINUE_RESETS_ALLOWED;
    state->blink_countdown = 30u; /* player_cursor #CNTR initialization */
    state->prompt_visible = true;
    restart_counter(state);
}

void wm_select_continue_init(wm_select_continue_state *state) {
    if (!state)
        return;
    memset(state, 0, sizeof(*state));
    state->phase = WM_SELECT_CONTINUE_IDLE;
}

void wm_select_continue_begin(wm_select_continue_state *state,
                              unsigned player,
                              uint8_t source_wrestler,
                              bool initials_active,
                              uint32_t credits,
                              uint8_t credits_needed,
                              bool free_play,
                              bool can_continue,
                              wm_award_state *awards) {
    if (!state || player > 1u)
        return;

    wm_select_continue_init(state);
    state->player = (uint8_t)player;
    state->source_wrestler = source_wrestler;
    state->initials_active = initials_active;
    state->credit_snapshot = credits;
    state->credits_needed = credits_needed;
    state->free_play = free_play;
    state->can_continue = can_continue;

    /* player_cursor calls winstreak_check before waiting on HI_INPUT_PID. */
    if (awards)
        wm_award_check_winstreak(awards, player);

    if (initials_active)
        state->phase = WM_SELECT_CONTINUE_WAIT_INITIALS;
    else
        enter_offer(state);
}

void wm_select_continue_set_initials_active(wm_select_continue_state *state,
                                            bool active) {
    if (!state)
        return;

    state->initials_active = active;
    if (state->phase == WM_SELECT_CONTINUE_WAIT_INITIALS && !active)
        enter_offer(state);
}

void wm_select_continue_set_can_continue(wm_select_continue_state *state,
                                         bool can_continue) {
    if (state)
        state->can_continue = can_continue;
}

static void tick_prompt_blink(wm_select_continue_state *state) {
    /* player_cursor #blink: initial CNTR 30, then reload 22 forever. */
    if (state->blink_countdown > 0u)
        --state->blink_countdown;
    if (state->blink_countdown == 0u) {
        state->blink_countdown = 22u;
        state->prompt_visible = !state->prompt_visible;
    }
}

static wm_select_continue_event digit_elapsed(
    wm_select_continue_state *state,
    wm_audio_state *audio,
    wm_award_state *awards) {
    if (state->digit > 0u)
        --state->digit;

    if (state->digit == 0u) {
        state->phase = WM_SELECT_CONTINUE_TIMEOUT;

        /* player_cursor timeout path: clear_icon_total for this player. */
        if (awards)
            wm_award_clear_icon_total(awards, state->player);

        return WM_SELECT_CONTINUE_TIMEOUT_EVENT;
    }

    state->subcounter = WM_SELECT_CONTINUE_TICKS_PER_DIGIT;

    /* setup_clock_sound is skipped when the decrement reaches timeout. */
    if (audio)
        (void)wm_audio_send_command(audio, WM_SELECT_CONTINUE_TICK_DCS);

    return WM_SELECT_CONTINUE_NO_EVENT;
}

wm_select_continue_event wm_select_continue_tick(
    wm_select_continue_state *state,
    bool player_active,
    uint32_t credits,
    bool either_start_down,
    bool player_button_down,
    bool player_button_current,
    wm_audio_state *audio,
    wm_award_state *awards) {
    if (!state)
        return WM_SELECT_CONTINUE_NO_EVENT;

    if (state->phase == WM_SELECT_CONTINUE_WAIT_INITIALS)
        return WM_SELECT_CONTINUE_NO_EVENT;

    if (state->phase != WM_SELECT_CONTINUE_OFFER)
        return state->phase == WM_SELECT_CONTINUE_ACCEPTED
            ? WM_SELECT_CONTINUE_ACCEPT_EVENT
            : state->phase == WM_SELECT_CONTINUE_TIMEOUT
                ? WM_SELECT_CONTINUE_TIMEOUT_EVENT
                : WM_SELECT_CONTINUE_NO_EVENT;

    tick_prompt_blink(state);

    /* #buyin: PSTATUS bit became active. */
    if (player_active) {
        state->phase = WM_SELECT_CONTINUE_ACCEPTED;
        return WM_SELECT_CONTINUE_ACCEPT_EVENT;
    }

    /* #coin_loop_reset: any credit-count change resets digit and reset quota. */
    if (credits != state->credit_snapshot) {
        state->credit_snapshot = credits;
        state->resets_remaining = WM_SELECT_CONTINUE_RESETS_ALLOWED;
        restart_counter(state);
        return WM_SELECT_CONTINUE_NO_EVENT;
    }

    /*
     * #start_hit: either player's Start can reset the offered continue timer,
     * but only 25 resets are accepted before Start is ignored.
     */
    if (state->resets_remaining > 0u && either_start_down) {
        --state->resets_remaining;
        restart_counter(state);
        return WM_SELECT_CONTINUE_NO_EVENT;
    }

    /* #new_button_press: immediately skip one displayed digit. */
    if (player_button_down)
        return digit_elapsed(state, audio, awards);

    /*
     * #old_button_press: SUBK 20 from the TSEC*2 subcounter.  If it crosses
     * zero, take the same #sec_elapsed path; otherwise keep counting.
     */
    if (player_button_current) {
        if (state->subcounter <= 20u)
            return digit_elapsed(state, audio, awards);
        state->subcounter = (uint16_t)(state->subcounter - 20u);
        return WM_SELECT_CONTINUE_NO_EVENT;
    }

    /* Normal DSJ a10,#loop path. */
    if (state->subcounter > 0u)
        --state->subcounter;
    if (state->subcounter == 0u)
        return digit_elapsed(state, audio, awards);

    return WM_SELECT_CONTINUE_NO_EVENT;
}

bool wm_select_continue_visible(const wm_select_continue_state *state) {
    return state && state->phase == WM_SELECT_CONTINUE_OFFER;
}
