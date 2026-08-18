#!/usr/bin/env python3
"""Normalize historical animation data statements without inventing semantics.

The Midway animation source mixes WORD and LONG fields.  This tool preserves
that packing across the complete source tree and emits a mechanical frontier
for data forms which still need a translator.  It intentionally does *not*
resolve symbolic pointers or emulate animation commands.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys
from collections import Counter

SUBR_RE = re.compile(r"^\s*SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b", re.I)
DATA_RE = re.compile(r"^\s*\.(word|long)\s+(.+?)\s*$", re.I)
MACRO_RE = re.compile(r"^\s*([WL]{1,8})\s+(.+?)\s*$", re.I)
LABEL_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*):\s*$")
DIRECTIVE_RE = re.compile(r"^\s*\.")
ANI_SYMBOL_RE = re.compile(r"\bANI_[A-Za-z0-9_]+\b", re.I)


def strip_comment(line: str) -> str:
    return line.split(";", 1)[0].rstrip()


def split_args(text: str) -> list[str]:
    """Split simple assembler macro/data arguments, preserving expressions."""
    out: list[str] = []
    cur: list[str] = []
    depth = 0
    quote: str | None = None
    for ch in text:
        if quote:
            cur.append(ch)
            if ch == quote:
                quote = None
            continue
        if ch in "\"'":
            quote = ch
            cur.append(ch)
        elif ch in "([":
            depth += 1
            cur.append(ch)
        elif ch in ")]":
            depth = max(0, depth - 1)
            cur.append(ch)
        elif ch == "," and depth == 0:
            item = "".join(cur).strip()
            if item:
                out.append(item)
            cur = []
        else:
            cur.append(ch)
    item = "".join(cur).strip()
    if item:
        out.append(item)
    return out


def typed_items(types: str, args: list[str], lineno: int, text: str) -> tuple[list[dict], dict | None]:
    if len(types) != len(args):
        return [], {"line": lineno, "text": text.strip(), "reason": f"{types} expects {len(types)} args; got {len(args)}"}
    return [
        {"type": typ, "expr": expr}
        for typ, expr in zip(types, args)
    ], None


def source_files(root: pathlib.Path) -> list[pathlib.Path]:
    seen: set[pathlib.Path] = set()
    out: list[pathlib.Path] = []
    for pat in ("*.ASM", "*.asm"):
        for p in root.rglob(pat):
            rp = p.resolve()
            if rp not in seen:
                seen.add(rp)
                out.append(p)
    return sorted(out)


def scan_file(path: pathlib.Path, root: pathlib.Path) -> list[dict]:
    routines: list[dict] = []
    current: dict | None = None
    started_data = False

    for no, raw in enumerate(path.read_text(errors="replace").splitlines(), 1):
        code = strip_comment(raw)
        if not code.strip():
            continue

        sub = SUBR_RE.match(code)
        if sub:
            if current is not None:
                current["has_typed_data"] = bool(current["items"])
                routines.append(current)
            current = {
                "name": sub.group(1),
                "file": path.relative_to(root).as_posix(),
                "line": no,
                "items": [],
                "ani_symbols": [],
                "unresolved": [],
                "executable_after_data": [],
            }
            started_data = False
            continue

        if current is None:
            continue

        dm = DATA_RE.match(code)
        if dm:
            typ = "W" if dm.group(1).lower() == "word" else "L"
            args = split_args(dm.group(2))
            items = [{"type": typ, "expr": a} for a in args]
            current["items"].extend(items)
            started_data = started_data or bool(items)
            for a in args:
                for s in ANI_SYMBOL_RE.findall(a):
                    u = s.upper()
                    if u not in current["ani_symbols"]:
                        current["ani_symbols"].append(u)
            continue

        mm = MACRO_RE.match(code)
        if mm:
            types = mm.group(1).upper()
            args = split_args(mm.group(2))
            items, err = typed_items(types, args, no, code)
            if err:
                current["unresolved"].append(err)
            else:
                current["items"].extend(items)
                started_data = started_data or bool(items)
                for a in args:
                    for s in ANI_SYMBOL_RE.findall(a):
                        u = s.upper()
                        if u not in current["ani_symbols"]:
                            current["ani_symbols"].append(u)
            continue

        # A nested local label or assembler directive can be part of a routine;
        # keep it explicit but don't turn it into invented stream bytes.
        if started_data:
            if LABEL_RE.match(code) or DIRECTIVE_RE.match(code):
                current["unresolved"].append({"line": no, "text": code.strip(), "reason": "non-data boundary inside typed-data routine"})
            else:
                current["executable_after_data"].append({"line": no, "text": code.strip()})

    if current is not None:
        current["has_typed_data"] = bool(current["items"])
        routines.append(current)
    return routines


def build(root: pathlib.Path) -> dict:
    routines: list[dict] = []
    for p in source_files(root):
        routines.extend(scan_file(p, root))

    typed = [r for r in routines if r["has_typed_data"]]
    type_counts = Counter()
    symbol_counts = Counter()
    unresolved_count = 0
    executable_after_data = 0
    for r in typed:
        type_counts.update(x["type"] for x in r["items"])
        symbol_counts.update(r["ani_symbols"])
        unresolved_count += len(r["unresolved"])
        executable_after_data += len(r["executable_after_data"])

    # Animation-like is deliberately conservative: source names commonly use
    # *_anim, and ANI_* tokens are direct evidence of animation command data.
    animation_like = [
        r for r in typed
        if r["name"].lower().endswith("_anim") or r["ani_symbols"]
    ]
    return {
        "schema": 1,
        "source_root": root.as_posix(),
        "routine_count": len(routines),
        "typed_data_routine_count": len(typed),
        "animation_like_routine_count": len(animation_like),
        "typed_item_counts": dict(sorted(type_counts.items())),
        "ani_symbol_counts": dict(sorted(symbol_counts.items())),
        "unresolved_data_forms": unresolved_count,
        "executable_lines_after_data": executable_after_data,
        "animation_like_routines": animation_like,
    }


def render_md(data: dict) -> str:
    items = data["typed_item_counts"]
    symbols = sorted(data["ani_symbol_counts"].items(), key=lambda kv: (-kv[1], kv[0]))
    lines = [
        "# Animation translation frontier",
        "",
        "Generated mechanically from the historical ASM tree. WORD/LONG packing is preserved; symbolic pointers and unknown source forms are not guessed.",
        "",
        f"- Source routines scanned: **{data['routine_count']}**",
        f"- Routines containing typed data: **{data['typed_data_routine_count']}**",
        f"- Animation-like typed routines: **{data['animation_like_routine_count']}**",
        f"- WORD items preserved: **{items.get('W', 0)}**",
        f"- LONG items preserved: **{items.get('L', 0)}**",
        f"- Unresolved non-data forms inside typed routines: **{data['unresolved_data_forms']}**",
        f"- Executable lines following typed data: **{data['executable_lines_after_data']}**",
        "",
        "## Most common `ANI_*` source tokens",
        "",
    ]
    if symbols:
        lines.extend(f"- `{name}`: {count}" for name, count in symbols[:50])
    else:
        lines.append("- No `ANI_*` tokens found.")
    lines += ["", "## Policy", "",
              "Anything not represented as an explicit WORD/LONG item remains on the translation frontier. The runtime must implement the original command semantics before such routines are executable.", ""]
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True, type=pathlib.Path)
    ap.add_argument("--json", required=True, type=pathlib.Path)
    ap.add_argument("--md", required=True, type=pathlib.Path)
    ns = ap.parse_args()
    data = build(ns.root)
    ns.json.parent.mkdir(parents=True, exist_ok=True)
    ns.md.parent.mkdir(parents=True, exist_ok=True)
    ns.json.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n")
    ns.md.write_text(render_md(data))
    print(
        "animation IR: "
        f"{data['animation_like_routine_count']} animation-like typed routines; "
        f"W={data['typed_item_counts'].get('W', 0)} "
        f"L={data['typed_item_counts'].get('L', 0)}"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"animation_ir: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
