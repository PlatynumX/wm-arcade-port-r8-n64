#!/usr/bin/env python3
from __future__ import annotations
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
REPORT = ROOT / "reports" / "combat_non_smove_source_reference.md"
failures: list[str] = []


def text(rel: str) -> str:
    p = ROOT / rel
    if not p.exists():
        failures.append(f"missing {rel}")
        return ""
    return p.read_text(encoding="utf-8", errors="replace")

cmake = text("CMakeLists.txt")
required_cmake = [
    "wm_combat_non_smove_core_regression_tests",
    "wm_combat_non_smove_core_regression",
    "audit_combat_non_smove_source_reference.py",
    "wm_combat_non_smove_source_reference_audit",
]
for needle in required_cmake:
    if needle not in cmake:
        failures.append(f"CMake missing non-SMOVE combat gate: {needle}")

combat = text("src/fix39/wm_arcade_combat.c")
wrestle_core = text("src/fix39/wm_arcade_wrestle_core.c")
wrestle_input = text("src/fix39/wm_arcade_wrestle_input.c")
movement = text("src/fix39/wm_arcade_movement.c")
header = text("src/fix39/wm_arcade_combat.h")
test = text("tests/combat_non_smove_core_regression.c")

combat_needles = [
    "COLLIS.ASM uses < and > rejection",
    "wm_arcade_set_hurt_box",
    "wm_arcade_set_attack_box",
    "wm_arcade_resolve_overlap",
    "wm_arcade_try_attack_hit",
    "wm_arcade_check_wrestler_collisions",
    "Original check_collisions exits after first successful hit",
    "GETUP.ASM labels entry 10 as _hiptoss",
    "wm_arcade_set_getup_time",
]
for needle in combat_needles:
    if needle not in combat:
        failures.append(f"combat core missing source marker/function: {needle}")

core_needles = [
    "wm_arcade_auto_pin_check",
    "WRESTLE.ASM comment says four seconds; executable code compares TSEC*3",
    "wm_arcade_can_pin",
    "wm_arcade_wrestler_countdown_tail",
    "wm_arcade_reset_wrestle2_state",
]
for needle in core_needles:
    if needle not in wrestle_core:
        failures.append(f"wrestle core missing source marker/function: {needle}")

input_needles = [
    "WRESTLE.ASM dtime banks -- source explicitly says KEEP THIS ORDER",
    "wm_arcade_update_joystat",
    "wm_arcade_update_joy_dtime",
    "wm_arcade_wrestle_pattern_match",
]
for needle in input_needles:
    if needle not in wrestle_input + text("src/fix39/wm_arcade_wrestle_input.h"):
        failures.append(f"wrestle input missing source marker/function: {needle}")

movement_needles = [
    "wm_arcade_calc_ground_y",
    "wm_arcade_wrestler_veladd",
    "wm_arcade_wrestler_friction",
    "wm_arcade_xflip_joy",
    "wm_arcade_stick_relative_new",
]
for needle in movement_needles:
    if needle not in movement:
        failures.append(f"movement core missing function: {needle}")

state_needles = [
    "whack_cnt", "puppet_time", "ring_time", "last_fling", "last_hiptoss",
    "last_spunch", "last_skick", "buckoff_count", "damage_given", "new_wrestler_num",
]
for needle in state_needles:
    if needle not in header:
        failures.append(f"combat actor missing remaining source-facing state: {needle}")

# The regression must exercise all non-SMOVE core surfaces that this pass claims.
test_needles = [
    "wm_arcade_set_hurt_box",
    "wm_arcade_set_attack_box",
    "wm_arcade_try_attack_hit",
    "wm_arcade_check_wrestler_collisions",
    "wm_arcade_resolve_overlap",
    "wm_arcade_set_getup_time",
    "wm_arcade_auto_pin_check",
    "wm_arcade_can_pin",
    "wm_arcade_wrestler_countdown_tail",
    "wm_arcade_reset_wrestle2_state",
    "wm_arcade_update_joystat",
    "wm_arcade_update_joy_dtime",
    "wm_arcade_xflip_joy",
    "wm_arcade_stick_relative_new",
]
for needle in test_needles:
    if needle not in test:
        failures.append(f"non-SMOVE regression does not exercise: {needle}")

bad_words = ["TODO combat", "placeholder combat", "fake-complete", "same purpose"]
for rel in [
    "src/fix39/wm_arcade_combat.c",
    "src/fix39/wm_arcade_wrestle_core.c",
    "src/fix39/wm_arcade_wrestle_input.c",
    "src/fix39/wm_arcade_movement.c",
]:
    body = text(rel).lower()
    for bad in bad_words:
        if bad in body:
            failures.append(f"suspicious non-SMOVE combat marker in {rel}: {bad}")

lines = [
    "# Non-SMOVE combat source-reference report",
    "",
    "## Scope",
    "",
    "This gate is intentionally outside the Combat2ES SMOVE scheduler. It covers combat-facing core services that the rest of the port depends on:",
    "",
    "- collision boxes and overlap resolution",
    "- attack hit accept/reject gates and first-hit collision loop",
    "- GETUP timing and maybe-gidd-up callback",
    "- WRESTLE core pin/countdown/reset state",
    "- WRESTLE input history and dtime counters",
    "- movement xflip/relative stick helpers",
    "- remaining PLYR.EQU-style combat actor fields",
    "",
    "## Result",
    "",
]
if failures:
    lines.append(f"- status: FAIL ({len(failures)} finding(s))")
    lines.append("")
    lines.append("## Findings")
    lines.append("")
    for f in failures:
        lines.append(f"- {f}")
else:
    lines.append("- status: PASS")
    lines.append("- non_smove_core_runtime_regression: present")
    lines.append("- non_smove_source_reference_audit: present")
    lines.append("- source_semantic_markers: present")
    lines.append("- remaining_actor_state_fields: present")
    lines.append("")
    lines.append("## Findings")
    lines.append("")
    lines.append("- none")
REPORT.parent.mkdir(exist_ok=True)
REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")
if failures:
    print("non-SMOVE combat source-reference audit FAILED", file=sys.stderr)
    for f in failures:
        print(f" - {f}", file=sys.stderr)
    sys.exit(1)
print("non-SMOVE combat source-reference audit passed")
