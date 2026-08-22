#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ast
import re
import tempfile
from dataclasses import dataclass
from pathlib import Path

ROOT_TABLES = ("wnshort_t", "wnmed_t", "wnlong_t")
WRESTLER_COUNT = 9


def fail(msg: str) -> None:
    raise SystemExit(f"Fix39 DRONE range-table error: {msg}")


def strip_comment(line: str) -> str:
    return line.split(";", 1)[0].strip()


def split_args(text: str) -> list[str]:
    # Williams source macro/table arguments in DRONE.ASM are flat assembler
    # expressions.  Keep this deliberately small and fail on unbalanced
    # grouping instead of silently guessing.
    out: list[str] = []
    cur: list[str] = []
    depth = 0
    for ch in text:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
            if depth < 0:
                fail(f"unbalanced argument expression: {text!r}")
        if ch == ',' and depth == 0:
            out.append(''.join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    if depth != 0:
        fail(f"unbalanced argument expression: {text!r}")
    if cur or text.strip():
        out.append(''.join(cur).strip())
    return [x for x in out if x != ""]


def norm_number_tokens(expr: str) -> str:
    # TMS/Williams source hex form: 1fh, 0ffh, 0ffffh.
    return re.sub(r"(?i)\b([0-9][0-9a-f]*)h\b", r"0x\1", expr)


def eval_numeric(expr: str) -> int:
    expr = norm_number_tokens(expr.strip())
    try:
        node = ast.parse(expr, mode="eval")
    except SyntaxError as e:
        fail(f"unsupported numeric expression {expr!r}: {e}")

    def ev(n: ast.AST) -> int:
        if isinstance(n, ast.Expression):
            return ev(n.body)
        if isinstance(n, ast.Constant) and isinstance(n.value, int):
            return int(n.value)
        if isinstance(n, ast.UnaryOp) and isinstance(n.op, (ast.UAdd, ast.USub, ast.Invert)):
            v = ev(n.operand)
            if isinstance(n.op, ast.UAdd):
                return v
            if isinstance(n.op, ast.USub):
                return -v
            return ~v
        if isinstance(n, ast.BinOp) and isinstance(
            n.op, (ast.Add, ast.Sub, ast.Mult, ast.LShift, ast.RShift, ast.BitOr, ast.BitAnd)
        ):
            a, b = ev(n.left), ev(n.right)
            if isinstance(n.op, ast.Add): return a + b
            if isinstance(n.op, ast.Sub): return a - b
            if isinstance(n.op, ast.Mult): return a * b
            if isinstance(n.op, ast.LShift): return a << b
            if isinstance(n.op, ast.RShift): return a >> b
            if isinstance(n.op, ast.BitOr): return a | b
            return a & b
        fail(f"non-numeric source expression {expr!r}")
        return 0

    return ev(node)


def signed_width(v: int, bits: int) -> int:
    lo = -(1 << (bits - 1))
    hi_unsigned = (1 << bits) - 1
    if lo <= v < (1 << (bits - 1)):
        return v
    if 0 <= v <= hi_unsigned:
        if v & (1 << (bits - 1)):
            return v - (1 << bits)
        return v
    fail(f"value {v} does not fit source signed/unsigned {bits}-bit datum")
    return 0


def is_label(line: str) -> str | None:
    s = strip_comment(line)
    if not s:
        return None
    m = re.fullmatch(r"#?([A-Za-z_][A-Za-z0-9_]*)\s*:?\s*", s)
    if not m:
        return None
    low = m.group(1).lower()
    if low in {"byte", "word", "long", "text", "data", "even", "align", "endm"}:
        return None
    return m.group(1)


@dataclass(frozen=True)
class Macro:
    params: tuple[str, ...]
    body: tuple[str, ...]


def parse_macros(lines: list[str]) -> tuple[dict[str, Macro], set[int]]:
    macros: dict[str, Macro] = {}
    consumed: set[int] = set()
    i = 0
    while i < len(lines):
        s = strip_comment(lines[i])
        m = re.match(r"(?i)^([A-Za-z_][A-Za-z0-9_]*)\s+\.macro(?:\s+(.*))?$", s)
        if not m:
            i += 1
            continue
        name = m.group(1).upper()
        params = tuple(x.strip() for x in split_args(m.group(2) or ""))
        body: list[str] = []
        consumed.add(i)
        i += 1
        while i < len(lines):
            consumed.add(i)
            if re.match(r"(?i)^\.endm\b", strip_comment(lines[i])):
                break
            body.append(lines[i])
            i += 1
        else:
            fail(f"unterminated .macro {name}")
        macros[name] = Macro(params, tuple(body))
        i += 1
    return macros, consumed


def substitute_macro(line: str, macro: Macro, args: list[str]) -> str:
    if len(args) != len(macro.params):
        fail(f"macro argument mismatch: expected {len(macro.params)}, got {len(args)}")
    out = line
    for p, a in zip(macro.params, args):
        # Historical source uses :param: substitution (confirmed by SKLM).
        out = out.replace(f":{p}:", a)
        # Also tolerate conventional backslash substitution if another local
        # macro uses it; this is purely syntactic expansion, not interpretation.
        out = out.replace(f"\\{p}", a)
    return out


def expand_source(lines: list[str]) -> list[str]:
    macros, consumed = parse_macros(lines)

    def expand_line(raw: str, depth: int = 0) -> list[str]:
        if depth > 24:
            fail("macro expansion depth exceeded")
        s = strip_comment(raw)
        if not s:
            return [raw]
        m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\b(?:\s+(.*))?$", s)
        if not m:
            return [raw]
        macro = macros.get(m.group(1).upper())
        if macro is None:
            return [raw]
        args = split_args(m.group(2) or "")
        out: list[str] = []
        for body_line in macro.body:
            sub = substitute_macro(body_line, macro, args)
            out.extend(expand_line(sub, depth + 1))
        return out

    out: list[str] = []
    for i, raw in enumerate(lines):
        if i in consumed:
            continue
        out.extend(expand_line(raw))
    return out


def label_starts(lines: list[str], label: str) -> list[int]:
    return [i for i, line in enumerate(lines) if is_label(line) == label]


def final_label_start(lines: list[str], label: str) -> int:
    starts = label_starts(lines, label)
    if not starts:
        fail(f"source label {label} not found")
    # Like the scalar-table extractor, use the final active definition.  The
    # source contains older historical bodies earlier in the file.
    return starts[-1]


def directive(line: str) -> tuple[str, list[str]] | None:
    s = strip_comment(line)
    m = re.match(r"(?i)^\.?(byte|word|long)\s+(.+)$", s)
    if not m:
        return None
    return m.group(1).lower(), split_args(m.group(2))


def ptr_label(expr: str) -> str:
    s = expr.strip()
    while s.startswith('#') or s.startswith('@'):
        s = s[1:].strip()
    m = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)", s)
    if not m:
        fail(f"expected source pointer label, got {expr!r}")
    return m.group(1)


