#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ast
import re
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

# Reuse the source-proven range parser for macro expansion and for the exact
# script labels reachable through wnshort_t/wnmed_t/wnlong_t.
sys.path.insert(0, str(Path(__file__).resolve().parent))
import fix39_drone_ranges as ranges  # type: ignore

DIRECT_SCRIPTS = (
    "slhtoss", "drn_enterring", "drn_opinair", "drn_oprun", "drn_roll",
    "drn_inair", "drn_ontb", "drn_run", "drn_combo",
    "drn_seekclose", "drn_oppdead",
)

# These are NOT script bodies. In the historical DRONE runtime they are
# loaded into a9 and branch to #getscrpt, which reads a WORD max-index then
# chooses one of the following LONG script pointers. Treating them as direct
# scripts makes the decoder read the first LONG pointer (for example #hgrab)
# as though it were the next 16-bit script command.
EXTRA_SCRIPT_LISTS = ("M_shrtblkr", "M_shrtblkrdl")
SKILL_COUNT = 30


def symkey(name: str) -> str:
    """Historical Williams/TMS assembler symbols are case-insensitive."""
    return name.casefold()


def fail(msg: str) -> None:
    raise SystemExit(f"Fix39 DRONE script error: {msg}")


def strip_comment(line: str) -> str:
    return line.split(";", 1)[0].strip()


def norm_number_tokens(expr: str) -> str:
    # Williams/TMS source uses MASM-style suffixes (1234h, 01111b).
    # Convert those before handing the expression to Python's AST.
    expr = re.sub(r"(?i)\b([0-9][0-9a-f]*)h\b", r"0x\1", expr)
    expr = re.sub(r"(?i)\b([01]+)b\b", r"0b\1", expr)
    return expr


def split_args(text: str) -> list[str]:
    return ranges.split_args(text)


def source_active_lines(path: Path) -> list[str]:
    raw = path.read_text(encoding="latin-1").splitlines()

    # IMPORTANT: expand the complete historical translation unit *before*
    # selecting the final DRONE.ASM body.  The file contains older archived
    # bodies followed by the production body, and Williams source can define
    # macros before the final `.file "drone.asm"` marker then invoke them in
    # that final body.  C3b sliced first, which discarded those still-live
    # macro definitions.  The range parser expands the whole file, so it could
    # legitimately discover pointers such as drn_seek while the script-image
    # builder failed to materialize the corresponding macro-generated label.
    # Expanding first mirrors the assembler's translation-unit scope and keeps
    # the range and script parsers on the same source view.
    expanded = ranges.expand_source(raw)
    starts = [
        i for i, line in enumerate(expanded)
        if re.search(r'(?i)^\s*\.file\s+["\']drone\.asm["\']', strip_comment(line))
    ]
    if starts:
        expanded = expanded[starts[-1]:]
    return expanded


def _resolve_include(parent: Path, include_name: str) -> Path:
    """Resolve assembler includes the way the original DOS build did.

    The historical checkout stores many include files as upper-case names
    (GAME.EQU, PLYR.EQU, ...), while DRONE.ASM includes them in lower case.
    Android/Termux is case-sensitive, so a literal Path lookup silently misses
    those files.  Walk each path component case-insensitively when needed.
    """
    rel = Path(include_name.replace("\\", "/"))
    direct = parent / rel
    if direct.is_file():
        return direct

    cur = parent
    for part in rel.parts:
        exact = cur / part
        if exact.exists():
            cur = exact
            continue
        if not cur.is_dir():
            return direct
        match = next((child for child in cur.iterdir() if child.name.casefold() == part.casefold()), None)
        if match is None:
            return direct
        cur = match
    return cur


