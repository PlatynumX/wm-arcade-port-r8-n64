# Combat2ES R29B runtime all-manifest regression

- Base: fix39-v13e-combat2es-r28-runtime-timing-regression
- Adds/fixes: wm_combat2es_runtime_all_manifest_regression
- Scope: test-only; no gameplay source changes
- Fix: drives yok_salt_throw under HEADHOLD gate state because the runtime source body requires HEADHOLD/HEADHELD even though the label is not spelled hdhold.
- Coverage: iterates all 63 strict SMOVE manifest entries and drives each one through the live scheduler to a queued source result.
