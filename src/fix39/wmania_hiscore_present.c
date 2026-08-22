#include "wmania_hiscore_present.h"

#include "wmania_hiscore_special.h"

#include <stdio.h>
#include <string.h>

const WmHsPresentDescriptor wm_hs_present_sequence[WM_HS_PRESENT_COUNT] = {
    {
        WM_HS_PRESENT_INTER, WM_HS_TABLE_INTER,
        "INTERCONTINENTAL CHAMPS", 3u, 30u, 1u, true, "hstd_mod",
        { 42, 8, 63, 10, 70, 0,0,0,0,
          200,10,10u,"osgemd_ascii","BLUE" }
    },
    {
        WM_HS_PRESENT_BEATEN, WM_HS_TABLE_BEATEN,
        "WORLD CHAMPIONS", 3u, 30u, 1u, true, "hstd_mod",
        { 42, 8, 63, 10, 70, 0,0,0,0,
          200,10,10u,"osgemd_ascii","BLUE" }
    },
    {
        WM_HS_PRESENT_TAG, WM_HS_TABLE_TAG,
        "TAG TEAM CHAMPIONS", 9u, 9u, 2u, false, "hstd_mod",
        { 40, 8, 38, 72, 23, 0,0,0,0,
          200,10,10u,"osgemd_ascii","BLUE" }
    },
    {
        WM_HS_PRESENT_PIN, WM_HS_TABLE_PIN_SPEED,
        "FASTEST PINDOWN TIMES", 9u, 9u, 1u, false, "hstd_mod",
        { 40, 8, 38, 72, 23, 0,0,0,0,
          200,10,10u,"osgemd_ascii","BLUE" }
    },
    {
        WM_HS_PRESENT_STREAK, WM_HS_TABLE_STREAK,
        "LONGEST WINNING STREAKS", 18u, 18u, 1u, false, "hstd_mod",
        { 40, 8, 38, 44, 23, 40,213,38,249,
          200,10,10u,"osgemd_ascii","BLUE" }
    }
};

static void copy_initials(
    const WmHsEntry *entry,
    uint8_t out[WM_HS_NUM_INITIALS + 1u])
{
    unsigned i;
    for (i = 0; i < WM_HS_NUM_INITIALS; ++i) {
        uint8_t c = entry->initials[i];
        out[i] = c == 0u ? (uint8_t)' ' : c;
    }
    out[WM_HS_NUM_INITIALS] = 0u;
}

static void fill_row(
    const WmHsSystem *system,
    WmHsTableId table_id,
    uint16_t rank,
    uint16_t physical,
    uint16_t partner,
    WmHsDisplayRow *row)
{
    const WmHsTable *table = wm_hs_system_table_const(system, table_id);
    const WmHsEntry *entry = &table->entries[physical];
    uint32_t binary = 0u;

    memset(row, 0, sizeof(*row));
    row->rank = rank;
    row->physical_index = physical;
    row->partner_physical_index = partner;
    row->score_bcd = wm_hs_entry_score_bcd(entry);
    (void)wm_hs_bcd_to_u32(row->score_bcd, &binary);
    row->score_binary = binary;
    copy_initials(entry, row->initials);
    row->highlighted = false;
    row->wrestler_index = -1;

    if (table_id == WM_HS_TABLE_STREAK ||
        table_id == WM_HS_TABLE_PIN_SPEED) {
        uint8_t hidden = entry->initials[3];
        if (hidden >= (uint8_t)'A') {
            row->wrestler_index = (int8_t)(hidden - (uint8_t)'A');
        }
    }

    if (table_id == WM_HS_TABLE_BEATEN ||
        table_id == WM_HS_TABLE_INTER) {
        uint32_t mask = row->score_bcd;
        unsigned n;
        row->defeated_count = wm_hs_defeated_icon_count(mask);
        for (n = 0u; n < 8u; ++n) {
            if (((mask >> (n * 4u)) & 1u) != 0u) {
                row->defeated_wrestler_bits |= (uint8_t)(1u << n);
            }
        }
    }

    if (partner != 0u && partner <= table->template_def->last_entry) {
        copy_initials(&table->entries[partner], row->partner_initials);
    }

    if (table_id == WM_HS_TABLE_TAG) {
        uint16_t recent = system->recent_index[WM_HS_TABLE_TAG];
        uint16_t row_from_recent =
            recent == 0u ? 0u : (uint16_t)((recent + 1u) >> 1u);
        row->highlighted = row_from_recent == rank;
    } else {
        row->highlighted =
            system->recent_index[table_id] == physical;
    }
}

size_t wm_hs_present_rows(
    const WmHsSystem *system,
    WmHsPresentScreen screen,
    uint16_t start_rank,
    WmHsDisplayRow *out_rows,
    size_t capacity)
{
    const WmHsPresentDescriptor *desc;
    size_t count = 0u;
    uint16_t rank;
    uint16_t physical;

    if (system == 0 || out_rows == 0 ||
        screen < 0 || screen >= WM_HS_PRESENT_COUNT) {
        return 0u;
    }

    desc = &wm_hs_present_sequence[screen];

    if (screen == WM_HS_PRESENT_INTER ||
        screen == WM_HS_PRESENT_BEATEN) {
        if (start_rank < 1u) {
            start_rank = 1u;
        }
        for (rank = start_rank;
             rank < start_rank + 3u &&
             rank <= desc->total_rows &&
             count < capacity;
             ++rank) {
            fill_row(system, desc->table_id, rank, rank, 0u,
                     &out_rows[count++]);
        }
        return count;
    }

    if (screen == WM_HS_PRESENT_TAG) {
        /*
         * print_tag advances HS_SIZE*2. Each displayed row consumes two
         * physical entries and prints both sets of five initials.
         */
        for (rank = 1u, physical = 1u;
             rank <= 9u && count < capacity;
             ++rank, physical += 2u) {
            fill_row(system, desc->table_id, rank, physical,
                     (uint16_t)(physical + 1u), &out_rows[count++]);
        }
        return count;
    }

    for (rank = 1u;
         rank <= desc->total_rows && count < capacity;
         ++rank) {
        fill_row(system, desc->table_id, rank, rank, 0u,
                 &out_rows[count++]);
    }

    return count;
}

bool wm_hs_format_time(
    uint32_t time_bcd,
    char *buffer,
    size_t buffer_size)
{
    uint32_t value;
    uint32_t tenths;
    uint32_t whole;

    if (buffer == 0 || buffer_size == 0u ||
        !wm_hs_bcd_to_u32(time_bcd, &value)) {
        return false;
    }

    /*
     * val_to_dec_tenths_asc:
     *  A0 /= 10; fractional digit = A0 % 10; whole = A0 / 10.
     * Thus the original binary timer has hundredths-like units and the
     * display truncates to tenths.
     */
    value /= 10u;
    tenths = value % 10u;
    whole = value / 10u;

    {
        int written = snprintf(buffer, buffer_size, "%u.%u",
                               (unsigned)whole, (unsigned)tenths);
        return written >= 0 && (size_t)written < buffer_size;
    }
}