def collect_equates(source: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    seen: set[Path] = set()

    def scan(path: Path) -> None:
        try:
            rp = path.resolve()
        except OSError:
            rp = path
        if rp in seen or not path.is_file():
            return
        seen.add(rp)
        for raw in path.read_text(encoding="latin-1").splitlines():
            s = strip_comment(raw)
            mi = re.match(r'(?i)^\.include\s+["\']([^"\']+)["\']', s)
            if mi:
                scan(_resolve_include(path.parent, mi.group(1)))
                continue
            # The Midway source uses both "FOO .equ 1" and "FOO equ 1"
            # (same for SET).  The C3 generator originally accepted only the
            # dotted spelling, which is why MOVE_UP from GAME.EQU was missed.
            m = re.match(r"(?i)^([A-Za-z_][A-Za-z0-9_]*)\s+\.?(?:equ|set)\s+(.+)$", s)
            if m:
                out[m.group(1).upper()] = m.group(2).strip()
    scan(source)
    return out


class Numeric:
    def __init__(self, equates: dict[str, str]):
        self.equates = equates
        self.cache: dict[str, int] = {}
        self.stack: set[str] = set()

    def symbol(self, name: str) -> int:
        # TMS34010 assembler symbols are case-insensitive.
        key = name.upper()
        if key in self.cache:
            return self.cache[key]
        if key in self.stack:
            fail(f"recursive .equ involving {name}")
        expr = self.equates.get(key)
        if expr is None:
            fail(f"unresolved numeric source symbol {name}")
        self.stack.add(key)
        v = self.eval(expr)
        self.stack.remove(key)
        self.cache[key] = v
        return v

    def eval(self, expr: str) -> int:
        e = norm_number_tokens(expr.strip())
        # Williams source occasionally prefixes immediates with #.  A data
        # expression does not need that marker for semantic evaluation.
        e = re.sub(r"(?<![A-Za-z0-9_])#(?=[A-Za-z0-9_(~+-])", "", e)
        try:
            node = ast.parse(e, mode="eval")
        except SyntaxError as ex:
            fail(f"unsupported numeric expression {expr!r}: {ex}")

        def ev(n: ast.AST) -> int:
            if isinstance(n, ast.Expression): return ev(n.body)
            if isinstance(n, ast.Constant) and isinstance(n.value, int): return int(n.value)
            if isinstance(n, ast.Name): return self.symbol(n.id)
            if isinstance(n, ast.UnaryOp) and isinstance(n.op, (ast.UAdd, ast.USub, ast.Invert)):
                v = ev(n.operand)
                if isinstance(n.op, ast.UAdd): return v
                if isinstance(n.op, ast.USub): return -v
                return ~v
            if isinstance(n, ast.BinOp) and isinstance(n.op, (
                ast.Add, ast.Sub, ast.Mult, ast.FloorDiv, ast.Div,
                ast.LShift, ast.RShift, ast.BitOr, ast.BitAnd, ast.BitXor,
            )):
                a, b = ev(n.left), ev(n.right)
                if isinstance(n.op, ast.Add): return a + b
                if isinstance(n.op, ast.Sub): return a - b
                if isinstance(n.op, ast.Mult): return a * b
                if isinstance(n.op, (ast.FloorDiv, ast.Div)):
                    if b == 0: fail(f"division by zero in {expr!r}")
                    return int(a / b)
                if isinstance(n.op, ast.LShift): return a << b
                if isinstance(n.op, ast.RShift): return a >> b
                if isinstance(n.op, ast.BitOr): return a | b
                if isinstance(n.op, ast.BitAnd): return a & b
                return a ^ b
            fail(f"non-numeric source expression {expr!r}")
            return 0
        return ev(node)


@dataclass(frozen=True)
class Datum:
    kind: str
    expr: str
    bit_addr: int


@dataclass
class Image:
    data: dict[int, Datum]
    labels: dict[str, int]
    num: Numeric

    def word_expr(self, addr: int) -> str:
        d = self.data.get(addr)
        if not d or d.kind != "word":
            fail(f"expected WORD at source bit address {addr}, saw {d}")
        return d.expr

    def word(self, addr: int) -> int:
        return self.num.eval(self.word_expr(addr)) & 0xffff

    def long_label(self, addr: int) -> str:
        d = self.data.get(addr)
        if not d or d.kind != "long":
            fail(f"expected LONG source pointer at bit address {addr}, saw {d}")
        s = d.expr.strip()
        while s.startswith(("#", "@")):
            s = s[1:].strip()
        m = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)", s)
        if not m:
            fail(f"expected LONG label pointer at {addr}, got {d.expr!r}")
        return m.group(1)


