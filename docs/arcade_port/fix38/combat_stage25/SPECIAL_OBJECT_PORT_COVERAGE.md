# Special-object combat coverage — Stage 24

## Directly translated and executable

- three arcade special-object collision lists
- source insertion/deletion semantics
- special-object 3D collision-box generation including X flip
- P1-special vs P2-special collision pass
- P1/P2/neutral special-object vs wrestler collision passes
- exact collision ordering and first-hit exit behavior
- `wrestler_hit_special`
- `special_hit` for valid source IDs 0..2
- source hazard reporting for unchecked out-of-range `special_hit` IDs
- spirit hit/immobilize behavior
- reaper damage/reaction behavior
- salt damage/block/reaction behavior
- shared `hit_stuff` cleanup called with projectile-process identity
- Doink pie constructor state
- Bam Bam fireball constructor state
- Taker spirit constructor state
- Taker reaper constructor state
- Yoko salt constructor state
- recycled `SP_ID` behavior for pie/fireball
- projectile fixed-point velocity update
- source standard-bounce helper
- exact combat-state timing for reaper collision activation
- exact combat-state timing for salt grow/live/collision-disable sequence
- spirit/reaper/salt splat collision-state effects needed by combat

## Kept as exact integration seams

- renderer image objects and palette/image assignment
- complete visual-only special animation interpreter
- shadow object rendering
- offscreen/world-scroll removal
- visual debris/explosion processes

These seams must be connected to existing/ported arcade-derived N64 systems. They are not permission to approximate combat behavior.

## Next combat phase

CPU/drone combat AI (`drone_main` and its supporting decision routines).