def mode_value(expr: str) -> tuple[str, int | str]:
    s = expr.strip()
    if re.fullmatch(r"MODE_[A-Za-z0-9_]+", s, flags=re.I):
        # Source combat-mode enum has already been directly translated as
        # WM_PMODE_*; preserve symbolic identity instead of re-keying numbers.
        return "token", "WM_PMODE_" + s[5:].upper()
    try:
        v = signed_width(eval_numeric(s), 8)
        return "int", v
    except SystemExit:
        raise


def mode_c(expr: str) -> str:
    kind, value = mode_value(expr)
    return str(value)


def collect_root_ptrs(lines: list[str], label: str) -> list[str]:
    start = final_label_start(lines, label)
    out: list[str] = []
    for raw in lines[start + 1:]:
        lab = is_label(raw)
        if lab is not None:
            if out:
                break
            continue
        d = directive(raw)
        if d is None:
            s = strip_comment(raw)
            if not s or re.match(r"(?i)^\.(?:even|align)\b", s):
                continue
            if out:
                fail(f"unexpected source line in {label}: {s!r}")
            continue
        kind, vals = d
        if kind != "long":
            if out:
                fail(f"{label} expected LONG pointer table, saw {kind}")
            continue
        out.extend(ptr_label(v) for v in vals)
        if len(out) >= WRESTLER_COUNT:
            break
    if len(out) != WRESTLER_COUNT:
        fail(f"{label} expected {WRESTLER_COUNT} wrestler pointers, parsed {len(out)}: {out}")
    return out


@dataclass(frozen=True)
class ModeRecord:
    my_expr: str
    opp_expr: str
    script_list_label: str


