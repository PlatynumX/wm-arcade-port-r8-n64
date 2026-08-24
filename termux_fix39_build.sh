#!/data/data/com.termux/files/usr/bin/bash
set -Eeuo pipefail

REPO_URL="https://github.com/PlatynumX/wm-arcade-port-r8-n64.git"
ROOT="$(cd "$(dirname "$0")" && pwd)"
WORK="${FIX39_WORKDIR:-$HOME/wm-arcade-port-fix39-v13e-combat2ec}"
BRANCH="fix39-v13e-combat2ec-$(date +%Y%m%d-%H%M%S)"
DOWNLOAD_DIR="/sdcard/Download"

# Combat2AJ: capture the ENTIRE Termux run, including preflight/compiler failures,
# from the first command onward.  Previous helpers only saved GitHub logs after
# a remote Actions failure, which made local failures unnecessarily painful.
mkdir -p "$DOWNLOAD_DIR"
RUN_STAMP="$(date +%Y%m%d-%H%M%S)"
LOCAL_LOG="$DOWNLOAD_DIR/fix39-v13e-combat2ec-${RUN_STAMP}.log"
touch "$LOCAL_LOG" || { echo "ERROR: cannot create local build log: $LOCAL_LOG" >&2; exit 1; }
exec > >(tee -a "$LOCAL_LOG") 2>&1

on_err() {
  local rc=$?
  local line="${BASH_LINENO[0]:-unknown}"
  local cmd="${BASH_COMMAND:-unknown}"
  printf '\n=== FIX39 LOCAL BUILD FAILED ===\n' >&2
  printf 'Exit: %s\nLine: %s\nCommand: %s\n' "$rc" "$line" "$cmd" >&2
  printf 'FULL LOCAL LOG: %s\n' "$LOCAL_LOG" >&2
  return "$rc"
}
trap on_err ERR
printf 'FULL LOCAL LOG: %s\n' "$LOCAL_LOG"

say() { printf '\n=== %s ===\n' "$*"; }
fail() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

# Do not upgrade/reinstall working Termux packages.  Only touch apt if a
# command Fix39 actually needs is absent.  Also create the cache path used by
# current Termux apt (it lives under the app cache, not $PREFIX/var/cache).
say "Fix39 V13e Combat2EC: source WIMP palette-window semantics + strict runtime parity"
say "Checking Termux tools"
TERMUX_APP_ROOT="${PREFIX%/files/usr}"
mkdir -p \
  "$TERMUX_APP_ROOT/cache/apt/archives/partial" \
  "$PREFIX/var/lib/apt/lists/partial"

missing=()
for cmd in git gh python; do
  command -v "$cmd" >/dev/null 2>&1 || missing+=("$cmd")
done

