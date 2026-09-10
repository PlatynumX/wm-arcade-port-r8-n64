#include "wm/award.h"

#include <string.h>

/* AWARD.ASM::award_icons, in GAME.EQU NUM_AWARDS order. */
static const uint8_t award_icons[WM_AWARD_COUNT] = {
    0, 0, 0, 2, 0, 0, 0, 0,
    2, 0, 2, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    5, 1, 0, 0, 0, 3, 0, 0
};

void wm_award_init(wm_award_state *state) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
}

uint8_t wm_award_icon_value(unsigned award_index) {
    return award_index < WM_AWARD_COUNT ? award_icons[award_index] : 0u;
}

void wm_award_reset_round(wm_award_state *state) {
    if (!state) return;
    /* rst_awards is `movi (NUM_AWARDS*2)-1,a0` with dsjs, so it clears
       SIXTY-THREE bytes starting at p1rnd_award -- straight through into
       the adjacent p2rnd_award and one short of its end. Player 2's
       award-31 round slot is never reset. Award 31 is unused (its icon
       value is 0 and GAME.EQU gives it no name), which is why nothing
       ever noticed; reproduced rather than tidied. */
    memset(state->round[0], 0, sizeof(state->round[0]));
    memset(state->round[1], 0, sizeof(state->round[1]) - 1u);
    /* rst_awards also zeroes both pcomeback longs. */
    state->comeback_armed[0] = false;
    state->comeback_armed[1] = false;
}

void wm_award_reset_match(wm_award_state *state) {
    if (!state) return;
    memset(state->match, 0, sizeof(state->match));
}

void wm_award_reset_winstreak(wm_award_state *state, unsigned player) {
    if (!state || player >= WM_AWARD_PLAYER_COUNT) return;
    memset(state->winstreak_awards[player], 0,
           sizeof(state->winstreak_awards[player]));
}

static void add_byte_award(uint8_t table[WM_AWARD_COUNT], unsigned award_index) {
    if (!table || award_index >= WM_AWARD_COUNT) return;
    /* round_award/match_award both store with MOVB, so overflow is byte wrap. */
    table[award_index] = (uint8_t)(table[award_index] + award_icons[award_index]);
}

void wm_award_round_award(wm_award_state *state, unsigned player, unsigned award_index) {
    if (!state || player >= WM_AWARD_PLAYER_COUNT) return;
    add_byte_award(state->round[player], award_index);
}

void wm_award_match_award(wm_award_state *state, unsigned player, unsigned award_index) {
    if (!state || player >= WM_AWARD_PLAYER_COUNT) return;
    add_byte_award(state->match[player], award_index);
}

void wm_award_accumulate_round(wm_award_state *state, bool royal_rumble,
                               const bool human_active[WM_AWARD_PLAYER_COUNT]) {
    if (!state) return;
    for (unsigned player = 0; player < WM_AWARD_PLAYER_COUNT; ++player) {
        /* accumulate_player_awards exits for Royal Rumble or a missing human proc. */
        if (!royal_rumble && (!human_active || human_active[player])) {
            /* BLOCKS_AWD is inverted here by the arcade source: no blocks earns 2. */
            state->round[player][WM_AWARD_BLOCKS] =
                state->round[player][WM_AWARD_BLOCKS] ? 0u : 2u;
            for (unsigned i = 0; i < WM_AWARD_ROUND_COUNT; ++i)
                state->match[player][i] =
                    (uint8_t)(state->match[player][i] + state->round[player][i]);
        }
    }
    /* accumulate_awards ends by clearing both players' per-round arrays. */
    wm_award_reset_round(state);
}

