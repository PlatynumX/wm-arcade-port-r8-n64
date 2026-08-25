#!/usr/bin/env python3
"""Audit live combat source tables against the final Midway source manifest.

Default mode is structural: fail when the port has live tables that contradict
GAME.EQU / *_smove_table or contains known cheat/prune patterns.

Use --strict-complete when the target is full source-accurate combat closure;
that additionally fails for active SMOVE monitor labels whose runtime bodies are
not yet explicitly represented in wm_arcade_smove_runtime.c.
"""
from __future__ import annotations
import argparse
import json
import re
import sys
from pathlib import Path

from combat_source_manifest import (
    ACTIVE_SMOVE_TABLES,
    ACTIVE_SECRET_TABLES,
    DISABLED_FINISH_LABELS,
    GAME_EQU_FINISH_COUNTS,
    ROSTER_TABLES,
)

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    p = ROOT / rel
    if not p.exists():
        raise AssertionError(f"missing required file: {rel}")
    return p.read_text(encoding="utf-8", errors="replace")


def extract_string_array(text: str, name: str) -> list[str]:
    # Handles both `name[] = { ... };` and `name[]={...};`.
    pat = re.compile(
        r"static\s+const\s+char\s*\*\s*const\s+" + re.escape(name) +
        r"\s*\[\s*\]\s*=\s*\{(?P<body>.*?)\}\s*;",
        re.S,
    )
    m = pat.search(text)
    if not m:
        raise AssertionError(f"could not locate string array {name}")
    return re.findall(r'"([^"]+)"', m.group("body"))


def source_runtime_body_labels() -> set[str]:
    p = ROOT / "src/fix39/wm_arcade_smove_runtime.c"
    if not p.exists():
        return set()
    text = p.read_text(encoding="utf-8", errors="replace")
    return set(re.findall(r'"((?:hrt|rzr|und|yok|shn|bam|dnk|lex|std)_[^"]+)"', text))


def audit_tables() -> list[str]:
    errors: list[str] = []
    for wrestler, expected in ACTIVE_SMOVE_TABLES.items():
        rel, array_name = ROSTER_TABLES[wrestler]
        try:
            actual = extract_string_array(read(rel), array_name)
        except AssertionError as e:
            errors.append(f"{wrestler}: {e}")
            continue
        if actual != expected:
            errors.append(
                f"{wrestler}: live smove table mismatch\n"
                f"  expected={expected}\n"
                f"  actual  ={actual}"
            )
        for disabled in DISABLED_FINISH_LABELS:
            if disabled in actual:
                errors.append(f"{wrestler}: disabled GAME.EQU finisher is live: {disabled}")
    return errors


def audit_no_cheats() -> list[str]:
    errors: list[str] = []
    cmake = read("CMakeLists.txt")
    for rel in ["src/generated/bret_sprites.c", "src/generated/character_assets.c"]:
        if f"COMBAT2ES_R13B_PRUNED_MISSING_SOURCE {rel}" in cmake:
            errors.append(f"CMake still contains R13B prune marker for real source: {rel}")
        if rel not in cmake:
            errors.append(f"CMake does not compile required generated asset source: {rel}")
        if not (ROOT / rel).exists():
            errors.append(f"required generated asset source missing on disk: {rel}")

    runtime_rel = "src/fix39/wm_fix39_runtime.c"
    if (ROOT / runtime_rel).exists():
        runtime = read(runtime_rel)
        for bad in ["native_last_fling[", "native_last_hiptoss[", "native_last_skick[", "native_last_spunch["]:
            if bad in runtime:
                errors.append(f"source-owned LAST_* state still stored in runtime side array: {bad}")

    smove_rel = "src/fix39/wm_arcade_smove_runtime.c"
    if (ROOT / smove_rel).exists():
        smove = read(smove_rel)
        if "WM_BTN_ATTACK_MASK" in smove or "stick_rel_new & 0x000f" in smove:
            errors.append("WAITSWITCH_DWN port is pre-masking input; source uses raw (BUT_VAL_DOWN << 4) | STICK_REL_NEW before andni MASK")
        if "(((actor->but_val_down << 4) | actor->stick_rel_new)" not in smove and "((actor->but_val_down << 4) | actor->stick_rel_new)" not in smove:
            errors.append("WAITSWITCH_DWN raw input expression was not found")
    else:
        errors.append(f"missing {smove_rel}; SMOVE_PID runtime not installed")

    all_fix39 = "\n".join(p.read_text(encoding="utf-8", errors="replace") for p in (ROOT / "src/fix39").glob("wm_arcade*.c"))
    malformed = re.search(r"\{\s*,\s*WM_[A-Z0-9_]+_MON_FINISH[12]\s*\}", all_fix39)
    if malformed:
        errors.append(f"malformed removed finisher entry still present: {malformed.group(0)}")
    return errors


def completion_report() -> dict[str, object]:
    body_labels = source_runtime_body_labels()
    unresolved: dict[str, list[str]] = {}
    resolved: dict[str, list[str]] = {}
    for wrestler, labels in ACTIVE_SMOVE_TABLES.items():
        unresolved[wrestler] = [x for x in labels if x not in body_labels and not x.startswith("std_")]
        resolved[wrestler] = [x for x in labels if x in body_labels or x.startswith("std_")]
    return {
        "finish_counts": GAME_EQU_FINISH_COUNTS,
        "active_smove_tables": ACTIVE_SMOVE_TABLES,
        "active_secret_tables": ACTIVE_SECRET_TABLES,
        "translated_or_standard_smove_labels_visible_to_runtime": resolved,
        "unresolved_smove_monitor_bodies": unresolved,
    }


def write_report(report: dict[str, object]) -> None:
    out = ROOT / "build" / "combat_source_completion_report.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    md = ROOT / "build" / "combat_source_completion_report.md"
    lines = ["# Combat source completion report", "", "## Unresolved active SMOVE monitor bodies", ""]
    unresolved = report["unresolved_smove_monitor_bodies"]
    assert isinstance(unresolved, dict)
    for wrestler, labels in unresolved.items():
        lines.append(f"### {wrestler}")
        if labels:
            for label in labels:
                lines.append(f"- {label}")
        else:
            lines.append("- none")
        lines.append("")
    md.write_text("\n".join(lines), encoding="utf-8")


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--strict-complete", action="store_true", help="fail if any active SMOVE monitor body is not translated")
    args = ap.parse_args(argv)

    errors = []
    errors.extend(audit_tables())
    errors.extend(audit_no_cheats())
    report = completion_report()
    write_report(report)

    unresolved = report["unresolved_smove_monitor_bodies"]
    if args.strict_complete:
        for wrestler, labels in unresolved.items():
            if labels:
                errors.append(f"{wrestler}: unresolved active SMOVE bodies: {labels}")

    if errors:
        print("Combat source manifest audit FAILED")
        for e in errors:
            print("- " + e)
        print("Report written to build/combat_source_completion_report.md/json")
        return 1
    print("Combat source manifest audit OK")
    print("Report written to build/combat_source_completion_report.md/json")
    if not args.strict_complete:
        print("Note: default mode is structural. Use --strict-complete for the final source-accurate-complete gate.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
