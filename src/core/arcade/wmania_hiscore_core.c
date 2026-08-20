#include "wm/arcade/wmania_hiscore_core.h"

#include <string.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(sizeof(WmHsEntry) == 10u, "WmHsEntry must remain 10 logical bytes");
#endif

static uint16_t wm_hs_last_entry(const WmHsTable *table)
{
    return table->template_def->last_entry;
}

uint32_t wm_hs_entry_score_bcd(const WmHsEntry *entry)
{
    return ((uint32_t)entry->score_be[0] << 24) |
           ((uint32_t)entry->score_be[1] << 16) |
           ((uint32_t)entry->score_be[2] << 8) |
           (uint32_t)entry->score_be[3];
}

void wm_hs_entry_set_score_bcd(WmHsEntry *entry, uint32_t score_bcd)
{
    entry->score_be[0] = (uint8_t)(score_bcd >> 24);
    entry->score_be[1] = (uint8_t)(score_bcd >> 16);
    entry->score_be[2] = (uint8_t)(score_bcd >> 8);
    entry->score_be[3] = (uint8_t)score_bcd;
}

uint8_t wm_hs_checksum_value(const WmHsEntry *entry)
{
    const uint8_t *bytes = (const uint8_t *)entry;
    uint8_t sum = 0;
    size_t i;

    for (i = 0; i < sizeof(WmHsEntry) - 1u; ++i) {
        sum = (uint8_t)(sum + bytes[i]);
    }

    return (uint8_t)~sum;
}

void wm_hs_rechecksum(WmHsEntry *entry)
{
    entry->checksum = wm_hs_checksum_value(entry);
}

bool wm_hs_checksum_ok(const WmHsEntry *entry)
{
    return entry->checksum == wm_hs_checksum_value(entry);
}

bool wm_hs_score_is_packed_bcd(uint32_t score_bcd)
{
    unsigned shift;

    for (shift = 0; shift < 32u; shift += 4u) {
        if (((score_bcd >> shift) & 0x0fu) > 9u) {
            return false;
        }
    }
    return true;
}

bool wm_hs_initial_is_valid(uint8_t c)
{
    if (c >= (uint8_t)'A' && c <= (uint8_t)'Z') {
        return true;
    }

    return c == (uint8_t)' ' ||
           c == (uint8_t)'.' ||
           c == (uint8_t)'!' ||
           c == (uint8_t)'%' ||
           c == (uint8_t)'?';
}

bool wm_hs_entry_is_valid(const WmHsEntry *entry)
{
    size_t i;

    if (!wm_hs_checksum_ok(entry)) {
        return false;
    }

    if (!wm_hs_score_is_packed_bcd(wm_hs_entry_score_bcd(entry))) {
        return false;
    }

    for (i = 0; i < WM_HS_NUM_INITIALS; ++i) {
        if (!wm_hs_initial_is_valid(entry->initials[i])) {
            return false;
        }
    }

    return true;
}

bool wm_hs_u32_to_bcd(uint32_t value, uint32_t *out_bcd)
{
    uint32_t bcd = 0;
    unsigned digit;

    if (out_bcd == NULL || value > 99999999u) {
        return false;
    }

    for (digit = 0; digit < 8u; ++digit) {
        bcd |= (value % 10u) << (digit * 4u);
        value /= 10u;
    }

    *out_bcd = bcd;
    return true;
}

bool wm_hs_bcd_to_u32(uint32_t bcd, uint32_t *out_value)
{
    uint32_t value = 0;
    unsigned digit;

    if (out_value == NULL || !wm_hs_score_is_packed_bcd(bcd)) {
        return false;
    }

    for (digit = 0; digit < 8u; ++digit) {
        value = value * 10u + ((bcd >> ((7u - digit) * 4u)) & 0x0fu);
    }

    *out_value = value;
    return true;
}

void wm_hs_bind(WmHsTable *table,
                const WmHsTableTemplate *template_def,
                WmHsEntry *storage)
{
    table->template_def = template_def;
    table->entries = storage;
}

void wm_hs_init_table(WmHsTable *table)
{
    uint16_t i;

    for (i = 0; i <= wm_hs_last_entry(table); ++i) {
        const WmHsFactoryEntry *src = &table->template_def->factory[i];
        WmHsEntry *dst = &table->entries[i];

        wm_hs_entry_set_score_bcd(dst, src->score_bcd);
        memcpy(dst->initials, src->initials, WM_HS_NUM_INITIALS);
        wm_hs_rechecksum(dst);
    }
}

