# Combat2ES R33B whole combat source-parity cutover

- Base: fix39-v13e-combat2es-r32-non-smove-combat-onepass
- Scope: whole live combat spine, not another SMOVE-only/test-only package
- Gameplay source files changed: yes
- Main change: current source animation frames now bind the generated historical ASM attack-frame corpus into the live actor attack window before COLLIS/REACT1.
- New CTest: wm_combat2es_whole_combat_source_cutover
- New audit: wm_combat_whole_source_parity_audit
- Keeps required prior guards: strict SMOVE source-complete, behavioral parity, non-SMOVE source-reference audit.

This pass is intended to stop treating ordinary attack windows as presenter/demo metadata. The live N64 actor state now receives source-generated attack windows directly from current animation frames, then runs through the existing COLLIS/REACT1/SPECIAL collision path.
