# Combat2EI — WRESTLE.ASM Source-Parity Audit

## Audit identity

- N64 port repository: `PlatynumX/wm-arcade-port-r8-n64`
- Audited port commit: `40cc4dd45409b8e26fc8ff7340d1699c62a914a5` (`Combat2EH`)
- Combat2EH parent/runtime authority: `5d91be9b5ed4f3c6a73cc78dba4bd15740fba4cc`
- Midway source repository: `historicalsource/wwf-wrestlemania`
- Midway source revision inspected: `1280555b4d041dd025198c8e85ed14b4c1c91cfb`
- `WRESTLE.ASM` blob: `0598f7ddeb5a65f812390c3838b21caea159410d`
- Audit scope: `WRESTLE.ASM` ownership and directly related live systems: process/match lifecycle, input history, movement, ring confinement/climbing, animation, character dispatch, DRONE, collision/reaction, attachments, ropes, specials, camera, match timer/round results, postmatch, audio, and platform presentation seams.

## Executive verdict

Combat2EH is **not yet a source-parity port of `WRESTLE.ASM` as a whole**. It contains substantial direct translations of individual Midway subsystems and a useful live 1v1 combat spine, but the spine currently behaves as a two-actor combat/demo runtime rather than the complete Midway match-process engine.

The strongest live source-backed areas are the DRONE corpus, animation bytecode/frame selection, source WIMP collision metadata, core hit/overlap math, basic 16.16 movement/friction, ring line geometry, keep-onscreen behavior, ROPES interpreter/render bridge, ring background/crowd presentation, and several SPECIAL/REACT services.

The highest-priority parity failures are not cosmetic. They are control-flow/state-machine differences: source RUN is discarded, attack buttons are fed as one-frame pulses rather than held current states, the 16-entry per-wrestler joystick history and hold-duration service are absent from the live spine, character secret detection is therefore not live, full `confine_wrestler`/`final_confine` is not executed, overlap/attachment/x-flip ordering differs from the source, the runtime is hardcoded to two actors, and the full match/round/timer/winner process lifecycle is not executing.

Until those gates are fixed and traced in source order, `WRESTLE.ASM parity = green` would be inaccurate.

## Status legend

| Status | Meaning |
|---|---|
| **GREEN** | Direct source behavior exists and is on the live path with no major mismatch found in this audit. |
| **YELLOW** | Source-derived/direct implementation is live, but a material seam or scope limitation remains. |
| **ORANGE** | A direct/source-backed implementation exists in the repository but is not fully called from the live `WRESTLE` path. |
| **RED** | Missing, bypassed, structurally incompatible, or demonstrably divergent from the source. |

## Functional parity matrix

