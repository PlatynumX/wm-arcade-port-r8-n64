# Source Audit — Combat Stage 9 (`REACT7.ASM`)

Primary arcade source: `REACT7.ASM` from `historicalsource/wwf-wrestlemania`.

This source module likewise contains only legacy placeholder labels:

- `hit_att30` -> bare `rets`
- `hit_att31` -> bare `rets`
- `hit_att32` -> bare `rets`
- `hit_att33` -> bare `rets`
- `hit_att34` -> bare `rets`

They are preserved as explicit no-op functions in `wm_arcade_react7_core.c`.

## Critical routing note

Do **not** route live attack IDs 30..34 to these placeholders. The current
`REACT1.ASM` hit table already routes them to:

- 30 -> `hit_buttstomp` (REACT4)
- 31 -> `hit_puppet2` (REACT5)
- 32 -> `hit_puppet_hdgrab` (REACT5)
- 33 -> `hit_tomb` (REACT1)
- 34 -> `hit_bigknee` (REACT1)

The legacy REACT7 `ATT30..ATT34` names are therefore preserved for source audit
completeness only.

## Cumulative bridge

Use:

    wm_arcade_react1234567_reaction_callback

This deliberately delegates live reactions through REACT5 -> REACT4 -> REACT3
-> REACT2/REACT1; the REACT6/7 placeholder stubs are not inserted into the live
hit table.

## End-to-end regression

Stage 9 adds a complete `wm_arcade_wrestler_hit` integration test:

- exact REACT5 `good_run_hit` is installed as the Stage 2 prefilter;
- the cumulative Stage 9 reaction callback is installed;
- a valid run collision performs source hit bookkeeping then reaches `hit_run`;
- an invalid Z-offset run returns `WM_WRESTLER_HIT_IGNORED_RUN` before WHOIHIT,
  WHOHITME or LAST_HIT_TIME are changed.

This specifically guards the source's duplicated `good_run_hit` architecture
instead of simplifying it away.
