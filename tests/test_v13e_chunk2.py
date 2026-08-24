#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def need(path: Path, text: str) -> None:
    data = path.read_text()
    if text not in data:
        raise SystemExit(f"V13e-c2 regression: {path.name} missing {text!r}")

need(ROOT / "src/fix39/wm_fix39_runtime.h", "drone_range_tables_ready")
need(ROOT / "src/fix39/wm_fix39_runtime.c", "wm_arcade_drone_source_range_script_list")
need(ROOT / "src/fix39/wm_fix39_runtime.c", "g.status.drone_range_tables_ready = wm_arcade_drone_source_ranges_ready()")
need(ROOT / "src/fix39/wm_arcade_drone_source_ranges.c", "r->my_mode >= 0 && r->my_mode != my_mode")
need(ROOT / "src/fix39/wm_arcade_drone_source_ranges.c", "r->opp_mode >= 0 && r->opp_mode != opp_mode")
need(ROOT / "src/fix39/wm_arcade_drone_source_ranges_generated.h", "WM_FIX39_DRONE_RANGES_GENERATED 0")
need(ROOT / "tools/fix39_drone_ranges.py", 'ROOT_TABLES = ("wnshort_t", "wnmed_t", "wnlong_t")')

need(ROOT / "tools/fix39_drone_ranges.py", "WRESTLER_COUNT = 9")
need(ROOT / "tools/fix39_drone_ranges.py", 'mwl = re.match(r"(?i)^WL\\s+(.+)$", s)')
need(ROOT / "tools/fix39_drone_ranges.py", "only the source terminal -1 wildcard is decoded in chunk 2")
need(ROOT / "src/fix39/wm_arcade_roster.h", "WM_ROSTER_LEX = 8")
need(ROOT / "src/fix39/wm_arcade_move_dispatch.c", "a->wrestler_num == 7")
need(ROOT / "src/fix39/wm_arcade_drone_source_ranges_generated.h", "WM_FIX39_DRONE_RANGE_WRESTLER_COUNT 9")
need(ROOT / "tools/fix39_drone_ranges.py", "expected = abs(max_index) + 1")
need(ROOT / "tools/fix39_drone_ranges.py", "source_count_end_label")
need(ROOT / "tools/fix39_drone_ranges.py", "max_index = len(scripts) - 1")
need(ROOT / "tools/fix39_drone_ranges.py", 'return "token", "WM_PMODE_" + s[5:].upper()')
need(ROOT / "termux_fix39_build.sh", "Generating exact DRONE range/mode tables from historical DRONE.ASM")
need(ROOT / "termux_fix39_build.sh", "WM_FIX39_DRONE_RANGES_GENERATED 1")
need(ROOT / "tools/apply_fix39.py", "fix39_drone_ranges.py")

runtime = (ROOT / "src/fix39/wm_fix39_runtime.c").read_text()
if "g.status.drone_scripts_ready = true" in runtime:
    raise SystemExit("V13e-c2 regression: script bodies are not a chunk-2 claim")
if "g.status.drone_runtime_ready = true" in runtime:
    raise SystemExit("V13e-c2 regression: full DRONE runtime must remain false")

# Chunk 2 still must not alias the unrecovered plain rnd service.
if ".rnd_upto = drone_rndrng0_upto" in runtime:
    raise SystemExit("V13e-c2 regression: plain rnd illegally aliased to RNDRNG0")

print("Fix39 V13e chunk-2 DRONE range binding regression: PASS")