| Midway responsibility | Combat2EH status | Evidence / issue |
|---|---|---|
| `start_match` master process | **RED** | Midway creates one persistent master fight process, suspends it between rounds, creates timer/crowd/rope/wrestler processes, and supports 0/1/2-player plus multiple drones. Combat2EH exposes `wm_fix39_match_begin/tick` as a flat two-actor runtime instead. |
| `process_ptrs` / wrestler cardinality | **RED** | Midway allocates `process_ptrs` for `NUM_WRES` and eight wrestler object banks. Combat2EH hardcodes `WM_FIX39_ACTOR_COUNT 2u` throughout the live spine. |
| First 1v1 start positions | **GREEN** | Combat2EH explicitly seeds P1/P2 from the first `reset_start`/team entries and maps frontend roster order to Midway roster ids. |
| Human input current/down/up | **RED** | N64 platform sends C-button attacks using `pressed.*`; runtime reconstructs `BUT_VAL_CUR` from those one-frame pulses. This cannot represent held attack-button current state. |
| RUN input | **RED** | `wm_fix39_match_tick` explicitly casts `run` to void and documents it as a separate missing source service. |
| P2 human input | **RED** | Live match API receives only one human controller input set. Actor 1 is driven by DRONE; two-human source behavior cannot be represented. |
| `init_joystat` / 16-entry history | **RED** | Midway allocates and clears 16 timestamped input-history records per wrestler. The live actor/runtime has no equivalent 16-entry history buffer. |
| `update_joystat` | **RED** | Midway records facing-relative stick transitions and each newly-pressed button separately with `round_tickcount` timestamps. No live equivalent is called. |
| `update_joy_dtime` / held durations | **RED** | Midway calls it after x-flip every wrestler tick. Live spine does not call a source-equivalent hold-duration service. Charge moves therefore lack the source input-time basis. |
| `count_button_presses` | **YELLOW** | Direct per-button counters exist and are called, but they consume the already-divergent one-frame platform current state. |
| Secret move tables | **GREEN (data)** | Character secret patterns and monitor tables are present for the translated wrestler modules. |
| Secret move detection/fire path | **RED (live)** | Example: `wm_arcade_move_bret` only checks secrets through `cb->check_secret_moves`, but the live Bret callback table does not bind that callback. Manual fire APIs existing elsewhere do not make source detection live. |
| `ARE_WE_IN_RING` / ring line geometry | **GREEN** | Eight source trapezoid line seeds and line-X interpolation are directly represented; live tick refreshes in-ring state. |
| `confine_wrestler` + fix1/fix2 | **RED live / ORANGE pieces** | Midway clamps top/bottom and slanted side ropes, handles attached-pair correction, climb transitions, bounce velocities, rope commands/sounds, and movement masks. Combat2EH live tick does not call a complete `confine_wrestler` implementation despite comments describing the source seam. |
| `final_confine` | **RED** | Source master fight loop calls it each cycle. No live source-equivalent call was found in Combat2EH. |
| Ring climb helpers | **ORANGE** | Detailed direct translations exist in `wmania_ring_climb.c`, but the live `wm_fix39_match_tick` does not invoke the climb/confine pipeline that reaches them. |
| `keep_onscreen` | **YELLOW/GREEN** | Detailed ROM/source translation exists and is live before velocity integration. It still lives inside the current 2-actor / platform-camera scope. |
| `wrestler_veladd` | **GREEN/YELLOW** | 16.16 velocity/gravity/ground behavior is directly represented. Full correctness still depends on source confinement and animation/object state supplied around it. |
| `wrestler_friction` | **GREEN** | Direct friction behavior exists and is called in the proper movement section. |
| `update_newfacing` / closest opponent state | **YELLOW** | Live source-facing/closest refresh exists, but Combat2EH currently calls `live_source_face_opponents()` plus `refresh_distances()` twice consecutively before DRONE instead of the single source pass. |
| `animate_wrestler` / source VM | **YELLOW/GREEN** | Source animation programs/tables are streamed and primary/secondary runtimes tick live. Remaining native `ANI_CODE` service seams are explicitly tracked rather than silently guessed. |
| Source frame WIMP metadata | **GREEN** | Current source frame is resolved to exact WIMP sprite metadata; collision is disabled if exact frame geometry is unavailable rather than fabricated. |
| `move_wrestler` dispatch | **YELLOW/GREEN** | Both live actors enter translated wrestler dispatch. Eight roster modules are present. Input-history, RUN, multi-wrestler context, and some native callbacks prevent whole-path parity. |
| Bret/Razor character control | **YELLOW** | Dedicated source-backed modules and animation tables exist. Live secret-input detection/charge timing and some external callbacks remain incomplete. |
| Taker/Yoko/Shawn/Bam/Doink/Lex control | **YELLOW** | Dedicated translated modules are dispatched live through common bindings. Same shared WRESTLE input/confine/process gaps apply. |
| DRONE source bodies/services | **GREEN within 1v1** | Combat2EH requires generated 15/15 DRONE bodies plus source tables/ranges/scripts/services before marking DRONE runtime ready. The world passed to it still has only two actors. |
| `set_collision_boxes` / hurt boxes | **GREEN/YELLOW** | Source IANI3 geometry is used. Collision readiness is gated on exact frame metadata. |
| `overlap_collision` | **YELLOW** | Direct overlap separation exists and is called, but its location in the live tick is too late relative to attachment/x-flip/countdowns. |
| Wrestler hit collision / REACT | **YELLOW/GREEN** | Direct hit gates, attack boxes, reaction bridge, health callbacks and source reaction modules are live. Ordering and broader match-state gaps remain. |
| `update_links` | **YELLOW/GREEN** | Stale one-way attachment cleanup is live. |
| `master_keep_attached` | **RED order / GREEN primitive** | Attachment primitive exists, but Combat2EH calls it before the later overlap pass. Midway calls it after `overlap_collision`. |
| `set_wrestler_xflip` | **RED order / GREEN primitive** | X-flip logic is present but currently runs before the later overlap/collision pass; source runs it after overlap and attachment. |
| Countdown tail (`DELAY_BUTNS`, `SAFE_TIME`, etc.) | **YELLOW/RED order** | Fields are decremented, but they execute before Combat2EH's later collision pass, while Midway performs them after overlap/attachment/x-flip/update_joy_dtime. |
| GETUP timing | **YELLOW** | Recovery countdown logic is source-oriented and avoids forcing stand animation. Missing match clock and `update_joy_dtime` context means whole behavior is not yet source complete. |
| ROPES processes | **GREEN/YELLOW** | Four source rope runtime banks tick live and N64 rendering consumes source image symbols/positions. Wrestler-to-rope confine/bounce interaction remains incomplete because full confinement is not live. |
| Ring BMOD / arena presentation | **GREEN/YELLOW** | N64 renderer is explicitly source-coordinate/BMOD based. Presentation is platform-specific but source data/order is retained. |
| Crowd animation | **GREEN/YELLOW** | Source crowd scripts and cheer selection are represented in platform runtime. Audio and full process scheduler remain separate. |
| SPECIAL objects/projectiles | **YELLOW/GREEN** | Source spawn/tick/collision services are live for mapped specials. Unknown/native process labels are preserved as explicit external seams rather than guessed. |
| Camera / `scroll_world` | **YELLOW/GREEN** | Source-derived scroller state is maintained for current 1v1. Full source process/cardinality contexts are not represented. |
| Match timer (`match_time`) | **RED** | Midway owns a dedicated displayed game clock and timer-expiry behavior. Combat2EH's live match state has no equivalent complete clock/winner lifecycle. |
| Round lifecycle / `match_over` / `match_winner` | **RED** | Source state includes current round, rounds won, winner, realtime, audits, round start/end. Combat2EH's live tick does not execute the complete round-end/winner process. |
| Win streak / awards / audits | **ORANGE/RED live** | Source-derived completion/high-score helpers exist, but the complete `WRESTLE` master-process state transitions and audit timing are not live as one source flow. |
| Royal Rumble / 8-on-1 | **RED live / ORANGE plans** | Source-derived plan builders exist, but hardcoded two-actor runtime cannot execute the Midway multi-wrestler match topology. |
| Postmatch routing | **ORANGE/YELLOW** | Source-derived routing/plan APIs exist. They are not equivalent to executing the original persistent process graph. |
| Audio labels / source sound services | **RED/YELLOW** | DCS/audio infrastructure exists, but runtime status explicitly leaves `audio_label_service_ready=false`; character sound callbacks often record tokens/labels instead of proving live DCS command behavior. |
| High-score system/persistence | **YELLOW** | HSTD tables and persistence adapters are source-backed. Save backend is explicitly false until bound. This is related match/completion infrastructure, not evidence that `WRESTLE` itself is complete. |
| Attract scheduler/demo | **YELLOW** | Source-backed attract/DRONE pieces are substantial, but existing frontend/platform ownership and renderer capability gates remain. |

