# R35 remaining non-combat systems manifest

- status: PASS_WITH_REMAINING_GAPS_REPORTED
- scope: all remaining non-combat systems manifest + install/wire obvious completed pieces + fail/report remaining gaps
- base expected: fix39-v13e-dcs-r2b-decoded-port-assets
- ignored deltas: intentional Midway Sports / Be a Man / PlatynumX branding assets
- green: 3
- yellow: 4
- red: 0

## audio_dcs_runtime

- status: GREEN_WIRED

Evidence:
- decoded_stream_count=647
- decoded command stream bindings >=704
- filesystem/dcs has 647 WAV64 assets
- generated C command->DragonFS WAV64 binding table present
- wm_audio_send_command path records decoded DCS binding state
- command 0 kept as stop/reset control

## boot_attract_entry

- status: GREEN_WIRED

Evidence:
- wm_app_init starts ATTRACT mode
- 8-tick ATTRACT boot boundary
- startup SNDSND command 0
- DCS logo entry command 1005
- source attract cycle begin
- gameplay demo enters match path
- gameplay demo uses live combat tick

Ignored / intentional:
- intentional Midway Sports / Be a Man / PlatynumX replacement assets are ignored; timing/control flow is still audited

## frontend_select_continue

- status: GREEN_WIRED

Evidence:
- select core compiled
- continue select compiled
- P2 start bridge present
- title start bridge present
- Howard/select bonus state carried

Ignored / intentional:
- cabinet coin/PSTATUS accounting remains an explicit N64 bridge where comments say so

## pregame_progression_match_start

- status: YELLOW_BOUNDARY

Evidence:
- pregame core compiled
- match lifecycle compiled
- matchflow compiled
- story compiled
- select->pregame handoff present

Remaining gaps / boundaries:
- MATCH_INIT still stops at explicit start_match boundary

## hiscore_persistence

- status: YELLOW_BOUNDARY

Evidence:
- hiscore adapter compiled
- hiscore core compiled
- hiscore persistence compiled
- hiscore presentation compiled

Remaining gaps / boundaries:
- SD-card filesystem persistence path not proven by audit

Ignored / intentional:
- must not use SRAM/EEPROM/FlashRAM/Controller Pak for final persistent data

## rendering_presentation_adapter

- status: YELLOW_BOUNDARY

Evidence:
- visual core compiled
- composite compiled
- WIMP/source frame binding compiled
- ring geometry compiled
- ring onscreen compiled
- ring/crowd generated assets compiled

Remaining gaps / boundaries:
- platform renderer equivalence for palettes/z-order/transparency is not proven in this pass

## operator_service_cabinet_leftovers

- status: YELLOW_BOUNDARY

Evidence:
- operator attract screen compiled
- time/date attract screen compiled
- copyright/AAMA mapped in attract switch

Remaining gaps / boundaries:
- arcade bookkeeping/coin/PSTATUS remains N64-boundaried, not a native cabinet port

