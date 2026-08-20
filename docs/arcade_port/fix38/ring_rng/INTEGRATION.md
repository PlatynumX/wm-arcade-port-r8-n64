# WrestleMania Arcade hi-score system — N64 merge map

This bundle is intended to be handed directly to the conversation doing the
main N64 merge.

## Files to compile

- `wmania_hiscore_core.c/.h`
- `wmania_hiscore_factory.c/.h`
- `wmania_hiscore_special.c/.h`
- `wmania_hiscore_counter.c/.h`
- `wmania_hiscore_entry.c/.h`
- `wmania_hiscore_system.c/.h`
- `wmania_hiscore_present.c/.h`
- `wmania_hiscore_persist.c/.h`
- `wmania_hiscore_adapter.c/.h`

`test_hiscore_complete.c` is host-side verification and should not ship in
the ROM unless wanted.

## Boot / warm-start

Create one `WmHsSystem`.

1. Try `wm_hs_save_read()`.
2. If there is no save, `wm_hs_system_init()` seeds all five tables from the
   arcade factory tables.
3. The source's current `INIT_TAB` body is a stub. Do not invent a separate
   daily/today table reset.

The arcade warm-start calls `table_cmos_check`. `wm_hs_save_read()` performs
that validation after a persisted load; if tables are kept in RAM without a
load, call:

```c
wm_hs_system_table_cmos_check(&hiscore);
```

This also preserves the source `INIT_HSTRING` recent-audit clearing behavior.

## Start / continue hook

At the same point a human player starts or continues:

```c
wm_hs_system_player_start_or_continue(&hiscore);
```

This ports `DEC_HSR`. The source adjustment value for the counter should be
fed into `adjusted_reset_value` by the main port's adjustment/operator-data
layer instead of hardcoding a guessed value.

## Result hooks

### Winning streak

At the arcade `winstreak_check` point:

```c
if (wm_hs_begin_winstreak(&hiscore, player, wrestler,
                          old_winstreak_binary, &pending)) {
    /* 3-character initials UI, then hidden wrestler byte */
}
```

The old streak is binary at this point and is converted to packed BCD before
qualification, matching the source.

### Fast pin

At `pin_speed_check`:

```c
if (wm_hs_begin_pin_speed(&hiscore, actor_index, wrestler,
                          current_round, match_timer_bcd, &pending)) {
    /* 3-character initials UI */
}
```

The helper preserves the two source exclusions:
- actor index >= 2 (drone)
- `current_round == 3`

It also rejects zero timer values.

### World / Intercontinental completion

At `DO_BEATEN_GAME`:

```c
wm_hs_begin_beaten_game(&hiscore, player, wrestler,
                        world_belt, &pending);
```

`world_belt=true` selects the World/BEATEN table; false selects INTER. The
source maps wrestler number 8 back to 7 before selecting its defeated bit.
The "score" is an 8-nibble mask, not a normal numeric score.

These use five typed initials and then:
`SPECIAL_ADD_ENTRY -> SORT_BEATEN_TABLE`.

### Tag completion

At `DO_TAG_GAME`:

```c
wm_hs_begin_tag_time(&hiscore, player, match_timer_bcd, &pending);
```

Five typed initials. Keep the source's Jake hack: even a time that fails the
normal visible-table check is still inserted at physical slot 17.

## Initials-entry UI

For a pending entry that requires UI:

```c
WmHsEntryState entry;
wm_hs_entry_begin(&entry,
    pending.typed_initial_count == 3
        ? WM_HS_ENTRY_THREE_PLUS_WRESTLER
        : WM_HS_ENTRY_FIVE,
    pending.player_index,
    pending.wrestler_index,
    adapter.random_range,
    adapter.user);
```

For the RNG callback, bind the shared translated RNG:

```c
adapter.random_range = wm_rng_rndrng0_callback;
entry_rng_user = &shared_rng;
```

Once per arcade logic tick, map the N64 controller to `WmHsEntryInput` and
call `wm_hs_entry_tick()`.

Important: `stick_current` and `stick_down` are a 4-bit RLDU mask:
bit0 Up, bit1 Down, bit2 Left, bit3 Right. Do not feed raw N64 button bits
directly.

When `WM_HS_ENTRY_EVENT_FINISHED` fires:

```c
uint8_t inits[5];
wm_hs_entry_get_initials(&entry, inits);
wm_hs_commit_pending(&hiscore, &pending, inits);
wm_hs_save_write(&hiscore, &adapter.save);
```

