#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ast
import re
import tempfile
from pathlib import Path

TABLES = {
    "getup_t": 30,
    "blkbase_t": 30,
    "blkatk_t": 10,
    "sklhhdly_t": 30,
    "sklhrdly_t": 30,
}

KNOWN_GETUP = [
    10,12,14,16,18, 20,22,24,26,28, 30,32,34,36,38,
    40,42,44,46,48, 50,52,54,56,58, 60,70,80,90,100,
]


def fail(msg: str) -> None:
    raise SystemExit(f"Fix39 DRONE table error: {msg}")


def strip_comment(line: str) -> str:
    return line.split(";", 1)[0].strip()


def norm_number_tokens(expr: str) -> str:
    # Williams source uses hex numbers like 1fh / 0ffh.
    return re.sub(r"(?i)\b([0-9][0-9a-f]*)h\b", r"0x\1", expr)


def eval_int(expr: str) -> int:
    expr = norm_number_tokens(expr.strip())
    try:
        node = ast.parse(expr, mode="eval")
    except SyntaxError as e:
        fail(f"unsupported numeric expression {expr!r}: {e}")

    def ev(n: ast.AST) -> int:
        if isinstance(n, ast.Expression): return ev(n.body)
        if isinstance(n, ast.Constant) and isinstance(n.value, int): return int(n.value)
        if isinstance(n, ast.UnaryOp) and isinstance(n.op, (ast.UAdd, ast.USub)):
            v = ev(n.operand)
            return v if isinstance(n.op, ast.UAdd) else -v
        if isinstance(n, ast.BinOp) and isinstance(n.op, (ast.Add, ast.Sub, ast.Mult)):
            a, b = ev(n.left), ev(n.right)
            if isinstance(n.op, ast.Add): return a + b
            if isinstance(n.op, ast.Sub): return a - b
            return a * b
        fail(f"unsupported numeric expression {expr!r}")
        return 0

    return ev(node)


def is_label(line: str) -> str | None:
    s = strip_comment(line)
    if not s:
        return None
    # Local/source labels commonly use '#name'; table symbols can also be bare.
    m = re.fullmatch(r"#?([A-Za-z_][A-Za-z0-9_]*)\s*:?\s*", s)
    if not m:
        return None
    # Don't mistake directives/opcodes for labels.
    if m.group(1).lower() in {"word", "long", "text", "data", "even", "endm"}:
        return None
    return m.group(1)


def collect_table(lines: list[str], label: str, expected: int) -> list[int]:
    starts = [i for i, line in enumerate(lines) if is_label(line) == label]
    if not starts:
        fail(f"DRONE.ASM table {label} not found")
    # DRONE.ASM contains historical material before the active shipping body.
    # References in the current source resolve to the final definition.
    start = starts[-1]
    out: list[int] = []
    for raw in lines[start + 1:]:
        s = strip_comment(raw)
        if not s:
            continue
        lab = is_label(raw)
        if lab is not None:
            break
        m = re.match(r"(?i)^\.word\s+(.+)$", s)
        if m:
            for item in m.group(1).split(','):
                out.append(eval_int(item))
            if len(out) >= expected:
                break
            continue
        m = re.match(r"(?i)^SKLM\s+([^,]+),\s*(.+)$", s)
        if m:
            w = eval_int(m.group(1))
            ad = eval_int(m.group(2))
            out.extend(w + ad * i for i in range(5))
            if len(out) >= expected:
                break
            continue
        # Alignment/directive lines may occur between a label and data.
        if re.match(r"(?i)^\.(?:even|align)\b", s):
            continue
        # Any real instruction/directive before enough data is source drift.
        if out:
            fail(f"unexpected line while reading {label}: {s!r}")
    if len(out) != expected:
        fail(f"{label} expected {expected} words, parsed {len(out)}: {out}")
    for v in out:
        if not -32768 <= v <= 32767:
            fail(f"{label} value outside signed WORD range: {v}")
    return out


