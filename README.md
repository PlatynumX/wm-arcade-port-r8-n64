# WrestleMania Arcade Fix39 V13e Combat2EA

Combat2EA continues Combat2DZ and replaces the invalid assumption that each WIMP palette record is a dense palette beginning at CI8 index zero. Bam Bam source frames proved that assumption false. EA derives a palette destination/base only from real words already present in the original 0x1A-byte WIMP palette directory record, and accepts it only when it uniquely covers every non-transparent source CI8 index. Pixels are never clamped or remapped and colors are never invented. The generated N64 TLUT preserves the source index positions.

# Combat2DQ

Coherent ATTRACT ABI ownership and original-source hint parity on top of Combat2DP.

# Combat2DL

Legacy-test reconciliation on top of Combat2DI build-graph convergence. No intentional gameplay change.

# Combat2CP checkpoint

ATTR.ASM SHOW_GAMEPLAY single-authority cutover: translated match runtime owns AI, animation, movement, collision and reactions; wm_demo gameplay simulation is disabled. See README_COMBAT2CO_AUDIT.md.

# WrestleMania Arcade Fix39 V13e Combat2CJ

Combat2CG keeps the complete Combat2CF ANIM.ASM VM semantics while moving the bulky generated animation program/table payload out of the resident N64 ELF and into DragonFS.

## Source-parity boundary

No ANIM.ASM opcode or wrestler-script behavior is removed to solve the N64 memory overflow. The canonical parser still produces 1,527 executable programs, 40,777 instructions, 106 used ANI command names, and 579 source tables from the historical Midway source. The runtime interpreter remains resident; per-program bytecode and source tables are serialized mechanically and loaded on demand.

## Memory-layout correction

The previous generated `wm_arcade_source_animation_program.c` materialized every instruction as pointer-heavy C structures in `.rodata`, overflowing the N64's resident memory. Combat2CG keeps only a compact label/source/path index resident and writes the command payload to `filesystem/fix39_anim/` for DragonFS.

The loader preserves the existing `wm_source_anim_program_find` / table APIs, so the translated ANIM.ASM runtime does not change semantics. A match-boundary cache reset releases animation data from the prior match without invalidating live runtime pointers mid-match.

## Validation

- Canonical source parse: 1527 programs / 40777 instructions / 106 commands / 579 tables.
- Streamed payload: approximately 1.19 MB of binary VM/table data before DragonFS packing.
- Resident generated program index C source: under 200 KB instead of the multi-megabyte pointer-expanded instruction corpus.
- Host loader round-trip tested against `hrt_stand2_anim` and source table 0.
- `-Wall -Wextra -Werror` compile of the generated streaming loader passes.
- No combat, collision, DRONE, camera, arena, or animation opcode behavior is intentionally changed from Combat2CF.


## Combat2CJ staging fix
The streamed ANIM.ASM DragonFS payload is still force-staged, but only after the primary tracked-source `git add` contract. This preserves the historical Combat2AJ staging audit while keeping `filesystem/fix39_anim` in the commit. No gameplay or VM semantics changed.

## Combat2CV
Late commit/staging guard updated for the post-Combat2CO single-authority architecture: the demo roster API may remain for standalone tests, but `src/core/app.c` is now forbidden from consuming `wm_demo_set_roster(&app->demo, ...)`. This removes the stale Combat2AJ contradiction without weakening the final generated-tree ownership audit.

## Combat2CW recovery fix
Combat2CW keeps the Combat2CU/CV gameplay and renderer intent unchanged. CV's runtime parity patcher assumed the N64 torso fallback still used the older `wm_bret_sprite_find()` text, but `apply_fix39.py` had already converted the integrated tree to the roster-aware `wm_character_sprite_find()` form. That stale exact-text assumption made the installer stop before compilation. CW locates the torso declaration structurally, accepts both forms, preserves the roster-aware fallback, and is idempotent. A regression test reproduces the CV failure form before the real build begins.

## Combat2DB
Hardens the WRESTLE.ASM execute_walk source-parity regression so it ignores formatting-only C whitespace. The DA run reached the real CZ walk patch successfully, then stopped because the legacy BO test expected spaced expressions such as `xv * 230` while CZ emits equivalent `xv*230`. DB fixes both BO/BN copies of that regression and changes no gameplay behavior from CZ/DA.

