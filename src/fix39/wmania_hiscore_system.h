#ifndef WMANIA_HISCORE_SYSTEM_H
#define WMANIA_HISCORE_SYSTEM_H

#include "wmania_hiscore_core.h"
#include "wmania_hiscore_counter.h"
#include "wmania_hiscore_factory.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WM_HS_TABLE_STREAK = 0,
    WM_HS_TABLE_PIN_SPEED = 1,
    WM_HS_TABLE_BEATEN = 2,
    WM_HS_TABLE_INTER = 3,
    WM_HS_TABLE_TAG = 4,
    WM_HS_TABLE_COUNT = 5
} WmHsTableId;

typedef struct {
    WmHsEntry streak[WM_HS_STREAK_LAST_ENTRY + 1u];
    WmHsEntry pin_speed[WM_HS_PIN_SPEED_LAST_ENTRY + 1u];
    WmHsEntry beaten[WM_HS_BEATEN_LAST_ENTRY + 1u];
    WmHsEntry inter[WM_HS_INTER_LAST_ENTRY + 1u];
    WmHsEntry tag[WM_HS_TAG_LAST_ENTRY + 1u];

    WmHsTable tables[WM_HS_TABLE_COUNT];
    uint16_t recent_index[WM_HS_TABLE_COUNT];

    WmHsResetCounter reset_counter;
    uint32_t adjusted_reset_value;

    /* Source entered_inits cache: one five-byte buffer per human player. */
    uint8_t entered_initials[2][WM_HS_NUM_INITIALS];
} WmHsSystem;

typedef struct {
    bool active;
    bool requires_entry_ui;
    WmHsTableId table_id;
    uint8_t player_index;
    uint8_t wrestler_index;
    uint8_t typed_initial_count;
    uint32_t score_bcd;
} WmHsPendingEntry;

void wm_hs_system_init(WmHsSystem *system, uint32_t adjusted_reset_value);
void wm_hs_system_rebind(WmHsSystem *system);

WmHsTable *wm_hs_system_table(WmHsSystem *system, WmHsTableId id);
const WmHsTable *wm_hs_system_table_const(const WmHsSystem *system, WmHsTableId id);

void wm_hs_system_clear_entered_initials(WmHsSystem *system);

/*
 * Source TABLE_CMOS_CHECK / VAL_TAB behavior.
 * Returns false only if a table still fails after source-style reinit.
 * If any VAL_TAB reinitializes a table, INIT_HSTRING clears the source
 * audit markers for streak, pin, beaten and tag (not INTER).
 */
bool wm_hs_system_table_cmos_check(WmHsSystem *system);

uint32_t wm_hs_system_player_start_or_continue(WmHsSystem *system);
uint32_t wm_hs_system_delay_reset(WmHsSystem *system);

/* Source result hooks. */
bool wm_hs_begin_winstreak(
    WmHsSystem *system,
    uint8_t human_player_index,
    uint8_t wrestler_index,
    uint32_t old_winstreak_binary,
    WmHsPendingEntry *pending);

bool wm_hs_begin_pin_speed(
    WmHsSystem *system,
    uint8_t actor_index,
    uint8_t wrestler_index,
    uint8_t current_round,
    uint32_t match_timer_bcd,
    WmHsPendingEntry *pending);

bool wm_hs_begin_beaten_game(
    WmHsSystem *system,
    uint8_t human_player_index,
    uint8_t wrestler_index,
    bool world_belt,
    WmHsPendingEntry *pending);

bool wm_hs_begin_tag_time(
    WmHsSystem *system,
    uint8_t human_player_index,
    uint32_t match_timer_bcd,
    WmHsPendingEntry *pending);

/*
 * Commit after initials entry (or immediately when the source's auto_init
 * cache already contains initials). Returns final physical table index.
 */
uint16_t wm_hs_commit_pending(
    WmHsSystem *system,
    const WmHsPendingEntry *pending,
    const uint8_t initials[WM_HS_NUM_INITIALS]);

/* Cache helpers preserving source auto_init behavior. */
bool wm_hs_system_has_cached_initials(
    const WmHsSystem *system,
    uint8_t human_player_index);
void wm_hs_system_cache_initials(
    WmHsSystem *system,
    uint8_t human_player_index,
    const uint8_t initials[WM_HS_NUM_INITIALS]);
void wm_hs_system_get_cached_initials(
    const WmHsSystem *system,
    uint8_t human_player_index,
    uint8_t out[WM_HS_NUM_INITIALS]);

#ifdef __cplusplus
}
#endif

#endif