## Exact source-order discrepancy in `wrestler_main`

The Midway per-wrestler loop around movement/combat is, materially:

1. `ARE_WE_IN_RING`
2. `set_collision_boxes`
3. `confine_wrestler`
4. `confine_wrestler_fix2`
5. `update_newfacing`
6. `update_positions`
7. `drone_main` for CPU wrestlers
8. `update_joystat`
9. `count_button_presses`
10. `keep_onscreen`
11. `wrestler_veladd`
12. `wrestler_friction`
13. `animate_wrestler`
14. `set_collision_boxes`
15. `confine_wrestler`
16. `confine_wrestler_fix1`
17. `calc_closest2`
18. `move_wrestler`
19. `update_links`
20. `set_collision_boxes`
21. `overlap_collision`
22. `master_keep_attached` when requested
23. `set_wrestler_xflip` unless `MODE_NOAUTOFLIP`
24. `update_joy_dtime`
25. decrement input/safety/meter/immobilize/walk/getup state

Combat2EH currently does a different sequence. The most important discrepancies are:

- no live `update_joystat` history insertion;
- no live `update_joy_dtime` service;
- no complete `confine_wrestler`/fix pipeline;
- repeated facing/closest refresh before DRONE;
- overlap collision is deferred until after attachment, x-flip, countdowns and SPECIAL ticking;
- therefore attachment and x-flip see state from the wrong source phase.

