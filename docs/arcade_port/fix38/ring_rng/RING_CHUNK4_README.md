# WrestleMania Arcade -> N64 Ring Chunk 4

Chunk 4 completes the directly translated static rope animation corpus from
`ROPES.ASM`.

New files:
- `wmania_rope_source_data.c`
- `wmania_rope_source_data.h`
- `test_ring_chunk4.c`

Runtime change:
- script `RANI_GOTO` can now jump to a different script label, required by
  the source spring-release transitions.

Coverage:
- 70 source command programs
- complete front/back/side bounce scripts
- complete side in/out scripts
- complete sideways-spring program/script/sequence data
- complete down-spring program/script/sequence data
- both spring-release paths
- 134 side-rope image-pair labels

There is no longer a placeholder rope-animation resolver. Use the built-in
`wm_rope_source_program_resolver`.
