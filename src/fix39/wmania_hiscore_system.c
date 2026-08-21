#include "wmania_hiscore_system.h"

#include "wmania_hiscore_special.h"

#include <string.h>

static void setup_pending(
    WmHsSystem *system,
    WmHsPendingEntry *pending,
    WmHsTableId id,
    uint8_t player,
    uint8_t wrestler,
    uint8_t typed_count,
    uint32_t score)
{
    memset(pending, 0, sizeof(*pending));
    pending->active = true;
    pending->requires_entry_ui =
        !wm_hs_system_has_cached_initials(system, player);
    pending->table_id = id;
    pending->player_index = player;
    pending->wrestler_index = wrestler;
    pending->typed_initial_count = typed_count;
    pending->score_bcd = score;
}

void wm_hs_system_rebind(WmHsSystem *system)
{
    wm_hs_bind(&system->tables[WM_HS_TABLE_STREAK],
               &wm_hs_streak_template, system->streak);
    wm_hs_bind(&system->tables[WM_HS_TABLE_PIN_SPEED],
               &wm_hs_pin_speed_template, system->pin_speed);
    wm_hs_bind(&system->tables[WM_HS_TABLE_BEATEN],
               &wm_hs_beaten_template, system->beaten);
    wm_hs_bind(&system->tables[WM_HS_TABLE_INTER],
               &wm_hs_inter_template, system->inter);
    wm_hs_bind(&system->tables[WM_HS_TABLE_TAG],
               &wm_hs_tag_template, system->tag);
}

void wm_hs_system_init(WmHsSystem *system, uint32_t adjusted_reset_value)
{
    unsigned i;

    memset(system, 0, sizeof(*system));
    system->adjusted_reset_value = adjusted_reset_value;
    wm_hs_system_rebind(system);

    for (i = 0; i < WM_HS_TABLE_COUNT; ++i) {
        wm_hs_init_table(&system->tables[i]);
    }

    wm_hs_counter_init(&system->reset_counter, adjusted_reset_value);
}

WmHsTable *wm_hs_system_table(WmHsSystem *system, WmHsTableId id)
{
    if (id < 0 || id >= WM_HS_TABLE_COUNT) {
        return 0;
    }
    return &system->tables[id];
}

const WmHsTable *wm_hs_system_table_const(const WmHsSystem *system, WmHsTableId id)
{
    if (id < 0 || id >= WM_HS_TABLE_COUNT) {
        return 0;
    }
    return &system->tables[id];
}

void wm_hs_system_clear_entered_initials(WmHsSystem *system)
{
    memset(system->entered_initials, 0, sizeof(system->entered_initials));
}

bool wm_hs_system_table_cmos_check(WmHsSystem *system)
{
    unsigned i;
    bool ok = true;
    bool any_reinitialized = false;

    for (i = 0u; i < WM_HS_TABLE_COUNT; ++i) {
        WmHsValidateResult result =
            wm_hs_validate_table(&system->tables[i]);

        if (result == WM_HS_VALIDATE_REINITIALIZED) {
            any_reinitialized = true;
        } else if (result == WM_HS_VALIDATE_STORAGE_FAILED) {
            ok = false;
        }
    }

    if (any_reinitialized) {
        /*
         * Exact INIT_HSTRING audit set from HSTD.ASM.
         * AUD_INTER is conspicuously not killed by the source routine.
         */
        system->recent_index[WM_HS_TABLE_STREAK] = 0u;
        system->recent_index[WM_HS_TABLE_PIN_SPEED] = 0u;
        system->recent_index[WM_HS_TABLE_BEATEN] = 0u;
        system->recent_index[WM_HS_TABLE_TAG] = 0u;
    }

    return ok;
}

uint32_t wm_hs_system_player_start_or_continue(WmHsSystem *system)
{
    return wm_hs_counter_decrement(
        &system->reset_counter, system->adjusted_reset_value);
}

uint32_t wm_hs_system_delay_reset(WmHsSystem *system)
{
    return wm_hs_counter_delay(
        &system->reset_counter, system->adjusted_reset_value);
}

bool wm_hs_system_has_cached_initials(
    const WmHsSystem *system,
    uint8_t human_player_index)
{
    if (human_player_index >= 2u) {
        return false;
    }
    return system->entered_initials[human_player_index][0] != 0u;
}

void wm_hs_system_cache_initials(
    WmHsSystem *system,
    uint8_t human_player_index,
    const uint8_t initials[WM_HS_NUM_INITIALS])
{
    if (human_player_index >= 2u || initials == 0) {
        return;
    }
    memcpy(system->entered_initials[human_player_index],
           initials, WM_HS_NUM_INITIALS);
}

void wm_hs_system_get_cached_initials(
    const WmHsSystem *system,
    uint8_t human_player_index,
    uint8_t out[WM_HS_NUM_INITIALS])
{
    if (out == 0) {
        return;
    }
    memset(out, 0, WM_HS_NUM_INITIALS);
    if (human_player_index < 2u) {
        memcpy(out, system->entered_initials[human_player_index],
               WM_HS_NUM_INITIALS);
    }
}