def collect_mode_list(lines: list[str], label: str) -> list[ModeRecord]:
    start = final_label_start(lines, label)
    tokens: list[tuple[str, str]] = []
    out: list[ModeRecord] = []
    for raw in lines[start + 1:]:
        lab = is_label(raw)
        if lab is not None:
            if tokens or out:
                fail(f"mode list {label} reached label {lab} before a -1/-1 default record")
            continue
        d = directive(raw)
        if d is None:
            s = strip_comment(raw)
            if not s or re.match(r"(?i)^\.(?:even|align)\b", s):
                continue

            # DRONE.ASM includes macros.h, where the Williams WL shorthand is
            # a WORD followed by a LONG.  The final/default mode-list record
            # in production source is written as `WL -1,<script_list>` rather
            # than separate BYTE/BYTE/LONG directives.  drone_main reads that
            # WORD as two signed bytes in order; a source WORD of -1 is all
            # ones, so both bytes are -1 independent of byte ordering.  Only
            # accept that source-proven wildcard form here; do not guess how
            # arbitrary packed mode words should be split.
            mwl = re.match(r"(?i)^WL\s+(.+)$", s)
            if mwl:
                args = split_args(mwl.group(1))
                if len(args) != 2:
                    fail(f"mode list {label} WL expected 2 args, got {len(args)}: {s!r}")
                packed = signed_width(eval_numeric(args[0]), 16)
                if packed != -1:
                    fail(
                        f"mode list {label} unsupported packed WL mode word {args[0]!r}; "
                        "only the source terminal -1 wildcard is decoded in chunk 2"
                    )
                tokens.extend((("byte", "-1"), ("byte", "-1"), ("long", args[1])))
            else:
                if tokens or out:
                    fail(f"unexpected source line in mode list {label}: {s!r}")
                continue
        else:
            kind, vals = d
            if kind == "byte":
                tokens.extend(("byte", v) for v in vals)
            elif kind == "long":
                tokens.extend(("long", v) for v in vals)
            else:
                fail(f"mode list {label} expected BYTE/BYTE/LONG records, saw WORD")

        while len(tokens) >= 3:
            if [tokens[i][0] for i in range(3)] != ["byte", "byte", "long"]:
                fail(f"mode list {label} source layout is not BYTE/BYTE/LONG: {tokens[:3]}")
            my_expr, opp_expr = tokens[0][1], tokens[1][1]
            rec = ModeRecord(my_expr, opp_expr, ptr_label(tokens[2][1]))
            out.append(rec)
            tokens = tokens[3:]
            mk, mv = mode_value(my_expr)
            ok, ov = mode_value(opp_expr)
            if mk == "int" and ok == "int" and int(mv) < 0 and int(ov) < 0:
                if tokens:
                    fail(f"mode list {label} has trailing data after default record")
                return out
    fail(f"mode list {label} has no terminating negative/negative default record")
    return []


@dataclass(frozen=True)
class ScriptList:
    max_index: int
    scripts: tuple[str, ...]


def source_count_end_label(expr: str) -> str | None:
    """Decode the source's self-sized pointer-list expression.

    Williams' TMS source is bit-addressed.  DRONE.ASM sometimes writes the
    first WORD of a script-pointer list as `(end_label-$)/32-1`: the distance
    to a forward end label, divided by one LONG (32 bits), minus one, yielding
    the maximum valid RNDRNG0 index.  Do not numerically guess `$` or label
    addresses; resolve this form structurally from the actual LONG records.
    """
    s = re.sub(r"\s+", "", expr)
    m = re.fullmatch(r"\(([A-Za-z_][A-Za-z0-9_]*)-\$\)/32-1", s)
    return m.group(1) if m else None


