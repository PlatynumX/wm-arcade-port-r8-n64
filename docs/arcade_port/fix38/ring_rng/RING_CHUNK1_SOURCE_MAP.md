# Ring Chunk 1 — arcade source map

This chunk intentionally ports only source paths that were positively
located and verified.

## Live authority

### `RING.EQU`
Translated into `wmania_ring_geometry.*`

- `Y_SCALE_MULTIPLIER`
- `RING_X_CENTER`, `RING_Z_CENTER`
- arena/fence geometry
- rope/ring geometry
- mat geometry
- expanded `MAT2` geometry
- `MAT_Y`

### `WRESTLE.ASM`
The eight active runtime boundary seed records are translated literally:

- `vln_right_rope_r`
- `vln_left_rope_r`
- `vln_right_matedge_r`
- `vln_left_matedge_r`
- `vln_right_matedge2_r`
- `vln_left_matedge2_r`
- `vln_right_fence_r`
- `vln_left_fence_r`

The corresponding generated BSS arrays and `box_matedge` pointers confirm
that these are the live boundary inputs.

### `GAME.EQU`
Translated into `wmania_rope_command.h`

- `ROPE_FRONT/BACK/LEFT/RIGHT`
- `R_TOP/R_MIDDLE/R_BOTTOM`
- `RZ_HIGH/RZ_NORM`
- all six `ROPE_*` command IDs
- `ROPE_COMMANDS`

### `ROPES.ASM`
Translated into `wmania_rope_command.*`

- command-bank routing from `rope_command`
- four up/down magnitudes
- strict five-lane side/down-spring Z selection
- exact `#sspring` 5x6 table, including its NULL sixth column
- exact `#dspring` 5x6 table
- `side_bounceio_t`
- `sspr_trans_t`, `dspr_trans_t`
- priorities:
  - `SSPRING_PRI = 10`
  - `DSPRING_PRI = 9`
  - `SHAKE_PRI = 5`
- `new_command_wake` priority rule:
  current > incoming rejects; incoming >= current replaces
- `set_rope_z`:
  high second half Z = `0x15a9`; normal copies first-half `OZPOS`

### `PLYR.EQU`
Ring-relevant live PDATA names/semantics are mapped in
`wmania_ring_player_fields.h`, including `INRING`, `GROUND_Y`,
`PLYR_ROPE_X_LEFT/RIGHT`, `CLIMBING_THRU`, `OUTSIDE_ALONE`,
`RING_TIME`, `CLIMB_START/LAST`, and the bouncing/turnbuckle modes.

## Explicitly NOT used

`RING.ASM`

The file itself states:

> This entire ASM file is no longer required.

Its old pregenerated vertical-line tables are therefore not copied into this
port.

## Deliberately deferred — no recreation

The bodies of these externally referenced live routines were not located in
the first source pass, so Chunk 1 does NOT invent equivalents:

- `keep_onscreen`

The previously listed `ARE_WE_IN_RING` and `ck_climb_*` routines were later
located in `SPECIAL.ASM` and `WRESTLE2.ASM` and are now directly translated
by Ring Chunk 3.

Likewise, this chunk does not invent ring-out damage arithmetic or
turnbuckle climb decisions merely from field names.

Those are the next source-audit/translation target.
