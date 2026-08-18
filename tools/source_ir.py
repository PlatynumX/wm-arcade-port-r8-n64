#!/usr/bin/env python3
"""Build a conservative routine/call/process graph from the original ASM tree.

This is a port-frontier tool, not an interpreter: source operations that cannot
be statically resolved stay explicit in the report instead of being guessed.
"""
from __future__ import annotations
import argparse, json, pathlib, re, sys
from collections import deque

SUBR_RE = re.compile(r"^\s*SUBR(P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b", re.I)
CALL_RE = re.compile(r"\b(?:JSRP|CALLA|CALLR)\s+#?([A-Za-z_][A-Za-z0-9_]*)\b", re.I)
CREATE_RE = re.compile(r"\bCREATE0?\b[^;\n]*?,\s*#?([A-Za-z_][A-Za-z0-9_]*)\b", re.I)
OP_RE = re.compile(r"^\s*([A-Za-z_.][A-Za-z0-9_.]*)\b")

SKIP_OPS = {"subr", "subrp", ".word", ".long", ".byte", ".include", ".ref", ".if", ".endif",
            ".bss", ".file", ".title", ".width", ".option", ".mnolist", ".end"}


def scan_file(path: pathlib.Path) -> dict[str, dict]:
    routines: dict[str, dict] = {}
    current: dict | None = None
    for no, raw in enumerate(path.read_text(errors="replace").splitlines(), 1):
        code = raw.split(";", 1)[0].rstrip()
        if not code.strip():
            continue
        m = SUBR_RE.match(code)
        if m:
            name = m.group(2)
            current = routines.setdefault(name, {"name": name, "file": path.name, "line": no,
                                                   "calls": [], "creates": [], "ops": {},
                                                   "dynamic": []})
            continue
        if current is None:
            continue
        for cm in CALL_RE.finditer(code):
            target = cm.group(1)
            if target.lower() not in {x.lower() for x in current["calls"]}:
                current["calls"].append(target)
        for pm in CREATE_RE.finditer(code):
            target = pm.group(1)
            if target.lower() not in {x.lower() for x in current["creates"]}:
                current["creates"].append(target)
        if re.search(r"\b(?:CALL|JUMP)\s+a\d+\b|\bGETPRC\b", code, re.I):
            current["dynamic"].append({"line": no, "text": code.strip()})
        om = OP_RE.match(code)
        if om:
            op = om.group(1).lower()
            if op not in SKIP_OPS and not op.startswith("#"):
                current["ops"][op] = current["ops"].get(op, 0) + 1
    return routines


def build(root: pathlib.Path) -> dict:
    routines: dict[str, dict] = {}
    for p in sorted(root.rglob("*.ASM")) + sorted(root.rglob("*.asm")):
        for name, rec in scan_file(p).items():
            # First definition wins; duplicate source symbols are reported.
            if name in routines:
                rec["duplicate_of"] = routines[name]["file"]
                continue
            routines[name] = rec
    by_lower = {k.lower(): k for k in routines}
    unresolved: dict[str, list[str]] = {}
    for name, rec in routines.items():
        for target in rec["calls"] + rec["creates"]:
            if target.lower() not in by_lower:
                unresolved.setdefault(target, []).append(name)
    return {"routine_count": len(routines), "routines": routines,
            "unresolved_targets": unresolved}


def closure(data: dict, root_name: str) -> dict:
    routines = data["routines"]
    by_lower = {k.lower(): k for k in routines}
    canonical = by_lower.get(root_name.lower())
    if canonical is None:
        return {"root": root_name, "defined": False, "routines": [], "unresolved": [root_name]}
    seen, unresolved = set(), set()
    q = deque([canonical])
    while q:
        name = q.popleft()
        if name in seen: continue
        seen.add(name)
        rec = routines[name]
        for target in rec["calls"] + rec["creates"]:
            c = by_lower.get(target.lower())
            if c is None: unresolved.add(target)
            elif c not in seen: q.append(c)
    return {"root": canonical, "defined": True, "routines": sorted(seen),
            "unresolved": sorted(unresolved)}


def render_md(data: dict, roots: list[str]) -> str:
    lines = ["# Source dependency frontier", "",
             "Generated mechanically from the historical ASM tree. It is a translation queue, not an emulator.", "",
             f"- Discovered `SUBR/SUBRP` routines: **{data['routine_count']}**",
             f"- Unresolved static call/process targets: **{len(data['unresolved_targets'])}**", ""]
    for root in roots:
        c = closure(data, root)
        lines += [f"## `{root}`", ""]
        if not c["defined"]:
            lines += ["- Definition not discovered as `SUBR/SUBRP`; target remains on the frontier.", ""]
            continue
        dyn = sum(len(data["routines"][r]["dynamic"]) for r in c["routines"])
        lines += [f"- Static closure: **{len(c['routines'])}** routines",
                  f"- Unresolved targets in closure: **{len(c['unresolved'])}**",
                  f"- Dynamic call/GETPRC sites in closure: **{dyn}**", ""]
        if c["unresolved"]:
            lines += ["Unresolved: " + ", ".join(f"`{x}`" for x in c["unresolved"][:40]), ""]
    return "\n".join(lines) + "\n"


def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--root", required=True, type=pathlib.Path)
    ap.add_argument("--json", required=True, type=pathlib.Path)
    ap.add_argument("--md", required=True, type=pathlib.Path)
    ap.add_argument("--root-routine", action="append", default=[])
    ns=ap.parse_args()
    data=build(ns.root)
    roots=ns.root_routine or ["attract_mode", "start_match"]
    data["closures"]={r:closure(data,r) for r in roots}
    ns.json.parent.mkdir(parents=True, exist_ok=True); ns.md.parent.mkdir(parents=True, exist_ok=True)
    ns.json.write_text(json.dumps(data, indent=2, sort_keys=True)+"\n")
    ns.md.write_text(render_md(data, roots))
    print(f"source IR: {data['routine_count']} routines; roots={','.join(roots)}")
    return 0

if __name__ == "__main__":
    try: raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"source_ir: error: {exc}", file=sys.stderr); raise SystemExit(2)
