#include "wm_arcade_completion.h"

#include <string.h>


static uint32_t completion_beaten_mask(uint8_t wrestler)
{
    uint8_t compact = wrestler;
    if (compact == 8u) compact = 7u;
    if (compact > 7u) return 0u;
    return 1u << ((uint32_t)compact * 4u);
}

static WmCompletionHsRequest completion_make_beaten(uint8_t side,
                                                     uint8_t wrestler,
                                                     bool belt_type)
{
    WmCompletionHsRequest r;
    memset(&r, 0, sizeof(r));
    r.enabled = side < 2u;
    r.player_side = side;
    r.table = belt_type ? WM_HS_TABLE_BEATEN : WM_HS_TABLE_INTER;
    r.rank_mode = WM_COMPLETION_HS_RANK_LOW;
    r.entry_mode = WM_COMPLETION_HS_ENTRY_SPECIAL_BEATEN;
    r.beaten_wrestler_mask = completion_beaten_mask(wrestler);
    r.source_score = r.beaten_wrestler_mask;
    if (r.beaten_wrestler_mask == 0u) r.enabled = false;
    return r;
}

static WmCompletionHsRequest completion_make_tag(uint8_t side,
                                                  uint32_t time_bcd)
{
    WmCompletionHsRequest r;
    memset(&r, 0, sizeof(r));
    r.enabled = side < 2u;
    r.player_side = side;
    r.table = WM_HS_TABLE_TAG;
    r.rank_mode = WM_COMPLETION_HS_RANK_LOW;
    r.entry_mode = WM_COMPLETION_HS_ENTRY_TIME;
    r.source_score = time_bcd;
    return r;
}

wm_postmatch_result wm_match_route_after_match(const wm_postmatch_input *in) {
    wm_postmatch_result r;
    uint8_t lost_humans;
    memset(&r, 0, sizeof(r));
    r.route = WM_POSTMATCH_NEXT_PREGAME;
    r.clear_done_howard = true;
    r.selection_delay_frames = 60u;
    if (!in) return r;

    r.next_pstatus = in->pstatus;

    if (in->royal_rumble) {
        /* WRESTLE creates the image updater, shows most damage, forces
         * OLD_PSTATUS=1, and clears all four current/old win-streak fields
         * before deciding whether humans won the rumble. */
        r.show_most_damage = true;
        r.old_pstatus = 1u;
        r.clear_all_rumble_winstreaks = true;
        if (in->rumble_human_won) {
            r.route = WM_POSTMATCH_RUMBLE_WIN;
            /* Source does not clear royal_rumble on this path here; it enters
             * INPARTY, fireworks, AUD_RRWINS, then GAME_BEATEN. */
            return r;
        }

        /* WRESTLE.ASM #rr_cpuwon: clear PSTATUS, then set OLD_PSTATUS=3 and
         * rr_loss=3 before JSRP buyin_select; clear rr_loss/royal_rumble only
         * after buyin_select returns. */
        r.next_pstatus = 0u;
        r.old_pstatus = 3u;
        r.clear_royal_rumble = true;
        r.route = in->anyone_bought_in_after_rumble ?
            WM_POSTMATCH_NEXT_PREGAME : WM_POSTMATCH_GAME_OVER;
        return r;
    }

    lost_humans = (uint8_t)(in->pstatus & (uint8_t)~in->match_winner);
    if (lost_humans == 0u) {
        r.run_pin_speed_check = true;
        r.route = (in->final_match && in->pstatus != 3u) ?
            WM_POSTMATCH_FINALE : WM_POSTMATCH_NEXT_PREGAME;
        return r;
    }

    r.run_loser_sound = true;
    r.save_old_pstatus = true;
    r.old_pstatus = in->pstatus;
    if ((in->pstatus & in->match_winner) == 0u) {
        /* CPU won: match_winner is explicitly cleared and CURRENT_LADDER is
         * backed up 0x20 because NEXT_IN_LADDER advances it later. */
        r.next_pstatus = 0u;
        r.clear_match_winner_before_buyin = true;
        r.decrement_ladder_before_buyin = true;
    } else {
        r.next_pstatus = in->match_winner;
    }

    /* Source checks is_final_match after replacing PSTATUS with match_winner. */
    if (in->final_match && r.next_pstatus != 3u) {
        r.route = WM_POSTMATCH_FINALE;
        return r;
    }
    r.route = WM_POSTMATCH_BUYIN;
    return r;
}

bool wm_match_pregame_enters_finale(uint8_t pstatus, bool final_match) {
    /* WRESTLE skips the final check for a two-human PSTATUS of 3. */
    return pstatus != 3u && final_match;
}

