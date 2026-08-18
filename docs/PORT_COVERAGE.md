# Source-port coverage — r9

Original source routine/data first. Shared source-engine translation replaces one-off recreation; unresolved semantics fail closed.

## Attract/frontend entry points

| Original entry point | Status | Note |
|---|---|---|
| `show_hstd` | **not-started** | Original high-score display not translated. |
| `DCS_LOGO` | **partial-source** | Original object/timing path translated; original ADD_PIXEL_ROT/ADD_PIXEL_VEL plotter effects remain missing. |
| `show_sports_logo` | **partial-source** | Original 17 WIMP logo pieces, source timing and MOVE_BACK_OFF_SCREEN process scheduling translated; packed SPORTBKBMOD records are now ingested generically, artwork/palette block renderer still partial. |
| `show_gameplay` | **harness-only** | r8 demo combat is a hardware bring-up harness, not the original start_match process graph; excluded from normal arcade flow. |
| `creditscreen` | **not-started** | Original credits routine not translated. |
| `show_title` | **partial-source** | Original setup/button timing translated; cycle_lava now runs as a source PID on the shared scheduler. NTITLESC packed BMOD data is ingested generically; final per-block palette/art backend remains partial. |
| `DO_HINTS` | **not-started** | Original hints controller not translated. |
| `show_gen_tips` | **not-started** | Original general tips routine not translated. |
| `show_bios` | **not-started** | Original bios routine not translated. |
| `show_bios_tips` | **not-started** | Original bio tips routine not translated. |
| `show_operatormsg` | **not-started** | Original operator message routine not translated. |
| `show_time_date` | **not-started** | Original time/date routine not translated. |
| `show_copyright` | **not-started** | Original copyright routine not translated. |
| `aama_message` | **not-started** | Original AAMA routine not translated. |

## Shared systems

| System | Status | Note |
|---|---|---|
| `attract_mode_scheduler` | **partial-source** | Active source JSRP order, AMODE_LOOPS branch structure, and initial SLEEPK 8 blank are translated on the original 53 Hz source clock. |
| `wimp_ci8_renderer` | **partial-source** | CI8/TLUT pixels/hotspots translated for converted assets; general object/process semantics incomplete. |
| `two_channel_wrestler_composite` | **partial-source** | set_image attachment offset math translated for Bret bring-up. |
| `start_match` | **not-started** | Original start_match subtree is mechanically mapped by source IR but game/process/player dependencies are not yet translated; no harness substitution. |
| `credit_start_select` | **partial-source** | SELECT.ASM source tables are regenerated in CI; cursor grid/BMOD placement/scramble_table roster mapping/player start-mug-sound data/attributes, exact four-way cursor legality, START+UP random-select gate, random wander/home movement, source select_clock PSTATUS reset/OLD_PSTATUS behavior, 15-second timer and 30-tick final wait constant are translated. Credit/buy-in process wiring, original objects/text/mug art and full select process orchestration remain. |
| `combat` | **harness-only** | Portable demo AI/health/contact logic is test scaffolding and is not part of normal arcade execution. |
| `audio` | **not-started** | Original game-side DCS command semantics/backend not translated. |
| `background_artist_crop` | **partial-source** | BDB source block bounds + BGNDTBL BMOD dimensions recover exact full-screen source composites directly from original WIMP artist files; currently wired for NTITLESC. |
| `source_clock` | **exact-source** | DISPLAY.EQU TSEC=53 is executed independently of 60 Hz N64 presentation using an accumulator; literal 60-tick source sleeps remain literal. |
| `source_process_scheduler` | **partial-source** | Cooperative CREATE/sleep/kill-by-PID execution is shared by translated attract processes; full MPROC semantics and process pdata remain to port. |
| `bmod_packed_records` | **partial-source** | BAKGND.ASM 64-bit block records decode exactly and CI extracts NTITLESCBMOD/SPORTBKBMOD verbatim from BGNDTBL.ASM; header art/palette object construction remains incomplete. |
| `typed_animation_stream` | **partial-source** | Mixed WORD/LONG source stream supports frame refs, REPEAT, SETMODE, velocity LONG operands/modes, PAUSE, SETSPEED, zero velocities, facing/xflip and END. Unknown commands stop explicitly. |
| `source_dependency_ir` | **exact-source** | CI scans every SUBR/SUBRP and static CALL/JSRP/CREATE edge and emits dependency frontiers for attract_mode and start_match without inventing unresolved dynamic semantics. |
| `animation_source_ir` | **exact-source** | CI scans typed WORD/LONG animation data and W/L packing macros across the original ASM tree, preserving source expressions and unresolved forms without inventing pointer or command semantics. |

`harness-only` is never eligible for normal arcade execution.