def packed_directive(raw: str) -> list[tuple[str, str]] | None:
    """Decode the source-proven Williams W/L data-packing macros.

    MACROS.H defines names such as WL, WLL, WLW, LWW, WWL, ... where
    each W emits a WORD and each L emits a LONG.  DRONE.ASM can use those
    include-defined macros, but source_active_lines() deliberately expands
    only macros local to DRONE.ASM.  Interpret the packing spelling here
    rather than silently losing script bytes.
    """
    s = strip_comment(raw)
    m = re.match(r"(?i)^([WL]+)\s+(.+)$", s)
    if m:
        layout = m.group(1).upper()
        vals = split_args(m.group(2))
        if len(vals) != len(layout):
            fail(f"{layout} expects {len(layout)} packed args, got {len(vals)}: {s!r}")
        return [("word" if ch == "W" else "long", val) for ch, val in zip(layout, vals)]
    # Two zero-suffix helpers present in the historical Williams macro set.
    if re.fullmatch(r"(?i)W0", s):
        return [("word", "0")]
    m = re.match(r"(?i)^LWL0\s+(.+)$", s)
    if m:
        vals = split_args(m.group(1))
        if len(vals) != 2:
            fail(f"LWL0 expects LONG,WORD args: {s!r}")
        return [("long", vals[0]), ("word", vals[1]), ("long", "0")]
    return None


def directive(raw: str) -> tuple[str, list[str]] | None:
    return ranges.directive(raw)


def inline_data_label(raw: str) -> tuple[str | None, str]:
    """Peel LABEL from `LABEL .word ...` / `LABEL WLL ...` data lines."""
    s = strip_comment(raw)
    m = re.match(r"^#?([A-Za-z_][A-Za-z0-9_]*)\s*:?\s+(.+)$", s)
    if not m:
        return None, s
    rest = m.group(2).strip()
    if directive(rest) is not None or packed_directive(rest) is not None:
        return m.group(1), rest
    return None, s


def address_equate(raw: str) -> str | None:
    """Recognize assembler address aliases such as `foo equ $`."""
    s = strip_comment(raw)
    m = re.match(r"(?i)^([A-Za-z_][A-Za-z0-9_]*)\s+\.?(?:equ|set)\s+\$\s*$", s)
    return m.group(1) if m else None


def subroutine_macro_label(raw: str) -> str | None:
    """Recognize labels emitted by MACROS.H SUBR/SUBRP invocations.

    The historical source does not spell every script label as `#name`.
    MACROS.H defines both `SUBR name` and `SUBRP name` as macros whose body
    emits an optional `.def`, `.even`, and then the actual `name:` label.
    The chunk-3 parser intentionally does not expand the entire include macro
    universe (many of those macros contain conditional assembler logic), so
    preserve the exact label semantics here.  This is source-proven by
    MACROS.H rather than a DRONE-specific guess.
    """
    s = strip_comment(raw)
    m = re.match(r"(?i)^SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b", s)
    return m.group(1) if m else None


def build_image(source: Path) -> Image:
    lines = source_active_lines(source)
    num = Numeric(collect_equates(source))
    data: dict[int, Datum] = {}
    labels: dict[str, int] = {}
    bit = 0
    section_live = False

    def add_label(name: str) -> None:
        # Match assembler semantics: symbol spelling is case-insensitive.
        # Preserve the existing parser's last-definition-wins behavior.
        labels[symkey(name)] = bit

    def add_datum(kind: str, expr: str) -> None:
        nonlocal bit
        width = {"byte": 8, "word": 16, "long": 32}[kind]
        data[bit] = Datum(kind, expr, bit)
        bit += width

    for raw in lines:
        s = strip_comment(raw)
        if re.match(r"(?i)^\.(?:text|data)\b", s):
            section_live = True
            continue
        lab = ranges.is_label(raw)
        if lab is not None:
            add_label(lab)
            continue
        alias = address_equate(raw)
        if alias is not None:
            add_label(alias)
            continue
        subr_lab = subroutine_macro_label(raw)
        if subr_lab is not None:
            add_label(subr_lab)
            continue

        inline_lab, payload = inline_data_label(raw)
        if inline_lab is not None:
            add_label(inline_lab)
            raw_for_data = payload
        else:
            raw_for_data = raw

        d = directive(raw_for_data)
        packed = None if d is not None else packed_directive(raw_for_data)
        if d is None and packed is None:
            continue
        if not section_live:
            section_live = True
        if packed is not None:
            for kind, expr in packed:
                add_datum(kind, expr)
            continue
        assert d is not None
        kind, vals = d
        for v in vals:
            add_datum(kind, v)
    return Image(data, labels, num)


