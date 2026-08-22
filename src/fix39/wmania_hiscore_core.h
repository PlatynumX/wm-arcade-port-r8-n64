#ifndef WMANIA_HISCORE_CORE_H
#define WMANIA_HISCORE_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WM_HS_NUM_INITIALS 5u

/*
 * Logical arcade high-score entry.
 *
 * The original CMOS layout spaces logical bytes according to the arcade
 * CMOS bus; the portable N64-side representation intentionally stores only
 * the ten logical bytes:
 *   4 packed-BCD score bytes
 *   5 initials bytes
 *   1 checksum byte
 */
typedef struct {
    uint8_t score_be[4];
    uint8_t initials[WM_HS_NUM_INITIALS];
    uint8_t checksum;
} WmHsEntry;

typedef struct {
    uint32_t score_bcd;
    char initials[WM_HS_NUM_INITIALS];
} WmHsFactoryEntry;

typedef enum {
    WM_HS_HIGHER_IS_BETTER = 0,
    WM_HS_LOWER_IS_BETTER = 1
} WmHsOrder;

typedef enum {
    WM_HS_INSERT_NORMAL = 0,
    WM_HS_INSERT_SPECIAL_BEATEN = 1,
    WM_HS_INSERT_SPECIAL_TAG = 2
} WmHsInsertMode;

typedef struct {
    const char *name;
    const WmHsFactoryEntry *factory;
    uint16_t last_entry;       /* entry 0 is the hidden filler */
    uint16_t visible_entries;  /* source TB_VISIBLE */
    uint16_t reset_threshold;  /* source TB_ERROR_COUNT */
    WmHsOrder order;            /* source which_level routine */
    WmHsInsertMode insert_mode; /* source addition path */
} WmHsTableTemplate;

typedef struct {
    const WmHsTableTemplate *template_def;
    WmHsEntry *entries;
} WmHsTable;

typedef enum {
    WM_HS_VALIDATE_OK = 0,
    WM_HS_VALIDATE_REINITIALIZED = 1,
    WM_HS_VALIDATE_STORAGE_FAILED = 2
} WmHsValidateResult;

uint32_t wm_hs_entry_score_bcd(const WmHsEntry *entry);
void wm_hs_entry_set_score_bcd(WmHsEntry *entry, uint32_t score_bcd);

uint8_t wm_hs_checksum_value(const WmHsEntry *entry);
void wm_hs_rechecksum(WmHsEntry *entry);
bool wm_hs_checksum_ok(const WmHsEntry *entry);

bool wm_hs_score_is_packed_bcd(uint32_t score_bcd);
bool wm_hs_initial_is_valid(uint8_t c);
bool wm_hs_entry_is_valid(const WmHsEntry *entry);

bool wm_hs_u32_to_bcd(uint32_t value, uint32_t *out_bcd);
bool wm_hs_bcd_to_u32(uint32_t bcd, uint32_t *out_value);

void wm_hs_bind(WmHsTable *table,
                const WmHsTableTemplate *template_def,
                WmHsEntry *storage);

void wm_hs_init_table(WmHsTable *table);
void wm_hs_remove_entry(WmHsTable *table, uint16_t entry_index);
WmHsValidateResult wm_hs_validate_table(WmHsTable *table);

/*
 * Returns the source-style insertion level, or 0 if no level exists.
 * Search begins at entry 1 because entry 0 is the hidden filler.
 */
uint16_t wm_hs_find_level(const WmHsTable *table, uint32_t score_bcd);

/*
 * Mirrors the executable CHECK_SCORE behavior: a found level qualifies
 * only when level < TB_VISIBLE.  This is intentionally strict rather than
 * "fixed" to <=, because the goal of this bundle is source fidelity.
 */
uint16_t wm_hs_check_score_arcade(WmHsTable *table, uint32_t score_bcd);

/*
 * Ports the normal arcade ADD_ENTRY path and returns the inserted level.
 * Returns 0 for special BEATEN/INTER/TAG tables because those source paths
 * require SPECIAL_ADD_ENTRY/TAG_ENTRY and are deliberately not fabricated
 * in Chunk 1. NUL initials are converted to spaces like normal ADD_ENTRY.
 */
uint16_t wm_hs_add_entry_arcade(WmHsTable *table,
                                uint32_t score_bcd,
                                const uint8_t initials[WM_HS_NUM_INITIALS]);

#ifdef __cplusplus
}
#endif

#endif
