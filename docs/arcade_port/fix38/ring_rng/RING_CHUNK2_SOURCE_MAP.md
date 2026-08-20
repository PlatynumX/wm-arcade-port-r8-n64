# Ring Chunk 2 source map

Primary verified live source: `ROPES.ASM`.

## Directly translated

### `STRUCTPD` rope channel state
`wmania_rope_runtime.*` carries the same semantic state for each of:
- red/top rope
- white/middle rope
- blue/bottom rope
- shadow

Per channel:
- object 1 / object 2 existence
- current script
- script repeat counter
- current sequence
- frame-hold counter
- current priority

### `#front_ptable`, `#back_ptable`, `#left_ptable`, `#right_ptable`
`wmania_rope_spawn.*`

The source object/image labels, flip state, X/Y/Z values and the ugly but
live BEGINOBJ offset (`X + 104`, `Y - 258`) are preserved.

Front/back have 3 channels and no shadow.
Left/right have red/white/blue plus shadow.

### `reduce_bog`
The source creates rope objects, sleeps two ticks, then if `reduce_bog` is
set it kills only front/back rope processes and clears their process
pointers. Side processes continue.

`wm_rope_process_survives_reduce_bog()` and runtime initialization preserve
that distinction: objects still exist, but the front/back process is not
commandable/updating.

### `new_command_wake`
`wm_rope_runtime_apply_program()`

For each existing channel:
- current priority > incoming priority -> skip
- incoming priority >= current -> replace
- install script
- load first script repeat count and sequence
- set frame counter to 1 so the next process tick advances immediately

### `rope_update`
`wm_rope_runtime_tick()`

The translated state machine preserves:
- no active sequence -> return
- nonexistent object -> return
- decrement current frame hold
- sequence end -> decrement script repeat
- repeat the sequence if repeats remain
- otherwise advance to next script entry
- end of script clears frame counter + priority
- high-bit special command is treated as GOTO
- front/back frame image is applied to both halves
- side frame resolves independent first/second images
- all four channel blocks update once per process tick

### `fastanic`
Not copied as a Midway image-header memory write.

The portable runtime calls `WmRopeImageUpdateFn` with the exact source image
symbol selected by the translated animation. The N64 renderer/asset layer
owns the equivalent image/mesh/texture change.

This keeps animation-selection logic source-backed while avoiding assumptions
about N64 object-header layout.

## Data intentionally separated

Ring Chunk 4 directly translates the static ROPES.ASM script/sequence
corpus and supplies it through `wm_rope_source_program_resolver`.
No fake procedural spring/shake animation is generated.

## Still unresolved in current source audit

`WRESTLE.ASM` declares these as external `.ref` symbols, and the current
audit did not locate their bodies in the available game source files:

- `keep_onscreen`

The previously listed `ARE_WE_IN_RING` and `ck_climb_*` routines were later
located in `SPECIAL.ASM` and `WRESTLE2.ASM` and are now directly translated
by Ring Chunk 3.

Chunk 2 does **not** infer them from boundary geometry or names.

They remain explicit future source/dependency targets.
