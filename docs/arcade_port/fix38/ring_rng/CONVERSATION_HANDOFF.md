# WrestleMania Arcade N64 port — combined conversation handoff

This directory supersedes all separate ZIPs produced earlier in this
conversation.

## Included subsystems

### Shared Williams/Midway RNG
- exact WrestleMania `RAND` state mixer
- exact TMS34010 `RL Rs,Rd` semantics
- exact `MPYU` multiply-high range scaling
- `RNDRNG0` inclusive 0..X
- `RNDRNG` inclusive lower..upper
- `RNDRNGS` inclusive -X..+X
- main-loop RAND advance hook
- HCOUNT/SP environment adapter
- high-score and attract random callers corrected

### Complete high-score system
All `wmania_hiscore_*` files:
- source factory tables
- BCD/checksum validation
- normal/special/tag insertion paths
- initials input
- reset counter
- presentation data
- persistence adapter
- merge/system hooks

### Non-gameplay attract mode
All `wmania_attract_*` files:
- exact top-level loop scheduler
- hi-score integration
- existing DCS/Midway/title callbacks
- explicit gameplay-demo no-op slots
- credits external hook
- active designer hints
- general tips
- bios and bio-tips variant
- operator message
- date/time
- copyright/AAMA
- hidden attract sequence
- visual/layout metadata

Gameplay demos remain deliberately excluded.

### Ring Chunk 1
- live `RING.EQU` geometry
- eight WRESTLE.ASM runtime boundary seeds
- rope command routing
- spring lanes/matrices
- rope priorities
- set_rope_z
- ring-relevant player state map

### Ring Chunk 2
- exact live rope object seed tables
- first/second rope halves and shadows
- BEGINOBJ source coordinate offset
- reduce_bog process behavior
- four-channel rope runtime state
- new_command_wake priority/restart semantics
- rope_update script/sequence interpreter
- front/back vs side image semantics
- source-program resolver bridge

### Ring Chunk 3
- direct `climb_turnbuckle`
- all six `ck_climb_*` source routines
- `idiot_check`
- `any_opp_outside`
- exact climb animation-label tables
- rotate/CODE_ADDR continuation semantics
- zombie top-roll path
- direct `ARE_WE_IN_RING`
- `do_ringout_dufus`
- `kill_when_hit_ground`
- ring-time transition/damage/death/disqualification logic

### Ring Chunk 4
- complete static `ROPES.ASM` animation corpus
- 70 built-in source command programs
- exact fall-through script entry points
- exact sequence/script `RANI_GOTO` behavior
- cross-script release jumps
- 134 side-rope image-pair mappings
- built-in `wm_rope_source_program_resolver`

## Still not ported by this combined package

Normal attract gameplay demos are intentionally absent.

For the ring system, the current audit still lacks source bodies for the
external routines:
- `keep_onscreen`

The previously listed `ARE_WE_IN_RING` and `ck_climb_*` routines were later
located in `SPECIAL.ASM` and `WRESTLE2.ASM` and are now directly translated
by Ring Chunk 3.

Do not replace those with inferred geometry behavior in the merge.

The static rope script/sequence corpus is now directly translated by Ring
Chunk 4 and is available through `wm_rope_source_program_resolver`.

## Recommended merge order

1. Merge `wmania_rng.c/.h` and bind translated HCOUNT/SP inputs.
2. Add `wm_rng_mainloop_step()` at the WrestleMania main-dispatch RNG hook.
3. Merge high-score files and bind `wm_rng_rndrng0_callback`.
4. Merge attract files and wire existing frontend callbacks.
5. Merge `wmania_ring_geometry.*` and `wmania_rope_command.*`.
6. Merge `wmania_rope_spawn.*` and `wmania_rope_runtime.*`.
7. Bind the current renderer's rope objects/images through
   `WmRopeRuntimeAdapter`.
8. Merge `wmania_ring_climb.*` and bind exact `get_rope_x` /
   `calc_line_x` results plus the documented out-side source-quirk read.
9. Merge `wmania_ring_out.*` and bind `adjust_health` / process events.
10. Bind the built-in `wm_rope_source_program_resolver`; no separate
    rope-script translation step remains.
11. Keep `keep_onscreen` unresolved unless a matching historical source/map
    is found; do not replace it with inferred clamp behavior.
