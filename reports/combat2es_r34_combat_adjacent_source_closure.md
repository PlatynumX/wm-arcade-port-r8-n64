# Combat2ES R34 combat-adjacent source-parity closure

- Base: fix39-v13e-combat2es-r33b-whole-combat-source-cutover
- Scope: combat-adjacent systems feeding/reacting to the R33B combat spine
- Gameplay source files changed: yes
- Main gameplay change: SPECIAL process spawn labels now use an exact source-label table instead of substring classification.
- New CTest: wm_combat_adjacent_source_closure
- New audit: wm_combat_adjacent_source_parity_audit
- Required prior guards retained: strict SMOVE, behavioral parity, non-SMOVE source-reference, whole-combat source-parity, whole cutover regression, all-manifest SMOVE regression.

## Covered systems

- DRONE CPU brain feeding live WRESTLE input
- SPECIAL object spawn/tick/velocity/bounce/collision inclusion
- ROPES/RING-OUT/keep-onscreen combat constraints
- ANI_CODE combat callback surface and source animation VM command execution
- finish/match-result combat hooks remain guarded through the already-live match lifecycle/source plans

## Findings

- none
