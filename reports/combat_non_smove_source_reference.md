# Non-SMOVE combat source-reference report

## Scope

This gate is intentionally outside the Combat2ES SMOVE scheduler. It covers combat-facing core services that the rest of the port depends on:

- collision boxes and overlap resolution
- attack hit accept/reject gates and first-hit collision loop
- GETUP timing and maybe-gidd-up callback
- WRESTLE core pin/countdown/reset state
- WRESTLE input history and dtime counters
- movement xflip/relative stick helpers
- remaining PLYR.EQU-style combat actor fields

## Result

- status: PASS
- non_smove_core_runtime_regression: present
- non_smove_source_reference_audit: present
- source_semantic_markers: present
- remaining_actor_state_fields: present

## Findings

- none
