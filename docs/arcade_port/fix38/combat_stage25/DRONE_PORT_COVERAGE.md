# Stage 25 DRONE port coverage

## Executable direct-port code included

- `DRONE.ASM::drone_main` shared decision/order core
- logical process-local drone state
- team target ranking / alive-team count
- hang-back and random drone-mode changes
- source passive seek hook and seek direction/distance state
- charged-button delay handling
- exact 30-entry GETUP_TIME skill table and behavior
- `DRN_DELAY` countdown semantics
- attack-block eligibility geometry
- attack-specific missed-block counters (`atkcnt_t` logical slice)
- literal team-size block penalty
- minimum 15-tick block time
- leaping `slhtoss` route
- puppet special-block direction
- existing-script continuation ordering
- block-mode punch/push attempt
- normal-mode passive/aggressive scheduler
- `drn_enterring`, `drn_opinair`, `drn_oprun` source routes
- source self-mode scripts (`drn_roll`, `drn_inair`, `drn_ontb`, `drn_run`, `drn_combo`)
- charged-script release
- blocking-opponent routes (`M_shrtblkr`, `M_shrtblkrdl`, `drn_seekclose`)
- dead-opponent route (`drn_oppdead`)
- exact short/medium/long range metric and band thresholds
- source list max-index random selection + hard-skill headhold restriction
- HEADHOLD / HEADHELD timing rules
- source AI script command interpreter semantics
- packed source input decoding and facing flip
- final button/joy down/up transition generation

## Source-data seams intentionally not recreated

The following must be filled with literal original data before the AI can be called source-data-complete:

- `blkbase_t`
- `blkatk_t`
- `sklhhdly_t`
- `sklhrdly_t`
- `wnshort_t`
- `wnmed_t`
- `wnlong_t`
- the named script bodies referenced by those tables
- script command-2 skill tables
- script call/function targets that are not already represented in the cumulative port

Stage 25 exposes these through exact-label/table callbacks. Do not replace the callbacks with guessed weights, generic difficulty curves, or a newly designed behavior tree.

## Status wording

Correct: **shared DRONE CPU decision core + source script VM translated; raw AI data/script materialization remains.**

Incorrect: **all arcade CPU AI data is finished.**
