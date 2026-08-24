#!/usr/bin/env python3
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]

def need(path: Path, text: str) -> None:
    data = path.read_text()
    if text not in data:
        raise SystemExit(f"V13e-c3 regression: {path.name} missing {text!r}")

need(ROOT / "tools/fix39_drone_scripts.py", "DIRECT_SCRIPTS")
need(ROOT / "tools/fix39_drone_scripts.py", "WM_DRONE_SC_SKILL_ABORT")
need(ROOT / "tools/fix39_drone_scripts.py", "WM_FIX39_DRONE_C4_SEAM_COUNT")
need(ROOT / "tools/fix39_drone_scripts.py", "def symkey")
need(ROOT / "tools/fix39_drone_scripts.py", "def inline_data_label")
need(ROOT / "tools/fix39_drone_scripts.py", "def packed_directive")
need(ROOT / "src/fix39/wm_arcade_drone_source_scripts.c", "wm_fix39_source_symbol_equal")
need(ROOT / "src/fix39/wm_arcade_drone_source_scripts.c", "wm_arcade_drone_source_resolve_script")
need(ROOT / "src/fix39/wm_arcade_drone_source_scripts.c", "wm_arcade_drone_source_script_skill_pct")
need(ROOT / "src/fix39/wm_fix39_runtime.c", "g.drone_callbacks.resolve_script = wm_arcade_drone_source_resolve_script")
need(ROOT / "src/fix39/wm_fix39_runtime.c", "g.drone_callbacks.script_skill_pct = wm_arcade_drone_source_script_skill_pct")
need(ROOT / "src/fix39/wm_fix39_runtime.c", "g.status.drone_scripts_ready = wm_arcade_drone_source_scripts_ready()")
need(ROOT / "src/fix39/wm_arcade_drone.c", "#dsdone stores the already-advanced a9")
need(ROOT / "src/fix39/wm_arcade_drone.c", "if (!cb || !cb->script_call) return WM_DRONE_STEP_SCRIPT")
need(ROOT / "src/fix39/wm_arcade_drone_source_scripts_generated.h", "WM_FIX39_DRONE_SCRIPTS_GENERATED 0")
need(ROOT / "termux_fix39_build.sh", "Generating exact DRONE script bodies from historical DRONE.ASM")
need(ROOT / "tools/apply_fix39.py", "fix39_drone_scripts.py")


need(ROOT / "tests/fix39_smoke.c", '#include "wm_arcade_drone_source_scripts.h"')
need(ROOT / "tests/fix39_smoke.c", "assert(st->drone_scripts_ready == wm_arcade_drone_source_scripts_ready())")

runtime = (ROOT / "src/fix39/wm_fix39_runtime.c").read_text()
if "g.status.drone_runtime_ready = true" in runtime:
    raise SystemExit("V13e-c3 regression: full DRONE runtime was claimed early")
print("Fix39 V13e chunk-3 DRONE script binding regression: PASS")
