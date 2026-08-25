# Combat2ES R32 non-SMOVE combat one-pass

- Base: fix39-v13e-combat2es-r29b-runtime-all-manifest-regression
- Scope: non-SMOVE combat core, not another SMOVE pass
- Added test: wm_combat_non_smove_core_regression
- Added audit: wm_combat_non_smove_source_reference_audit
- Runtime surfaces covered: boxes, overlap, attack hit gates, collision loop, GETUP, pin/countdown/reset, input/dtime, xflip/relative stick
- Gameplay source files changed: none
- Purpose: prove and gate the non-SMOVE combat core already present in the source-direct tree before doing deeper per-wrestler/base-combat rewrites.
