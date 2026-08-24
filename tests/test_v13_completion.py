#!/usr/bin/env python3
"""Structural guards for the Fix39 V13 source-completion wiring pass."""
from pathlib import Path
import importlib.util
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FIX = ROOT / "src" / "fix39"


def need(path: Path, token: str) -> None:
    text = path.read_text()
    assert token in text, f"{path.name}: missing {token!r}"


def forbid(path: Path, token: str) -> None:
    text = path.read_text()
    assert token not in text, f"{path.name}: stale token {token!r}"


# Project target intentionally retains the Midway Sports screen in ATTRACT.
# Keep the existing source-backed slot active for later repurposing.
attract = FIX / "wmania_attract_core.c"
need(attract, "ADD(WM_FIX39_ATTRACT_SPORTS_LOGO, false, false)")
need(attract, "WM_FIX39_ATTRACT_GAMEPLAY_DEMO_1")
need(attract, "WM_FIX39_ATTRACT_GAMEPLAY_DEMO_2")
need(attract, "wm_attract_demo_plan_make")

# V13c regression: retaining the Sports step in the portable cycle is not
# enough; apply_fix39 must also map that step back into the live app enum.
patcher = ROOT / "tools" / "apply_fix39.py"
need(patcher, "case WM_FIX39_ATTRACT_SPORTS_LOGO: *out = WM_ATTRACT_SHOW_SPORTS_LOGO; return true;")

# Combat2DN/DO regression: ownership is explicit and dependency-closed. Combat
# overlaps plus source-backed shared providers (ring geometry, RNG, attract core)
# move to Fix39; presenter/adapter/roster/high-score/rope stay core-owned.
need(patcher, 'FIX39_COMBAT_OWNERS')
need(patcher, 'fix39_overlaps = [src for src in overlaps if fix39_owns_overlap(src)]')
need(patcher, 'preserved_overlaps = [src for src in overlaps if not fix39_owns_overlap(src)]')
forbid(patcher, 'arcade_names.difference_update(BASELINE_OVERRIDES)')

# Exact WRESTLE2/WRESTLE movement services are live, not a placeholder flag.
runtime = FIX / "wm_fix39_runtime.c"
need(runtime, "wm_arcade_wrestler_veladd(&g.actors[i], false, false)")
need(runtime, "wm_arcade_wrestler_friction(&g.actors[i])")
need(runtime, "wm_ring_inring_field")
need(runtime, "wm_fix39_game_beaten_plan")
need(runtime, "wm_fix39_hiscore_begin_tag_time")

# Completion handoff modules must be carried into the build as source files.
for name in (
    "wm_arcade_movement.c",
    "wm_arcade_matchflow.c",
    "wm_arcade_story.c",
    "wm_arcade_fireworks.c",
):
    assert (FIX / name).is_file(), f"missing V13 source module {name}"

# Do not silently declare still-unbound platform services complete.
need(runtime, "g.status.drone_runtime_ready = g.status.drone_scalar_tables_ready")
need(runtime, "g.status.animation_backend_ready = true")
need(runtime, "g.status.collision_boxes_ready = false")
need(runtime, "g.status.camera_onscreen_inputs_ready = false")
need(runtime, "g.status.hiscore_persistence_ready = false")

# Exercise the actual N64 Makefile dedupe path that failed in V13c.
spec = importlib.util.spec_from_file_location("apply_fix39_v13d", patcher)
mod = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(mod)
with tempfile.TemporaryDirectory() as td:
    repo = Path(td)
    arcade = repo / "src" / "core" / "arcade"
    arcade.mkdir(parents=True)
    (arcade / "wmania_ring_geometry.c").write_text("/* old partial baseline */\n")
    mf = repo / "Makefile"
    mf.write_text(
        "CFLAGS += -I$(CURDIR)/include\n"
        "FIX38_ARCADE_C := \\\n"
        "    src/core/arcade/wmania_ring_geometry.c\\\n"
        "CORE_C += $(FIX38_ARCADE_C)\n"
        "ASSET_C := asset.c\n"
        "C_FILES := $(CORE_C) $(ASSET_C)\n"
        "$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)\n"
    )
    make_sources, deduped = mod.patch_makefile(
        mf, ["wm_fix39_runtime.c", "wmania_ring_geometry.c"]
    )
    out = mf.read_text()
    # Combat2DN/DO: ring geometry is a dependency-closed Fix39 provider.
    assert "src/fix39/wmania_ring_geometry.c" in out
    assert "src/core/arcade/wmania_ring_geometry.c" not in out
    assert "wmania_ring_geometry.c" in make_sources
    assert "wmania_ring_geometry.c" in deduped

print("Fix39 V13 completion structural regression: PASS")