void wm_award_adjust_perfects_and_blocks(wm_award_state *state) {
    if (!state) return;
    /* AWARD.ASM::adjust_perfects checks P1 first and skips the P2 perfect check
       when P1 converted two PERFECT awards into DBL_PERFECT. Preserve that branch. */
    if (state->match[0][WM_AWARD_PERFECT] == 4u) {
        state->match[0][WM_AWARD_PERFECT] = 0u;
        state->match[0][WM_AWARD_DOUBLE_PERFECT] =
            (uint8_t)(state->match[0][WM_AWARD_DOUBLE_PERFECT] + 5u);
    } else if (state->match[1][WM_AWARD_PERFECT] == 4u) {
        state->match[1][WM_AWARD_PERFECT] = 0u;
        state->match[1][WM_AWARD_DOUBLE_PERFECT] =
            (uint8_t)(state->match[1][WM_AWARD_DOUBLE_PERFECT] + 5u);
    }
    for (unsigned player = 0; player < WM_AWARD_PLAYER_COUNT; ++player) {
        if (state->match[player][WM_AWARD_BLOCKS] >= 4u)
            state->match[player][WM_AWARD_BLOCKS] = 5u;
    }
}

void wm_award_total_icons(wm_award_state *state, unsigned player) {
    if (!state || player >= WM_AWARD_PLAYER_COUNT) return;
    uint32_t total = state->icon_total[player];
    for (unsigned i = 0; i < WM_AWARD_COUNT; ++i)
        total += state->match[player][i];
    state->icon_total[player] = total;
}

void wm_award_clear_icon_total(wm_award_state *state, unsigned player) {
    if (!state || player >= WM_AWARD_PLAYER_COUNT) return;
    state->icon_total[player] = 0u;
}

void wm_award_set_win_streak(wm_award_state *state, unsigned player, uint16_t value) {
    if (!state || player >= WM_AWARD_PLAYER_COUNT) return;
    state->win_streak[player] = value;
}

void wm_award_arm_winstreak(wm_award_state *state, unsigned player, uint16_t value) {
    if (!state || player >= WM_AWARD_PLAYER_COUNT || value == 0u) return;
    state->winstreak_armed[player] = (value % 5u) == 0u;
}

void wm_award_check_winstreak(wm_award_state *state, unsigned player) {
    if (!state || player >= WM_AWARD_PLAYER_COUNT || !state->winstreak_armed[player]) return;
    wm_award_match_award(state, player, WM_AWARD_FIVE_WINS);
    state->winstreak_armed[player] = false;
}

bool wm_award_prepare_select_bonus(wm_award_state *state, unsigned player) {
    if (!state || player >= WM_AWARD_PLAYER_COUNT) return false;
    /* SHOW_ACCUM_ICONS is 0 in the shipping source. show_bonus_icons resets a
       player's accumulated total whenever that player's current winstreak is 0. */
    if (state->win_streak[player] == 0u) {
        state->icon_total[player] = 0u;
        return false;
    }
    return state->icon_total[player] != 0u;
}

uint32_t wm_award_select_total(const wm_award_state *state, unsigned player) {
    if (!state || player >= WM_AWARD_PLAYER_COUNT || state->win_streak[player] == 0u)
        return 0u;
    return state->icon_total[player];
}

uint32_t wm_award_progress_total(const wm_award_state *state) {
    if (!state) return 0u;
    /* show_progress_bicons explicitly prefers P2 if P2 has any accumulated icons. */
    return state->icon_total[1] ? state->icon_total[1] : state->icon_total[0];
}

wm_bonus_icon_list wm_award_get_bonus_icons(uint32_t total) {
    wm_bonus_icon_list out = {{0}, 0u};
    /* AWARD.ASM::get_bonus_icons clamps the display representation at 100 by
       emitting one BICON_100A and no remainder. It does not clamp icon_total. */
    if (total >= 100u) {
        out.icon[out.count++] = WM_BONUS_ICON_100;
        return out;
    }
    uint32_t tens = total / 10u;
    uint32_t ones = total % 10u;
    if (tens)
        out.icon[out.count++] = (uint8_t)(tens * 10u);
    if (ones >= 5u) {
        out.icon[out.count++] = WM_BONUS_ICON_5;
        ones -= 5u;
    }
    while (ones && out.count < WM_BONUS_ICON_LIST_MAX) {
        out.icon[out.count++] = WM_BONUS_ICON_1;
        --ones;
    }
    return out;
}

