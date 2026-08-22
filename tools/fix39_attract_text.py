#!/usr/bin/env python3
"""Generate ATTRACT.ASM screen text from the historical source checkout.

The N64 tree already fetches historicalsource/wwf-wrestlemania during CI.
This generator deliberately reads ATTRACT.ASM there instead of carrying a
second hand-maintained copy of arcade strings in the Fix39 bundle.
"""
from __future__ import annotations

import argparse
import ast
import re
import tempfile
from pathlib import Path

HINT_ORDER = ("2", "4", "3", "9", "7", "5", "8", "1", "6", "A")
HINT_COUNTS = (4, 6, 6, 5, 6, 5, 6, 4, 3, 4)
GENERAL_LABELS = (
    "gen_tip1", "gen_tip1a", None,
    "gen_tip2", "gen_tip2a", None,
    "gen_tip3", "gen_tip3a", None,
    "gen_tip4", "gen_tip4a",
)


def die(msg: str) -> None:
    raise SystemExit(f"fix39 attract text generator: {msg}")


def section(text: str, start: str, end: str) -> str:
    a = text.find(start)
    if a < 0:
        die(f"missing section start {start!r}")
    b = text.find(end, a + len(start))
    if b < 0:
        die(f"missing section end {end!r}")
    return text[a:b]


def decode_asm_string(token: str) -> str:
    # Source strings are ASCII C-like quoted literals. ast handles doubled
    # punctuation and escapes without inventing any content.
    try:
        value = ast.literal_eval(token)
    except (SyntaxError, ValueError) as exc:
        die(f"could not decode source string {token!r}: {exc}")
    if not isinstance(value, str):
        die("decoded non-string source literal")
    return value


def label_string(sec: str, label: str) -> str:
    # Most strings are on the same line as #label. Some JAM_STR records put the
    # .byte literal on the next line, so allow a short continuation window.
    m = re.search(rf"(?mi)^\s*#{re.escape(label)}\b([^\n]*)(?:\n([^\n]*))?", sec)
    if not m:
        die(f"missing source label #{label}")
    probe = "\n".join(g for g in m.groups() if g is not None)
    q = re.search(r'\"(?:[^\"\\]|\\.)*\"', probe)
    if not q:
        # Search up to four following physical lines for JAM_STR + .byte form.
        tail = sec[m.start():].splitlines()[:5]
        q2 = re.search(r'\"(?:[^\"\\]|\\.)*\"', "\n".join(tail))
        if not q2:
            die(f"label #{label} has no quoted source string nearby")
        return decode_asm_string(q2.group(0))
    return decode_asm_string(q.group(0))


def c_quote(s: str) -> str:
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def parse_source(path: Path) -> dict[str, object]:
    text = path.read_text(encoding="latin-1", errors="replace")

    hint_sec = section(text, "WHICH_HINT", ";GENERAL_HINT")
    # Include the string tables following ;GENERAL_HINT.
    hint_tail_start = text.find("#SETUP_LINE_1", text.find("WHICH_HINT"))
    if hint_tail_start < 0:
        die("missing hint string table")
    hint_tail = text[hint_tail_start:]

    titles: list[str] = []
    bodies: list[list[str]] = []
    for suffix, expected_count in zip(HINT_ORDER, HINT_COUNTS):
        # Ensure WHICH_HINT really references this row; this protects the
        # executable source order from drifting under us.
        if f"#HNTT_{suffix}" not in hint_sec or f"#HNT_{suffix}" not in hint_sec:
            die(f"WHICH_HINT does not contain HNT row {suffix}")
        titles.append(label_string(hint_tail, f"HNTT_{suffix}"))
        table = re.search(
            rf"(?mi)^\s*#HNT_{re.escape(suffix)}\s+\.long\s+(\d+)\s*,([^\n]+)",
            hint_tail,
        )
        if not table:
            die(f"missing HNT_{suffix} body table")
        count = int(table.group(1))
        if count != expected_count:
            die(f"HNT_{suffix} line count drift: source={count} expected={expected_count}")
        labels = re.findall(rf"#(HNT_{re.escape(suffix)}[A-Z])", table.group(2))
        if len(labels) < count:
            die(f"HNT_{suffix} body table has only {len(labels)} labels for {count} lines")
        bodies.append([label_string(hint_tail, lab) for lab in labels[:count]])

    gen_sec = section(text, "SUBRP\tprint_gen_tips", "SUBRP\tshow_hstd")
    # Some source mirrors normalize tabs, so retry by textual headings.
    if "gen_tip_mes" not in gen_sec:
        gen_sec = section(text, "Print general tips", "Show high score tables")
    general_title = label_string(gen_sec, "gen_tip_mes")
    general_rows = ["" if lab is None else label_string(gen_sec, lab) for lab in GENERAL_LABELS]

    copy_sec = section(text, "SUBRP\tshow_copyright", "Print general tips")
    if "#ln19" not in copy_sec:
        copy_sec = section(text, "show_copyright", "Print general tips")
    copyright_lines = [label_string(copy_sec, f"ln{i}") for i in range(1, 20)]

    aama_start = re.search(r"(?mi)^\s*aama_message\s*$", text)
    if not aama_start:
        die("missing aama_message routine label")
    aama_end = re.search(r"(?mi)^\s*do_the_grad_thang\s*$", text[aama_start.end():])
    if not aama_end:
        die("missing do_the_grad_thang routine label")
    aama_sec = text[aama_start.start():aama_start.end() + aama_end.start()]
    aama_lines = [label_string(aama_sec, lab) for lab in ("ln1", "ln2", "ln2b", "ln3", "ln4", "ln5")]

    return {
        "hint_titles": titles,
        "hint_bodies": bodies,
        "general_title": general_title,
        "general_rows": general_rows,
        "copyright": copyright_lines,
        "aama": aama_lines,
    }