wm_two_player_pregame_result wm_select_two_player_pregame(
    uint8_t pstatus, bool royal_rumble, bool rr_award_enabled) {
    wm_two_player_pregame_result r;
    memset(&r, 0, sizeof(r));
    if (pstatus != 3u) return r;

    r.applicable = true;
    /* SELECT.ASM pregame_show: with RR_AWARD enabled, a two-human game that
     * follows Royal Rumble clears royal_rumble and skips ask_belt_question.
     * Otherwise it sets question_type=1 and asks the belt question. */
    if (rr_award_enabled && royal_rumble) {
        r.clear_royal_rumble = true;
        return r;
    }
    r.ask_belt_question = true;
    r.question_type = 1u;
    return r;
}

static void rumble_append(wm_rumble_plan *p, wm_rumble_step step) {
    if (p->step_count < WM_ATTR_MAX_PHASES) p->steps[p->step_count++] = step;
}

void wm_rumble_build_plan(bool human_won, bool anyone_bought_in,
                          wm_rumble_plan *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    rumble_append(out, WM_RUMBLE_START_IMAGE_UPDATER);
    rumble_append(out, WM_RUMBLE_SHOW_MOST_DAMAGE);
    rumble_append(out, WM_RUMBLE_SET_OLD_PSTATUS_ONE);
    rumble_append(out, WM_RUMBLE_CLEAR_ALL_WINSTREAKS);
    if (human_won) {
        rumble_append(out, WM_RUMBLE_SET_INPARTY);
        rumble_append(out, WM_RUMBLE_FIREWORKS);
        rumble_append(out, WM_RUMBLE_STOP_IMAGE_UPDATER);
        rumble_append(out, WM_RUMBLE_AUDIT_HUMAN_WIN);
        rumble_append(out, WM_RUMBLE_GAME_BEATEN);
        return;
    }
    rumble_append(out, WM_RUMBLE_SET_PSTATUS_ZERO);
    rumble_append(out, WM_RUMBLE_SET_OLD_PSTATUS_THREE);
    rumble_append(out, WM_RUMBLE_SET_RR_LOSS_THREE);
    rumble_append(out, WM_RUMBLE_BUYIN_SELECT);
    rumble_append(out, WM_RUMBLE_CLEAR_RR_LOSS_AND_MODE);
    rumble_append(out, anyone_bought_in ? WM_RUMBLE_NEXT_PREGAME :
                                         WM_RUMBLE_GAME_OVER);
}

static void finale_append(wm_finale_plan *p, wm_finale_step step) {
    if (p->step_count < WM_ATTR_MAX_PHASES) p->steps[p->step_count++] = step;
}

void wm_finale_build_plan(bool eight_on_one, wm_finale_plan *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    finale_append(out, WM_FINALE_SET_INPARTY);
    finale_append(out, WM_FINALE_AUDIT_CREDIT_LENGTH);
    finale_append(out, WM_FINALE_CLEAR_GAME_TIME_AND_STARTS);
    finale_append(out, WM_FINALE_START_IMAGE_UPDATER);
    finale_append(out, WM_FINALE_FIREWORKS);
    finale_append(out, WM_FINALE_STOP_IMAGE_UPDATER);
    if (eight_on_one) finale_append(out, WM_FINALE_WRESTLER_STORY);
    finale_append(out, WM_FINALE_GAME_BEATEN);
    finale_append(out, WM_FINALE_CREATE_TEXT_LINE);
    finale_append(out, WM_FINALE_TERMINAL_GAME_BEATEN);
}

uint8_t wm_match_music_command(bool royal_rumble, bool eight_on_one,
                               bool selector_bit) {
    if (royal_rumble || eight_on_one) return 16u;
    return selector_bit ? 15u : 25u;
}

bool wm_match_should_play_vince_intro(uint16_t total_matches, uint8_t pstatus) {
    return total_matches == 1u && pstatus == 3u;
}

bool wm_match_should_play_marble_voice(uint32_t rng_0_to_5) {
    return rng_0_to_5 == 0u;
}

static void game_beaten_append(wm_game_beaten_plan *p, wm_game_beaten_step step) {
    if (p->step_count < WM_GAME_BEATEN_MAX_STEPS)
        p->steps[p->step_count++] = step;
}

uint8_t wm_game_beaten_display_level(int32_t audit_value) {
    int32_t level = audit_value - 1;
    if (level < 1) level = 1;
    if (level > 48) level = 48;
    return (uint8_t)level;
}

uint32_t wm_game_beaten_tag_time(int32_t p1_time_bcd, int32_t p2_time_bcd) {
    /* SELECT.ASM uses P1 only if positive.  Otherwise it accepts any nonzero
     * P2 value, and falls back to 123.4 seconds (12340) if both are unusable. */
    if (p1_time_bcd > 0) return (uint32_t)p1_time_bcd;
    if (p2_time_bcd != 0) return (uint32_t)p2_time_bcd;
    return 12340u;
}

