#!/usr/bin/env python3
"""Inventory the complete historical WrestleMania source tree.

This is deliberately parser-light: it establishes whole-tree coverage and gives
us machine-readable counts while individual subsystems are translated to C.
"""
from __future__ import annotations
import argparse
import json
import pathlib
import re
import sys

SUBR_RE = re.compile(r"^\s*SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b", re.I | re.M)
FRAME_RE = re.compile(r"\b([A-Za-z][A-Za-z0-9_]*)\s*\+\s*FR([0-9]+)\b", re.I)
ANI_RE = re.compile(r"\bANI_[A-Za-z0-9_]+\b", re.I)

ROSTER = [
    ("Bret Hart", "BRET.ASM", "hrt"),
    ("Bam Bam Bigelow", "BAM.ASM", "bam"),
    ("Yokozuna", "YOKO.ASM", "yok"),
    ("Doink", "DOINK.ASM", "dnk"),
    ("Razor Ramon", "RAZOR.ASM", "rzr"),
    ("Lex Luger", "LEX.ASM", "lex"),
    ("Undertaker", "TAKER.ASM", "und"),
    ("Shawn Michaels", "SHAWN.ASM", "shn"),
]


def inventory(root: pathlib.Path) -> dict:
    if not root.is_dir():
        raise ValueError(f"source root not found: {root}")
    asm = sorted(root.glob("*.ASM")) + sorted(root.glob("*.asm"))
    # De-duplicate case-insensitive globs on case-insensitive hosts.
    unique = {}
    for p in asm:
        unique[p.name.upper()] = p
    asm = [unique[k] for k in sorted(unique)]
    seq_files = [p for p in asm if "SEQ" in p.stem.upper()]
    subr_total = 0
    frames: set[str] = set()
    opcodes: set[str] = set()
    module_stats = []
    for p in asm:
        text = p.read_text(errors="replace")
        subrs = SUBR_RE.findall(text)
        frame_refs = {f"{m.group(1).upper()}{int(m.group(2)):02d}" for m in FRAME_RE.finditer(text)}
        anis = {m.group(0).upper() for m in ANI_RE.finditer(text)}
        subr_total += len(subrs)
        frames |= frame_refs
        opcodes |= anis
        if subrs or frame_refs:
            module_stats.append({"file": p.name, "subroutines": len(subrs), "frames": len(frame_refs)})

    roster = []
    for name, module, prefix in ROSTER:
        exists = (root / module).is_file()
        finish_labels = 0
        for p in asm:
            txt = p.read_text(errors="replace")
            finish_labels += len(re.findall(rf"\bSUBR\s+{re.escape(prefix)}_finish[0-9]+_move\b", txt, re.I))
        roster.append({"name": name, "module": module, "module_present": exists,
                       "finish_labels": finish_labels})

    lods = sorted((root / "IMG").glob("*.LOD")) if (root / "IMG").is_dir() else []
    imgs = sorted((root / "IMG").glob("*.IMG")) if (root / "IMG").is_dir() else []
    return {
        "asm_files": len(asm),
        "sequence_files": len(seq_files),
        "subroutines": subr_total,
        "unique_frame_refs": len(frames),
        "unique_animation_opcodes": len(opcodes),
        "lod_files": len(lods),
        "wimp_img_files": len(imgs),
        "roster": roster,
        "largest_modules": sorted(module_stats, key=lambda x: (x["subroutines"] + x["frames"]), reverse=True)[:20],
    }


def render_md(data: dict) -> str:
    lines = [
        "# Whole-source inventory",
        "",
        "Generated from the historical Midway source tree during CI.",
        "",
        f"- ASM files: **{data['asm_files']}**",
        f"- Sequence-style ASM files: **{data['sequence_files']}**",
        f"- SUBR/SUBRP definitions: **{data['subroutines']}**",
        f"- Unique frame references: **{data['unique_frame_refs']}**",
        f"- Unique ANI_* opcodes referenced: **{data['unique_animation_opcodes']}**",
        f"- IMG/*.LOD manifests: **{data['lod_files']}**",
        f"- IMG/*.IMG WIMP containers: **{data['wimp_img_files']}**",
        "",
        "## Roster source presence",
        "",
        "| Wrestler | Main module | Present | Finish labels |",
        "|---|---|---:|---:|",
    ]
    for r in data["roster"]:
        lines.append(f"| {r['name']} | `{r['module']}` | {'yes' if r['module_present'] else 'NO'} | {r['finish_labels']} |")
    lines += ["", "## Largest source modules by discovered routines/frame references", ""]
    for m in data["largest_modules"]:
        lines.append(f"- `{m['file']}`: {m['subroutines']} subroutines, {m['frames']} unique frame refs")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True, type=pathlib.Path)
    ap.add_argument("--json", type=pathlib.Path)
    ap.add_argument("--md", type=pathlib.Path)
    ap.add_argument("--strict-roster", action="store_true")
    ns = ap.parse_args()
    data = inventory(ns.root)
    if ns.strict_roster:
        missing = [r["module"] for r in data["roster"] if not r["module_present"]]
        if missing:
            raise ValueError("missing roster modules: " + ", ".join(missing))
    if ns.json:
        ns.json.parent.mkdir(parents=True, exist_ok=True)
        ns.json.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n")
    md = render_md(data)
    if ns.md:
        ns.md.parent.mkdir(parents=True, exist_ok=True)
        ns.md.write_text(md)
    else:
        print(md, end="")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"source_inventory: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