bool wm_hs_begin_winstreak(
    WmHsSystem *system,
    uint8_t human_player_index,
    uint8_t wrestler_index,
    uint32_t old_winstreak_binary,
    WmHsPendingEntry *pending)
{
    uint32_t bcd;
    uint16_t level;

    if (human_player_index >= 2u || pending == 0 ||
        !wm_hs_u32_to_bcd(old_winstreak_binary, &bcd)) {
        return false;
    }

    level = wm_hs_check_score_arcade(
        &system->tables[WM_HS_TABLE_STREAK], bcd);
    if (level == 0u) {
        memset(pending, 0, sizeof(*pending));
        return false;
    }

    setup_pending(system, pending, WM_HS_TABLE_STREAK,
                  human_player_index, wrestler_index, 3u, bcd);
    return true;
}

bool wm_hs_begin_pin_speed(
    WmHsSystem *system,
    uint8_t actor_index,
    uint8_t wrestler_index,
    uint8_t current_round,
    uint32_t match_timer_bcd,
    WmHsPendingEntry *pending)
{
    uint16_t level;

    if (pending == 0 || actor_index >= 2u ||
        current_round == 3u || match_timer_bcd == 0u ||
        !wm_hs_score_is_packed_bcd(match_timer_bcd)) {
        if (pending != 0) {
            memset(pending, 0, sizeof(*pending));
        }
        return false;
    }

    level = wm_hs_check_score_arcade(
        &system->tables[WM_HS_TABLE_PIN_SPEED], match_timer_bcd);
    if (level == 0u) {
        memset(pending, 0, sizeof(*pending));
        return false;
    }

    setup_pending(system, pending, WM_HS_TABLE_PIN_SPEED,
                  actor_index, wrestler_index, 3u, match_timer_bcd);
    return true;
}

bool wm_hs_begin_beaten_game(
    WmHsSystem *system,
    uint8_t human_player_index,
    uint8_t wrestler_index,
    bool world_belt,
    WmHsPendingEntry *pending)
{
    static const uint32_t which_to_or[8] = {
        0x00000001u, 0x00000010u, 0x00000100u, 0x00001000u,
        0x00010000u, 0x00100000u, 0x01000000u, 0x10000000u
    };
    WmHsTableId id;
    uint8_t index;

    if (pending == 0 || human_player_index >= 2u) {
        return false;
    }

    index = wrestler_index;
    if (index == 8u) {
        --index; /* AVOID_NUMBER_8 */
    }
    if (index >= 8u) {
        memset(pending, 0, sizeof(*pending));
        return false;
    }

    /*
     * DO_BEATEN_GAME clears this player's end-game initials before creating
     * the input process, so championship entries do not auto-reuse a stale
     * three-character streak/pin identity.
     */
    memset(system->entered_initials[human_player_index], 0,
           WM_HS_NUM_INITIALS);

    id = world_belt ? WM_HS_TABLE_BEATEN : WM_HS_TABLE_INTER;

    /*
     * do_entry_time runs CHECK_SCORE before entering SPECIAL_ENTRY.
     * Preserve that odd numeric low-table qualification even though the
     * eventual table is sorted by defeated-icon count.
     */
    if (wm_hs_check_score_arcade(&system->tables[id],
                                 which_to_or[index]) == 0u) {
        memset(pending, 0, sizeof(*pending));
        return false;
    }

    setup_pending(system, pending, id, human_player_index,
                  wrestler_index, 5u, which_to_or[index]);
    return true;
}

bool wm_hs_begin_tag_time(
    WmHsSystem *system,
    uint8_t human_player_index,
    uint32_t match_timer_bcd,
    WmHsPendingEntry *pending)
{
    if (pending == 0 || human_player_index >= 2u ||
        !wm_hs_score_is_packed_bcd(match_timer_bcd)) {
        return false;
    }

    memset(system->entered_initials[human_player_index], 0,
           WM_HS_NUM_INITIALS);

    /*
     * The source's outer process returns to insertion at physical slot 17
     * even when CHECK_SCORE fails, so tag entry remains active regardless
     * of ordinary qualification.
     */
    setup_pending(system, pending, WM_HS_TABLE_TAG,
                  human_player_index, 0u, 5u, match_timer_bcd);
    return true;
}

uint16_t wm_hs_commit_pending(
    WmHsSystem *system,
    const WmHsPendingEntry *pending,
    const uint8_t supplied_initials[WM_HS_NUM_INITIALS])
{
    uint8_t initials[WM_HS_NUM_INITIALS];
    WmHsTable *table;
    uint16_t index = 0u;

    if (pending == 0 || !pending->active) {
        return 0u;
    }

    if (supplied_initials != 0) {
        memcpy(initials, supplied_initials, sizeof(initials));
        wm_hs_system_cache_initials(
            system, pending->player_index, supplied_initials);
    } else if (wm_hs_system_has_cached_initials(
                   system, pending->player_index)) {
        wm_hs_system_get_cached_initials(
            system, pending->player_index, initials);
    } else {
        return 0u;
    }

    table = wm_hs_system_table(system, pending->table_id);
    if (table == 0) {
        return 0u;
    }

    switch (table->template_def->insert_mode) {
    case WM_HS_INSERT_NORMAL:
        index = wm_hs_add_entry_arcade(
            table, pending->score_bcd, initials);
        break;
    case WM_HS_INSERT_SPECIAL_BEATEN:
        index = wm_hs_add_special_mask_arcade(
            table, pending->score_bcd, initials);
        break;
    case WM_HS_INSERT_SPECIAL_TAG:
        index = wm_hs_add_tag_arcade(
            table, pending->score_bcd, initials);
        break;
    default:
        break;
    }

    if (index != 0u) {
        system->recent_index[pending->table_id] = index;
    }

    return index;
}
