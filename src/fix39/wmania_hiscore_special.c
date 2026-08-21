#include "wmania_hiscore_special.h"

#include <stdbool.h>
#include <string.h>

static uint8_t space_if_zero(uint8_t c)
{
    return c == 0u ? (uint8_t)' ' : c;
}

static bool initials_match_and_normalize(
    WmHsEntry *entry,
    uint8_t initials[WM_HS_NUM_INITIALS])
{
    unsigned i;

    for (i = 0; i < WM_HS_NUM_INITIALS; ++i) {
        uint8_t stored = space_if_zero(entry->initials[i]);
        uint8_t incoming = space_if_zero(initials[i]);

        /* COMPARE_INITS writes spaces back over incoming NUL bytes. */
        initials[i] = incoming;

        if (stored != incoming) {
            return false;
        }
    }

    return true;
}

static void write_entry(
    WmHsEntry *entry,
    uint32_t score_bcd,
    const uint8_t initials[WM_HS_NUM_INITIALS])
{
    unsigned i;

    wm_hs_entry_set_score_bcd(entry, score_bcd);
    for (i = 0; i < WM_HS_NUM_INITIALS; ++i) {
        entry->initials[i] =
            initials[i] == 0u ? (uint8_t)' ' : initials[i];
    }
    wm_hs_rechecksum(entry);
}

uint8_t wm_hs_defeated_icon_count(uint32_t score_mask)
{
    uint8_t count = 0u;
    unsigned nibble;

    /*
     * Exact TEST_NUM_ICON shape: test one low bit, then advance four bits.
     * Only the low bit of each of the eight nibbles participates.
     */
    for (nibble = 0; nibble < 8u; ++nibble) {
        if ((score_mask & 1u) != 0u) {
            ++count;
        }
        score_mask >>= 4u;
    }
    return count;
}

uint16_t wm_hs_add_special_mask_arcade(
    WmHsTable *table,
    uint32_t new_mask,
    const uint8_t input_initials[WM_HS_NUM_INITIALS])
{
    uint8_t initials[WM_HS_NUM_INITIALS];
    uint16_t source_index = 0u;
    uint16_t target_index = 0u;
    uint16_t i;
    uint16_t last;
    uint8_t new_count;
    WmHsEntry moving;

    if (table == NULL || table->template_def == NULL ||
        table->entries == NULL || input_initials == NULL) {
        return 0u;
    }

    if (table->template_def->insert_mode != WM_HS_INSERT_SPECIAL_BEATEN) {
        return 0u;
    }

    if (wm_hs_validate_table(table) == WM_HS_VALIDATE_STORAGE_FAILED) {
        return 0u;
    }

    memcpy(initials, input_initials, sizeof(initials));
    last = table->template_def->last_entry;

    /*
     * COMPARE_INITIALS scans 1..last-1 in the source.  Entry "last" is
     * the scratch/factory-bottom insertion point for a new identity.
     */
    for (i = 1u; i < last; ++i) {
        uint8_t probe[WM_HS_NUM_INITIALS];
        memcpy(probe, initials, sizeof(probe));
        if (initials_match_and_normalize(&table->entries[i], probe)) {
            source_index = i;
            memcpy(initials, probe, sizeof(initials));
            new_mask |= wm_hs_entry_score_bcd(&table->entries[i]);
            break;
        }
    }

    if (source_index == 0u) {
        source_index = last;
        for (i = 0; i < WM_HS_NUM_INITIALS; ++i) {
            initials[i] = space_if_zero(initials[i]);
        }
    }

    /*
     * SPECIAL_ADD_ENTRY writes directly into source_index, with no table
     * shift, before SORT_BEATEN_TABLE moves the completed entry upward.
     */
    write_entry(&table->entries[source_index], new_mask, initials);
    moving = table->entries[source_index];
    new_count = wm_hs_defeated_icon_count(new_mask);

    /*
     * SORT_BEATEN_TABLE scans from 1 and chooses the first entry whose
     * icon count is <= the new icon count (CMP A3,A5 / JRGE).
     * Equality therefore puts the moving entry before an equal-count row.
     */
    target_index = source_index;
    for (i = 1u; i <= source_index; ++i) {
        uint8_t existing_count =
            wm_hs_defeated_icon_count(wm_hs_entry_score_bcd(&table->entries[i]));
        if (new_count >= existing_count) {
            target_index = i;
            break;
        }
    }

    if (target_index < source_index) {
        for (i = source_index; i > target_index; --i) {
            table->entries[i] = table->entries[i - 1u];
        }
        table->entries[target_index] = moving;
    }

    return target_index;
}

uint16_t wm_hs_add_tag_arcade(
    WmHsTable *table,
    uint32_t time_bcd,
    const uint8_t initials[WM_HS_NUM_INITIALS])
{
    uint16_t target;
    uint16_t i;
    uint16_t last;

    if (table == NULL || table->template_def == NULL ||
        table->entries == NULL || initials == NULL) {
        return 0u;
    }

    if (table->template_def->insert_mode != WM_HS_INSERT_SPECIAL_TAG ||
        !wm_hs_score_is_packed_bcd(time_bcd)) {
        return 0u;
    }

    if (wm_hs_validate_table(table) == WM_HS_VALIDATE_STORAGE_FAILED) {
        return 0u;
    }

    target = wm_hs_check_score_arcade(table, time_bcd);

    /*
     * TAG_ENTRY's explicit fallback, reached by the outer "Jake hack".
     * The source hardcodes physical slot 17 in an 18-entry table.
     */
    if (target == 0u) {
        target = 17u;
    }

    last = table->template_def->last_entry;
    if (target > last) {
        return 0u;
    }

    for (i = last; i > target; --i) {
        table->entries[i] = table->entries[i - 1u];
    }

    write_entry(&table->entries[target], time_bcd, initials);
    return target;
}
