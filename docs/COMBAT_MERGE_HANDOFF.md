# WrestleMania Arcade Combat — cumulative Stage 25 DRONE core direct-port handoff

This package supersedes Stage 24 and every earlier combat package. Source only; no ROM was built.

## Critical rule

This remains a direct port of the original arcade code. Do not replace `wm_arcade_drone.c` with a generic bot/behavior-tree implementation and do not tune missing AI values by feel.

All eight wrestlers remain dedicated source-corresponding modules from Stage 23. Stage 24 special-object combat remains intact.

## Stage 25 adds

- `wm_arcade_drone.c/.h`
- direct shared decision/order core from `DRONE.ASM::drone_main`
- exact 30-entry source GETUP_TIME skill table
- team spacing/hang-back behavior
- attack block eligibility and per-attack missed-block learning
- literal source team-size arithmetic
- passive/aggressive scheduler
- self/opponent mode script routes
- short/medium/long range-selection mechanics
- headhold/headheld delay mechanics
- decoded source AI script VM/interpreter
- packed source AI input/facing decode
- final input-transition generation
- source-labelled callback seams for raw AI tables/scripts that were not materialized in this pass

Read `SOURCE_AUDIT_STAGE25.md`, `DRONE_PORT_COVERAGE.md`, and `DRONE_SOURCE_SEAMS.md` before integration.

## Merge order

1. Use ONLY this Stage 25 ZIP.
2. Run `./run_all_tests.sh` unchanged before edits.
3. Preserve Stages 1-24 behavior unchanged.
4. Map `wm_arcade_drone_state_t` onto the N64 per-CPU-wrestler state without deleting source fields or merging them into generic AI state.
5. Keep source `rnd` and `rndrng0` as separate random-service adapters.
6. Map closest-target/team/range callbacks to the existing N64 equivalents without changing source ordering.
7. Materialize the unresolved tables/scripts listed in `DRONE_SOURCE_SEAMS.md` directly from the arcade source/ROM. Do not invent default percentages, weights, or move scripts.
8. Feed literal `wnshort_t` / `wnmed_t` / `wnlong_t` data through `range_script_list` and literal named scripts through `resolve_script`.
9. Resolve command-2 skill tables and command-call labels to exact source-derived implementations.
10. Call `wm_arcade_drone_main()` in the source-equivalent CPU input/update point, then use the generated actor button/stick current/down/up state as arcade CPU input.
11. Re-run all Stages 1-25 host tests before the first ROM build.

## Fidelity notes that must not regress

- Missile blocks bypass the X<=180 test and use Z<=50; normal attacks use X<=180/Z<=100.
- A positive GETUP_TIME always aborts the current script; the skill roll only controls the five-tick reduction.
- Missed blocks are tracked per attack type.
- The source team block penalty is `(alive_team-1)<<5`.
- HEADHELD normally uses `(alive_team-1)<<4`, with the source rnd(7) skip branch.
- Skill >26 restricts negative/headhold script lists to the first two entries.
- MODE_NORMAL and MODE_BLOCK are treated as the same script mode during script continuity.

## Remaining combat work after Stage 25

1. Materialize the raw DRONE AI tables and named script bodies listed in `DRONE_SOURCE_SEAMS.md`.
2. Resolve any exact shared process/seek/call helpers reached by those scripts.
3. N64-native adapter integration and end-to-end arcade comparison.