`DELAY_HSRESET` is included as `wm_hs_system_delay_reset()`, but the
translated HSTD entry routines do not themselves call it and the symbol is
not exported in the module's `.DEF` list. Do **not** automatically call it
from insertion unless the main merge finds the original external caller.

`wm_hs_entry_layout[0/1]` retains the exact source grid, initials, prompt,
mode-title and streak-label positions/palettes. The source background symbol
is `wwfselbkBMOD` at X offset -44.

The state machine ports:
- 5x6 / 30-cell grid
- A-Z, !, space, delete, end
- exact 16-entry diagonal movement table
- 30/10 tick repeat and 8 tick debounce
- 0x700 timeout
- countdown beginning at 750 and stepping every 150 ticks
- empty/dirty replacement from the seven source random initial sets
- source dirty-word list
- hidden wrestler byte for three-character tables
- TJM/SMJ voice easter-egg events

The original process waits another 30 ticks after selection before returning.
If the current frontend is process/state based, keep that delay in the
screen owner; it is intentionally not hidden inside the input-state object.
After that wait, when UI actually ran, call
`wm_hs_adapter_play_post_entry()` for the source `0xB8` sound before
continuing into insertion.

## auto_init / reused initials

The arcade caches entered initials and can skip a second initials screen for
the same player. `WmHsSystem.entered_initials` and its helper functions
preserve that behavior.

`DO_BEATEN_GAME` and `DO_TAG_GAME` explicitly clear the player's cached
initials before their five-character entry path; the begin helpers do this.

## Presentation / attract mode

Arcade order:

1. INTERCONTINENTAL CHAMPS
2. WORLD CHAMPIONS
3. TAG TEAM CHAMPIONS
4. FASTEST PINDOWN TIMES
5. LONGEST WINNING STREAKS

`wm_hs_present_sequence` contains this exact order and the source background
symbol `hstd_mod`.

Use `wm_hs_present_rows()` for renderer-neutral rows.

Special presentation rules:
- INTER/WORLD: three rows at once and scroll through the 30 physical ranks.
- TAG: nine displayed rows, each consuming two adjacent physical table
  entries. Both sets of five initials belong on the same displayed row.
- PIN: nine rows.
- STREAK: 18 rows, presented by the arcade in two columns of nine.
- STREAK/PIN hidden initial byte 4 is the wrestler identifier (`'A'+index`).
- BEATEN/INTER row masks can be expanded into wrestler icon bits using the
  supplied row fields.

The presentation descriptors also retain the arcade row bases/deltas and
title metadata. The packed assembly position comments are `[Y,X]`; the C
layout fields split those into explicit Y/X values. The common backdrop
symbols/positions (`MVEBAR_R`, `SHADOW01`) are exported as constants.

Source timing constants are exported symbolically:
- transition: 1/2 `TSEC`
- final per-screen hold: 5 `TSEC`
- scrolling group hold: 85 ticks
- scroll cadence values: 36 / 34 ticks
- final three-row off-scroll: 0x15 ticks

Do not guess an N64 `TSEC` value. Bind these to the frontend's translated
arcade logic clock.

## Rendering assets

This source bundle intentionally does not manufacture substitute graphics.
The arcade source references `hstd_mod` plus its own fonts/object images.
Wire the already-ported/extracted arcade assets in the main frontend merge.
If one of those assets is still missing, extract/translate it separately
rather than replacing it with an approximation.

## Persistence

`wmania_hiscore_persist` stores:
- the source reset-counter value and complement
- all five logical tables
- recent-entry highlight indices

Every table entry remains the source 10 logical bytes:
4 score bytes + 5 initials + checksum.

The wrapper adds a version/magic/CRC only so a truncated or wrong-version
N64 save cannot be misinterpreted as valid arcade data. The storage device
is deliberately abstract. The main port may bind its existing SRAM,
FlashRAM, Controller Pak, SD, or other save backend without changing
high-score logic.

## Source-fidelity traps not to "clean up"

- Entry 0 is a hidden filler.
- `CHECK_SCORE` is strict `level < TB_VISIBLE`, despite a stale comment
  saying <=.
- Low-time comparison uses strict `<`; ties land after an existing tie.
- World/IC masks use one bit in the low bit of each nibble.
- Special championship insertion ORs a returning player's old mask into the
  new mask when the same five initials match.
- Special sorting puts a newly moved equal-icon-count entry before the equal
  row it encounters.
- Tag has the slot-17 fallback.
- Three-character displays use byte 4 as wrestler identity.
- The source dirty-word filter includes `SEGA`.
