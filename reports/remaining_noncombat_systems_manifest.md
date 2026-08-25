# R35 remaining non-combat systems manifest

- status: PASS_WITH_REMAINING_GAPS_REPORTED
- scope: all remaining non-combat systems manifest + install/wire obvious completed pieces + fail/report remaining gaps
- base expected: fix39-v13e-dcs-r2b-decoded-port-assets
- ignored deltas: intentional Midway Sports / Be a Man / PlatynumX branding assets
- green: 7
- yellow: 0
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

- status: GREEN_WIRED

Evidence:
- pregame core compiled
- match lifecycle compiled
- matchflow compiled
- story compiled
- select->pregame handoff present
- MATCH_INIT enters live source match runtime using CURRENT_LADDER opponent mapping

## hiscore_persistence

- status: GREEN_WIRED

Evidence:
- hiscore adapter compiled
- hiscore core compiled
- hiscore persistence compiled
- hiscore presentation compiled
- SD-card filesystem persistence backend is bound and regression-proven

Ignored / intentional:
- must not use SRAM/EEPROM/FlashRAM/Controller Pak for final persistent data

## rendering_presentation_adapter

- status: GREEN_CODE_AUDITED_WITH_HARDWARE_CAPTURE_BOUNDARY

Evidence:
- visual core compiled
- composite compiled
- WIMP/source frame binding compiled
- ring geometry compiled
- ring onscreen compiled
- ring/crowd generated assets compiled
- renderer equivalence invariants compiled and audited

Ignored / intentional:
- pixel-perfect hardware screenshot comparison remains external validation

## operator_service_cabinet_leftovers

- status: GREEN_PLATFORM_ADAPTER

Evidence:
- operator attract screen compiled
- time/date attract screen compiled
- copyright/AAMA mapped in attract switch
- PSTATUS/player-start bookkeeping is centralized in explicit N64 cabinet adapter

Ignored / intentional:
- physical coin switch hardware is absent on N64; no fake coin credits are generated

