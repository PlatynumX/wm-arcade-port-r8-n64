# Source Audit — Stage 25: DRONE.ASM CPU decision core + script VM

## Scope

Stage 25 translates the executable shared CPU/drone control core from `DRONE.ASM::drone_main` and the source AI script interpreter into portable C. It is cumulative on Stages 1-24 and does not build a ROM.

This is a direct-port layer. It does **not** substitute a new N64 bot heuristic for the arcade AI.

## Source correspondence

Primary source: `DRONE.ASM` from the original WrestleMania arcade source tree. `MACROS.H` was used to audit literal assembler shift macros used in value arithmetic (`X16`, `X32`) so they were not mistaken for host pointer-size/address scaling.

### Process-local state translated

`wm_arcade_drone_state_t` represents the combat-relevant source drone fields:

- `DRN_MODE`
- `DRN_SKILL`
- `DRN_DELAY`
- `DRN_BUT`, `DRN_JOY`
- `DRN_BUTDT`, `DRN_BUTUT`, `DRN_JOYDT`, `DRN_JOYUT`
- `DRN_BUTCHRG`, `DRN_BUTCHRGDLY`, `DRN_BUTCHRG_p`
- `DRN_ACT_p`
- `DRN_SPMODE`
- `DRN_SEEKDIR`, `DRN_SEEKDIST`
- the logical per-player/per-attack slice of global `atkcnt_t`

`missed_blocks[WM_AT_NUM]` deliberately represents the logical `(PLYRNUM * AT_NUM) + ATTACK_TYPE` source counter slice. It is not stored in a spare wrestler user variable.

### `drone_main` behavior translated

The Stage 25 decision core preserves the source ordering for:

1. resolving the closest opponent;
2. team-alive count and same-target distance rank;
3. hang-back mode (`DRN_MODE = -3`) for teammates who are not closest;
4. periodic random mode changes from `PCNT` and `rnd`;
5. passive seek behavior;
6. charged-button delay decrement;
7. GETUP_TIME skill cheating and unconditional current-script abort while GETUP_TIME is positive;
8. `DRN_DELAY` handling;
9. attack block detection and attack-specific missed-block learning;
10. continuation of an existing source script before new action selection;
11. block-mode punch/push attempt;
12. normal-mode passive/aggressive scheduling;
13. self-mode special script selection (`drn_roll`, `drn_inair`, `drn_ontb`, `drn_run`, `drn_combo`);
14. charged-script release;
15. blocked-opponent / dead-opponent routes;
16. short/medium/long range selection using `max(XDIST, ZDIST*2)` with `<100`, `<180`, and long bands;
17. headhold and headheld source delays;
18. final button/joy transition calculation.

### Exact block geometry and learning behavior

The source's geometry split is preserved:

- non-missile attack: X distance <= 180 and Z distance <= 100;
- missile attack (`AT_MSL`): bypasses the X test and uses Z distance <= 50;
- `AT_PUSH` is excluded;
- opponent GETUP_TIME and MODE_INAIR2 suppress the block attempt;
- already holding block suppresses another block decision.

Missed-block counts are attack-type-specific, clamp to index 9 for the `blkatk_t` lookup, and increment only on a failed eligible block attempt. The source cancels the opponent attack timestamp on a miss; that ordering is retained.

The team penalty is the literal source arithmetic `(alive_team - 1) << 5` (the source applies `X32` to this value). It must not be normalized to a guessed percentage decrement.

### GETUP_TIME table

The source `SKLM` construction is materialized exactly as 30 values:

`10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,70,80,90,100`

When GETUP_TIME is positive, the source always exits through script-abort. The random skill check only determines whether five ticks are removed first.

### Range script selection

The core preserves source range bands and source list semantics. A range-mode resolver supplies the literal `wnshort_t` / `wnmed_t` / `wnlong_t` mode-list result. The returned list carries the original first-word max index; a negative source max identifies the source headhold list convention. For skill > 26, the source forces max index 1 (first two entries only) for those negative/headhold lists.

No replacement move weights are invented if the source list data is not supplied.

### Script VM translated

`wm_arcade_drone_script_step()` directly represents the source script interpreter behavior using decoded operations. It preserves:

- source done bit / script termination;
- command 1: seek until complete;
- command 2: skill-table abort with 10-tick fail delay and immediate abort if opponent blocks;
- command 3: wait until animation is interruptible;
- command 4: abort if opponent is blocking;
- command 5: call code;
- command 6: random-percent jump;
- command 7: unconditional jump;
- source fallback executable/function call;
- packed input words with low five button bits and higher movement-direction bits;
- source-facing left/right direction flip;
- source input delay behavior, including script abort on nonpositive delay;
- MODE_NORMAL and MODE_BLOCK being treated as the same script mode for continuity;
- abort when other self modes no longer match `DRN_SPMODE`.

The VM representation is decoded C data, not a newly designed behavior tree.

### Headhold/headheld delays

The source value arithmetic is retained:

- HEADHOLD base delay 1, or 22 when at least two teammates are alive, then `rndrng0(sklhhdly_t[skill])`, with the odd-PCNT cap of 65;
- HEADHELD normally applies `(alive_team - 1) << 4` (source `X16`) before adding `1 + rndrng0(sklhrdly_t[skill])`; one `rnd(7)==0` path skips that multiply and keeps the raw alive-team count; odd-PCNT cap is 70.

## Deliberately unresolved source-data seams

The current public-source extraction available to this pass exposed the executable `drone_main` and interpreter code but not enough raw data to responsibly reproduce every early-file data table/script body. Therefore Stage 25 does **not** invent values for these source objects:

- `blkbase_t`
- `blkatk_t`
- `sklhhdly_t`
- `sklhrdly_t`
- wrestler-specific `wnshort_t`, `wnmed_t`, `wnlong_t` mode-list contents
- the named source AI script bodies selected through those lists
- per-script skill percentage tables reached by command #2
- exact code/function targets reached by script commands #5 / fallback

They are represented by source-labelled resolver callbacks in `wm_arcade_drone_callbacks_t`. Missing callbacks do not produce made-up AI values; the port remains inert at that seam.

This is an explicit port boundary, not permission to recreate those tables heuristically. The next AI-data substage should materialize those values directly from the complete source/ROM data and feed the existing Stage 25 core unchanged.

## Files

- `wm_arcade_drone.h`
- `wm_arcade_drone.c`
- `test_combat_stage25.c`
- `DRONE_PORT_COVERAGE.md`
- `DRONE_SOURCE_SEAMS.md`

## Verification target

Stage 25 must compile with the same strict C11 flags as Stages 1-24 and all cumulative regression tests must continue to pass. UBSan is also required before packaging.
