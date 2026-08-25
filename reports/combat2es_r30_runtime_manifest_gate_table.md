# Combat2ES R30 runtime manifest gate table cleanup

- Base: fix39-v13e-combat2es-r29b-runtime-all-manifest-regression
- Scope: test-only; no gameplay source changes
- Replaces substring-driven all-manifest precondition guessing with an explicit 63-entry manifest gate table.
- Removes the yok_salt_throw one-off heuristic while preserving the correct HEADHOLD gate state.
- Keeps R29B's intent: every strict manifest entry must drive through the live scheduler to a queued source result.