def range_script_labels(source: Path) -> list[str]:
    r = ranges.parse_source(source)
    out: list[str] = []
    seen: set[str] = set()

    def add(label: str) -> None:
        key = symkey(label)
        if key not in seen:
            seen.add(key)
            out.append(label)

    # Normal distance/mode tables already resolve to their script bodies.
    for sl in r.script_lists.values():
        for label in sl.scripts:
            add(label)

    # M_shrtblkr / M_shrtblkrdl are special-case script POINTER LISTS selected
    # by #getscrpt in the live DRONE code, not direct script bodies. Decode the
    # list and add the pointed scripts (hgrab et al.) instead of trying to run
    # the list's count WORD as an input command. This exactly matches the
    # historical #getscrpt path: WORD max-index -> rnd -> LONG script pointer.
    expanded = ranges.expand_source(source.read_text(encoding="latin-1").splitlines())
    for list_label in EXTRA_SCRIPT_LISTS:
        sl = ranges.collect_script_list(expanded, list_label)
        for label in sl.scripts:
            add(label)

    # These are the true direct #newsc entry points used outside the range
    # tables.
    for label in DIRECT_SCRIPTS:
        add(label)
    return out


@dataclass
class Op:
    addr: int
    opcode: str
    input_word: int = 0
    delay: int = 0
    percent: int = 0
    target_label: str | None = None
    source_label: str | None = None
    next_addr: int | None = None


@dataclass
class Script:
    label: str
    ops: list[Op]
    unresolved_inline: bool = False


def decode_script(image: Image, label: str) -> Script:
    label_key = symkey(label)
    if label_key not in image.labels:
        fail(f"reachable script label {label} not found in active DRONE.ASM body")
    start = image.labels[label_key]
    pending = [start]
    decoded: dict[int, Op] = {}
    unresolved_inline = False

    while pending:
        addr = pending.pop()
        if addr in decoded:
            continue
        word = image.word(addr)
        next_addr = addr + 16
        if (word & 0x8000) == 0:
            delay_u = image.word(next_addr)
            delay = delay_u - 0x10000 if delay_u & 0x8000 else delay_u
            op = Op(addr, "WM_DRONE_SC_INPUT", input_word=word, delay=delay, next_addr=next_addr + 16)
            decoded[addr] = op
            if delay > 0:
                pending.append(next_addr + 16)
            continue

        if word & 0x4000:
            op = Op(addr, "WM_DRONE_SC_DONE", next_addr=next_addr)
            decoded[addr] = op
            pending.append(next_addr)
            continue

        cmd = word & 0x3fff
        if cmd == 1:
            op = Op(addr, "WM_DRONE_SC_SEEK", next_addr=next_addr)
            decoded[addr] = op; pending.append(next_addr)
        elif cmd == 2:
            table = image.long_label(next_addr)
            op = Op(addr, "WM_DRONE_SC_SKILL_ABORT", source_label=table, next_addr=next_addr + 32)
            decoded[addr] = op; pending.append(next_addr + 32)
        elif cmd == 3:
            op = Op(addr, "WM_DRONE_SC_WAIT_INTERRUPTIBLE", next_addr=next_addr)
            decoded[addr] = op; pending.append(next_addr)
        elif cmd == 4:
            op = Op(addr, "WM_DRONE_SC_ABORT_IF_BLOCKING", next_addr=next_addr)
            decoded[addr] = op; pending.append(next_addr)
        elif cmd == 5:
            code = image.long_label(next_addr)
            op = Op(addr, "WM_DRONE_SC_CALL_CODE", source_label=code, next_addr=next_addr + 32)
            decoded[addr] = op; pending.append(next_addr + 32)
        elif cmd == 6:
            pct_u = image.word(next_addr)
            pct = pct_u - 0x10000 if pct_u & 0x8000 else pct_u
            target = image.long_label(next_addr + 16)
            op = Op(addr, "WM_DRONE_SC_RANDOM_JUMP", percent=pct, target_label=target, next_addr=next_addr + 48)
            decoded[addr] = op
            pending.append(next_addr + 48)
            target_key = symkey(target)
            if target_key not in image.labels: fail(f"script {label} random jump target {target} not found")
            pending.append(image.labels[target_key])
        elif cmd == 7:
            target = image.long_label(next_addr)
            op = Op(addr, "WM_DRONE_SC_JUMP", target_label=target)
            decoded[addr] = op
            target_key = symkey(target)
            if target_key not in image.labels: fail(f"script {label} jump target {target} not found")
            pending.append(image.labels[target_key])
        else:
            # Source EXGPC executes inline TMS code beginning at the current a9.
            # Chunk C3 deliberately exposes this seam instead of guessing its
            # instruction length or silently skipping it. C4 ports the service.
            op = Op(addr, "WM_DRONE_SC_CALL_FUNCTION", source_label=f"{label}@EXGPC_{addr-start:04x}")
            decoded[addr] = op
            unresolved_inline = True

        if len(decoded) > 4096:
            fail(f"script {label} exceeded 4096 decoded ops")

    ordered = [decoded[a] for a in sorted(decoded)]
    addr_to_pc = {op.addr: i for i, op in enumerate(ordered)}
    for op in ordered:
        if op.target_label:
            ta = image.labels[symkey(op.target_label)]
            if ta not in addr_to_pc:
                fail(f"script {label} target {op.target_label} did not decode to an op")
    return Script(label, ordered, unresolved_inline)


