#!/usr/bin/env python3
"""Combat2ES/R13 source-parity audit gate.
This gate catches the two mistakes that caused R11/R12 to look green while combat
was still structurally wrong: disabled GAME.EQU finish entries left live, and
missing persistent SMOVE_PID runtime wiring.
"""
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
if "src/fix39/wm_arcade_smove_runtime.c" not in cmake:
    failures.append("CMake does not compile wm_arcade_smove_runtime.c")
if "wm_combat_source_parity_audit" not in cmake:
    failures.append("CMake does not run combat source parity audit")

runtime = text("src/fix39/wm_fix39_runtime.c")
for needle in [
    "wm_arcade_smove_runtime_t smoves",
    "wm_arcade_smove_init_for_wrestler",
    "wm_arcade_smove_runtime_tick",
    "source_label_token",
    "live_start_source_anim(a, label, false)",
]:
    if needle not in runtime:
        failures.append(f"runtime missing {needle}")

# GAME.EQU verified by the R11e audit: only Undertaker finish 1 compiles in.
disabled_finish_re = re.compile(r'"(?:hrt|rzr|yok|shn|bam|dnk|lex)_finish_move[12]"|"und_finish_move2"')
for rel in [
    "src/fix39/wm_arcade_wrestler_port.c",
    "src/fix39/wm_arcade_taker.c",
    "src/fix39/wm_arcade_yoko.c",
    "src/fix39/wm_arcade_shawn.c",
    "src/fix39/wm_arcade_bam.c",
    "src/fix39/wm_arcade_doink.c",
    "src/fix39/wm_arcade_lex.c",
]:
    body = text(rel)
    for m in disabled_finish_re.finditer(body):
        failures.append(f"disabled GAME.EQU finish remains live in {rel}: {m.group(0)}")

smove = text("src/fix39/wm_arcade_smove_runtime.c")
for needle in [
    "wm_arcade_smove_waitswitch_down",
    "actor->special_move_addr",
    "timeout = (uint16_t)(*timeout_io - 1u)",
    "und_hdhold_neckbrk",
    "und_finish_move1",
]:
    if needle not in smove:
        failures.append(f"smove runtime missing source semantic marker {needle}")

# Do not allow the old side-state arrays to be added back for source-owned LAST_* fields.
for bad in ["native_last_fling[", "native_last_hiptoss[", "native_last_skick[", "native_last_spunch["]:
    if bad in runtime:
        failures.append(f"source-owned PLYR LAST_* state still shadowed by runtime side array: {bad}")

if failures:
    print("combat source-parity audit FAILED:", file=sys.stderr)
    for f in failures:
        print(f" - {f}", file=sys.stderr)
    sys.exit(1)
print("combat source-parity audit passed")
