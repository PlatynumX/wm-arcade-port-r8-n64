#ifndef WM_AWARD_H
#define WM_AWARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* GAME.EQU */
#define WM_AWARD_COUNT 32u
#define WM_AWARD_ROUND_COUNT 24u
#define WM_AWARD_PLAYER_COUNT 2u

/* AWARD.ASM get_bonus_icons: worst case <100 is 90 + 5 + four 1s. */
#define WM_BONUS_ICON_LIST_MAX 6u

/* AWARD.ASM source coordinates, in the native 400x256 coordinate space. */
#define WM_BONUS_MSG_XPOS1 81
#define WM_BONUS_MSG_XPOS2 321
#define WM_BONUS_MSG_YPOS 198
#define WM_BONUS_ICON_XPOS1 WM_BONUS_MSG_XPOS1
#define WM_BONUS_ICON_XPOS2 WM_BONUS_MSG_XPOS2
#define WM_BONUS_ICON_YPOS 220
#define WM_PROGRESS_BONUS_ICON_XPOS 56
#define WM_PROGRESS_BONUS_ICON_YPOS 93

/* AWARD.ASM:80 redeclares LIFE_MAX next to the comeback threshold, and
 * arm_comeback_award compares against (LIFE_MAX*80)/100 -- 130. */
#define WM_AWARD_LIFE_MAX 163
#define WM_AWARD_COMEBACK_HEALTH ((WM_AWARD_LIFE_MAX * 80) / 100)
/* GAME.EQU:438 NUM_WRES -- the process_ptrs slots arm_comeback walks. */
#define WM_AWARD_NUM_WRES 7
/* get_num_awards caps the end-of-match bar at six rows. */
#define WM_AWARD_MAX_ROWS 6

typedef enum {
    WM_AWARD_POWER_MOVE = 0,
    WM_AWARD_REVERSAL = 1,
    WM_AWARD_HIGH_RISK = 2,
    WM_AWARD_BLOCKS = 3,
    WM_AWARD_COMBOS = 4,
    WM_AWARD_COMBO_REVERSAL = 5,
    WM_AWARD_COMBO_BREAKER = 6,
    WM_AWARD_ULTRA_COMBOS = 7,
    WM_AWARD_PERFECT = 8,
    WM_AWARD_FIRST_HIT = 9,
    WM_AWARD_COMEBACK = 10,
    WM_AWARD_SUPER_QUICK = 11,
    WM_AWARD_VERY_QUICK = 12,
    WM_AWARD_QUICK = 13,
    WM_AWARD_DEFEAT_HUMAN = 14,
    WM_AWARD_DOUBLE_PERFECT = 24,
    WM_AWARD_TWO_ROUND = 25,
    WM_AWARD_GAME_COMPLETE = 28,
    WM_AWARD_FIVE_WINS = 29
} wm_award_index;

typedef enum {
    WM_BONUS_ICON_1 = 1,
    WM_BONUS_ICON_5 = 5,
    WM_BONUS_ICON_10 = 10,
    WM_BONUS_ICON_20 = 20,
    WM_BONUS_ICON_30 = 30,
    WM_BONUS_ICON_40 = 40,
    WM_BONUS_ICON_50 = 50,
    WM_BONUS_ICON_60 = 60,
    WM_BONUS_ICON_70 = 70,
    WM_BONUS_ICON_80 = 80,
    WM_BONUS_ICON_90 = 90,
    WM_BONUS_ICON_100 = 100
} wm_bonus_icon;

typedef struct {
    uint8_t icon[WM_BONUS_ICON_LIST_MAX];
    size_t count;
} wm_bonus_icon_list;

typedef struct {
    /* AWARD.ASM p1/p2 rnd/mtch/ws award arrays, compacted from TMS bit spacing. */
    uint8_t round[WM_AWARD_PLAYER_COUNT][WM_AWARD_COUNT];
    uint8_t match[WM_AWARD_PLAYER_COUNT][WM_AWARD_COUNT];
    uint16_t winstreak_awards[WM_AWARD_PLAYER_COUNT][WM_AWARD_COUNT];
    uint32_t icon_total[WM_AWARD_PLAYER_COUNT];
    uint16_t win_streak[WM_AWARD_PLAYER_COUNT];
    bool winstreak_armed[WM_AWARD_PLAYER_COUNT];
    /* AWARD.ASM's pcomeback pair, armed by arm_comeback_award. */
    bool comeback_armed[WM_AWARD_PLAYER_COUNT];
} wm_award_state;