def parse_source(path: Path) -> dict[str, list[int]]:
    text = path.read_text(encoding="latin-1")
    lines = text.splitlines()
    data = {name: collect_table(lines, name, n) for name, n in TABLES.items()}
    if data["getup_t"] != KNOWN_GETUP:
        fail("getup_t no longer matches the already-audited direct port; refusing silent drift")
    return data


def c_array(name: str, vals: list[int]) -> str:
    parts = []
    for i in range(0, len(vals), 10):
        parts.append("    " + ", ".join(str(v) for v in vals[i:i+10]))
    return f"static const int16_t {name}[{len(vals)}] = {{\n" + ",\n".join(parts) + "\n};\n"


def emit_header(data: dict[str, list[int]], out: Path) -> None:
    body = [
        "#ifndef WM_ARCADE_DRONE_SOURCE_TABLES_GENERATED_H",
        "#define WM_ARCADE_DRONE_SOURCE_TABLES_GENERATED_H",
        "",
        "/* GENERATED DIRECTLY FROM historical DRONE.ASM. DO NOT HAND-EDIT. */",
        "#define WM_FIX39_DRONE_SOURCE_GENERATED 1",
        "#define WM_FIX39_DRONE_SKILL_COUNT 30",
        "#define WM_FIX39_DRONE_BLOCK_MISS_COUNT 10",
        "",
        c_array("wm_fix39_drone_getup_t", data["getup_t"]).rstrip(),
        c_array("wm_fix39_drone_blkbase_t", data["blkbase_t"]).rstrip(),
        c_array("wm_fix39_drone_blkatk_t", data["blkatk_t"]).rstrip(),
        c_array("wm_fix39_drone_sklhhdly_t", data["sklhhdly_t"]).rstrip(),
        c_array("wm_fix39_drone_sklhrdly_t", data["sklhrdly_t"]).rstrip(),
        "",
        "#endif",
        "",
    ]
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(body))


def self_test() -> None:
    # Includes an obsolete first definition to verify that the active/final
    # source table wins, plus both .word and SKLM syntaxes.
    fixture = """
#getup_t
.word 1,2,3
#obsolete
.word 9
#getup_t
SKLM 10,2
SKLM 20,2
SKLM 30,2
SKLM 40,2
SKLM 50,2
SKLM 60,10
#blkbase_t
.word 1,2,3,4,5,6,7,8,9,10
.word 11,12,13,14,15,16,17,18,19,20
.word 21,22,23,24,25,26,27,28,29,30
#blkatk_t
.word 0,1,2,3,4,5,6,7,8,9
#sklhhdly_t
SKLM 2,1
SKLM 7,1
SKLM 12,1
SKLM 17,1
SKLM 22,1
SKLM 27,1
#sklhrdly_t
SKLM 3,2
SKLM 13,2
SKLM 23,2
SKLM 33,2
SKLM 43,2
SKLM 53,2
#after
"""
    with tempfile.TemporaryDirectory() as td:
        src = Path(td) / "DRONE.ASM"
        out = Path(td) / "generated.h"
        src.write_text(fixture, encoding="latin-1")
        data = parse_source(src)
        assert data["getup_t"] == KNOWN_GETUP
        assert data["blkbase_t"][0] == 1 and data["blkbase_t"][-1] == 30
        assert data["blkatk_t"] == list(range(10))
        emit_header(data, out)
        generated = out.read_text()
        assert "WM_FIX39_DRONE_SOURCE_GENERATED 1" in generated
        assert "wm_fix39_drone_sklhrdly_t[30]" in generated
    print("Fix39 DRONE scalar-table source generator: PASS")


def main() -> None:
    ap = argparse.ArgumentParser(description="Extract exact DRONE.ASM scalar AI tables")
    ap.add_argument("--source", type=Path)
    ap.add_argument("--out", type=Path)
    ap.add_argument("--self-test", action="store_true")
    ns = ap.parse_args()
    if ns.self_test:
        self_test()
        return
    if ns.source is None or ns.out is None:
        fail("--source and --out are required")
    data = parse_source(ns.source)
    emit_header(data, ns.out)
    print("Fix39 DRONE scalar tables generated: " + ", ".join(
        f"{k}={len(v)}" for k, v in data.items()))

if __name__ == "__main__":
    main()
