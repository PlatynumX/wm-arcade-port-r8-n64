#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def need(path: Path, text: str) -> None:
    data = path.read_text()
    if text not in data:
        raise SystemExit(f"V13e-c1 regression: {path.name} missing {text!r}")

need(ROOT / "src/fix39/wm_fix39_runtime.h", "drone_scalar_tables_ready")
need(ROOT / "src/fix39/wm_fix39_runtime.h", "drone_plain_rnd_ready")
need(ROOT / "src/fix39/wm_fix39_runtime.c", "init_drone_callbacks")
need(ROOT / "src/fix39/wm_fix39_runtime.c", "wm_arcade_drone_source_block_base_pct")
need(ROOT / "src/fix39/wm_fix39_runtime.c", "g.status.drone_plain_rnd_ready = true")
need(ROOT / "src/fix39/wm_arcade_drone_source_tables_generated.h", "WM_FIX39_DRONE_SOURCE_GENERATED 0")
need(ROOT / "tools/fix39_drone_tables.py", '"blkbase_t": 30')
need(ROOT / "tools/fix39_drone_tables.py", '"blkatk_t": 10')
need(ROOT / "termux_fix39_build.sh", "Generating exact DRONE scalar tables from historical DRONE.ASM")
need(ROOT / "termux_fix39_build.sh", "WM_FIX39_DRONE_SOURCE_GENERATED 1")
need(ROOT / "tools/apply_fix39.py", "fix39_drone_tables.py")

need(ROOT / "src/fix39/wm_arcade_drone.c", "if (!cb || !cb->rnd_upto) return 0;")
need(ROOT / "src/fix39/wm_arcade_drone.c", "if (!cb || !cb->rndrng0_upto) return 0;")

# The two arcade RNG services are not interchangeable.  Chunk 1 binds only
# RNDRNG0, so the core must not contain the old cross-fallbacks that silently
# substituted one RNG service for the other.
drone_core = (ROOT / "src/fix39/wm_arcade_drone.c").read_text()
if "if (cb->rndrng0_upto) return cb->rndrng0_upto(maxv, cb->user);" in drone_core:
    raise SystemExit("V13e-c1 regression: plain rnd still falls back to RNDRNG0")
if "if (cb->rnd_upto) return cb->rnd_upto(maxv, cb->user);" in drone_core:
    raise SystemExit("V13e-c1 regression: RNDRNG0 still falls back to plain rnd")

# Chunk boundary: do not silently claim DRONE is live before its distinct plain
# RND service and script/range data are bound in later chunks.
runtime = (ROOT / "src/fix39/wm_fix39_runtime.c").read_text()
if "g.status.drone_runtime_ready = true" in runtime:
    raise SystemExit("V13e-c1 regression: chunk 1 must not mark full DRONE runtime ready")

print("Fix39 V13e chunk-1 DRONE binding regression: PASS")
