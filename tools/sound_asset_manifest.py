#!/usr/bin/env python3
"""Generate the complete source-referenced DCS command manifest.

The manifest distinguishes table-routed commands (four logical channel variants)
from direct SNDSND/crowd calls and DCS control words.  It is intended to drive
asset extraction/coverage auditing; it does not guess missing PCM.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("wm_import_dcssound", ROOT / "import_dcssound.py")
IMP = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = IMP
SPEC.loader.exec_module(IMP)

EXTRA_DIRECT_COMMANDS = {
    11: "wmania_tune",
    13: "wmania_tune",
    14: "wmania_tune",
    0x0200: "select_speech_doink",
    0x0300: "select_speech_razor",
    1005: "ATTRACT.ASM DCS logo",
}
CONTROL_WORDS = [0, 994, 995, 996, 997, 0x55AA, 0x55AB, 0x55AC, 0x55AD, 0x55AE]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("dcssound", type=pathlib.Path)
    ap.add_argument("sound_equ", type=pathlib.Path)
    ap.add_argument("output", type=pathlib.Path)
    args = ap.parse_args()

    dcs = args.dcssound.read_text(encoding="latin-1")
    symbols = IMP.load_source_symbols(args.dcssound, args.sound_equ)
    lines = dcs.splitlines()
    labels = IMP.label_positions(lines)
    triple = IMP.parse_triple(lines, labels, symbols)
    crowd = IMP.parse_crowd(lines, labels, symbols)

    commands: dict[int, dict] = {}
    for idx, (pd, base, comment) in enumerate(triple):
        if pd == 0 or base == 0:
            continue
        for logical in range(4):
            cmd = (base + logical) & 0xffff
            e = commands.setdefault(cmd, {"command": cmd, "routes": []})
            e["routes"].append({
                "kind": "triple",
                "triple_index": idx,
                "source_channel": logical + 1,
                "base_command": base,
                "comment": comment,
            })

    for table in crowd:
        for entry in table.entries:
            cmd = entry[0]
            if cmd == 0:
                continue
            e = commands.setdefault(cmd, {"command": cmd, "routes": []})
            e["routes"].append({"kind": "crowd", "table": table.name})

    for cmd, owner in EXTRA_DIRECT_COMMANDS.items():
        e = commands.setdefault(cmd, {"command": cmd, "routes": []})
        e["routes"].append({"kind": "direct", "owner": owner})

    out = {
        "schema": 1,
        "note": "Every command is source-referenced. Presence here does not claim that a WAV/WAV64 asset already exists.",
        "playable_commands": [commands[k] for k in sorted(commands)],
        "control_words": CONTROL_WORDS,
        "counts": {
            "unique_playable_commands": len(commands),
            "control_words": len(CONTROL_WORDS),
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(out, indent=2) + "\n", encoding="utf-8")
    print(f"generated {args.output}: {len(commands)} unique playable DCS commands")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