## Combat2DA
Repairs a CZ package-preflight self-contradiction before build: the late-staging regression no longer hard-codes the previous revision prefix and instead checks the actual ATTR single-authority invariant. Also fixes current work/log/branch/ROM artifact naming to Combat2DA. No gameplay changes from CZ.

Combat2DI: host/N64 build-graph convergence. Every src/fix39 module now owns its basename over any stale src/core/arcade copy in both CMake and the N64 Makefile. Adds a real-tree ownership audit to Termux and GitHub Actions. No new arcade gameplay approximation is introduced.


## Combat2DL semantic build-graph audit fix
The host/N64 source-authority verifier now validates the generated build graphs themselves rather than requiring a historical comment marker. Every bundled src/fix39 C module must be active in both CMake and the N64 Makefile; overlapping src/core/arcade owners must be absent; duplicate ownership is rejected; and C_FILES must actually consume $(FIX39_C). A regression strips all integration marker comments and proves the semantic audit still passes, then reintroduces a stale owner and proves it fails.

## Combat2DM
See README_COMBAT2DM_SELECTIVE_OWNERSHIP.md for the selective ownership correction.

## Combat2DN
See `README_COMBAT2DN_DEPENDENCY_CLOSED_OWNERSHIP.md` for the dependency-closed ownership correction and arcade-source comparison.

## Combat2DP
Legacy ownership tests reconciled to the Combat2DN dependency-closed source graph. No intentional gameplay change from DN. See `README_COMBAT2DO_LEGACY_OWNERSHIP_RECONCILIATION.md`.

## Combat2DP update
See `README_COMBAT2DP_FULL_HOST_LINK_CLOSURE.md`. DP closes the five ATTRACT providers exposed by the DN full host link and adds a generated-tree host link-surface audit before CMake.

## Combat2DT runtime identity chain
See `README_COMBAT2DR_RUNTIME_IDENTITY_CHAIN.md`. DQ ownership is retained; ATTR source IDs now survive fail-closed through actor creation and N64 streamed rendering, with DRONE executable-service coverage reported explicitly.


## Combat2DT
See `README_COMBAT2DT_DRONE_VISUAL_PARITY.md`.

## Combat2DT
See `README_COMBAT2DT_ACTOR_VISUAL_PARITY.md`. DT uses each live WIMP frame's own TLUT, validates CI8/palette round-trip constraints during generation, and adds independent P1/P2 DRONE/input/position liveness counters for the asymmetric attract failure seen on hardware.

## Combat2DV
See `README_COMBAT2DV_SPORTS_PREFLIGHT_CLEANUP.md`. DU removes a stale duplicated Sports companion checksum that blocked DT before integration; gameplay/runtime behavior is unchanged from DT.

## Combat2DW
WIMP wrestler palettes are reconstructed as CI8-visible source banks from consecutive real palette-directory fragments (up to 256 colors), rather than assuming each individual directory record is a complete TLUT. See `README_COMBAT2DW_WIMP_PALETTE_BANK_RECONSTRUCTION.md`.


## Combat2DY

Reconciles post-integration regression tests with the actual package-vs-worktree boundary. Tests invoked with `$WORK` now validate generated/integrated outputs instead of assuming package-only patcher sources were copied into the clone. Adds a fail-fast ROOT/WORK path-contract audit. No gameplay, ownership, DRONE, or WIMP behavior changes from Combat2DW.


Combat2DY adds strict source-runtime parity enforcement; see README_COMBAT2DY_STRICT_RUNTIME_PARITY.md.


## Combat2DZ

Combat2DZ keeps Combat2DY runtime behavior unchanged and repairs the post-integration DRONE seek regression so it validates the active source-derived implementation and host/N64 build ownership rather than historical comment wording. It also adds a meta-regression that rejects prose/comment archaeology as a negative assertion in tests executed against `$WORK`. See `README_COMBAT2DZ_STRUCTURAL_DRONE_SEEK_TEST.md`.

## Combat2EC
Adds image-local WIMP CI8 index-bias handling for the source-observed 1..N convention while preserving fail-closed behavior for unexplained wider ranges. See `README_COMBAT2EB_WIMP_IMAGE_INDEX_BIAS.md`.

## Combat2EC strict source-proof gate
Combat2EC removes the EB one-based WIMP pixel remap from the live converter and forbids inferred WIMP palette windows/banks from being promoted to gameplay behavior without original Midway proof. See `README_COMBAT2EC_SOURCE_PROOF_LEDGER.md`.
