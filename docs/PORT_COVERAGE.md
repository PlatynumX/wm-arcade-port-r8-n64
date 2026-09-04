# Source-port coverage — r9

Original source routine/data first. Shared source-engine translation replaces one-off recreation; unresolved semantics fail closed.

## Attract/frontend entry points

| Original entry point | Status | Note |
|---|---|---|
| `show_hstd` | **not-started** | Original high-score display not translated. |
| `DCS_LOGO` | **partial-source** | Original object/timing path translated; original ADD_PIXEL_ROT/ADD_PIXEL_VEL plotter effects remain missing. |
| `show_sports_logo` | **partial-source** | Original 17 WIMP logo pieces, source timing and MOVE_BACK_OFF_SCREEN process scheduling translated; packed SPORTBKBMOD records are now ingested generically, artwork/palette block renderer still partial. |
| `show_gameplay` | **partial-source** | WRESTLE.ASM::start_match's PSTATUS==0 (#0plyr) path now runs from the real attract flow: the RNDRNG0(7)-skip-7 index1 draw, LIFEBAR.ASM::init_life_data (LIFE_MAX=163), and #0plyr's PLYRNUM/PSIDE wrestler-actor creation are translated (wm/match.h), and the SLEEP 3*60 + wait_on_butn 10*TSEC timing is exact. Whichever actor draws WM_ROSTER_BRET runs the real wm_arcade_move_bret control layer every tick with a real visual backend (wm/bret_backend.h): its idle stance (WM_BRET_ANIM_STAND2/4, TORSO2/4) and 6 of its light/power punch/kick animations resolve to actual wm_visual_sequence data; every other BRET.ASM animation id, and every other wrestler, still resolves to nothing. Not translated: INIT_LADDER_TABLE/CURRENT_LADDER/NUM_OPPS multi-drone team selection (only one placeholder opponent is created), DRONE.ASM's wnshort_t/wnmed_t/wnlong_t script data and the blkbase_t/blkatk_t/sklhhdly_t/sklhrdly_t skill tables (their SKLM macro expansion is not present in the checked-in source tree, so they are not guessed -- the drone AI therefore never actually gives Bret a button/stick to act on), and WRESTLE.ASM::execute_walk (movement velocity and idle-animation reselection). Wrestlers hold real health/ring state and Bret can visibly stand in the ring correctly animated, but nothing moves, walks, or fights yet. |
| `creditscreen` | **not-started** | Original credits routine not translated. |
| `show_title` | **partial-source** | Original setup/button timing translated; cycle_lava runs as a source PID on the shared scheduler. NTITLESC executes the original BIGWWF.BDD CI8 blocks and RGB555 palettes through the verbatim NTITLESCBMOD placement/depth/transparent records. The original 69-frame SPARKLE.IMG WIMP artwork is generated and the source FLASH_PID/ATTRACT_ANIMPID create/kill lifetime is translated. The checked-in WWF tree only references external SPRINKLE_GLINTS, RANDOM_SPARKLE and WHERE_WRESTLMANIA_SPARKLES; current sparkle placement/cadence is explicitly provisional until that shared helper/table source is recovered. Lava palette substitution remains. |
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
| `start_match` | **partial-source** | Only the PSTATUS==0 (#0plyr, attract-mode) creation path is translated (wm/match.h), driven from WM_ATTRACT_SHOW_GAMEPLAY -- see the show_gameplay note above for exactly what is and is not real, including the Bret-only visual backend. The #1plyr/#2plyr credited-match paths (WM_APP_MODE_MATCH_INIT after select/pregame) remain an explicit dead-end stub; CLOSE_PROGRESS_SCREEN, ring/rope/crowd/timer process creation, and collision/pin/finisher lifecycle are all still unported. |
| `credit_start_select` | **partial-source** | SELECT.ASM source tables are regenerated in CI; cursor grid/BMOD placement/scramble_table roster mapping/player start-mug-sound data/attributes, exact four-way cursor legality, START+UP random-select gate, random wander/home movement, source select_clock PSTATUS reset/OLD_PSTATUS behavior, 15-second timer and 30-tick final wait constant are translated. Credit/buy-in process wiring, original objects/text/mug art and full select process orchestration remain. |
| `combat` | **harness-only** | Portable demo AI/health/contact logic is test scaffolding and is not part of normal arcade execution. |
| `audio` | **not-started** | Original game-side DCS command semantics/backend not translated. |
| `background_blimp_bdd` | **partial-source** | Paired BDB/BDD BLIMP data is decoded as raw source blocks plus palettes and cross-validated against compiled BMOD records; NTITLESC is wired block-for-block with no WIMP crop or flattened recreation. |
| `source_clock` | **exact-source** | DISPLAY.EQU TSEC=53 is executed independently of 60 Hz N64 presentation using an accumulator; literal 60-tick source sleeps remain literal. |
| `source_process_scheduler` | **partial-source** | Cooperative CREATE/sleep/kill-by-PID execution is shared by translated attract processes; full MPROC semantics and process pdata remain to port. |
| `bmod_packed_records` | **partial-source** | BAKGND.ASM 64-bit block records decode exactly and CI extracts NTITLESCBMOD/SPORTBKBMOD verbatim from BGNDTBL.ASM. NTITLESC now consumes header index, palette, Z and MAP_FLAGS transparency at render time; the reusable renderer still needs extending to remaining backgrounds. |
| `typed_animation_stream` | **partial-source** | Mixed WORD/LONG source stream supports frame refs, REPEAT, SETMODE, velocity LONG operands/modes, PAUSE, SETSPEED, zero velocities, facing/xflip and END. Unknown commands stop explicitly. |
| `source_dependency_ir` | **exact-source** | CI scans every SUBR/SUBRP and static CALL/JSRP/CREATE edge and emits dependency frontiers for attract_mode and start_match without inventing unresolved dynamic semantics. |
| `animation_source_ir` | **exact-source** | CI scans typed WORD/LONG animation data and W/L packing macros across the original ASM tree, preserving source expressions and unresolved forms without inventing pointer or command semantics. |

`harness-only` is never eligible for normal arcade execution.