def collect_skill_tables(image: Image, scripts: list[Script]) -> dict[str, list[int]]:
    names: list[str] = []
    seen: set[str] = set()
    for sc in scripts:
        for op in sc.ops:
            if op.opcode == "WM_DRONE_SC_SKILL_ABORT" and op.source_label:
                key = symkey(op.source_label)
                if key not in seen:
                    seen.add(key); names.append(op.source_label)
    out: dict[str, list[int]] = {}
    for name in names:
        key = symkey(name)
        if key not in image.labels:
            fail(f"command-2 skill table {name} not found")
        addr = image.labels[key]
        vals: list[int] = []
        for _ in range(SKILL_COUNT):
            u = image.word(addr)
            vals.append(u - 0x10000 if u & 0x8000 else u)
            addr += 16
        out[name] = vals
    return out


def cid(s: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", s)


def cstr(s: str | None) -> str:
    if s is None: return "NULL"
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def emit(source: Path, out: Path) -> tuple[int, int, int]:
    image = build_image(source)
    labels = range_script_labels(source)
    scripts = [decode_script(image, label) for label in labels]
    skills = collect_skill_tables(image, scripts)
    seam_labels = []
    for sc in scripts:
        # Script.unresolved_inline is a boolean marker, not a collection.
        # The actual seam names live on CALL_CODE / CALL_FUNCTION ops.
        for op in sc.ops:
            if op.opcode in ("WM_DRONE_SC_CALL_CODE", "WM_DRONE_SC_CALL_FUNCTION") and op.source_label:
                if op.source_label not in seam_labels:
                    seam_labels.append(op.source_label)
    unresolved = len(seam_labels)

    body = [
        "#ifndef WM_ARCADE_DRONE_SOURCE_SCRIPTS_GENERATED_H",
        "#define WM_ARCADE_DRONE_SOURCE_SCRIPTS_GENERATED_H",
        "",
        "/* GENERATED DIRECTLY FROM historical DRONE.ASM. DO NOT HAND-EDIT. */",
        "#define WM_FIX39_DRONE_SCRIPTS_GENERATED 1",
        f"#define WM_FIX39_DRONE_SCRIPT_COUNT {len(scripts)}",
        f"#define WM_FIX39_DRONE_SKILL_TABLE_COUNT {len(skills)}",
        f"#define WM_FIX39_DRONE_C4_SEAM_COUNT {unresolved}",
        "",
    ]
    body.append(f"static const char *const wm_fix39_drone_c4_seam_labels[{max(1, unresolved)}] = {{")
    if seam_labels:
        for label in seam_labels:
            body.append(f"    {cstr(label)},")
    else:
        body.append("    NULL,")
    body.append("};")
    body.append("")
    for sc in scripts:
        ident = cid(sc.label)
        addr_to_pc = {op.addr: i for i, op in enumerate(sc.ops)}
        body.append(f"static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_{ident}[{len(sc.ops)}] = {{")
        for op in sc.ops:
            target_pc = 0
            if op.target_label:
                target_pc = addr_to_pc[image.labels[symkey(op.target_label)]]
            body.append(
                f"    {{ {op.opcode}, 0x{op.input_word & 0xffff:04x}u, {op.delay}, {op.percent}, {target_pc}u, {cstr(op.source_label)} }},"
            )
        body.append("};")
        body.append("")
    body.append(f"static const wm_arcade_drone_script_t wm_fix39_drone_scripts[{len(scripts)}] = {{")
    for sc in scripts:
        ident = cid(sc.label)
        body.append(f"    {{ {cstr(sc.label)}, wm_fix39_drone_ops_{ident}, {len(sc.ops)}u }},")
    body.append("};")
    body.append("")
    for name, vals in skills.items():
        body.append(f"static const int16_t wm_fix39_drone_skill_{cid(name)}[{SKILL_COUNT}] = {{")
        for i in range(0, len(vals), 10):
            body.append("    " + ", ".join(str(v) for v in vals[i:i+10]) + ",")
        body.append("};")
        body.append("")
    body.append(f"static const WmFix39DroneSkillTable wm_fix39_drone_skill_tables[{len(skills) if skills else 1}] = {{")
    if skills:
        for name in skills:
            body.append(f"    {{ {cstr(name)}, wm_fix39_drone_skill_{cid(name)} }},")
    else:
        body.append("    { NULL, NULL },")
    body.append("};")
    body.extend(["", "#endif", ""])
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(body))
    return len(scripts), len(skills), unresolved


