# Source Audit — Combat Stage 8 (`REACT6.ASM`)

Primary arcade source: `REACT6.ASM` from `historicalsource/wwf-wrestlemania`.

This source module contains no live combat implementation in the checked-in
arcade source. It defines only two legacy labels:

- `hit_att28` -> bare `rets`
- `hit_att29` -> bare `rets`

They are preserved as explicit no-op functions:

- `wm_arcade_react6_att28_stub()`
- `wm_arcade_react6_att29_stub()`

## Critical routing note

Do **not** map current attack IDs 28 and 29 to these stubs. The current
`REACT1.ASM` hit table routes attack 28 to `hit_earslap` in REACT5 and attack 29
to `hit_hammer` in REACT4. Therefore Stage 8 is an audit-fidelity port of dead /
legacy source labels, not a change to live reaction dispatch.

The regression suite calls both stubs and verifies they remain harmless.
