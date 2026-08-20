# Source map — original arcade symbols -> portable C

Primary source: Midway `HSTD.ASM` from the original WrestleMania source tree.

| Arcade symbol / path | Portable implementation | Notes |
|---|---|---|
| `TABLE_CMOS_CHECK` | `wm_hs_system_table_cmos_check` | Validates all five tables. |
| `VAL_TAB` / `VERIFY_LETTER` | `wm_hs_validate_table`, `wm_hs_entry_is_valid` | Repairs bad entries and can reinitialize a table. |
| `INIT_HSTRING` | inside `wm_hs_system_table_cmos_check` | Clears streak/pin/beaten/tag audits after reinit; source does **not** clear INTER audit here. |
| `INIT_TB` | `wm_hs_init_table` | Loads factory entries and recomputes checksums. |
| `INIT_TAB` | intentionally no implementation | Current source body is `RETS`; no fake daily-table reset added. |
| `CREATE_CHECKSUM` / checksum test | `wm_hs_checksum_value`, `wm_hs_rechecksum`, `wm_hs_checksum_ok` | 4 score + 5 initials + complement byte. |
| `VERIFY_LETTER` | `wm_hs_initial_is_valid` | A-Z, space, `.`, `!`, `%`, `?`. |
| `WHICH_LEVEL_HIGH` | `wm_hs_find_level` with `WM_HS_HIGHER_IS_BETTER` | Streak. |
| `WHICH_LEVEL_LOW` | `wm_hs_find_level` with `WM_HS_LOWER_IS_BETTER` | Pin, beaten/inter qualification, tag. |
| `CHECK_SCORE` | `wm_hs_check_score_arcade` | Preserves strict `level < TB_VISIBLE`. |
| `ADD_ENTRY` | `wm_hs_add_entry_arcade` | Normal streak/pin insertion. |
| `COMPARE_INITIALS` / `COMPARE_INITS` | `wm_hs_add_special_mask_arcade` internals | NUL compares as space. |
| `SPECIAL_ADD_ENTRY` | `wm_hs_add_special_mask_arcade` | ORs an existing player's beaten mask. |
| `TEST_NUM_ICON` | `wm_hs_defeated_icon_count` | Counts bit 0 of each nibble only. |
| `SORT_BEATEN_TABLE` | `wm_hs_add_special_mask_arcade` | Sorts championship masks by defeated-icon count. |
| `TAG_ENTRY` | `wm_hs_add_tag_arcade` | Includes source slot-17 fallback/Jake hack. |
| `GET_HSC` / `PUT_HSC` | `wm_hs_counter_get`, `wm_hs_counter_put` | Value + bitwise complement verifier. |
| `DEC_HSR` | `wm_hs_counter_decrement` | Hook at player start/continue. |
| `DELAY_HSRESET` | `wm_hs_counter_delay` | Exported but not auto-called; no caller was invented. |
| `DO_INITIAL_ENTRY` / cursor loop | `wm_hs_entry_*` | 30-cell grid, exact joy table, repeat/debounce/timer/countdown. |
| `CHECK_DIRTY_WORD` | `wm_hs_entry_is_dirty` | Includes source's `SEGA` entry. |
| `GET_RANDOM_INITIALS` | `finalize_entry` in `wmania_hiscore_entry.c` | Seven original three-letter replacement sets. |
| hidden wrestler initial | `WM_HS_ENTRY_THREE_PLUS_WRESTLER` | Byte 4 = `'A'+wrestler_index`. |
| `WINSTREAK_CHECK` | `wm_hs_begin_winstreak` | Binary streak -> packed BCD before qualification. |
| `PIN_SPEED_CHECK` | `wm_hs_begin_pin_speed` | Rejects drone, round 3 and zero timer. |
| `DO_BEATEN_GAME` | `wm_hs_begin_beaten_game` | World/IC defeated-wrestler bit mask. |
| `DO_TAG_GAME` | `wm_hs_begin_tag_time` | Five initials + source tag fallback behavior. |
| high-score display printers | `wm_hs_present_rows` | Renderer-neutral decoded rows. |
| `PRINT_TAG` | tag branch of `wm_hs_present_rows` | 9 displayed rows from 18 physical entries, two names per row. |
| attract high-score sequence | `wm_hs_present_sequence` | IC, World, Tag, Pin, Streak. |
| initials/high-score sounds | `wmania_hiscore_adapter.*` | Original IDs exposed to the port audio adapter. |

## Source fidelity traps preserved

- Entry 0 remains the hidden filler/sentinel.
- Scores remain packed BCD internally.
- `CHECK_SCORE` uses strict `< TB_VISIBLE`; it is not changed to `<=`.
- Low-time ties are placed after an existing equal value.
- Championship "scores" are one-bit-per-wrestler masks using the low bit of each nibble.
- Returning championship initials merge the old defeated mask via OR.
- Equal defeated-icon counts move the new/updated entry before the first equal row encountered.
- Tag's nonqualifying fallback inserts at physical slot 17.
- Three-letter streak/pin entries hide wrestler identity in initial byte 4.
- Tag presentation is 9 rows, not 18 independent rows.
- `INIT_TAB` is a stub in the current source, so no invented today-table reset exists.
- `DELAY_HSRESET` is present but is not silently wired to insertion without a verified caller.
- Table reinitialization invokes the source `INIT_HSTRING` audit clear set; its omission of INTER is preserved.

## N64-specific boundaries

The translated logic deliberately does **not** choose:
- SRAM vs FlashRAM vs Controller Pak vs SD persistence,
- N64 controller button mapping,
- font/texture substitutions,
- a guessed arcade `TSEC` rate,
- a replacement pixel-wipe implementation.

Those remain adapter/merge responsibilities so the original arcade behavior stays separable from platform glue.
