# Combat2DP — full host-link closure

Combat2DP is Combat2DO plus the dependency closure exposed by the Combat2DN real host link.

The converged `wm_fix39_tests` executable required five ATTRACT services that were still excluded by the selective ownership policy: `wm_attract_hint_placements`, `wm_attract_general_tip_placements`, `wm_attract_time_date_placements`, `wm_attract_operator_placements`, and `wm_attract_operator_copy_line`.

Their implementations already exist in the bundled source-backed Fix39 ATTRACT translation. The placement routines follow the audited ATTRACT.ASM coordinates/order for designer hints, general tips, operator rows, and time/date rows; the operator-copy routine follows the source CUSTOM_MESSAGE row semantics (first-byte presence test and NUL-terminated row copy). Combat2DP therefore adds `wmania_attract_visuals.c` and `wmania_attract_operator.c` to the dependency-closed Fix39 provider set while retaining the newer core attract adapter, roster, high-score, rope, and remaining presenter modules.

A new host link-surface audit runs on the fully integrated tree before CMake. It parses `wm_*` calls made by `tests/fix39_smoke.c` and fails if no active C provider exists in the CMake graph. This is specifically designed to catch the exact class of failure seen in Combat2DN before the real host link.

The real CMake build and CTest remain mandatory in the Termux workflow. Static audits are preflight only, not proof that the integrated build links.