def emit(data: dict[str, object], out_c: Path, out_h: Path) -> None:
    titles = data["hint_titles"]
    bodies = data["hint_bodies"]
    general_title = data["general_title"]
    general_rows = data["general_rows"]
    copyright_lines = data["copyright"]
    aama_lines = data["aama"]
    assert isinstance(titles, list) and isinstance(bodies, list)
    assert isinstance(general_title, str) and isinstance(general_rows, list)
    assert isinstance(copyright_lines, list) and isinstance(aama_lines, list)

    out_h.parent.mkdir(parents=True, exist_ok=True)
    out_c.parent.mkdir(parents=True, exist_ok=True)
    guard = "WM_FIX39_ATTRACT_TEXT_GENERATED_H"
    out_h.write_text(f'''#ifndef {guard}\n#define {guard}\n\n#include <stddef.h>\n\n#ifdef __cplusplus\nextern "C" {{\n#endif\n\n#define WM_FIX39_SOURCE_HINT_COUNT 10u\n#define WM_FIX39_SOURCE_GENERAL_ROW_COUNT 11u\n#define WM_FIX39_SOURCE_COPYRIGHT_LINE_COUNT 19u\n#define WM_FIX39_SOURCE_AAMA_LINE_COUNT 6u\n\nconst char *wm_fix39_source_hint_title(size_t index);\nconst char *wm_fix39_source_hint_line(size_t index, size_t line);\nsize_t wm_fix39_source_hint_line_count(size_t index);\nconst char *wm_fix39_source_general_title(void);\nconst char *wm_fix39_source_general_row(size_t row);\nconst char *wm_fix39_source_copyright_line(size_t line);\nconst char *wm_fix39_source_aama_line(size_t line);\n\n#ifdef __cplusplus\n}}\n#endif\n\n#endif\n''')

    lines: list[str] = []
    lines.append('/* Generated directly from original/wwf-wrestlemania/ATTRACT.ASM. */')
    lines.append('#include "fix39_attract_text_generated.h"')
    lines.append('')
    lines.append('static const char *const hint_titles[10] = {')
    lines.extend(f'    {c_quote(s)},' for s in titles)
    lines.append('};')
    lines.append('static const unsigned char hint_counts[10] = {' + ', '.join(str(len(x)) for x in bodies) + '};')
    lines.append('static const char *const hint_lines[10][6] = {')
    for body in bodies:
        padded = list(body) + [None] * (6 - len(body))
        lines.append('    { ' + ', '.join('0' if s is None else c_quote(s) for s in padded) + ' },')
    lines.append('};')
    lines.append('static const char *const general_rows[11] = {')
    lines.extend(f'    {c_quote(s)},' for s in general_rows)
    lines.append('};')
    lines.append('static const char *const copyright_lines[19] = {')
    lines.extend(f'    {c_quote(s)},' for s in copyright_lines)
    lines.append('};')
    lines.append('static const char *const aama_lines[6] = {')
    lines.extend(f'    {c_quote(s)},' for s in aama_lines)
    lines.append('};')
    lines.append('')
    lines.append('const char *wm_fix39_source_hint_title(size_t index) { return index < 10u ? hint_titles[index] : 0; }')
    lines.append('size_t wm_fix39_source_hint_line_count(size_t index) { return index < 10u ? hint_counts[index] : 0u; }')
    lines.append('const char *wm_fix39_source_hint_line(size_t index, size_t line) { return index < 10u && line < hint_counts[index] ? hint_lines[index][line] : 0; }')
    lines.append(f'const char *wm_fix39_source_general_title(void) {{ return {c_quote(general_title)}; }}')
    lines.append('const char *wm_fix39_source_general_row(size_t row) { return row < 11u ? general_rows[row] : 0; }')
    lines.append('const char *wm_fix39_source_copyright_line(size_t line) { return line < 19u ? copyright_lines[line] : 0; }')
    lines.append('const char *wm_fix39_source_aama_line(size_t line) { return line < 6u ? aama_lines[line] : 0; }')
    out_c.write_text("\n".join(lines) + "\n")