void wm_award_init(wm_award_state *state);
uint8_t wm_award_icon_value(unsigned award_index);
void wm_award_reset_round(wm_award_state *state);
void wm_award_reset_match(wm_award_state *state);
void wm_award_reset_winstreak(wm_award_state *state, unsigned player);
void wm_award_round_award(wm_award_state *state, unsigned player, unsigned award_index);
void wm_award_match_award(wm_award_state *state, unsigned player, unsigned award_index);
void wm_award_accumulate_round(wm_award_state *state, bool royal_rumble,
                               const bool human_active[WM_AWARD_PLAYER_COUNT]);
void wm_award_adjust_perfects_and_blocks(wm_award_state *state);
void wm_award_total_icons(wm_award_state *state, unsigned player);
void wm_award_clear_icon_total(wm_award_state *state, unsigned player);
void wm_award_set_win_streak(wm_award_state *state, unsigned player, uint16_t value);
void wm_award_arm_winstreak(wm_award_state *state, unsigned player, uint16_t value);
void wm_award_check_winstreak(wm_award_state *state, unsigned player);
bool wm_award_prepare_select_bonus(wm_award_state *state, unsigned player);
uint32_t wm_award_select_total(const wm_award_state *state, unsigned player);
uint32_t wm_award_progress_total(const wm_award_state *state);
wm_bonus_icon_list wm_award_get_bonus_icons(uint32_t total);
const char *wm_award_bonus_icon_source_name(uint8_t denomination);

/*
 * The award *conditions* -- the routines that decide whether an award
 * is earned at all. AWARD.ASM:994 onward.
 */

/*
 * is_it_a_really_quick_win (AWARD.ASM:994). The score is
 * `(match_time & 0xF) * 10 + (match_time >> 16)`.
 *
 * The source then does `cmpi 69,a1 / jrgt #quick / jruc #done`, and
 * that jruc is unconditional -- so the SUPER_QUICK and VERY_QUICK
 * comparisons written below it can never run. Both awards are worth
 * zero icons in the shipped build, so the game plays the same either
 * way, but the branches really are dead and are left dead here.
 *
 * Returns the award granted, or -1.
 */
int wm_award_quick_win(wm_award_state *state, unsigned plyrnum,
                       unsigned player, uint32_t match_time);

/*
 * give_award_if_opponent_is_human (AWARD.ASM:1032). PSTATUS bit 0 is
 * "player 1 is in", bit 1 is "player 2 is in", and each side tests the
 * OTHER one's bit.
 */
void wm_award_defeat_human(wm_award_state *state, unsigned plyrnum,
                           unsigned player, uint32_t pstatus);

/*
 * arm_comeback_award (AWARD.ASM:1059): arm the flag only if every live
 * enemy is at 80% health or better. `live`, `side_of` and `health`
 * describe the NUM_WRES process_ptrs slots.
 *
 * The source compares each wrestler's PLYR_SIDE against the arming
 * player's *index* and skips a match, which its own comment justifies:
 * "humans only, so PLYRNUM == PLYR_SIDE". A drone (plyrnum >= 2) never
 * arms at all.
 */
void wm_award_arm_comeback(wm_award_state *state, unsigned plyrnum,
                           const bool live[WM_AWARD_NUM_WRES],
                           const unsigned side_of[WM_AWARD_NUM_WRES],
                           const int health[WM_AWARD_NUM_WRES]);

/* check_for_award_for_big_comeback (AWARD.ASM:1096). */
void wm_award_check_comeback(wm_award_state *state, unsigned plyrnum,
                             unsigned player);

/* get_num_awards (AWARD.ASM:620): how many rows the end-of-match bar
 * shows, capped at WM_AWARD_MAX_ROWS. */
unsigned wm_award_num_rows(const wm_award_state *state, unsigned player);

/* get_num_awards also decides each row's icon size: the big one once
 * the award is worth five or more (`cmpi 5,a11 / jrge`). */
bool wm_award_row_uses_big_icon(uint8_t award_value);

#endif
