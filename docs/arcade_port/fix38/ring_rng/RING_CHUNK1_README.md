# WrestleMania Arcade -> N64 ring port — Chunk 1

First merge-oriented ring-system chunk.

## Included

- exact live `RING.EQU` geometry/constants
- exact eight live boundary seed records from `WRESTLE.ASM`
- exact rope bank/strand/action enums from `GAME.EQU`
- direct `rope_command` table routing
- exact side/down-spring lane thresholds
- exact side/down-spring selection matrices
- rope command priorities and replacement rule
- direct `set_rope_z` behavior
- ring-relevant wrestler PDATA field map
- regression tests

## Important

Do **not** merge the old `RING.ASM` pregenerated arrays as the ring model.
That file explicitly says it is no longer required and its values differ
from the later live `RING.EQU` constants.

## Not in Chunk 1

No approximation has been substituted for source routines that have not yet
been located:

- in-ring/outside classification behavior
- climb-through-rope decisions/animations
- turnbuckle climb state machine
- keep-onscreen/fence collision behavior
- ring-out timer/damage behavior
- full rope image/sequence playback engine

The rope command router deliberately returns the original source script-table
label (for example `sspr32_t`) so the full `ROPES.ASM` animation scripts can
be translated later without changing command semantics.

## Merge

Compile:

- `wmania_ring_geometry.c`
- `wmania_rope_command.c`

Include:

- `wmania_ring_geometry.h`
- `wmania_rope_command.h`
- `wmania_ring_player_fields.h`

`test_ring_chunk1.c` is host-side verification only.