def self_test() -> None:
    # Synthetic source exercises local-label collisions between copyright/AAMA,
    # JAM_STR title continuation, hint order and blank general-tip rows.
    hint_parts = ["WHICH_HINT"]
    for suffix in HINT_ORDER:
        hint_parts.append(f" .LONG #HNTT_{suffix},#HNT_{suffix},TIP,MUG")
    hint_parts += [";GENERAL_HINT", "#SETUP_LINE_1"]
    for suffix, count in zip(HINT_ORDER, HINT_COUNTS):
        labels = [f"#HNT_{suffix}{chr(65+i)}" for i in range(count)]
        hint_parts += [f'#HNTT_{suffix} .byte "TITLE{suffix}",0', f'#HNT_{suffix} .long {count},' + ','.join(labels)]
        hint_parts += [f'{lab} .byte "BODY{suffix}{i}",0' for i, lab in enumerate(labels)]
    gen = ["Print general tips", " SUBRP\tprint_gen_tips", "#gen_tip_mes", " JAM_STR blah", ' .byte "TITLE",0']
    for lab in [x for x in GENERAL_LABELS if x]:
        gen.append(f'#{lab} .string "{lab}",0')
    gen += [" SUBRP\tshow_hstd", "Show high score tables"]
    copy = [" SUBRP\tshow_copyright"] + [f'#ln{i} .string "COPY{i}",0' for i in range(1,20)] + ["Print general tips"]
    aama = ["aama_message"] + [f'#{lab} .string "AAMA{lab}",0' for lab in ("ln1","ln2","ln2b","ln3","ln4","ln5")] + ["do_the_grad_thang"]
    sample = "\n".join(aama + copy + gen + hint_parts)
    with tempfile.TemporaryDirectory() as td:
        p = Path(td) / "ATTRACT.ASM"
        p.write_text(sample)
        d = parse_source(p)
        assert d["hint_titles"][0] == "TITLE2"
        assert len(d["hint_bodies"][1]) == 6
        assert d["general_title"] == "TITLE"
        assert d["general_rows"][2] == ""
        assert d["copyright"][18] == "COPY19"
        assert d["aama"][2] == "AAMAln2b"
    print("Fix39 ATTRACT source-text generator self-test: PASS")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", type=Path)
    ap.add_argument("--out-c", type=Path)
    ap.add_argument("--out-h", type=Path)
    ap.add_argument("--self-test", action="store_true")
    ns = ap.parse_args()
    if ns.self_test:
        self_test()
        return
    if not ns.source or not ns.out_c or not ns.out_h:
        ap.error("--source, --out-c and --out-h are required unless --self-test is used")
    data = parse_source(ns.source)
    emit(data, ns.out_c, ns.out_h)
    print(f"generated exact ATTRACT text: {ns.out_c} / {ns.out_h}")


if __name__ == "__main__":
    main()