void wm_game_beaten_build_plan(const wm_game_beaten_input *in,
                               WmCompletionRngFn rng, void *rng_user,
                               wm_game_beaten_plan *out) {
    uint32_t random_pick = 0u;
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!in) return;

    if (rng) random_pick = rng(rng_user, 2u);
    random_pick %= 3u;
    out->fixed_sound_command = 998u;
    out->random_sound_command = (uint16_t)(0x0fu + random_pick);
    out->fade_speed = 5u;
    out->fade_frames = 32u;
    out->display_seconds = 7u;

    game_beaten_append(out, WM_GB_INIT_LADDER);
    game_beaten_append(out, WM_GB_SET_INPARTY);
    game_beaten_append(out, WM_GB_SOUND_998);
    game_beaten_append(out, WM_GB_RANDOM_VICTORY_SOUND);
    game_beaten_append(out, WM_GB_FADE_MASTER_5_32);
    game_beaten_append(out, WM_GB_WIPEOUT);
    game_beaten_append(out, WM_GB_PAGE_FLIP_ON);
    game_beaten_append(out, WM_GB_START_CREDIT_BOX);
    game_beaten_append(out, WM_GB_PLAYER_SELECT_BACKGROUND);
    game_beaten_append(out, WM_GB_CLEAR_TIMEOUT);
    game_beaten_append(out, WM_GB_DISPLAY_CROUTONS);

    if (!in->royal_rumble) {
        /* Source derives the scoring player from PSTATUS >> 1 and creates the
         * cursor on the opposite logical side value (P1 -> 1, P2 -> 0). */
        out->selected_player_side = (uint8_t)(in->pstatus >> 1);
        if (out->selected_player_side > 1u) out->selected_player_side = 1u;
        out->cursor_side = out->selected_player_side == 0u ? 1u : 0u;
        out->score_request_p1 = completion_make_beaten(
            out->selected_player_side, in->selected_wrestler_index,
            in->belt_type);
        out->display_level = wm_game_beaten_display_level(in->display_audit_value);

        game_beaten_append(out, WM_GB_CLEAR_PLAYER_INITIALS);
        game_beaten_append(out, WM_GB_START_PLAYER_CURSOR);
        game_beaten_append(out, WM_GB_DO_BEATEN_GAME);
        game_beaten_append(out, WM_GB_WAIT_FOR_INITIAL_INPUT);
        game_beaten_append(out, WM_GB_WIPEOUT_FOR_TABLE);
        game_beaten_append(out, WM_GB_HSTD_BACKGROUND);
        game_beaten_append(out, in->belt_type ? WM_GB_PRINT_BEATEN :
                                                WM_GB_PRINT_INTER);
        game_beaten_append(out, WM_GB_HOLD_SEVEN_SECONDS);
        return;
    }

    out->tag_match_time_bcd = wm_game_beaten_tag_time(in->match_timer_p1,
                                                       in->match_timer_p2);
    out->score_request_p1 = completion_make_tag(0u, out->tag_match_time_bcd);
    out->score_request_p2 = completion_make_tag(1u, out->tag_match_time_bcd);
    game_beaten_append(out, WM_GB_CLEAR_ALL_INITIALS);
    game_beaten_append(out, WM_GB_DO_TAG_GAME_P1);
    game_beaten_append(out, WM_GB_DO_TAG_GAME_P2);
    game_beaten_append(out, WM_GB_WAIT_FOR_INITIAL_INPUT);

    /* SELECT.ASM wraps the Royal-Rumble/tag score presentation in the
     * compile-time RR_AWARD option. The entries are still processed above;
     * RR_AWARD=1 with royal_rumble set returns before drawing the tag table.
     * The production macro value is not guessed in this portable layer. */
    if (in->rr_award_enabled) {
        out->suppress_tag_table_display = true;
        return;
    }

    game_beaten_append(out, WM_GB_WIPEOUT_FOR_TABLE);
    game_beaten_append(out, WM_GB_HSTD_BACKGROUND);
    game_beaten_append(out, WM_GB_PRINT_TAG);
    game_beaten_append(out, WM_GB_HOLD_SEVEN_SECONDS);
}

bool wm_pin_speed_prompt_context_allowed(uint16_t total_matches,
                                         uint8_t current_round,
                                         bool final_match) {
    /* SELECT pin_speed_in_case does not run before any match, after a
     * three-round match, or on the final match. */
    return total_matches != 0u && current_round != 3u && !final_match;
}

wm_pin_speed_after_input wm_pin_speed_after_input_action(uint8_t saved_pstatus,
                                                          uint8_t live_pstatus) {
    /* If buy-in changed PSTATUS while initials were being entered, SELECT
     * rebuilds the cursor/active bit, resets the select clock and re-enters
     * waitloop; unchanged status simply returns to the caller. */
    return saved_pstatus == live_pstatus ? WM_PIN_SPEED_INPUT_EXIT :
                                          WM_PIN_SPEED_INPUT_RESTART_SELECT_WAIT;
}
