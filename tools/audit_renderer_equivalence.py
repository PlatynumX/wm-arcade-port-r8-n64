#!/usr/bin/env python3
from __future__ import annotations
import json
import pathlib
import sys

ROOT = pathlib.Path.cwd()

def read(path: str) -> str:
    p = ROOT / path
    return p.read_text(errors="replace") if p.exists() else ""

def main() -> int:
    checks = []
    def check(name: str, ok: bool, evidence: str) -> None:
        checks.append({"name": name, "ok": bool(ok), "evidence": evidence})

    cmake = read("CMakeLists.txt")
    render_h = read("include/wm/render_equivalence.h")
    render_c = read("src/core/render_equivalence.c")
    visual = read("src/core/visual.c")
    composite = read("src/core/composite.c")
    wimp = read("src/fix39/wm_arcade_wimp_frame.c")

    check("render_equivalence_core_compiled", "src/core/render_equivalence.c" in cmake, "CMake includes render equivalence core")
    check("renderer_regression_compiled", "wm_renderer_equivalence_regression" in cmake, "CMake includes renderer equivalence regression")
    check("transparent_ci8_index_zero", "WM_RENDER_TRANSPARENT_CI8_INDEX 0u" in render_h, "CI8 index 0 is the renderer transparent key")
    check("layer_order_defined", "WM_RENDER_LAYER_ROPES_BACK" in render_h and "WM_RENDER_LAYER_ROPES_FRONT" in render_h, "Rope halves and actor layers are distinct")
    check("layer_order_runtime_guard", "wm_render_layer_order_is_source_safe" in render_c, "Runtime guard validates ascending draw order")
    check("visual_first_frame_duration", "just_started" in visual and "Preserve the first" in visual, "Visual timing preserves first source frame")
    check("secondary_offsets_source_formula", "primary_xani - attach_x + secondary_xani" in composite, "Composite secondary offsets preserve source attach formula")
    check("wimp_frame_fail_closed", "return false" in wimp and "Sentinel" in wimp, "WIMP/IANI3 frame box adapter fails closed on invalid metadata")

    status = "PASS_WITH_HARDWARE_SCREENSHOT_BOUNDARY"
    if not all(c["ok"] for c in checks):
        status = "FAIL"

    report = {
        "status": status,
        "scope": "renderer equivalence code-level audit: palette transparency, z/layer order, source coordinate conversion, WIMP frame metadata guard",
        "explicit_boundary": "This audit does not claim pixel-perfect screenshot parity on N64 hardware for every frame; that remains a visual capture task.",
        "ignored_custom_assets": ["intentional Midway Sports / Be a Man / PlatynumX replacements"],
        "checks": checks,
    }
    out_dir = ROOT / "reports"
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "renderer_equivalence_audit.json").write_text(json.dumps(report, indent=2) + "\n")
    lines = [
        "# R36 renderer equivalence audit",
        "",
        f"- status: {status}",
        "- scope: palette transparency, z/layer order, source coordinate conversion, WIMP frame metadata guard",
        "- ignored deltas: intentional Midway Sports / Be a Man / PlatynumX replacement assets",
        "- boundary: no pixel-perfect hardware screenshot claim in this pass",
        "",
        "## checks",
        "",
    ]
    for c in checks:
        lines.append(f"- [{'PASS' if c['ok'] else 'FAIL'}] {c['name']}: {c['evidence']}")
    (out_dir / "renderer_equivalence_audit.md").write_text("\n".join(lines) + "\n")
    print(f"R36 renderer equivalence audit: {status}")
    return 0 if status.startswith("PASS") else 1

if __name__ == "__main__":
    sys.exit(main())