const char *wm_award_bonus_icon_source_name(uint8_t denomination) {
    switch (denomination) {
        case WM_BONUS_ICON_1: return "BICON_1A";
        case WM_BONUS_ICON_5: return "BICON_5A";
        case WM_BONUS_ICON_10: return "BICON_10A";
        case WM_BONUS_ICON_20: return "BICON_20A";
        case WM_BONUS_ICON_30: return "BICON_30A";
        case WM_BONUS_ICON_40: return "BICON_40A";
        case WM_BONUS_ICON_50: return "BICON_50A";
        case WM_BONUS_ICON_60: return "BICON_60A";
        case WM_BONUS_ICON_70: return "BICON_70A";
        case WM_BONUS_ICON_80: return "BICON_80A";
        case WM_BONUS_ICON_90: return "BICON_90A";
        case WM_BONUS_ICON_100: return "BICON_100A";
        default: return NULL;
    }
}

/* ------------------------------------------------------------------ */
/* the award conditions (AWARD.ASM:994 onward)                         */

int wm_award_quick_win(wm_award_state *state, unsigned plyrnum,
                       unsigned player, uint32_t match_time) {
    /* andi 0fh / movk 10 / mpyu / srl 16 / add */
    const uint32_t score = ((match_time & 0xFu) * 10u) + (match_time >> 16);
    /* cmpi 69,a1 / jrgt #quick / jruc #done -- everything after the jruc
       is unreachable, so SUPER_QUICK and VERY_QUICK never happen. */
    if ((int32_t)score > 69) {
        if (plyrnum < 2u) wm_award_round_award(state, player, WM_AWARD_QUICK);
        return (int)WM_AWARD_QUICK;
    }
    return -1;
}

void wm_award_defeat_human(wm_award_state *state, unsigned plyrnum,
                           unsigned player, uint32_t pstatus) {
    if (player >= WM_AWARD_PLAYER_COUNT) return;
    /* Side 0 tests PSTATUS bit 1, side 1 tests bit 0 -- each asks
       whether the OTHER player is a started human. */
    if (player ? (pstatus & 0x1u) : (pstatus & 0x2u)) {
        if (plyrnum < 2u)
            wm_award_round_award(state, player, WM_AWARD_DEFEAT_HUMAN);
    }
}

void wm_award_arm_comeback(wm_award_state *state, unsigned plyrnum,
                           const bool live[WM_AWARD_NUM_WRES],
                           const unsigned side_of[WM_AWARD_NUM_WRES],
                           const int health[WM_AWARD_NUM_WRES]) {
    if (!state) return;
    /* cmpi 2,a10 / jrlt #ok_to_arm -- drones never arm. */
    if (plyrnum >= WM_AWARD_PLAYER_COUNT) return;
    for (unsigned i = 0; i < WM_AWARD_NUM_WRES; ++i) {
        if (!live || !live[i]) continue;                /* skip inactive */
        /* "humans only, so PLYRNUM == PLYR_SIDE" -- teammates skipped. */
        if (side_of && side_of[i] == plyrnum) continue;
        /* Abort if any enemy is under 80% strength. */
        if (health && health[i] < WM_AWARD_COMEBACK_HEALTH) return;
    }
    state->comeback_armed[plyrnum] = true;
}

void wm_award_check_comeback(wm_award_state *state, unsigned plyrnum,
                             unsigned player) {
    if (!state || player >= WM_AWARD_PLAYER_COUNT) return;
    if (!state->comeback_armed[player]) return;
    if (plyrnum < 2u) wm_award_round_award(state, player, WM_AWARD_COMEBACK);
}

bool wm_award_row_uses_big_icon(uint8_t award_value) {
    return award_value >= 5u;   /* cmpi 5,a11 / jrge #gna_big_icon */
}

unsigned wm_award_num_rows(const wm_award_state *state, unsigned player) {
    if (!state || player >= WM_AWARD_PLAYER_COUNT) return 0u;
    unsigned rows = 0u;
    for (unsigned i = 0; i < WM_AWARD_COUNT; ++i)
        if (state->match[player][i]) ++rows;
    return rows > WM_AWARD_MAX_ROWS ? WM_AWARD_MAX_ROWS : rows;
}
