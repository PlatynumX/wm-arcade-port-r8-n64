# Combat2DA — CZ preflight self-consistency repair

Input baseline: Combat2CZ.

CZ's gameplay/runtime patch was not reached in the reported Termux run. The run stopped at the early package regression `test_combat2cv_late_staging_single_authority.py`, whose assertion still hard-coded the previous Combat2CY diagnostic even though the build script had advanced to Combat2CZ. This was packaging self-inconsistency, not a new combat failure.

Combat2DA removes the revision-sensitive assertion: the regression now verifies the semantic single-authority failure text instead of a Combat2XX prefix. This prevents the same stale-revision failure on future package renames. DA also corrects the generated Termux work/log/branch and downloaded ROM identity to Combat2DA; CZ had still emitted the ROM filename as Combat2CX.

No gameplay behavior is intentionally changed from Combat2CZ. The CZ WRESTLE.ASM walk/stance/turn animation wiring and independent primary/torso animation-channel patch remain intact and are still covered by `test_combat2cz_walk_and_dual_anim.py`.