def collect_script_list(lines: list[str], label: str) -> ScriptList:
    start = final_label_start(lines, label)
    max_index: int | None = None
    scripts: list[str] = []
    expected: int | None = None
    count_end: str | None = None
    saw_count_word = False

    for raw in lines[start + 1:]:
        lab = is_label(raw)
        if lab is not None:
            if count_end is not None:
                if lab != count_end:
                    fail(
                        f"script list {label} self-sized count expected end label {count_end}, "
                        f"reached {lab} after {len(scripts)} pointers"
                    )
                if not scripts:
                    fail(f"script list {label} self-sized count resolved to zero pointers")
                max_index = len(scripts) - 1
                expected = len(scripts)
                break
            if saw_count_word:
                break
            continue

        d = directive(raw)
        if d is None:
            s = strip_comment(raw)
            if not s or re.match(r"(?i)^\.(?:even|align)\b", s):
                continue
            if saw_count_word:
                fail(f"unexpected source line in script list {label}: {s!r}")
            continue

        kind, vals = d
        if not saw_count_word:
            if kind != "word" or len(vals) != 1:
                fail(f"script list {label} must begin with one WORD maximum-index value")
            saw_count_word = True
            expr = vals[0]
            count_end = source_count_end_label(expr)
            if count_end is not None:
                # The exact max index is resolved when the named forward label
                # is reached.  This preserves the source-authored list length.
                continue
            max_index = signed_width(eval_numeric(expr), 16)
            expected = abs(max_index) + 1
            if expected <= 0 or expected > 256:
                fail(f"script list {label} has implausible source max index {max_index}")
            continue

        if kind != "long":
            fail(f"script list {label} expected LONG script pointers after max index, saw {kind}")
        scripts.extend(ptr_label(v) for v in vals)
        if count_end is None and expected is not None and len(scripts) >= expected:
            break

    if not saw_count_word or max_index is None or expected is None:
        if count_end is not None:
            fail(f"script list {label} did not reach self-sized end label {count_end}")
        fail(f"script list {label} has no max-index WORD")
    if len(scripts) != expected:
        fail(f"script list {label} max index {max_index} requires {expected} pointers, parsed {len(scripts)}")
    return ScriptList(max_index, tuple(scripts))


@dataclass
class ParsedRanges:
    roots: dict[str, list[str]]
    mode_lists: dict[str, list[ModeRecord]]
    script_lists: dict[str, ScriptList]


def parse_source(path: Path) -> ParsedRanges:
    text = path.read_text(encoding="latin-1")
    lines = expand_source(text.splitlines())
    roots = {name: collect_root_ptrs(lines, name) for name in ROOT_TABLES}

    mode_labels: list[str] = []
    seen: set[str] = set()
    for root in ROOT_TABLES:
        for label in roots[root]:
            if label not in seen:
                seen.add(label)
                mode_labels.append(label)

    mode_lists = {label: collect_mode_list(lines, label) for label in mode_labels}
    list_labels: list[str] = []
    seen_lists: set[str] = set()
    for label in mode_labels:
        for rec in mode_lists[label]:
            if rec.script_list_label not in seen_lists:
                seen_lists.add(rec.script_list_label)
                list_labels.append(rec.script_list_label)
    script_lists = {label: collect_script_list(lines, label) for label in list_labels}
    return ParsedRanges(roots, mode_lists, script_lists)


def cid(label: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", label)


def cstr(s: str) -> str:
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def emit_header(data: ParsedRanges, out: Path) -> None:
    body: list[str] = [
        "#ifndef WM_ARCADE_DRONE_SOURCE_RANGES_GENERATED_H",
        "#define WM_ARCADE_DRONE_SOURCE_RANGES_GENERATED_H",
        "",
        "/* GENERATED DIRECTLY FROM historical DRONE.ASM. DO NOT HAND-EDIT. */",
        "#define WM_FIX39_DRONE_RANGES_GENERATED 1",
        f"#define WM_FIX39_DRONE_RANGE_WRESTLER_COUNT {WRESTLER_COUNT}",
        "#define WM_FIX39_DRONE_RANGE_BAND_COUNT 3",
        "",
    ]

    for label, sl in data.script_lists.items():
        ident = cid(label)
        body.append(f"static const char *const wm_fix39_drone_scripts_{ident}[{len(sl.scripts)}] = {{")
        body.append("    " + ", ".join(cstr(s) for s in sl.scripts))
        body.append("};")
        body.append(
            f"static const wm_arcade_drone_script_list_t wm_fix39_drone_script_list_{ident} = "
            f"{{ {sl.max_index}, wm_fix39_drone_scripts_{ident}, {len(sl.scripts)}u }};"
        )
        body.append("")

    for label, records in data.mode_lists.items():
        ident = cid(label)
        body.append(f"static const WmFix39DroneModeRecord wm_fix39_drone_modes_{ident}[{len(records)}] = {{")
        for r in records:
            body.append(
                f"    {{ {mode_c(r.my_expr)}, {mode_c(r.opp_expr)}, "
                f"&wm_fix39_drone_script_list_{cid(r.script_list_label)} }},"
            )
        body.append("};")
        body.append("")

    body.append("static const WmFix39DroneModeList wm_fix39_drone_range_table[3][9] = {")
    for root in ROOT_TABLES:
        body.append("    {")
        for label in data.roots[root]:
            ident = cid(label)
            body.append(
                f"        {{ wm_fix39_drone_modes_{ident}, "
                f"sizeof(wm_fix39_drone_modes_{ident}) / sizeof(wm_fix39_drone_modes_{ident}[0]) }},"
            )
        body.append("    },")
    body.append("};")
    body.extend(["", "#endif", ""])
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(body))


