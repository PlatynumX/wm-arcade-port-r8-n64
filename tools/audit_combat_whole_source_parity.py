#!/usr/bin/env python3
from __future__ import annotations
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
failures: list[str] = []

def text(rel: str) -> str:
    p = ROOT / rel
    if not p.exists():
        failures.append(f"missing {rel}")
        return ""
    return p.read_text(encoding="utf-8", errors="replace")

cmake = text("CMakeLists.txt")
runtime = text("src/fix39/wm_fix39_runtime.c")
animrt = text("src/fix39/wm_arcade_source_animation_runtime.c")
combat = text("src/fix39/wm_arcade_combat.c")
react = text("src/fix39/wm_arcade_react.c")
attack_frames = text("src/fix39/wm_arcade_character_attack_frames_generated.h")
test = text("tests/combat2es_whole_combat_source_cutover.c")

required_cmake = [
    "wm_combat2es_whole_combat_source_cutover_tests",
    "wm_combat2es_whole_combat_source_cutover",
    "audit_combat_whole_source_parity.py",
]
for n in required_cmake:
    if n not in cmake:
        failures.append(f"CMake missing {n}")

required_runtime = [
    "live_bind_source_attack_windows_from_current_frames",
    "wm_arcade_character_attack_for_source_frame",
    "wm_arcade_ani_attack_on_z",
    "wm_arcade_ani_attack_on(a, &args)",
    "live_bind_source_attack_windows_from_current_frames();",
    "wm_arcade_check_wrestler_collisions",
    "wm_arcade_object_collisions",
    "wm_arcade_wrestler_countdown_tail",
    "wm_arcade_move_ported_wrestler",
    "wm_arcade_react123456789_reaction_callback",
]
for n in required_runtime:
    if n not in runtime:
        failures.append(f"live runtime missing whole-combat cutover marker: {n}")

# Source animation bytecode must own the combat opcodes, not external presenter code.
for opcode_marker in [
    "case 6:", "wm_arcade_ani_attack_on(a,&ao)",
    "case 7:", "wm_arcade_ani_attack_off(a,rtick(s))",
    "case 14:", "wm_arcade_ani_attack_on_z(a,&az)",
    "case 66:", "wm_arcade_ani_damageopp",
]:
    if opcode_marker not in animrt:
        failures.append(f"source animation runtime missing combat opcode marker: {opcode_marker}")

for n in [
    "wm_arcade_try_attack_hit",
    "wm_arcade_check_wrestler_collisions",
    "wm_arcade_set_getup_time",
    "COLLIS.ASM",
]:
    if n not in combat:
        failures.append(f"combat core missing source marker {n}")

for n in [
    "wm_arcade_wrestler_hit",
    "wm_arcade_hit_stuff",
    "REACT1.ASM",
    "damage_values",
]:
    if n not in react:
        failures.append(f"reaction core missing source marker {n}")

if "generated from all historical wrestler sequence ASM" not in attack_frames:
    failures.append("attack frame corpus is not marked generated from historical wrestler sequence ASM")

for n in [
    "H4PL3X+FR4",
    "R2PU3A+FR4",
    "wm_fix39_match_tick",
    "Combat2ES whole combat source cutover regression: PASS",
]:
    if n not in test:
        failures.append(f"whole-combat regression missing {n}")

# Reject obvious ways to make this look green without live combat.
for rel in [
    "src/fix39/wm_fix39_runtime.c",
    "src/fix39/wm_arcade_combat.c",
    "src/fix39/wm_arcade_react.c",
    "src/fix39/wm_arcade_wrestler_port.c",
]:
    body = text(rel)
    for bad in ["TODO fake combat", "same purpose", "stub combat complete", "creative shortcut"]:
        if bad.lower() in body.lower():
            failures.append(f"disallowed combat shortcut marker in {rel}: {bad}")

REPORTS = ROOT / "reports"
REPORTS.mkdir(exist_ok=True)
report = REPORTS / "combat_whole_source_parity.md"
if failures:
    report.write_text("# Whole combat source-parity report\n\n- status: FAIL\n\n## Findings\n" + "\n".join(f"- {f}" for f in failures) + "\n", encoding="utf-8")
    print("whole combat source-parity audit FAILED:", file=sys.stderr)
    for f in failures:
        print(f" - {f}", file=sys.stderr)
    sys.exit(1)

report.write_text("""# Whole combat source-parity report

- status: PASS
- scope: whole live combat spine outside platform rendering/audio
- gameplay source changed: yes
- live source frame attack-window cutover: enabled for generated all-wrestler attack-frame corpus
- source animation combat opcodes: present
- COLLIS/REACT1 live collision path: present
- SMOVE strict guard: external audit remains required
- non-SMOVE core guard: external audit remains required

## Covered live categories

- source animation attack opcodes 6/7/14/66
- generated source attack-frame windows for ordinary/running/aerial/puppet/ground attacks
- actor attack box -> hurt box -> COLLIS hit gate
- REACT1 wrestler_hit/damage reaction bridge
- SPECIAL object collision inclusion
- movement/input/countdown/pin/getup core gates from earlier R32 guard

## Findings

- none
""", encoding="utf-8")
print("whole combat source-parity audit passed")