That ordering must be corrected before debugging move-specific symptoms, because individual character modules can be exact and still receive the wrong world/input state.

## Input pipeline: why current controls cannot be source-equivalent

The Midway input design distinguishes at least four concepts that are currently collapsed or missing:

- held current button state (`BUT_VAL_CUR`);
- new button edges (`BUT_VAL_DOWN`) and release edges (`BUT_VAL_UP`);
- a 16-record timestamped history (`wrest_joystat`) containing facing-relative stick changes and each newly-pressed button;
- per-control duration state updated by `update_joy_dtime`, used by charge/release logic.

The N64 adapter currently populates attack actions from `joypad_get_buttons_pressed`, then `wm_fix39_match_tick` treats those booleans as current state. A held C-button therefore becomes current for one tick and zero on subsequent ticks. Blocking uses `now.r`, so block happens to have a held current state while attacks do not. `run` uses `now.a` but is intentionally discarded by the runtime.

The correct fix is not a character-specific workaround. The platform should pass physical **current** button states for all five Midway action bits; the WRESTLE input service should derive down/up edges, maintain the exact 16-entry history, and update duration counters in the Midway order. Character modules should consume that shared source state.

## Ring/confine pipeline

The repository contains strong ring pieces, but they are currently fragmented:

- `wmania_ring_geometry.c`: source trapezoid boundaries / `ARE_WE_IN_RING` support;
- `wmania_ring_onscreen.c`: detailed `keep_onscreen` translation;
- `wmania_ring_climb.c`: detailed climb-in/out/turnbuckle routines;
- ROPES source interpreter and renderer are live;
- ring-out timing/health is live.

What is missing is the `WRESTLE.ASM` **confine orchestrator** that ties collision-box edges to those services in the source order. The source handles top/bottom clamps, slanted left/right rope boundaries, attached-pair corrections, side/top/bottom climb checks, first-hit rope bounce velocities, rope commands/audio, and movement-direction restrictions. That should be ported as one source-owned service rather than re-created piecemeal inside the renderer or character code.

## Process/match lifecycle gap

The original `start_match` is not merely setup. It is the master process for the entire match and is suspended between rounds rather than recreated. It owns or coordinates:

- source display/ring initialization;
- crowd process;
- match timer process;
- bar Z shifting / flashes;
- wrestler-count process where applicable;
- four rope processes;
- life/combo initialization;
- 0/1/2-human wrestler creation;
- multiple drone process creation;
- per-round state, winner handling, match end, audits and postmatch routing.

Combat2EH's `wm_fix39_match_begin/tick` is useful as a live source-combat integration spine, but it does not yet model that process topology. This is why adding another isolated helper cannot make the whole `WRESTLE` implementation source-equivalent; a source-owned match state/process layer is still needed.

## P0 remediation order

### P0.1 — Build the exact WRESTLE input service

Implement a shared per-wrestler input state with:

- physical current five-button mask;
- exact current/down/up derivation;
- 16 timestamped `wrest_joystat` entries per wrestler;
- facing-relative X flip plus real left/right bits exactly as source;
- one history insertion per button down bit;
- `update_joy_dtime` duration fields;
- source RUN behavior;
- P2 human input path.

Then bind `check_secret_moves` / charge-release scheduling to that shared history instead of manually firing character APIs.

### P0.2 — Port and wire full confinement

Translate `confine_wrestler`, `confine_wrestler_fix1`, `confine_wrestler_fix2`, and `final_confine` as source-owned functions. Reuse the already-translated ring geometry, climb, rope command, and attachment primitives rather than inventing new behavior.

### P0.3 — Make the live tick source-order exact

Rebuild the wrestler phase sequence from the numbered source order above. In particular, move collision-box refresh, overlap, attachment, x-flip and duration updates to the correct positions. Remove duplicated facing/closest updates unless a source call proves both passes.

### P0.4 — Replace the hardcoded two-actor world

Model `process_ptrs`/active wrestler slots up to the source `NUM_WRES` topology. This unlocks:

