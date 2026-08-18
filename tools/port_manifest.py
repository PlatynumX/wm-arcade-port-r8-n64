#!/usr/bin/env python3
"""Generate C/report data from the explicit source-port translation manifest."""
from __future__ import annotations
import argparse
import json
import pathlib
import re
import sys

STATUS_TO_ENUM = {
    "not-started": "WM_PORT_NOT_STARTED",
    "partial-source": "WM_PORT_PARTIAL_SOURCE",
    "exact-source": "WM_PORT_EXACT_SOURCE",
    "harness-only": "WM_PORT_HARNESS_ONLY",
}

CALL_TO_ENUM = {
    "show_hstd": "WM_ATTRACT_SHOW_HSTD",
    "DCS_LOGO": "WM_ATTRACT_DCS_LOGO",
    "show_sports_logo": "WM_ATTRACT_SHOW_SPORTS_LOGO",
    "show_gameplay": "WM_ATTRACT_SHOW_GAMEPLAY",
    "creditscreen": "WM_ATTRACT_CREDITSCREEN",
    "show_title": "WM_ATTRACT_SHOW_TITLE",
    "DO_HINTS": "WM_ATTRACT_DO_HINTS",
    "show_gen_tips": "WM_ATTRACT_SHOW_GEN_TIPS",
    "show_bios": "WM_ATTRACT_SHOW_BIOS",
    "show_bios_tips": "WM_ATTRACT_SHOW_BIOS_TIPS",
    "show_operatormsg": "WM_ATTRACT_SHOW_OPERATORMSG",
    "show_time_date": "WM_ATTRACT_SHOW_TIME_DATE",
    "show_copyright": "WM_ATTRACT_SHOW_COPYRIGHT",
    "aama_message": "WM_ATTRACT_AAMA_MESSAGE",
}

SUBR_RE = re.compile(r"^\s*SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b", re.I | re.M)


def load(path: pathlib.Path) -> dict:
    data = json.loads(path.read_text())
    attract = data.get("attract", {})
    if set(attract) != set(CALL_TO_ENUM):
        missing = sorted(set(CALL_TO_ENUM) - set(attract))
        extra = sorted(set(attract) - set(CALL_TO_ENUM))
        raise ValueError(f"attract manifest mismatch missing={missing} extra={extra}")
    for group in (attract, data.get("systems", {})):
        for name, ent in group.items():
            if ent.get("status") not in STATUS_TO_ENUM:
                raise ValueError(f"bad status for {name}: {ent.get('status')}")
    return data


def emit_c(data: dict, out: pathlib.Path) -> None:
    lines = [
        "/* Auto-generated from port/translation_manifest.json. */",
        '#include "wm/attract.h"',
        "",
        "wm_port_status wm_attract_call_port_status(wm_attract_call call) {",
        "    switch (call) {",
    ]
    for label, ent in data["attract"].items():
        lines.append(f"        case {CALL_TO_ENUM[label]}: return {STATUS_TO_ENUM[ent['status']]};")
    lines += [
        "        case WM_ATTRACT_CALL_COUNT: break;",
        "    }",
        "    return WM_PORT_NOT_STARTED;",
        "}",
        "",
        "const char *wm_port_status_name(wm_port_status status) {",
        "    switch (status) {",
        '        case WM_PORT_NOT_STARTED: return "not-started";',
        '        case WM_PORT_PARTIAL_SOURCE: return "partial-source";',
        '        case WM_PORT_EXACT_SOURCE: return "exact-source";',
        '        case WM_PORT_HARNESS_ONLY: return "harness-only";',
        "    }",
        '    return "?";',
        "}",
        "",
    ]
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines))


def source_labels(root: pathlib.Path) -> set[str]:
    labels: set[str] = set()
    for p in root.glob("*.ASM"):
        labels |= {m.group(1) for m in SUBR_RE.finditer(p.read_text(errors="replace"))}
    return {x.lower() for x in labels}


def verify_source(data: dict, root: pathlib.Path) -> list[str]:
    labels = source_labels(root)
    absent = []
    # Some manifest names can be macro labels rather than SUBR definitions;
    # these are still searched textually across the root before being called absent.
    all_text = "\n".join(p.read_text(errors="replace") for p in root.glob("*.ASM"))
    for name in data["attract"]:
        if name.lower() not in labels and not re.search(rf"\b{re.escape(name)}\b", all_text, re.I):
            absent.append(name)
    return absent


def emit_md(data: dict, out: pathlib.Path) -> None:
    lines = [
        f"# Source-port coverage — {data.get('revision','?')}", "",
        data.get("policy", ""), "",
        "## Attract/frontend entry points", "",
        "| Original entry point | Status | Note |", "|---|---|---|",
    ]
    for name, ent in data["attract"].items():
        lines.append(f"| `{name}` | **{ent['status']}** | {ent.get('note','')} |")
    lines += ["", "## Shared systems", "", "| System | Status | Note |", "|---|---|---|"]
    for name, ent in data.get("systems", {}).items():
        lines.append(f"| `{name}` | **{ent['status']}** | {ent.get('note','')} |")
    lines += ["", "`harness-only` is never eligible for normal arcade execution.", ""]
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifest", required=True, type=pathlib.Path)
    ap.add_argument("--out-c", type=pathlib.Path)
    ap.add_argument("--out-md", type=pathlib.Path)
    ap.add_argument("--source-root", type=pathlib.Path)
    ns = ap.parse_args()
    data = load(ns.manifest)
    if ns.out_c: emit_c(data, ns.out_c)
    if ns.out_md: emit_md(data, ns.out_md)
    if ns.source_root:
        absent = verify_source(data, ns.source_root)
        if absent:
            raise ValueError("manifest names absent from original source: " + ", ".join(absent))
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"port_manifest: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