void wm_hs_remove_entry(WmHsTable *table, uint16_t entry_index)
{
    uint16_t last = wm_hs_last_entry(table);
    uint16_t i;

    if (entry_index == 0u || entry_index > last) {
        return;
    }

    for (i = entry_index; i < last; ++i) {
        table->entries[i] = table->entries[i + 1u];
    }

    wm_hs_entry_set_score_bcd(
        &table->entries[last],
        table->template_def->factory[last].score_bcd);
    memcpy(table->entries[last].initials,
           table->template_def->factory[last].initials,
           WM_HS_NUM_INITIALS);
    wm_hs_rechecksum(&table->entries[last]);
}

static bool wm_hs_validate_pass(WmHsTable *table, uint16_t *out_error_count)
{
    uint16_t i = 1u;
    uint16_t errors = 0u;
    uint16_t last = wm_hs_last_entry(table);

    while (i <= last) {
        if (!wm_hs_entry_is_valid(&table->entries[i])) {
            wm_hs_remove_entry(table, i);
            ++errors;

            /*
             * Do not advance i: the next entry was bubbled into this slot
             * and must be checked, matching the source-table repair pass.
             */
            if (errors >= table->template_def->reset_threshold) {
                if (out_error_count != NULL) {
                    *out_error_count = errors;
                }
                return false;
            }
        } else {
            ++i;
        }
    }

    if (out_error_count != NULL) {
        *out_error_count = errors;
    }
    return true;
}

WmHsValidateResult wm_hs_validate_table(WmHsTable *table)
{
    uint16_t errors = 0u;

    if (wm_hs_validate_pass(table, &errors)) {
        return WM_HS_VALIDATE_OK;
    }

    wm_hs_init_table(table);

    /*
     * The arcade validates again after a threshold-triggered reinit.  In a
     * normal RAM implementation this should pass immediately; retaining the
     * second pass preserves the original storage-failure distinction.
     */
    if (!wm_hs_validate_pass(table, &errors)) {
        return WM_HS_VALIDATE_STORAGE_FAILED;
    }

    return WM_HS_VALIDATE_REINITIALIZED;
}

uint16_t wm_hs_find_level(const WmHsTable *table, uint32_t score_bcd)
{
    uint16_t i;
    uint16_t last = wm_hs_last_entry(table);

    if (!wm_hs_score_is_packed_bcd(score_bcd)) {
        return 0u;
    }

    for (i = 1u; i <= last; ++i) {
        uint32_t existing = wm_hs_entry_score_bcd(&table->entries[i]);

        if (table->template_def->order == WM_HS_HIGHER_IS_BETTER) {
            if (score_bcd >= existing) {
                return i;
            }
        } else {
            if (score_bcd < existing) {
                return i;
            }
        }
    }

    return 0u;
}

uint16_t wm_hs_check_score_arcade(WmHsTable *table, uint32_t score_bcd)
{
    uint16_t level;

    if (wm_hs_validate_table(table) == WM_HS_VALIDATE_STORAGE_FAILED) {
        return 0u;
    }

    level = wm_hs_find_level(table, score_bcd);

    /*
     * Preserve the executable CHECK_SCORE branch exactly: a level equal to
     * TB_VISIBLE falls through to failure after the disabled buddy logic.
     */
    if (level != 0u && level < table->template_def->visible_entries) {
        return level;
    }

    return 0u;
}

uint16_t wm_hs_add_entry_arcade(WmHsTable *table,
                                uint32_t score_bcd,
                                const uint8_t initials[WM_HS_NUM_INITIALS])
{
    uint16_t level;
    uint16_t i;
    uint16_t last;
    WmHsEntry *dst;

    if (initials == NULL || !wm_hs_score_is_packed_bcd(score_bcd)) {
        return 0u;
    }

    if (table->template_def->insert_mode != WM_HS_INSERT_NORMAL) {
        return 0u;
    }

    level = wm_hs_check_score_arcade(table, score_bcd);
    if (level == 0u) {
        return 0u;
    }

    last = wm_hs_last_entry(table);
    for (i = last; i > level; --i) {
        table->entries[i] = table->entries[i - 1u];
    }

    dst = &table->entries[level];
    wm_hs_entry_set_score_bcd(dst, score_bcd);

    for (i = 0; i < WM_HS_NUM_INITIALS; ++i) {
        uint8_t c = initials[i];
        dst->initials[i] = (c == 0u) ? (uint8_t)' ' : c;
    }

    /*
     * ADD_ENTRY does not run the full VERIFY_LETTER filter while writing;
     * it only substitutes NUL with space, then stores the checksum.
     */
    wm_hs_rechecksum(dst);
    return level;
}