- 2 human players;
- tag/team/multiple drone battles;
- Royal Rumble;
- 8-on-1/final battle;
- correct closest/live-teammate/target selection.

Do not change DRONE semantics to compensate for a two-actor world; give DRONE the source world instead.

### P0.5 — Port live match/round/timer/winner state

Bring over the source-owned state and transitions for:

- `match_time` display clock;
- `match_over`;
- `match_winner`;
- `current_round`, `p1rounds`, `p2rounds`;
- round start/end and realtime clocks;
- timer expiry;
- match-end suspension/resume behavior;
- win streak / award hooks in the same control flow.

Existing postmatch/rumble/finale plan helpers can then become adapters to this live state rather than substitutes for it.

### P0.6 — Close remaining native animation/audio services

Every unresolved `ANI_CODE`/process/audio label must have one of three explicit statuses: direct translated and live; direct translated but awaiting platform adapter; or source-unresolved and fail-closed. Do not convert diagnostic label recording into a fake successful service.

### P0.7 — Add a WRESTLE parity trace harness

For deterministic seeded input, capture per tick:

- PCNT / round tick;
- source input current/down/up/history/durations;
- actor position/velocity/mode/anim mode/facing;
- current primary/secondary source animation/frame;
- collision boxes / hit result;
- attachments;
- ring/climb/confine events;
- DRONE command/body/service ids;
- life/getup/safe/delay state;
- match/round timer/winner state.

The host audit should compare this trace against a source-oracle/reference trace at subsystem boundaries. Static symbol/table parity alone is insufficient for `WRESTLE.ASM` because call order is part of the behavior.

## What should remain untouched during the logo merge

The supplied Be a Man Sports-logo package correctly limits the branding change to the `SPRTLG01`–`SPRTLG17` artwork payload and build-generation ownership. The current N64 renderer should remain unchanged so the existing Midway screen choreography, background, source anchor, object order, motto and attract timing are preserved.

The logo integration commit should therefore contain only:

- tracked `src/generated/sports_logo.c` replacement;
- the branding source PNG and reproducibility tool;
- Makefile rule/clean guards so the checked-in replacement is not regenerated/deleted;
- `prepare_frontend_assets.sh` guard so normal frontend generation cannot overwrite it;
- this audit document.

No `src/fix39`, `src/core`, `src/platform/n64/main.c`, or combat behavior files need to change merely to merge the logo.

## Source references used in this audit

### Midway

- `WRESTLE.ASM` state allocations: roughly lines 180–260 at source revision above.
- `WRESTLE.ASM` `start_match`: roughly lines 1545–1745.
- Master fight loop: roughly lines 2030–2085.
- `wrestler_main` source ordering: roughly lines 2380–2560.
- `confine_wrestler` rope/attached interaction: roughly lines 3090–3405 and surrounding continuation.
- `init_joystat` / `update_joystat`: roughly lines 4490–4635.
- history insertion / button count continuation: roughly lines 4630–4765.

### Combat2EH

- `src/fix39/wm_fix39_runtime.c`
- `src/fix39/wm_fix39_runtime.h`
- `src/fix39/wm_arcade_combat.[ch]`
- `src/fix39/wm_arcade_movement.c`
- `src/fix39/wm_arcade_wrestler_port.c`
- `src/fix39/wm_arcade_bret.c`
- `src/fix39/wmania_ring_geometry.c`
- `src/fix39/wmania_ring_onscreen.c`
- `src/core/arcade/wmania_ring_climb.c`
- `src/platform/n64/main.c`
- `Makefile`
- `scripts/prepare_frontend_assets.sh`

## Gate for declaring WRESTLE source parity

Do not mark `WRESTLE.ASM` green until all of the following are true in one live build:

1. source current/down/up + history + duration input path is active for every wrestler;
2. RUN and charge/release moves use source input semantics;
3. full confine/final-confine + climb/rope interaction is in the live source order;
4. overlap/attachment/x-flip/duration ordering matches `wrestler_main`;
5. active wrestler slots are not hardcoded to two;
6. human-vs-human and multi-drone topology use the same source process/state model;
7. live match timer/round/winner lifecycle is source-owned;
8. remaining animation/audio/native callbacks are explicit and non-fabricated;
9. deterministic traces show source-order state transitions, not just matching tables/symbol names.