def self_test() -> None:
    # Synthetic source uses macros so the test covers the same source-level
    # expansion path used by Williams tables.  0ffh verifies MOVB signed
    # wildcard semantics and 0fffeh verifies signed WORD headhold-list count.
    fixture = r'''
PAIR .macro me,him,lst
.byte :me:,:him:
.long :lst:
.endm
ROOT9 .macro a,b,c,d,e,f,g,h,i
.long :a:,:b:,:c:,:d:,:e:,:f:,:g:,:h:,:i:
.endm
#wnshort_t
ROOT9 short0,short1,short2,short3,short4,short5,short6,short6,short8
#wnmed_t
ROOT9 med0,med1,med2,med3,med4,med5,med6,med6,med8
#wnlong_t
ROOT9 long0,long1,long2,long3,long4,long5,long6,long6,long8
'''
    # Give every mode-list two records and deliberately share the same script
    # lists.  Final definitions test the same historical-source rule as c1.
    for prefix in ("short", "med", "long"):
        for i in (0, 1, 2, 3, 4, 5, 6, 8):
            fixture += f"\n#{prefix}{i}\n"
            fixture += "PAIR MODE_NORMAL,MODE_BLOCK,list_a\n"
            fixture += "WL -1,list_head\n"
    # Force one mode list to reference a source-self-sized list as seen in
    # production DRONE.ASM, while retaining literal positive/negative cases.
    fixture = fixture.replace("#short0\nPAIR MODE_NORMAL,MODE_BLOCK,list_a",
                              "#short0\nPAIR MODE_NORMAL,MODE_BLOCK,list_counted", 1)
    fixture += r'''
#list_a
.word 1
.long sc_a,sc_b
#list_counted
.word (list_counted_n-$)/32-1
.long sc_c0,sc_c1,sc_c2
#list_counted_n
#list_head
.word 0fffeh
.long sc_h0,sc_h1,sc_h2
'''
    with tempfile.TemporaryDirectory() as td:
        src = Path(td) / "DRONE.ASM"
        out = Path(td) / "generated.h"
        src.write_text(fixture, encoding="latin-1")
        data = parse_source(src)
        assert data.roots["wnshort_t"] == ["short0","short1","short2","short3","short4","short5","short6","short6","short8"]
        assert len(data.mode_lists) == 24
        assert data.mode_lists["short0"][0].script_list_label == "list_counted"
        assert mode_c(data.mode_lists["short0"][1].my_expr) == "-1"
        assert mode_c(data.mode_lists["short0"][1].opp_expr) == "-1"
        assert data.script_lists["list_a"].max_index == 1
        assert data.script_lists["list_counted"].max_index == 2
        assert data.script_lists["list_counted"].scripts == ("sc_c0", "sc_c1", "sc_c2")
        assert data.script_lists["list_head"].max_index == -2
        assert data.script_lists["list_head"].scripts == ("sc_h0", "sc_h1", "sc_h2")
        emit_header(data, out)
        gen = out.read_text()
        assert "WM_FIX39_DRONE_RANGES_GENERATED 1" in gen
        assert "WM_PMODE_NORMAL, WM_PMODE_BLOCK" in gen
        assert "{ -1, -1" in gen
        assert "wm_fix39_drone_range_table[3][9]" in gen
    print("Fix39 DRONE range/mode source generator: PASS")


def main() -> None:
    ap = argparse.ArgumentParser(description="Extract exact DRONE.ASM range/mode/script-list metadata")
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
    print(
        "Fix39 DRONE source ranges generated: "
        f"roots=3x{WRESTLER_COUNT}, mode_lists={len(data.mode_lists)}, "
        f"script_lists={len(data.script_lists)}"
    )


if __name__ == "__main__":
    main()
