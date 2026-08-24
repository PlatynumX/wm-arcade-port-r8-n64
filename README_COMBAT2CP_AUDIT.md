# Combat2CP — generated-tree ownership audit ordering fix

Combat2CP preserves Combat2CO's ATTR.ASM single-authority cutover and fixes the build-pipeline ordering bug exposed by the CO hardware build attempt.

## Correction

`tools/fix39_attract_ownership_audit.py "$WORK"` no longer runs during the early source-fact preflight, before `$WORK/src/platform/n64/main.c` and `$WORK/src/fix39/wm_fix39_runtime.c` have been materialized.

The same strict audit now runs immediately after:

1. `apply_fix39.py` has generated/patched the work tree,
2. `fix39_combat_completion_patch.py`,
3. `fix39_combat_source_audit_patch.py`, and
4. `fix39_native_code_callbacks_patch.py`.

At that location, missing generated N64/runtime files remains a hard failure, as does any forbidden `wm_demo`/presenter gameplay ownership. The audit itself is not weakened.
