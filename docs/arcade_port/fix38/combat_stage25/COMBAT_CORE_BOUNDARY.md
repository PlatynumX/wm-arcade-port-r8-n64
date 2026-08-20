# Combat port boundary — Stage 25

## Shared combat core translated

- wrestler body/attack boxes and hit arbitration
- damage tables/dispatcher and repeat-damage timing
- REACT1 through REACT9 wrestler reaction families
- combat animation opcodes covered by Stages 1-13
- attachment/puppet positioning and ownership glue
- top-level `move_wrestler` dispatch seam

## Selectable wrestler control layers translated

Eight dedicated source-corresponding modules:

- Bret
- Razor
- Undertaker
- Yokozuna
- Shawn Michaels
- Bam Bam Bigelow
- Doink
- Lex Luger

## Special-object combat translated in Stage 24

- P1/P2/neutral special-object lists
- special-object collision and wrestler-hit paths
- Taker spirit/reaper, Yoko salt, Doink pie, Bam fireball combat state
- source timing/process-reuse/table hazards preserved rather than normalized

## CPU/drone AI translated in Stage 25

Stage 25 contains the shared executable `DRONE.ASM::drone_main` decision core plus the source AI script interpreter/VM semantics:

- team spacing/hang-back mode
- source difficulty/skill timing decisions
- get-up behavior
- block detection and per-attack learning
- passive/aggressive scheduler
- self/opponent mode routes
- range-band selection mechanics
- headhold/headheld timing mechanics
- script continuation/abort behavior
- source script command execution semantics
- packed AI input and transition generation

## Explicit remaining AI source-data boundary

The raw contents of `blkbase_t`, `blkatk_t`, `sklhhdly_t`, `sklhrdly_t`, `wnshort_t`, `wnmed_t`, `wnlong_t`, their named script bodies, script skill tables, and unresolved script-call targets remain source-data seams. Stage 25 exposes exact callbacks/labels for them and intentionally does not invent substitutes.

The next combat substage is to materialize those literal source tables/scripts. After that, the remaining major work is N64-native adapter integration and end-to-end arcade comparison.