def self_test() -> None:
    with tempfile.TemporaryDirectory() as td:
        p = Path(td) / "DRONE.ASM"
        o = Path(td) / "out.h"
        # Nine roots satisfy the range parser; every list points to sc0. The
        # body covers input, done/yield, commands 1..7, a command-2 table, and
        # command-5 fail-closed seam. Command words are 0x8000|n.
        roots = ",".join("m" + str(i) for i in range(9))
        modes = "\n".join(f"#m{i}\nWL -1,list0" for i in range(9))
        (Path(td) / "GAME.EQU").write_text('''
MOVE_UP equ 1
PLAYER_PUNCH_BIT equ 0
PLAYER_PUNCH_VAL equ 1<<PLAYER_PUNCH_BIT
BINMASK equ 01111b
''', encoding="latin-1")
        p.write_text(f'''
.file "drone.asm"
.include "game.equ"
U_M .equ MOVE_UP<<5
P_M .equ PLAYER_PUNCH_VAL
#wnshort_t
.long {roots}
#wnmed_t
.long {roots}
#wnlong_t
.long {roots}
{modes}
#list0
.word 0
.long drn_seek
#M_shrtblkr
.word 0
.long hgrab
#M_shrtblkrdl
.word 0
.long hgrabdl
#slhtoss
#drn_enterring
#drn_opinair
#drn_oprun
#drn_roll
#drn_inair
#drn_ontb
#drn_run
#drn_combo
#drn_seekclose
#drn_oppdead
#hgrab
#hgrabdl
SUBRP drn_seek
.word P_M+U_M,2
.word 0c000h
.word 08001h
.word 08002h
.long sktab
.word 08003h,08004h,08005h
.long code0
.word 08006h,100
.long branch
.word 08007h
.long end0
#branch
.word 0,-1
#end0
.word 0,-1
#sktab
.word {','.join(str(i) for i in range(30))}
#code0
.word 0
''', encoding="latin-1")
        nsc, nsk, seams = emit(p, o)
        txt = o.read_text()
        assert nsc >= 1 and nsk == 1 and seams >= 1
        assert "WM_DRONE_SC_DONE" in txt and "WM_DRONE_SC_RANDOM_JUMP" in txt
        assert '"drn_seek"' in txt, "SUBRP-generated drn_seek label from MACROS.H was not recognized"
        assert '"hgrab"' in txt and '"hgrabdl"' in txt, "#getscrpt pointer-list scripts were not extracted"
        assert '"M_shrtblkr"' not in txt and '"M_shrtblkrdl"' not in txt, "script pointer lists were incorrectly emitted as script bodies"
        assert "wm_fix39_drone_skill_sktab[30]" in txt
        assert "WM_FIX39_DRONE_C4_SEAM_COUNT" in txt
    print("Fix39 DRONE script generator self-test: PASS")


def main() -> None:
    ap = argparse.ArgumentParser(description="Generate exact DRONE script bodies + skill tables from DRONE.ASM")
    ap.add_argument("--source", type=Path)
    ap.add_argument("--out", type=Path)
    ap.add_argument("--self-test", action="store_true")
    ns = ap.parse_args()
    if ns.self_test:
        self_test(); return
    if not ns.source or not ns.out:
        fail("--source and --out are required")
    nsc, nsk, seams = emit(ns.source, ns.out)
    print(f"Generated {nsc} DRONE scripts, {nsk} command-2 skill tables, {seams} C4 code seams -> {ns.out}")


if __name__ == "__main__":
    main()
