#ifndef WMANIA_HISCORE_SPECIAL_H
#define WMANIA_HISCORE_SPECIAL_H

#include "wmania_hiscore_core.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TEST_NUM_ICON: one wrestler flag lives in bit 0 of each nibble. */
uint8_t wm_hs_defeated_icon_count(uint32_t score_mask);

/*
 * Source SPECIAL_ADD_ENTRY + SORT_BEATEN_TABLE.
 * If the initials already exist, their old mask is ORed with new_mask.
 * Otherwise the raw entry is first placed at the final table slot.
 * The resulting entry is then moved upward by defeated-icon count.
 * Returns the final table index, or 0 on validation/storage failure.
 */
uint16_t wm_hs_add_special_mask_arcade(
    WmHsTable *table,
    uint32_t new_mask,
    const uint8_t initials[WM_HS_NUM_INITIALS]);

/*
 * Source TAG_ENTRY.  A qualifying time is inserted normally; a
 * non-qualifying time is deliberately forced into slot 17 ("Jake hack").
 * Returns the physical insertion index.
 */
uint16_t wm_hs_add_tag_arcade(
    WmHsTable *table,
    uint32_t time_bcd,
    const uint8_t initials[WM_HS_NUM_INITIALS]);

#ifdef __cplusplus
}
#endif

#endif
