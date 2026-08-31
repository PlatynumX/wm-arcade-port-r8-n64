# R37N13 — Wrestler Process Retranslation

Baseline remains **R37N10** (`9653fd70b72a0c563b9b8bb29198760c6494ac5b`).
R37N11 is not part of this branch ancestry. R37N12 is used only as the already-
translated `wrestler_main` body to avoid retyping hundreds of source-backed calls;
this pass re-audits that body and replaces the pieces that do not match Midway's
actual process execution.

## Midway source evidence used

### MPROC.ASM / MPROC.EQU

- `PTIME` is a 16-bit process sleep counter.
- Dispatcher traversal decrements `PTIME` when an active-list entry is visited.
- The process resumes when the signed decremented result is not positive.
- `GETPRC` and `GETSPRC` insert the new process directly **after** the current
  process.
- `GETPRC_INSERT` inserts directly **before** the current process.
- `SLEEPK` / `SLEEPR` save the wake address and return control to the dispatcher.

### WRESTLE.ASM::wrestler_main

- Wrestler creation is `SCREATE WMAIN_PID,wrestler_main` followed immediately by
  `CREATE GETUP_PID,getup_meter`.
- Initial wrestler setup ends in `SLEEPK 1`.
- First wake resumes at `calc_closest`, then reaches the pre-sleep portion of
  `#loop` and executes `SLEEPR 1` (or `0x7fff` while `B_KOD`).
- Later wakes resume immediately after that `SLEEPR`.
- Post-sleep execution order includes input history, one velocity/friction
  integration, animation, collision boxes, confine/fix1, `calc_closest2`,
  `move_wrestler`, `update_links`, overlap, attachment maintenance, flip, and
  countdowns.
- The apparent second `wrestler_veladd` / `wrestler_friction` pair after
  `move_wrestler` is commented out in the shipped source and must not execute.

### WRESTLE2.ASM::init_smoves

- Each wrestler-specific `SMOVE_PID` watchdog is created with
  `GETPRC_INSERT`, immediately before its owning WMAIN process.
- Watchdogs are therefore not a global pre-wrestler phase.

## Exact active-list consequence

Because every WMAIN and GETUP is inserted after the same creator, later-created
wrestlers appear earlier in the active list. Because GETUP is created after
WMAIN, it sits immediately before WMAIN. When WMAIN initializes its watchdogs,
those watchdogs are inserted between GETUP and WMAIN.

For a two-wrestler match the relevant source group order is therefore:

`GETUP1 -> SMOVEs1 -> WMAIN1 -> GETUP0 -> SMOVEs0 -> WMAIN0`

This also makes wake timing directional. A process that has already been visited
cannot be made to execute again in the same dispatcher pass merely by writing
`PTIME=1`; a later process can.

## R37N13 corrections

1. Visit wrestler groups in reverse actor-creation order.
2. Dispatch GETUP before that actor's SMOVE watchdogs and WMAIN.
3. Tick only the current owner's SMOVE watchdogs at that process-list position.
4. Preserve the WMAIN `PTIME` coroutine resume state.
5. Remove the erroneous second velocity/friction integration after
   `move_wrestler`.
6. Leave renderer, camera, presentation, sprite assets, and unrelated gameplay
   code untouched.

## Why this is relevant to the grapple freeze

Grapple/headhold code uses reciprocal attachment state, wrestler modes,
`SPECIAL_MOVE_ADDR`, and wake writes such as `PTIME=1`. Those values cross WMAIN
and SMOVE process boundaries. Global batching changes which process sees a state
change in the current frame. Double-applying wrestler velocity after
`move_wrestler` also moves attached geometry twice in a single WMAIN wake.
R37N13 removes both source-proven timing/physics deviations without inventing a
new grapple rule.
