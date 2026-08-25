# Combat-adjacent source-parity report

- status: PASS
- scope: DRONE + SPECIAL + ropes/ring-out/keep-onscreen + ANI_CODE combat callbacks
- gameplay source changed: yes
- DRONE live CPU brain path: present and regression-covered
- SPECIAL object lifecycle/collision: present and regression-covered
- SPECIAL process spawn classification: exact source-label table; substring classifier removed
- rope and ring-out processes: live and regression-covered
- ANI_CODE combat callbacks and source animation VM command surface: present
- prior R33B whole-combat spine guard remains required

## Covered adjacent categories

- DRONE.ASM generated CPU scripts/tables/services feeding WRESTLE input
- SPECIAL.ASM spawn/tick/velocity/bounce/collision inclusion
- ROPES.ASM process runtime and ring-out/keep-onscreen combat constraints
- ANIM.ASM/native ANI_CODE combat callbacks that set target, velocities, attach, pinable, damage/reaction side effects
- live runtime counters proving these systems execute from match tick

## Findings

- none