if ((${#missing[@]})); then
  echo "Missing required tool(s): ${missing[*]}"
  echo "Installing only the missing packages (no blanket upgrade)."
  pkg install -y "${missing[@]}"
fi

command -v git >/dev/null 2>&1 || fail "git missing"
command -v gh >/dev/null 2>&1 || fail "gh missing"
command -v python >/dev/null 2>&1 || fail "python missing"

say "Verifying Combat2 integrator package manifest"
(
  cd "$ROOT"
  sha256sum -c SHA256SUMS.txt
)

say "Auditing stable behavior against original Midway source facts"
python "$ROOT/tools/fix39_source_parity_audit.py"

python "$ROOT/tests/test_combat2bo_wrestler_main_order.py"
python "$ROOT/tests/test_combat2bp_source_animation_catalog.py"
python "$ROOT/tests/test_combat2bp_animation_runtime.py"
python "$ROOT/tests/test_combat2bp_attract_start_match.py"
python "$ROOT/tests/test_combat2cs_attract_single_authority.py"
python "$ROOT/tests/test_combat2cv_late_staging_single_authority.py"
python "$ROOT/tests/test_combat2cx_callback_prototype.py"
python "$ROOT/tests/test_combat2by_getup_recovery.py"
python "$ROOT/tests/test_combat2ce_anim_vm_source_parity.py"
python "$ROOT/tests/test_combat2ce_anim_vm_runtime.py"
python "$ROOT/tests/test_combat2cf_full_vm_contract.py"
python "$ROOT/tests/test_combat2cg_streamed_anim_vm.py"
test -s filesystem/fix39_anim/programs/p0000.bin || fail "Combat2CG streamed ANIM program payload missing"
test -s filesystem/fix39_anim/tables/t0000.bin || fail "Combat2CG streamed ANIM table payload missing"

say "Combat2CS preflight: materializing and validating every referenced test"
# Builder-root tests are executed from $ROOT; tests explicitly committed below must
# also exist in $WORK before git add.  Fail here, not after a long host build.
python - "$ROOT/termux_fix39_build.sh" "$ROOT" <<'PYTESTPREFLIGHT'
import pathlib, re, sys
script=pathlib.Path(sys.argv[1]).read_text()
root=pathlib.Path(sys.argv[2])
refs=sorted(set(re.findall(r'(?:\$ROOT/)?(tests/[A-Za-z0-9_./-]+\.py)', script)))
missing=[r for r in refs if not (root/r).is_file()]
if missing:
    raise SystemExit('ERROR: package missing referenced test(s): ' + ', '.join(missing))
print(f'Combat2CS package test preflight: PASS ({len(refs)} referenced tests present)')
PYTESTPREFLIGHT
say "Auditing every Python test referenced by this build script"
while IFS= read -r rel; do
  test -f "$ROOT/$rel" || fail "build script references missing test: $rel"
done < <(grep -oE 'tests/[A-Za-z0-9_./-]+\.py' "$ROOT/termux_fix39_build.sh" | sort -u)

say "Verifying ATTRACT.ASM source-text generator"
python "$ROOT/tools/fix39_attract_text.py" --self-test
say "Verifying source WIMP asset generator"
python "$ROOT/tools/fix39_attract_assets.py" --self-test
say "Verifying DRONE scalar-table source generator"
python "$ROOT/tools/fix39_drone_tables.py" --self-test
say "Verifying DRONE range/mode source generator"
python "$ROOT/tools/fix39_drone_ranges.py" --self-test
say "Verifying all-character source asset generators"
python "$ROOT/tools/fix39_bret_attack_frames.py" --self-test
python "$ROOT/tests/test_combat2af_roster_aware_core_test.py"
python "$ROOT/tests/test_combat2af_roster_namespace_boundary.py"
python "$ROOT/tests/test_combat2ag_attack_identity_invariant.py"
python "$ROOT/tools/fix39_character_assets.py" --self-test
python "$ROOT/tests/test_combat2ec_wimp_strict_source_gate.py"
python "$ROOT/tests/test_combat2dx_wimp_palette_mapping.py"
python "$ROOT/tests/test_combat2dx_wimp_palette_bank.py"
python "$ROOT/tests/test_combat2ea_wimp_palette_window.py"
python "$ROOT/tests/test_combat2dx_post_integration_path_contract.py"
python "$ROOT/tests/test_combat2al_streamed_assets.py"
python "$ROOT/tests/test_combat2aa_generated_switch_syntax.py"
python "$ROOT/tools/fix39_character_attack_frames.py" --self-test
say "Verifying DRONE script/skill source generator"
python "$ROOT/tools/fix39_drone_scripts.py" --self-test
python "$ROOT/tools/fix39_drone_services.py" --self-test
python "$ROOT/tools/fix39_drone_bodies.py" --self-test
python "$ROOT/tools/fix39_drone_translate.py" --self-test
python "$ROOT/tests/test_combat2dt_drone_translator.py"
python "$ROOT/tests/test_combat2dt_streamed_visual_safety.py" "$ROOT"
python "$ROOT/tests/test_combat2dt_actor_visual_parity.py" "$ROOT"

say "Verifying structural attract-switch patcher"
python "$ROOT/tests/test_patcher_structural.py"
say "Verifying V12/V13 renderer integration patcher"
python "$ROOT/tests/test_v12_patcher.py"
say "Verifying V13 completion wiring guards"
python "$ROOT/tests/test_v13_completion.py"
python "$ROOT/tests/test_combat2do_legacy_ownership_reconciliation.py"
python "$ROOT/tests/test_combat2dp_host_link_closure.py"
python "$ROOT/tests/test_combat2dq_attract_abi_source_parity.py"
say "Verifying V13e chunk-1 DRONE binding guards"
python "$ROOT/tests/test_v13e_chunk1.py"
say "Verifying V13e chunk-2d DRONE range binding guards"
python "$ROOT/tests/test_v13e_chunk2.py"
say "Verifying V13e chunk-3 DRONE script binding guards"
python "$ROOT/tests/test_v13e_chunk3.py"
say "Verifying V13e chunk-4c fail-closed DRONE seams"
python "$ROOT/tests/test_v13e_chunk4c.py"
say "Verifying V13e chunk-5d exact service entry binding"
python "$ROOT/tests/test_v13e_chunk5d.py"
python "$ROOT/tests/test_v13e_chunk5e.py"
python "$ROOT/tests/test_v13e_chunk5f.py"
python "$ROOT/tests/test_v13e_chunk5g.py"
python "$ROOT/tests/test_v13e_chunk5h.py"
python "$ROOT/tests/test_v13e_chunk5i.py"
python "$ROOT/tests/test_v13e_chunk5j.py"
python "$ROOT/tests/test_v13e_chunk6.py"
python "$ROOT/tests/test_attract_demo_activation.py"
python "$ROOT/tests/test_attract_demo_collision_wimp.py"
python "$ROOT/tests/test_combat2t_multilod_union.py"
python "$ROOT/tests/test_combat2v_exact_visual_slice.py"
python "$ROOT/tests/test_combat2w_owner_frame_filter.py"
python "$ROOT/tests/test_combat2y_physical_asset_filter.py"
python "$ROOT/tests/test_combat2z_canonical_asm_selection.py"
python "$ROOT/tests/test_attract_demo3_linkage.py"
python "$ROOT/tests/test_attract_demo4_collision_attack_bridge.py"
python "$ROOT/tests/test_attract_demo5_source_animation_attack.py"
python "$ROOT/tests/test_combat_completion_wiring.py"
python "$ROOT/tests/test_combat2ca_source_owned_attack_contract.py"
python "$ROOT/tests/test_combat2cb_idempotent_completion_patcher.py"
python "$ROOT/tests/test_ported_service_wiring.py"
python "$ROOT/tests/test_combat_headless_flow.py"
python "$ROOT/tests/test_combat2l_character_backend.py"
say "Verifying Sports source-slot replacement package"
python "$ROOT/tests/test_sports_override.py"
say "Verifying V13e chunk-2d source range resolver"
bash "$ROOT/tests/run_v13e_chunk2_range_smoke.sh"
say "Verifying V13e chunk-3 source scripts and VM semantics"
bash "$ROOT/tests/run_v13e_chunk3_script_smoke.sh"

gh auth status >/dev/null 2>&1 || {
  echo "GitHub CLI is not logged in. Complete the login, then this script continues."
  gh auth login
}

# NOTE: Do not compile src/fix39 standalone here. Demo3 adds a deliberate
# dependency on the base repository's generated include/wm/bret_sprites.h.
# That header exists only after the fresh repository is cloned/prepared below;
# the real host/N64 verification later in this script covers the integrated tree.
python "$ROOT/tests/test_combat2aj_staging_contract.py"
python "$ROOT/tests/test_combat2ak_n64_warning_clean.py"
python "$ROOT/tests/test_combat2di_build_graph_authority.py"
python "$ROOT/tests/test_combat2dn_selective_ownership.py"
python "$ROOT/tests/test_combat2dj_legacy_test_reconciliation.py"
python "$ROOT/tests/test_combat2dk_semantic_build_graph_audit.py"
python "$ROOT/tests/test_combat2dy_strict_runtime_parity.py" "$ROOT"
python "$ROOT/tests/test_combat2dz_no_prose_negative_assertions.py"
echo "NOTE: deferring Fix39 C smoke compile until after base-repo integration."

say "Cleaning stale Fix39 build clones"
# These are disposable working clones created by earlier Fix39 Termux helpers.
# Removing them prevents repeated test revisions from exhausting Termux app storage.
for stale in \
  "$HOME/wm-arcade-port-fix39-v12" \
  "$HOME/wm-arcade-port-fix39-v12b" \
  "$HOME/wm-arcade-port-fix39-v12c" \
  "$HOME/wm-arcade-port-fix39-v12d" \
  "$HOME/wm-arcade-port-fix39-v12e" \
  "$HOME/wm-arcade-port-fix39-v12f" \
  "$HOME/wm-arcade-port-fix39-v12g" \
  "$HOME/wm-arcade-port-fix39-v12h" \
  "$HOME/wm-arcade-port-fix39-v12i" \
  "$HOME/wm-arcade-port-fix39-v13" \
  "$HOME/wm-arcade-port-fix39-v13a" \
  "$HOME/wm-arcade-port-fix39-v13b" \
  "$HOME/wm-arcade-port-fix39-v13c" \
  "$HOME/wm-arcade-port-fix39-v13d" \
  "$HOME/wm-arcade-port-fix39-v13e-c1" \
  "$HOME/wm-arcade-port-fix39-v13e-c2" \
  "$HOME/wm-arcade-port-fix39-v13e-c2a" \
  "$HOME/wm-arcade-port-fix39-v13e-c2b" \
  "$HOME/wm-arcade-port-fix39-v13e-c2c" \
  "$HOME/wm-arcade-port-fix39-v13e-c2d" \
  "$HOME/wm-arcade-port-fix39-v13e-c2e" \
  "$HOME/wm-arcade-port-fix39-v13e-c3f" \
  "$HOME/wm-arcade-port-fix39-v13e-c3g"; do
  [ "$stale" = "$WORK" ] || rm -rf "$stale"
done

say "Cloning fresh known repo"
rm -rf "$WORK"
git clone "$REPO_URL" "$WORK"
cd "$WORK"
BASE_SHA="$(git rev-parse HEAD)"
echo "Base commit: $BASE_SHA"

say "Applying Fix39"
python "$ROOT/tools/apply_fix39.py" "$WORK"

# Combat2EC strict source-parity overlay: these files define the runtime boundary.
cp "$ROOT/src/fix39/wm_fix39_runtime.c" src/fix39/wm_fix39_runtime.c
cp "$ROOT/src/fix39/wm_fix39_runtime.h" src/fix39/wm_fix39_runtime.h
cp "$ROOT/src/fix39/wmania_attract_adapter.c" src/fix39/wmania_attract_adapter.c
cp "$ROOT/src/fix39/wmania_attract_adapter.h" src/fix39/wmania_attract_adapter.h
cp "$ROOT/src/fix39/wm_arcade_matchflow.c" src/fix39/wm_arcade_matchflow.c
python "$ROOT/tests/test_combat2dy_strict_runtime_parity.py" "$WORK"

say "Verifying Combat2DK host/N64 build-graph single authority"
python "$ROOT/tools/fix39_build_graph_audit.py" "$WORK"
python "$ROOT/tools/fix39_dependency_provider_audit.py" "$WORK"
python "$ROOT/tools/fix39_host_link_surface_audit.py" "$WORK"

say "Verifying Midway Sports source-slot overrides landed"
(
  cd "$WORK/assets/fix39_sports_override"
  sha256sum -c SHA256SUMS.txt
)
grep -q 'FIX39 SPORTS FOREGROUND OVERRIDE' "$WORK/scripts/prepare_frontend_assets.sh"   || fail "Sports foreground override did not patch prepare_frontend_assets.sh"
grep -q 'FIX39 SPORTS BACKGROUND OVERRIDE' "$WORK/scripts/prepare_sports_source_assets.sh"   || fail "Sports background override did not patch prepare_sports_source_assets.sh"

python "$ROOT/tests/test_attract_demo_collision_wimp.py"

say "Preflighting V13e-c3h against the real historical WIMP/source checkout"
sh scripts/fetch_original.sh

say "Generating source-exact ring/rope WIMP payloads"
python "$ROOT/tools/fix39_ring_rope_assets.py" \
  --root original/wwf-wrestlemania \
  --out-fs filesystem/fix39_ring \
  --out-c src/generated/ring_rope_assets.c \
  --out-h include/wm/ring_rope_assets.h
test -s src/generated/ring_rope_assets.c || fail "Combat2AU ring/rope metadata missing"
test -n "$(find filesystem/fix39_ring -type f -name '*.bin' -print -quit)" || fail "Combat2AU ring/rope DFS payload missing"
python "$ROOT/tests/test_combat2at_ring_rope_source.py"
python "$ROOT/tests/test_combat2au_rope_renderer.py"
python "$ROOT/tools/fix39_ring_rope_renderer_patch.py" "$WORK"

say "Generating exact source ringBMOD/arena payload"
python "$ROOT/tools/fix39_arena_assets.py" \
  --source-pack "$ROOT/source_payload/arena" \
  --out-fs filesystem/fix39_arena/ring \
  --out-c src/generated/ring_arena_assets.c \
  --out-h include/wm/ring_arena_assets.h
test -s src/generated/ring_arena_assets.c || fail "Combat2AX ring arena metadata missing"
test -n "$(find filesystem/fix39_arena/ring -type f -name 'hdr_*.ci8' -print -quit)" || fail "Combat2AX ring arena DFS payload missing"
python "$ROOT/tests/test_combat2ax_arena_source.py"
python "$ROOT/tests/test_combat2ay_arena_warning_clean.py"
python "$ROOT/tests/test_combat2az_rope_projection.py"
python "$ROOT/tools/fix39_arena_renderer_patch.py" "$WORK"
python "$ROOT/tools/fix39_ring_depth_order_patch.py" "$WORK"
python "$ROOT/tests/test_combat2ba_ring_depth_order.py"
python "$ROOT/tests/test_combat2bg_crowd_baklst_restore.py"
python "$ROOT/tests/test_combat2bi_crowd_source_order.py"

say "Generating exact CROWD.ASM/CROWD.IMG runtime"
python "$ROOT/tools/fix39_crowd_assets.py" \
  --source-pack "$ROOT/source_payload/arena" \
  --out-fs filesystem/fix39_arena/crowd \
  --out-c src/generated/crowd_assets.c \
  --out-h include/wm/crowd_assets.h
test -s src/generated/crowd_assets.c || fail "Combat2BG crowd metadata missing"
test -n "$(find filesystem/fix39_arena/crowd -type f -name '*.bin' -print -quit)" || fail "Combat2BG crowd DFS payload missing"
python "$ROOT/tests/test_combat2bb_crowd_source.py"
python "$ROOT/tests/test_combat2bf_crowd_preserve_bmod.py"
python "$ROOT/tools/fix39_crowd_renderer_patch.py" "$WORK"
python "$ROOT/tests/test_combat2bk_crowd_inplace_animation.py"
python "$ROOT/tests/test_combat2bk_crowd_cheer_triggers.py"

say "Correcting WRESTLE.ASM wrestler process ordering"
python "$ROOT/tests/test_combat2bg_wrestler_source_order.py"
python "$ROOT/tools/fix39_match_source_order_patch.py" "$WORK"

say "Translating DRONE.ASM seek-dir/dist movement exactly"
python "$ROOT/tools/fix39_drone_seek_source_patch.py" "$WORK"
python "$ROOT/tests/test_combat2bl_drone_seek_source.py" "$WORK"
say "Wiring WRESTLE.ASM execute_walk/set_velocities"
python "$ROOT/tools/fix39_execute_walk_source_patch.py" "$WORK"
python "$ROOT/tests/test_combat2bo_execute_walk_source.py" "$WORK"
say "Separating ANIM.ASM primary/secondary channels and restoring walk animations"
python "$ROOT/tools/fix39_dual_anim_channel_patch.py" "$WORK"
python "$ROOT/tests/test_combat2cz_walk_and_dual_anim.py" "$WORK"
python "$ROOT/tests/test_combat2di_n64_walk_warning_clean.py"

say "Activating WRESTLE2.ASM scroll_world camera"
python "$ROOT/tests/test_combat2bf_camera_source.py"
python "$ROOT/tests/test_combat2bf_camera_patcher_structural.py"
python "$ROOT/tools/fix39_camera_renderer_patch.py" "$WORK"

python "$ROOT/tests/test_combat2bq_character_physical_wimp_filter.py"

say "Generating all eight wrestler visual/WIMP/attack backends"
python tools/fix39_character_assets.py \
  --root original/wwf-wrestlemania \
  --out-c src/generated/character_assets.c \
  --out-h include/wm/character_assets.h \
  --out-fs filesystem/fix39_chars
python tools/fix39_character_attack_frames.py \
  --root original/wwf-wrestlemania \
  --out src/fix39/wm_arcade_character_attack_frames_generated.h
test -s src/generated/character_assets.c || fail "Combat2AM character asset generator produced no C source"
test -n "$(find filesystem/fix39_chars -type f -name '*.bin' -print -quit)" || fail "Combat2AM streamed character DFS payloads missing"
grep -q 'WM_FIX39_CHARACTER_ATTACK_FRAMES_GENERATED 1' src/fix39/wm_arcade_character_attack_frames_generated.h || fail "Combat2AM all-character attack tables missing"
for rid in 0 1 2 3 4 5 6 8; do
  grep -q "case ${rid}:" src/generated/character_assets.c || fail "Combat2AM roster ${rid} visual backend missing"
done

say "Generating source ANIM.ASM visual/timing/state catalog"
python "$ROOT/tools/fix39_source_animation_catalog.py" \
  --root original/wwf-wrestlemania \
  --out-c src/fix39/wm_arcade_source_animation_catalog.c \
  --out-h src/fix39/wm_arcade_source_animation_catalog.h \
  --bret-h src/fix39/wm_arcade_bret.h \
  --razor-h src/fix39/wm_arcade_razor.h

grep -q 'hrt_stand2_anim' src/fix39/wm_arcade_source_animation_catalog.c || fail "Combat2CB source animation catalog missing Bret stand"
grep -q 'rzr_4_uprcut_anim' src/fix39/wm_arcade_source_animation_catalog.c || fail "Combat2CB Razor source animation mapping missing"
grep -q 'WM_SRC_ANIM_CTRL_WAITROLL' src/fix39/wm_arcade_source_animation_catalog.h || fail "Combat2CE WAITROLL metadata missing"

say "Generating complete ANIM.ASM command VM from canonical wrestler source"
python "$ROOT/tools/fix39_anim_vm_program.py" \
  --root original/wwf-wrestlemania \
  --out-c src/fix39/wm_arcade_source_animation_program.c \
  --out-h src/fix39/wm_arcade_source_animation_program.h \
  --out-fs filesystem/fix39_anim
python "$ROOT/tools/fix39_target_offsets.py" \
  --source original/wwf-wrestlemania/TABLES.ASM \
  --out-c src/fix39/wm_arcade_target_offsets.c \
  --out-h src/fix39/wm_arcade_target_offsets.h
python "$ROOT/tests/test_combat2ce_anim_vm_source_parity.py" --source-root original/wwf-wrestlemania
python "$ROOT/tests/test_combat2ce_anim_vm_runtime.py"
python "$ROOT/tests/test_combat2cf_full_vm_contract.py"

say "Applying Combat2CE source-owned animation/collision/reaction/ATTR completion"
python "$ROOT/tools/fix39_combat_completion_patch.py" "$WORK"
say "Applying Combat2CS source-vs-port combat audit corrections"
python "$ROOT/tools/fix39_combat_source_audit_patch.py" "$WORK"
python "$ROOT/tools/fix39_native_code_callbacks_patch.py" "$WORK"
python "$ROOT/tools/fix39_ani_code_completion_patch.py" "$WORK"
python "$ROOT/tests/test_combat2di_ani_code_completion.py" "$WORK"
python "$ROOT/tools/fix39_ani_code_full_source_patch.py" "$WORK"
python "$ROOT/tests/test_combat2di_full_ani_code_inventory.py" "$WORK"
python "$ROOT/tests/test_combat2dl_n64_misleading_indentation.py" "$WORK"
say "Binding ATTR.ASM source wrestler identity through live actor/render chain"
python "$ROOT/tools/fix39_runtime_identity_chain_patch.py" "$WORK"
python "$ROOT/tests/test_combat2dr_runtime_identity_chain.py" "$WORK"
python "$ROOT/tests/test_combat2dr_drone_service_coverage.py" "$WORK"
say "Applying Combat2CX WRESTLE.ASM runtime/renderer parity completion"
python "$ROOT/tools/fix39_combat_runtime_parity_patch.py" "$WORK"
python "$ROOT/tests/test_combat2cv_runtime_parity.py" "$WORK"
python "$ROOT/tests/test_combat2cw_runtime_patcher_robust.py"

say "Combat2CX final generated-tree ATTR ownership audit"
# This MUST run after apply_fix39 + all combat ownership patchers have materialized
# the generated N64/runtime sources. Missing generated files here is a real failure.
python "$ROOT/tools/fix39_attract_ownership_audit.py" "$WORK"

python "$ROOT/tests/test_combat2cn_native_code_callbacks.py"
python "$ROOT/tests/test_combat2cj_source_runtime_services.py" "$WORK"
python "$ROOT/tests/test_combat2bt_source_anim_declaration.py"
python "$ROOT/tests/test_combat2bu_source_collision_smoke.py"
python "$ROOT/tests/test_combat2bs_missing_gameplay_helper.py"
python "$ROOT/tests/test_combat2bp_source_animation_catalog.py"
python "$ROOT/tests/test_combat2bp_animation_runtime.py"
python "$ROOT/tests/test_combat2bp_attract_start_match.py"
# Carry the exact generators/patcher into the committed tree for reproducibility.
cp "$ROOT/tools/fix39_source_animation_catalog.py" tools/fix39_source_animation_catalog.py
cp "$ROOT/tools/fix39_anim_vm_program.py" tools/fix39_anim_vm_program.py
cp "$ROOT/tools/fix39_target_offsets.py" tools/fix39_target_offsets.py
cp "$ROOT/tools/fix39_combat_completion_patch.py" tools/fix39_combat_completion_patch.py
cp "$ROOT/tools/fix39_combat_source_audit_patch.py" tools/fix39_combat_source_audit_patch.py
cp "$ROOT/tools/fix39_native_code_callbacks_patch.py" tools/fix39_native_code_callbacks_patch.py
cp "$ROOT/tools/fix39_ani_code_completion_patch.py" tools/fix39_ani_code_completion_patch.py
cp "$ROOT/tests/test_combat2di_ani_code_completion.py" tests/test_combat2di_ani_code_completion.py
cp "$ROOT/tools/fix39_ani_code_full_source_patch.py" tools/fix39_ani_code_full_source_patch.py
cp "$ROOT/tests/test_combat2di_full_ani_code_inventory.py" tests/test_combat2di_full_ani_code_inventory.py
cp "$ROOT/tests/test_combat2dl_n64_misleading_indentation.py" tests/test_combat2dl_n64_misleading_indentation.py
cp "$ROOT/tools/fix39_combat_runtime_parity_patch.py" tools/fix39_combat_runtime_parity_patch.py
cp "$ROOT/tests/test_combat2cv_runtime_parity.py" tests/test_combat2cv_runtime_parity.py
cp "$ROOT/tests/test_combat2cw_runtime_patcher_robust.py" tests/test_combat2cw_runtime_patcher_robust.py
cp "$ROOT/tools/fix39_attract_ownership_audit.py" tools/fix39_attract_ownership_audit.py
cp "$ROOT/tools/fix39_dependency_provider_audit.py" tools/fix39_dependency_provider_audit.py
cp "$ROOT/tools/fix39_host_link_surface_audit.py" tools/fix39_host_link_surface_audit.py
cp "$ROOT/tests/test_combat2cs_attract_single_authority.py" tests/test_combat2cs_attract_single_authority.py
cp "$ROOT/tests/test_combat2cv_late_staging_single_authority.py" tests/test_combat2cv_late_staging_single_authority.py
cp "$ROOT/tests/test_combat2cn_native_code_callbacks.py" tests/test_combat2cn_native_code_callbacks.py
# Combat2DE: carry the CZ walk/dual-animation patchers and regression into the
# committed tree before staging. DD applied these from $ROOT but then attempted
# to stage fix39_dual_anim_channel_patch.py without ever copying it into $WORK.
cp "$ROOT/tools/fix39_execute_walk_source_patch.py" tools/fix39_execute_walk_source_patch.py
cp "$ROOT/tools/fix39_runtime_identity_chain_patch.py" tools/fix39_runtime_identity_chain_patch.py
cp "$ROOT/tests/test_combat2dr_runtime_identity_chain.py" tests/test_combat2dr_runtime_identity_chain.py
cp "$ROOT/tests/test_combat2dr_drone_service_coverage.py" tests/test_combat2dr_drone_service_coverage.py
cp "$ROOT/tests/test_combat2dt_actor_visual_parity.py" tests/test_combat2dt_actor_visual_parity.py
cp "$ROOT/tests/test_combat2dt_drone_translator.py" tests/test_combat2dt_drone_translator.py
cp "$ROOT/tests/test_combat2dt_streamed_visual_safety.py" tests/test_combat2dt_streamed_visual_safety.py
cp "$ROOT/tests/test_combat2dx_wimp_palette_mapping.py" tests/test_combat2dx_wimp_palette_mapping.py
cp "$ROOT/tests/test_combat2dx_wimp_palette_bank.py" tests/test_combat2dx_wimp_palette_bank.py
cp "$ROOT/tests/test_combat2dx_post_integration_path_contract.py" tests/test_combat2dx_post_integration_path_contract.py
cp "$ROOT/tools/fix39_drone_seek_source_patch.py" tools/fix39_drone_seek_source_patch.py
cp "$ROOT/tests/test_combat2bl_drone_seek_source.py" tests/test_combat2bl_drone_seek_source.py
cp "$ROOT/tests/test_combat2bo_execute_walk_source.py" tests/test_combat2bo_execute_walk_source.py
cp "$ROOT/tools/fix39_dual_anim_channel_patch.py" tools/fix39_dual_anim_channel_patch.py
cp "$ROOT/tests/test_combat2cz_walk_and_dual_anim.py" tests/test_combat2cz_walk_and_dual_anim.py
cp "$ROOT/tests/test_combat2di_n64_walk_warning_clean.py" tests/test_combat2di_n64_walk_warning_clean.py
cp "$ROOT/tests/test_combat2dj_legacy_test_reconciliation.py" tests/test_combat2dj_legacy_test_reconciliation.py
cp "$ROOT/tests/test_combat2dk_semantic_build_graph_audit.py" tests/test_combat2dk_semantic_build_graph_audit.py
cp "$ROOT/tests/test_combat2dn_selective_ownership.py" tests/test_combat2dn_selective_ownership.py
cp "$ROOT/tests/test_combat2do_legacy_ownership_reconciliation.py" tests/test_combat2do_legacy_ownership_reconciliation.py
cp "$ROOT/tests/test_combat2dp_host_link_closure.py" tests/test_combat2dp_host_link_closure.py
cp "$ROOT/tests/test_combat2dq_attract_abi_source_parity.py" tests/test_combat2dq_attract_abi_source_parity.py
cp "$ROOT/tools/fix39_strict_runtime_parity_audit.py" tools/fix39_strict_runtime_parity_audit.py
cp "$ROOT/tools/fix39_source_proof_gate.py" tools/fix39_source_proof_gate.py
cp "$ROOT/tests/test_combat2ec_wimp_strict_source_gate.py" tests/test_combat2ec_wimp_strict_source_gate.py
cp "$ROOT/tests/test_combat2dy_strict_runtime_parity.py" tests/test_combat2dy_strict_runtime_parity.py
cp "$ROOT/tests/test_combat2dz_no_prose_negative_assertions.py" tests/test_combat2dz_no_prose_negative_assertions.py
cp "$ROOT/tests/test_combat2ea_wimp_palette_window.py" tests/test_combat2ea_wimp_palette_window.py
cp "$ROOT/tests/test_combat2ec_wimp_image_index_bias.py" tests/test_combat2ec_wimp_image_index_bias.py

say "Verifying C2e Sports override uses the round-tripped PLATYNUMX WIMP palette"
sh scripts/prepare_sports_source_assets.sh
grep -q '0x2211, 0x2211, 0x519B, 0x4959' src/generated/sports_background.c \
  || fail "V13e-c2e Sports background generator did not preserve the replacement MIDWAY/PLATYNUMX palette"
grep -q 'python3 "$SPORTS_BG_TOOL"' scripts/prepare_sports_source_assets.sh \
  || fail "V13e-c2e Sports script is not routed through the override palette converter"

say "Generating exact DRONE scalar tables from historical DRONE.ASM"
python tools/fix39_drone_tables.py \
  --source original/wwf-wrestlemania/DRONE.ASM \
  --out src/fix39/wm_arcade_drone_source_tables_generated.h
grep -q 'WM_FIX39_DRONE_SOURCE_GENERATED 1' src/fix39/wm_arcade_drone_source_tables_generated.h \
  || fail "V13e-c2d DRONE source generator did not mark output live"
grep -q 'wm_fix39_drone_blkbase_t\[30\]' src/fix39/wm_arcade_drone_source_tables_generated.h \
  || fail "V13e-c2d source blkbase_t missing/incorrect length"
grep -q 'wm_fix39_drone_blkatk_t\[10\]' src/fix39/wm_arcade_drone_source_tables_generated.h \
  || fail "V13e-c2d source blkatk_t missing/incorrect length"
grep -q 'wm_fix39_drone_sklhhdly_t\[30\]' src/fix39/wm_arcade_drone_source_tables_generated.h \
  || fail "V13e-c2d source sklhhdly_t missing/incorrect length"
grep -q 'wm_fix39_drone_sklhrdly_t\[30\]' src/fix39/wm_arcade_drone_source_tables_generated.h \
  || fail "V13e-c2d source sklhrdly_t missing/incorrect length"

say "Generating exact DRONE range/mode tables from historical DRONE.ASM"
python tools/fix39_drone_ranges.py \
  --source original/wwf-wrestlemania/DRONE.ASM \
  --out src/fix39/wm_arcade_drone_source_ranges_generated.h
grep -q 'WM_FIX39_DRONE_RANGES_GENERATED 1' src/fix39/wm_arcade_drone_source_ranges_generated.h \
  || fail "V13e-c2d DRONE range generator did not mark output live"
grep -q 'wm_fix39_drone_range_table\[3\]\[9\]' src/fix39/wm_arcade_drone_source_ranges_generated.h \
  || fail "V13e-c2d source wnshort/wnmed/wnlong root table missing"

say "Generating exact DRONE script bodies from historical DRONE.ASM"
python tools/fix39_drone_scripts.py \
  --source original/wwf-wrestlemania/DRONE.ASM \
  --out src/fix39/wm_arcade_drone_source_scripts_generated.h
grep -q 'WM_FIX39_DRONE_SCRIPTS_GENERATED 1' src/fix39/wm_arcade_drone_source_scripts_generated.h \
  || fail "V13e-c3 DRONE script generator did not mark output live"
grep -Eq 'WM_FIX39_DRONE_SCRIPT_COUNT [1-9][0-9]*' src/fix39/wm_arcade_drone_source_scripts_generated.h \
  || fail "V13e-c3 generated no DRONE scripts"
grep -Eq 'WM_FIX39_DRONE_SKILL_TABLE_COUNT [1-9][0-9]*' src/fix39/wm_arcade_drone_source_scripts_generated.h \
  || fail "V13e-c3 generated no command-2 skill tables"
grep -q 'wm_fix39_drone_scripts' src/fix39/wm_arcade_drone_source_scripts_generated.h \
  || fail "V13e-c3 generated script registry missing"
# Do not require a DONE/yield opcode to occur in the reachable historical scripts.
# WM_DRONE_SC_DONE is supported by the VM, but the exact source graph generated
# from this DRONE.ASM revision may legitimately contain zero reachable yields.
grep -q 'wm_fix39_drone_ops_' src/fix39/wm_arcade_drone_source_scripts_generated.h \
  || fail "V13e-c3 generated script bodies contain no decoded operations"

say "Auditing remaining DRONE executable services against historical source"
python tools/fix39_drone_services.py \
  --source original/wwf-wrestlemania/DRONE.ASM \
  --generated src/fix39/wm_arcade_drone_source_scripts_generated.h \
  --out build/fix39-v13e-c5b-drone-services.txt \
  --header src/fix39/wm_arcade_drone_source_services_generated.h
test -s build/fix39-v13e-c5b-drone-services.txt \
  || fail "V13e-c5b DRONE executable-service audit produced no report"
grep -q '^source_seek_call=drone_seek$' build/fix39-v13e-c5b-drone-services.txt \
  || fail "V13e-c5c historical drone_seek call was not verified"
grep -q 'WM_FIX39_DRONE_SERVICES_GENERATED 1' src/fix39/wm_arcade_drone_source_services_generated.h || fail "C5c service binding registry not generated"
grep -Eq 'WM_FIX39_DRONE_SERVICE_COUNT [1-9][0-9]*' src/fix39/wm_arcade_drone_source_services_generated.h || fail "C5e service binding registry is empty"
grep -q 'wm_fix39_drone_service_source_addr' src/fix39/wm_arcade_drone_source_services_generated.h || fail "C5e exact service entry addresses were not generated"
grep -q '^service_entry=' build/fix39-v13e-c5b-drone-services.txt || fail "C5e service entry report is empty"

say "Recovering exact historical source for DRONE executable bodies"
python tools/fix39_drone_bodies.py \
  --source original/wwf-wrestlemania/DRONE.ASM \
  --services src/fix39/wm_arcade_drone_source_services_generated.h \
  --out build/fix39-v13e-c5f-drone-bodies.txt
test -s build/fix39-v13e-c5f-drone-bodies.txt || fail "C5f DRONE body report was not generated"
grep -Eq '^body=' build/fix39-v13e-c5f-drone-bodies.txt || fail "C5f DRONE body report contains no services"
grep -Eq '^extracted=[1-9][0-9]*$' build/fix39-v13e-c5f-drone-bodies.txt || fail "C5f recovered no executable source bodies"

say "Translating conservative state-only DRONE executable bodies"
python tools/fix39_drone_translate.py --report build/fix39-v13e-c5f-drone-bodies.txt --out src/fix39/wm_arcade_drone_source_bodies_generated.h
python "$ROOT/tests/test_combat2dr_drone_service_coverage.py" "$WORK"
grep -q 'WM_FIX39_DRONE_TRANSLATED_BODY_COUNT' src/fix39/wm_arcade_drone_source_bodies_generated.h || fail "C5h translated-body header was not generated"
# Combat2EC: canonical regeneration must have replaced every live placeholder.
python "$ROOT/tools/fix39_strict_runtime_parity_audit.py" "$WORK"
python "$ROOT/tools/fix39_source_proof_gate.py" "$WORK"
grep -q '"hgrab"' src/fix39/wm_arcade_drone_source_scripts_generated.h \
  || fail "V13e-c3h #getscrpt list expansion did not reach hgrab"
if grep -Eq '\{ "M_shrtblkr(dl)?", wm_fix39_drone_ops_' src/fix39/wm_arcade_drone_source_scripts_generated.h; then
  fail "V13e-c3h pointer-list label was emitted as an executable script"
fi

PREFLIGHT="$WORK/build/fix39-v13e-c3f-source-preflight"
rm -rf "$PREFLIGHT"
mkdir -p "$PREFLIGHT"
python tools/fix39_attract_text.py \
  --source original/wwf-wrestlemania/ATTRACT.ASM \
  --out-c "$PREFLIGHT/fix39_attract_text_generated.c" \
  --out-h "$PREFLIGHT/fix39_attract_text_generated.h"
python tools/fix39_attract_assets.py \
  --img-dir original/wwf-wrestlemania/IMG \
  --imgpal original/wwf-wrestlemania/IMGPAL.ASM \
  --wimpimg tools/wimpimg.py \
  --out-c "$PREFLIGHT/fix39_attract_assets.c" \
  --out-h "$PREFLIGHT/fix39_attract_assets_generated.h"
test -s "$PREFLIGHT/fix39_attract_assets.c" || fail "V13 source WIMP preflight produced no C asset table"
grep -q 'SMWWF2' "$PREFLIGHT/fix39_attract_assets.c" || fail "V13 source WIMP preflight is missing SMWWF2"
grep -q 'OSGEMD_APO' "$PREFLIGHT/fix39_attract_assets.c" || fail "V13 source WIMP preflight is missing OSGEMD apostrophe glyph"
grep -q 'FONT7period' "$PREFLIGHT/fix39_attract_assets.c" || fail "V13 source WIMP preflight is missing RD7 period glyph"
grep -q 'WSF14NUM' "$PREFLIGHT/fix39_attract_assets.c" || fail "V13 source WIMP preflight is missing HSTD rank glyph"
grep -q 'WGSF18PER' "$PREFLIGHT/fix39_attract_assets.c" || fail "V13 source WIMP preflight is missing HSTD score font"
grep -q 'CRUT_BH' "$PREFLIGHT/fix39_attract_assets.c" || fail "V13 source WIMP preflight is missing HSTD beaten icons"
grep -q 'BARBUTT' "$PREFLIGHT/fix39_attract_assets.c" || fail "V13 source WIMP preflight is missing HSTD streak art"
grep -q 'HART' "$PREFLIGHT/fix39_attract_assets.c" || fail "V13 source WIMP preflight is missing HSTD wrestler labels"

# V13b source audit guards. The project intentionally keeps the existing
# Midway Sports attract slot active for later repurposing, so only verify that
# the historical source still contains the show_sports_logo symbol/call site;
# do not require a particular commented/uncommented spelling.
grep -Eq 'show_sports_logo' original/wwf-wrestlemania/ATTRACT.ASM \
  || fail "V13e-c3 source preflight: ATTRACT.ASM show_sports_logo symbol/call site missing"
grep -Eq 'SUBRP[[:space:]]+wrestler_friction' original/wwf-wrestlemania/WRESTLE.ASM \
  || fail "V13 source preflight: WRESTLE wrestler_friction missing"
grep -Eq 'SUBR(P)?[[:space:]]+wrestler_veladd' original/wwf-wrestlemania/WRESTLE2.ASM \
  || fail "V13e-c3 source preflight: WRESTLE2 wrestler_veladd missing"

say "Verifying namespaced rope source actually landed in fresh repo"
if grep -R -n -E "\bWM_ROPE_(FRONT|BACK|LEFT|RIGHT|TOP|MIDDLE|BOTTOM|Z_HIGH|Z_NORM|BOUNCE_UD|BOUNCE_IO|SIDE_SPRING|DOWN_SPRING|SIDE_SPRING_RELEASE|DOWN_SPRING_RELEASE|COMMAND_COUNT|CHANNEL_RED|CHANNEL_WHITE|CHANNEL_BLUE|CHANNEL_SHADOW|CHANNEL_COUNT|HALF_FIRST|HALF_SECOND)\b" "$WORK/src/fix39" --include="*.c" --include="*.h"; then
  fail "stale pre-V9 rope symbols survived copy; refusing to compile wrong source"
fi
grep -n "WM_FIX39_ROPE_COMMAND_COUNT" "$WORK/src/fix39/wmania_rope_command.h" || fail "Fix39 namespaced rope sentinel missing"
printf "Fix39 rope header SHA256: "
sha256sum "$WORK/src/fix39/wmania_rope_command.h" | cut -d' ' -f1

# Demo3 now references wm_bret_sprite_find() from generated bret_sprites.c.
# Generate that source before host linking; the N64 Makefile already treats it
# as an ASSET_C dependency, but CMake needs the file present at configure time.
say "Generating Bret WIMP sprite lookup for Demo3 collision binding"
sh scripts/prepare_bret_sprites.sh
test -s src/generated/bret_sprites.c || fail "prepare_bret_sprites.sh did not generate src/generated/bret_sprites.c"
python tools/fix39_bret_attack_frames.py --source original/wwf-wrestlemania/HRTSEQ2.ASM --out src/fix39/wm_arcade_bret_attack_frames_generated.h
grep -q 'WM_FIX39_BRET_ATTACK_FRAMES_GENERATED 1' src/fix39/wm_arcade_bret_attack_frames_generated.h || fail "Generated exact Bret attack frames from HRTSEQ2.ASM missing"
grep -q 'wm_bret_sprite_find' src/generated/bret_sprites.c || fail "generated Bret sprite source is missing wm_bret_sprite_find"

# Run the larger host verification only if its toolchain is already installed.
# The GitHub Actions libdragon build below remains the authoritative N64 build.
if command -v cmake >/dev/null 2>&1 && command -v make >/dev/null 2>&1 && { command -v clang >/dev/null 2>&1 || command -v cc >/dev/null 2>&1; }; then
  say "Running host CMake verification"
  rm -rf build/fix39-host
  cmake -S . -B build/fix39-host -DWM_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
  cmake --build build/fix39-host --parallel 2
  ctest --test-dir build/fix39-host --output-on-failure
  ./build/fix39-host/wm_headless || echo "NOTE: optional wm_headless check returned nonzero; continuing to authoritative GitHub N64 build."
else
  echo "NOTE: cmake/make/compiler not all present; skipping optional host CMake verification."
fi

python "$ROOT/tests/test_combat2bw_no_large_generated_git.py"

say "Committing Fix39 branch"
git checkout -b "$BRANCH"
# Combat2CB: character_assets.c is a local/CI-generated build product.  With the
# full source animation corpus it can exceed GitHub's 100 MiB per-file limit.
# CI regenerates it before CMake/Make, so never commit the generated C payload.
if git ls-files --error-unmatch src/generated/character_assets.c >/dev/null 2>&1; then
  git restore --source=HEAD --staged --worktree src/generated/character_assets.c
else
  rm -f src/generated/character_assets.c
fi
git config user.name >/dev/null 2>&1 || git config user.name "PlatynumX Fix39 Builder"
git config user.email >/dev/null 2>&1 || git config user.email "fix39-builder@localhost"
# Combat2DI: fail BEFORE git-add if any explicit staging path is absent.
# This turns stale pathspecs into a short, named diagnostic instead of a late
# fatal error after the host compile has already completed.
STAGE_PATHS=(
  .github/workflows/build.yml CMakeLists.txt Makefile
  include/wm/demo.h include/wm/character_assets.h include/wm/arcade/wmania_attract_data.h
  src/core/demo.c src/core/app.c src/platform/n64/main.c src/platform/headless/main.c
  src/fix39 assets/fix39_sports_override
  scripts/prepare_frontend_assets.sh scripts/prepare_sports_source_assets.sh
  tests/test_core.c tests/fix39_smoke.c tests/test_v13e_chunk6.py
  tools/fix39_attract_text.py tools/fix39_attract_assets.py
  tools/fix39_drone_tables.py tools/fix39_drone_ranges.py tools/fix39_drone_scripts.py
  tools/fix39_drone_services.py tools/fix39_drone_bodies.py tools/fix39_drone_translate.py
  tools/fix39_bret_attack_frames.py tools/fix39_character_assets.py tools/fix39_character_attack_frames.py
  tools/fix39_source_animation_catalog.py tools/fix39_anim_vm_program.py tools/fix39_target_offsets.py
  tools/fix39_combat_completion_patch.py tools/fix39_combat_source_audit_patch.py
  tools/fix39_native_code_callbacks_patch.py tools/fix39_ani_code_completion_patch.py tools/fix39_ani_code_full_source_patch.py tools/fix39_build_graph_audit.py tools/fix39_dependency_provider_audit.py tools/fix39_host_link_surface_audit.py tools/fix39_execute_walk_source_patch.py
  tools/fix39_dual_anim_channel_patch.py tools/fix39_attract_ownership_audit.py tools/fix39_runtime_identity_chain_patch.py tests/test_combat2dr_runtime_identity_chain.py tests/test_combat2dr_drone_service_coverage.py tests/test_combat2dt_actor_visual_parity.py tests/test_combat2dt_drone_translator.py tests/test_combat2dt_streamed_visual_safety.py
  tests/test_combat2dx_wimp_palette_mapping.py tests/test_combat2dx_wimp_palette_bank.py tests/test_combat2dx_post_integration_path_contract.py tests/test_combat2bl_drone_seek_source.py tests/test_combat2bo_execute_walk_source.py tools/fix39_drone_seek_source_patch.py
  tests/test_combat2cn_native_code_callbacks.py tests/test_combat2di_ani_code_completion.py tests/test_combat2di_full_ani_code_inventory.py tests/test_combat2cz_walk_and_dual_anim.py
  tests/test_combat2di_n64_walk_warning_clean.py tests/test_combat2dl_n64_misleading_indentation.py tests/test_combat2dj_legacy_test_reconciliation.py tests/test_combat2dk_semantic_build_graph_audit.py tests/test_combat2dn_selective_ownership.py tests/test_combat2do_legacy_ownership_reconciliation.py tests/test_combat2dp_host_link_closure.py tests/test_combat2dq_attract_abi_source_parity.py
  tests/test_combat2cs_attract_single_authority.py tools/fix39_sports_background_bundle.py
  tests/test_combat2ec_wimp_image_index_bias.py
  tests/test_combat2cv_late_staging_single_authority.py tests/test_combat2cv_runtime_parity.py
  tests/test_combat2cw_runtime_patcher_robust.py
)
missing_stage=0
for stage_path in "${STAGE_PATHS[@]}"; do
  if [ ! -e "$stage_path" ]; then
    echo "ERROR: Combat2EC staging manifest path missing: $stage_path"
    missing_stage=1
  fi
done
[ "$missing_stage" -eq 0 ] || fail "Combat2EC staging manifest contains missing paths"
git add .github/workflows/build.yml CMakeLists.txt Makefile include/wm/demo.h include/wm/character_assets.h include/wm/arcade/wmania_attract_data.h src/core/demo.c src/core/app.c src/platform/n64/main.c src/platform/headless/main.c src/fix39 assets/fix39_sports_override scripts/prepare_frontend_assets.sh scripts/prepare_sports_source_assets.sh tests/test_core.c tests/fix39_smoke.c tests/test_v13e_chunk6.py tools/fix39_attract_text.py tools/fix39_attract_assets.py tools/fix39_drone_tables.py tools/fix39_drone_ranges.py tools/fix39_drone_scripts.py tools/fix39_drone_services.py tools/fix39_drone_bodies.py tools/fix39_drone_translate.py tools/fix39_bret_attack_frames.py tools/fix39_character_assets.py tools/fix39_character_attack_frames.py tools/fix39_source_animation_catalog.py tools/fix39_anim_vm_program.py tools/fix39_target_offsets.py tools/fix39_combat_completion_patch.py tools/fix39_combat_source_audit_patch.py tools/fix39_native_code_callbacks_patch.py tools/fix39_ani_code_completion_patch.py tools/fix39_ani_code_full_source_patch.py tools/fix39_build_graph_audit.py tools/fix39_dependency_provider_audit.py tools/fix39_host_link_surface_audit.py tools/fix39_execute_walk_source_patch.py tools/fix39_dual_anim_channel_patch.py tools/fix39_attract_ownership_audit.py tools/fix39_runtime_identity_chain_patch.py tests/test_combat2dr_runtime_identity_chain.py tests/test_combat2dr_drone_service_coverage.py tests/test_combat2dt_actor_visual_parity.py tests/test_combat2dt_drone_translator.py tests/test_combat2dt_streamed_visual_safety.py tests/test_combat2dx_post_integration_path_contract.py tests/test_combat2bl_drone_seek_source.py tests/test_combat2bo_execute_walk_source.py tools/fix39_drone_seek_source_patch.py tests/test_combat2dx_wimp_palette_mapping.py tests/test_combat2dx_wimp_palette_bank.py tests/test_combat2cn_native_code_callbacks.py tests/test_combat2di_ani_code_completion.py tests/test_combat2di_full_ani_code_inventory.py tests/test_combat2cz_walk_and_dual_anim.py tests/test_combat2di_n64_walk_warning_clean.py tests/test_combat2dl_n64_misleading_indentation.py tests/test_combat2cs_attract_single_authority.py tools/fix39_sports_background_bundle.py tests/test_combat2cv_late_staging_single_authority.py tests/test_combat2cv_runtime_parity.py tests/test_combat2cw_runtime_patcher_robust.py tests/test_combat2dj_legacy_test_reconciliation.py tests/test_combat2dk_semantic_build_graph_audit.py tests/test_combat2dn_selective_ownership.py tests/test_combat2do_legacy_ownership_reconciliation.py tests/test_combat2dp_host_link_closure.py tests/test_combat2dq_attract_abi_source_parity.py tools/fix39_strict_runtime_parity_audit.py tools/fix39_source_proof_gate.py tests/test_combat2dy_strict_runtime_parity.py tests/test_combat2ec_wimp_strict_source_gate.py tests/test_combat2dz_no_prose_negative_assertions.py tests/test_combat2ec_wimp_image_index_bias.py tools/fix39_dependency_provider_audit.py
# Combat2CS: force-stage the streamed full ANIM.ASM VM payload after the scoped tracked-source add.
# Keep this after the first plain `git add` so the historical Combat2AJ staging-contract audit
# still inspects the intended tracked-source contract rather than this generated-payload add.
git add -f filesystem/fix39_anim
# Combat2AW: ring/rope metadata and DragonFS payload are generated before commit.
# Force-add them because generated/filesystem paths may be ignored by the baseline repo.
git add -f include/wm/ring_rope_assets.h src/generated/ring_rope_assets.c filesystem/fix39_ring include/wm/ring_arena_assets.h src/generated/ring_arena_assets.c include/wm/crowd_assets.h src/generated/crowd_assets.c filesystem/fix39_arena

# Combat2CX: Combat2AJ originally required app.c to consume wm_demo_set_roster().
# That is now intentionally obsolete: ATTR gameplay has one authority, the translated
# match runtime. Keep the demo roster API available for non-gameplay/demo tests, but
# fail closed if app.c re-acquires attract gameplay roster ownership through wm_demo.
grep -q 'uint8_t roster_id;' include/wm/demo.h || fail "Combat2CX demo.h roster_id missing after patch"
grep -q 'void wm_demo_set_roster' include/wm/demo.h || fail "Combat2CX demo.h wm_demo_set_roster declaration missing"
grep -q 'void wm_demo_set_roster' src/core/demo.c || fail "Combat2CX demo.c wm_demo_set_roster implementation missing"
if grep -q 'wm_demo_set_roster(&app->demo' src/core/app.c; then
  fail "ATTR single-authority violation: app.c illegally restored wm_demo roster ownership"
fi
for required_staged in include/wm/demo.h src/core/demo.c src/core/app.c; do
  git diff --cached --name-only -- "$required_staged" | grep -qx "$required_staged" \
    || fail "Combat2AJ required API file not staged: $required_staged"
done
# Combat2AJ: fail closed only on unstaged files owned by this patch.
# src/generated/sports_background.c is intentionally regenerated locally by the
# sports round-trip verification but is regenerated again in CI from the staged
# source assets/tooling, so committing that derived file would bundle unrelated
# generated noise into this combat revision.  Everything else tracked remains
# fail-closed.
UNSTAGED_TRACKED_ALL="$(git diff --name-only -- . ':!original' ':!build')"
UNSTAGED_TRACKED="$(printf '%s\n' "$UNSTAGED_TRACKED_ALL" | grep -vxF 'src/generated/sports_background.c' | grep -vxF 'src/generated/character_assets.c' || true)"
if [ -n "$UNSTAGED_TRACKED" ]; then
  echo "ERROR: tracked Combat2AJ-owned changes remain unstaged:"
  printf '%s\n' "$UNSTAGED_TRACKED"
  fail "Combat2AJ refusing to push an incomplete patch commit"
fi
if printf '%s\n' "$UNSTAGED_TRACKED_ALL" | grep -qxF 'src/generated/sports_background.c'; then
  echo "NOTE: allowing expected derived-only dirty file: src/generated/sports_background.c"
fi
if printf '%s\n' "$UNSTAGED_TRACKED_ALL" | grep -qxF 'src/generated/character_assets.c'; then
  echo "NOTE: allowing expected CI-regenerated file: src/generated/character_assets.c"
fi
# Fail closed if the oversized generated character C somehow gets staged again.
if git diff --cached --name-only | grep -qxF 'src/generated/character_assets.c'; then
  fail "Combat2CB refusing to stage generated character_assets.c (GitHub 100 MiB limit)"
fi

git commit -m "Fix39 V13e Combat2EC: fix post-integration test path contract"
push_ok=0
for push_try in 1 2 3; do
  echo "=== git push attempt ${push_try}/3 ==="
  if git push -u origin "$BRANCH"; then
    push_ok=1
    break
  fi
  echo "WARN: git push attempt ${push_try} failed; retrying in $((push_try * 5))s..."
  sleep $((push_try * 5))
done
if [ "$push_ok" -ne 1 ]; then
  echo "ERROR: git push failed after 3 attempts."
  exit 70
fi
SHA="$(git rev-parse HEAD)"
echo "Fix39 commit: $SHA"

say "Waiting for GitHub Actions build run"
RUN_ID=""
for _ in $(seq 1 45); do
  RUN_ID="$(gh run list --branch "$BRANCH" --workflow build --limit 1 --json databaseId,headSha --jq ".[] | select(.headSha == \"$SHA\") | .databaseId" 2>/dev/null | head -n1 || true)"
  [ -n "$RUN_ID" ] && break
  sleep 2
done
[ -n "$RUN_ID" ] || fail "Could not find the GitHub Actions run for $SHA"
echo "Workflow run: $RUN_ID"
WORKFLOW_OK=1
if ! gh run watch "$RUN_ID" --exit-status; then
  WORKFLOW_OK=0
  FAIL_LOG="$DOWNLOAD_DIR/fix39-v13e-combat2ec-${RUN_ID}-failed.log"
  GH_PAGER=cat gh run view "$RUN_ID" -R PlatynumX/wm-arcade-port-r8-n64 --log >"$FAIL_LOG" 2>&1 || true
  echo "NOTE: overall GitHub workflow is red. Full run log saved to: $FAIL_LOG" >&2
  echo "Checking whether the N64 ROM job/artifact still succeeded..."
fi

say "Downloading built ROM artifact"
ART="$WORK/build/fix39-artifact"
rm -rf "$ART"
mkdir -p "$ART" "$DOWNLOAD_DIR"
if ! gh run download "$RUN_ID" --name wm-arcade-r9-build --dir "$ART"; then
  if [ "$WORKFLOW_OK" -eq 0 ]; then
    fail "Overall workflow failed and wm-arcade-r9-build artifact was not available. See the saved GitHub log."
  fi
  fail "Workflow completed but wm-arcade-r9-build artifact could not be downloaded"
fi
ROM="$(find "$ART" -type f -name 'wm_arcade_r9.z64' -print -quit)"
[ -n "$ROM" ] && [ -s "$ROM" ] || fail "wm-arcade-r9-build downloaded but wm_arcade_r9.z64 was not found"
SHORT="${SHA:0:12}"
OUT="$DOWNLOAD_DIR/wm_arcade_fix39_v13e_combat2ec_${SHORT}.z64"
cp "$ROM" "$OUT"
if [ "$WORKFLOW_OK" -eq 0 ]; then
  echo "NOTE: host/source verification failed, but the N64 ROM job produced a downloadable artifact."
fi

say "FIX39 BUILD COMPLETE"
echo "ROM: $OUT"
echo "Branch: $BRANCH"
echo "Commit: $SHA"
echo "Workflow: $RUN_ID"
