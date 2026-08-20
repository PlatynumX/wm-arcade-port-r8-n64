# WrestleMania Arcade -> N64 ring port — Chunk 2

Chunk 2 adds the verified live rope process/runtime layer on top of Ring
Chunk 1.

## New code

- `wmania_rope_spawn.c/.h`
  - exact four live rope-bank object seed tables
  - source object/image labels
  - first/second halves
  - side shadows
  - horizontal flip state
  - exact BEGINOBJ X/Y translation
  - `reduce_bog` process survival behavior

- `wmania_rope_runtime.c/.h`
  - four per-bank animation channels
  - priority replacement
  - new-command wake semantics
  - script repeat state
  - sequence/frame state
  - frame hold timing
  - sequence GOTO
  - script GOTO
  - end-of-script priority clearing
  - front/back one-image/two-half update
  - side independent top/bottom image update
  - one-tick four-channel update loop

- `wmania_rope_source_bridge.h`
  - contract for direct source script-table data

## Why the script data is a resolver

Chunk 2 originally left the static corpus behind `WmRopeProgramResolver`.
Ring Chunk 4 now directly translates that corpus and provides
`wm_rope_source_program_resolver`; no procedural approximation is used.

## Still not fabricated

The original Chunk-2 audit had not yet located the in/out/climb bodies.
They were subsequently found in `WRESTLE2.ASM` and `SPECIAL.ASM` and are
ported by Ring Chunk 3. `keep_onscreen` remains unresolved.

See `RING_CHUNK2_SOURCE_MAP.md`.
