# Ring Chunk 4 source map

Primary source: `ROPES.ASM`.

Ring Chunk 4 completes the static rope-animation data that Ring Chunk 2
deliberately left behind a resolver interface.

## Complete program coverage

`wmania_rope_source_data.c/.h` contains 70 source script-table entry points:

- `front_bounceud1_t` .. `front_bounceud4_t`
- `back_bounceud1_t` .. `back_bounceud4_t`
- `side_bounceud1_t` .. `side_bounceud4_t`
- `side_bounceio_t`
- every source-defined `sspr11_t` .. `sspr55_t` table (5x5)
- every source-defined `dspr11_t` .. `dspr56_t` table (5x6)
- `sspr_trans_t`
- `dspr_trans_t`

The source command lane lookup does not reach every defined spring table
(`sspr51_t` and `dspr51_t` are examples), but the source definitions are
still translated rather than dropped.

## Fall-through labels preserved

The assembly uses labels in the middle of scripts as entry points. For
example, `front_bounceud4_R` starts before `front_bounceud3_R`, which starts
before `front_bounceud2_R`, which starts before `front_bounceud1_R`.

Portable scripts therefore represent the exact suffix at each source label:

- level 4: sequence 1, sequence 2, sequence 3 x2, sequence 4 x3
- level 3: sequence 2, sequence 3 x2, sequence 4 x3
- level 2: sequence 3 x2, sequence 4 x3
- level 1: sequence 4 x3

This is done separately for red/white/blue where source sequence labels have
different fall-through offsets.

## RANI_GOTO

The source treats every script/sequence word with bit 15 set as `RANI_GOTO`.

Sequence GOTOs were already supported by Ring Chunk 2 and are now populated
for:
- all sideways-spring sequences -> `#s_stop`
- all sideways-spring shadow sequences -> `#s_stop_shadow`

Ring Chunk 4 extends `WmRopeScriptEntry` so a script GOTO may target another
script object, matching:
- `#sspr_trans_R` -> `side_bounceio2_R`
- `#sspr_trans_W` -> `side_bounceio2_W`
- `#dspr_trans_R` -> `side_bounceud2_R`
- `#dspr_trans_W` -> `side_bounceud2_W`

Those are jumps into interior source labels, not restarts from the beginning.

## Side-rope image pairs

`SIDEROPE_START` is directly translated into 134 label pairs:

- `ROPE_S_R/W/B`
- `RPSBUP01..06`
- `RPSBDN01..06`
- `RPSBIN01..08`
- `RPSBOU01..08`
- `RPSS1..5_01..06`
- `RPDS1..5_01..08`
- `ROPSHAD`
- `RCSH1..5_01..05`
- `RBSH_01..07`

The runtime frames contain the actual first/second image symbols selected by
these source pairs; no procedural rope deformation is substituted.

## Priorities

Static tables retain source priorities:
- bounce/shake = 5
- sideways spring = 10
- down spring = 9

## Result

The `WmRopeProgramResolver` interface is no longer an unimplemented
integration dependency. Bind:

`wm_rope_source_program_resolver`

directly to `wm_rope_runtime_apply_resolved_command()`.
