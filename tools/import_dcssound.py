#!/usr/bin/env python3
"""Strict DCSSOUND.ASM/SOUND.EQU -> portable C data importer.

Generated data:
  * complete triple_sndtab (must be exactly 0x303 entries),
  * DEFAULT_SOUND_TABLE (59 words),
  * all nine MASTER_SOUND_TABLE wrestler rows,
  * all 0x43 random sound tables,
  * standard ADD_TO_QUEUE/ADD_IF_SILENT speech tables,
  * source crowd tables used by those speech tables.

The parser is intentionally fail-closed.  An unresolved expression aborts the
build instead of substituting a guessed value.
"""
from __future__ import annotations

import argparse
import ast
import pathlib
import re
import sys
from dataclasses import dataclass

PRIORITIES = {
    "sp_robo": 1 << 8,
    "sp_mat1": 4 << 8,
    "sp_woosh": 8 << 8,
    "sp_attkv": 12 << 8,
    "sp_reacv": 14 << 8,
    "sp_losmack": 15 << 8,
    "sp_smack": 16 << 8,
    "sp_mat2": 20 << 8,
    "sp_wspch": 24 << 8,
    "sp_system1": 36 << 8,
    "sp_system2": 40 << 8,
    "sp_system3": 44 << 8,
    "sp_anncer": 100 << 8,
    "DEFLT": 0x8000,
}

SPEECH_LABELS = [
    "CLIMB_ROPES", "JUMP_ROPES", "MISSES", "SPECIAL_MOVE", "DROP_KICK",
    "FACE_HIT", "MID_HIT", "AVERAGE_MOVE", "REVERSAL", "MISS_YOKO",
    "THROWN_OUT", "OTHER_AVERAGE", "NASTY_MOVE", "SETUP_MOVE",
    "SPECIAL_LAST_STUFF", "MATCH_OVER", "MATCH_OVER_DL",
]
CROWD_LABELS = [
    "SETUP_TABLE", "CRESCENDO_TABLE", "ROPES_CHEER", "CROWD_FAIL",
    "CROWD_SPECIAL", "CROWD_CHEER", "CROWD_THROWN", "CROWD_ORDINARY",
]

class ParseError(RuntimeError):
    pass


def strip_comment(line: str) -> str:
    return line.split(";", 1)[0].rstrip()


def normalize_expr(text: str) -> str:
    s = text.strip()
    s = re.sub(r">([0-9a-fA-F]+)\b", r"0x\1", s)
    s = re.sub(r"\b([0-9][0-9a-fA-F]*)[hH]\b", r"0x\1", s)
    return s


def eval_node(node: ast.AST, symbols: dict[str, int]) -> int:
    if isinstance(node, ast.Expression):
        return eval_node(node.body, symbols)
    if isinstance(node, ast.Constant) and isinstance(node.value, int):
        return int(node.value)
    if isinstance(node, ast.Name):
        if node.id in symbols:
            return symbols[node.id]
        # assembly is case-insensitive
        for k, v in symbols.items():
            if k.lower() == node.id.lower():
                return v
        raise ParseError(f"unresolved symbol {node.id!r}")
    if isinstance(node, ast.UnaryOp):
        v = eval_node(node.operand, symbols)
        if isinstance(node.op, ast.USub): return -v
        if isinstance(node.op, ast.UAdd): return v
        if isinstance(node.op, ast.Invert): return ~v
    if isinstance(node, ast.BinOp):
        a = eval_node(node.left, symbols)
        b = eval_node(node.right, symbols)
        if isinstance(node.op, ast.Add): return a + b
        if isinstance(node.op, ast.Sub): return a - b
        if isinstance(node.op, ast.BitOr): return a | b
        if isinstance(node.op, ast.BitAnd): return a & b
        if isinstance(node.op, ast.LShift): return a << b
        if isinstance(node.op, ast.RShift): return a >> b
        if isinstance(node.op, ast.Mult): return a * b
        if isinstance(node.op, ast.FloorDiv): return a // b
    raise ParseError(f"unsupported expression AST: {ast.dump(node)}")


def expr(text: str, symbols: dict[str, int]) -> int:
    src = normalize_expr(text)
    try:
        tree = ast.parse(src, mode="eval")
    except SyntaxError as e:
        raise ParseError(f"invalid expression {text!r}: {e.msg}") from e
    return eval_node(tree, symbols)


def parse_equ(text: str) -> dict[str, int]:
    syms = dict(PRIORITIES)
    pending: list[tuple[str, str]] = []
    pat = re.compile(r"^\s*([A-Za-z_][\w]*)\s+\.?(?:equ|set)\s+(.+?)\s*$", re.I)
    for raw in text.splitlines():
        line = strip_comment(raw)
        m = pat.match(line)
        if m:
            pending.append((m.group(1), m.group(2).strip()))

    # Resolve forward/simple dependencies iteratively.
    for _ in range(len(pending) + 4):
        progress = False
        keep = []
        for name, value in pending:
            try:
                syms[name] = expr(value, syms)
                progress = True
            except ParseError:
                keep.append((name, value))
        pending = keep
        if not progress:
            break
    return syms


def load_source_symbols(dcssound: pathlib.Path,
                        sound_equ: pathlib.Path) -> dict[str, int]:
    """Resolve .EQU/.SET symbols from DCSSOUND's real direct include context.

    The arcade assembler sees GAME.EQU, SYS.EQU, PLYR.EQU, etc. before
    DCSSOUND.ASM.  Crowd-table flags such as C_LONG/C_RANDOM live in GAME.EQU,
    not SOUND.EQU, so parsing only SOUND.EQU is not source-equivalent.
    """
    dcs_text = dcssound.read_text(encoding="latin-1")
    root = dcssound.parent

    # Case-insensitive lookup matches the TMS34010 assembler source behavior
    # while still running on Termux/Linux's case-sensitive filesystem.
    files = {
        p.name.lower(): p
        for p in root.iterdir()
        if p.is_file()
    }

    ordered: list[pathlib.Path] = []
    seen: set[pathlib.Path] = set()

    def add(path: pathlib.Path) -> None:
        path = path.resolve()
        if path not in seen:
            seen.add(path)
            ordered.append(path)

    # SOUND.EQU is explicit authority from the command line.
    add(sound_equ)

    inc_rx = re.compile(
        r'^\s*\.include\s+["\']([^"\']+)["\']',
        re.I
    )
    for raw in dcs_text.splitlines():
        clean = strip_comment(raw)
        m = inc_rx.match(clean)
        if not m:
            continue
        name = pathlib.Path(m.group(1)).name.lower()
        dep = files.get(name)
        if dep is None:
            raise ParseError(
                f"DCSSOUND include {m.group(1)!r} not found beside "
                f"{dcssound.name}"
            )
        add(dep)

    # Local .set/.equ declarations inside DCSSOUND itself come last, exactly
    # as they do in the source translation unit.
    chunks = [
        p.read_text(encoding="latin-1")
        for p in ordered
    ]
    chunks.append(dcs_text)
    return parse_equ("\n".join(chunks))


def directive_values(line: str, directive: str, symbols: dict[str, int]) -> list[int] | None:
    clean = strip_comment(line).strip()
    m = re.match(rf"^\.?{directive}\s+(.+)$", clean, re.I)
    if not m:
        return None
    fields = [x.strip() for x in m.group(1).split(",") if x.strip()]
    return [expr(x, symbols) for x in fields]


def label_positions(lines: list[str]) -> dict[str, int]:
    out: dict[str, int] = {}
    for i, raw in enumerate(lines):
        clean = strip_comment(raw).strip()
        if not clean:
            continue
        # Local table labels sometimes begin with '#'.
        m = re.fullmatch(r"#?([A-Za-z_][\w]*)", clean)
        if m:
            out[m.group(1)] = i
    return out


def between(lines: list[str], labels: dict[str, int], start: str, end: str) -> list[str]:
    if start not in labels or end not in labels:
        raise ParseError(f"missing section label {start!r} or {end!r}")
    return lines[labels[start] + 1:labels[end]]


def parse_words(section: list[str], symbols: dict[str, int]) -> list[int]:
    out: list[int] = []
    for line in section:
        vals = directive_values(line, "word", symbols)
        if vals is not None:
            out.extend(v & 0xffff for v in vals)
    return out


def parse_triple(lines: list[str], labels: dict[str, int], symbols: dict[str, int]):
    out: list[tuple[int, int, str]] = []
    unresolved: list[str] = []
    section = between(lines, labels, "triple_sndtab", "triple_end")
    base_line = labels["triple_sndtab"] + 2
    for off, raw in enumerate(section):
        clean = strip_comment(raw).strip()
        m = re.match(r"^\.?word\s+([^,]+),\s*([^,\s]+)", clean, re.I)
        if not m:
            continue
        try:
            pd = expr(m.group(1), symbols) & 0xffff
            cmd = expr(m.group(2), symbols) & 0xffff
        except ParseError as e:
            unresolved.append(f"line {base_line + off}: {e}: {raw.strip()}")
            continue
        comment = raw.split(";", 1)[1].strip() if ";" in raw else ""
        out.append((pd, cmd, comment))
    if unresolved:
        raise ParseError("triple_sndtab unresolved:\n" + "\n".join(unresolved))
    if len(out) != 0x303:
        raise ParseError(f"triple_sndtab count {len(out)} != 0x303")
    return out


def parse_default_master(lines: list[str], labels: dict[str, int], symbols: dict[str, int]):
    default = parse_words(between(lines, labels, "DEFAULT_SOUND_TABLE", "MASTER_SOUND_TABLE"), symbols)
    if len(default) != 59:
        raise ParseError(f"DEFAULT_SOUND_TABLE count {len(default)} != 59")

    start = labels["MASTER_SOUND_TABLE"] + 1
    end = None
    for i in range(start, len(lines)):
        if re.match(r"^\s*SUBR\s+table_sound\b", strip_comment(lines[i]), re.I):
            end = i
            break
    if end is None:
        raise ParseError("SUBR table_sound not found after MASTER_SOUND_TABLE")
    master = parse_words(lines[start:end], symbols)
    if len(master) != 59 * 9:
        raise ParseError(f"MASTER_SOUND_TABLE count {len(master)} != {59*9}")
    return default, master


def parse_random(lines: list[str], labels: dict[str, int], symbols: dict[str, int]):
    if "random_sound_tables" not in labels:
        raise ParseError("random_sound_tables not found")
    refs: list[str] = []
    i = labels["random_sound_tables"] + 1
    while i < len(lines):
        clean = strip_comment(lines[i]).strip()
        if not clean:
            i += 1
            continue
        m = re.match(r"^\.long\s+#?([A-Za-z_][\w]*)", clean, re.I)
        if not m:
            break
        refs.append(m.group(1))
        i += 1
    if len(refs) != 0x43:
        raise ParseError(f"random_sound_tables pointer count {len(refs)} != 0x43")

    groups: list[list[int]] = []
    for ref in refs:
        if ref not in labels:
            raise ParseError(f"random table label {ref} not found")
        j = labels[ref] + 1
        vals = None
        while j < len(lines):
            clean = strip_comment(lines[j]).strip()
            if not clean:
                j += 1
                continue
            # Fall-through aliases: skip consecutive labels and use next word.
            if re.fullmatch(r"#?[A-Za-z_][\w]*", clean):
                j += 1
                continue
            vals = directive_values(lines[j], "word", symbols)
            break
        if not vals:
            raise ParseError(f"random table {ref} has no .word data")
        max_index = vals[0]
        raw_values = [v & 0xffff for v in vals[1:]]
        if max_index < 0:
            raise ParseError(f"random table {ref}: negative max index {max_index}")
        reachable = max_index + 1
        if len(raw_values) < reachable:
            raise ParseError(
                f"random table {ref}: header {max_index}, values {len(raw_values)}; "
                f"needs at least {reachable}"
            )
        # DCSSOUND.ASM table_sound reads the header, calls RNDRNG0(header),
        # then indexes exactly one WORD from the following array.  Therefore
        # only header+1 entries are reachable.  The pinned source contains at
        # least one deliberate/legacy trailing word (doink_stomp_v: header 2
        # followed by four values); preserving that word as a selectable entry
        # would be less source-accurate than truncating to the reachable slice.
        values = raw_values[:reachable]
        groups.append(values)
    return groups


def previous_directive(lines: list[str], before: int, directive: str,
                       symbols: dict[str, int]) -> tuple[int, list[int]]:
    for i in range(before - 1, -1, -1):
        clean = strip_comment(lines[i]).strip()
        if not clean:
            continue
        vals = directive_values(lines[i], directive, symbols)
        if vals is not None:
            return i, vals
        # Do not walk across another real label/subroutine when recovering header.
        if re.fullmatch(r"#?[A-Za-z_][\w]*", clean) or re.match(r"^SUBR\b", clean, re.I):
            break
    raise ParseError(f".{directive} header not found before source line {before+1}")


def previous_raw_directive(lines: list[str], before: int, directive: str) -> tuple[int, str]:
    """Find a preceding directive without evaluating its argument.

    Source speech-table headers use symbolic .LONG label pointers.  Those
    labels are addresses in the original linker, not numeric .EQU symbols, so
    attempting to feed them through the expression evaluator is both wrong
    and needlessly fragile.
    """
    rx = re.compile(rf"^\.{re.escape(directive)}\s+(.+)$", re.I)
    for i in range(before - 1, -1, -1):
        clean = strip_comment(lines[i]).strip()
        if not clean:
            continue
        m = rx.match(clean)
        if m:
            return i, m.group(1).strip()
        if re.fullmatch(r"#?[A-Za-z_][\w]*", clean) or re.match(r"^SUBR\b", clean, re.I):
            break
    raise ParseError(f".{directive} header not found before source line {before+1}")


@dataclass
class Speech:
    name: str
    reset: int
    crowd: str | None
    entry_words: int
    entries: list[list[int]]


def parse_speech(lines: list[str], labels: dict[str, int], symbols: dict[str, int]):
    out: list[Speech] = []
    for name in SPEECH_LABELS:
        if name not in labels:
            raise ParseError(f"speech label {name} not found")
        at = labels[name]
        header_i, hdr = previous_directive(lines, at, "word", symbols)
        if len(hdr) != 2:
            raise ParseError(f"speech {name}: expected .word max,size header")
        max_entry, size_bits = hdr
        if size_bits not in (0x10, 0x20, 0x30, 0x40):
            raise ParseError(f"speech {name}: unsupported entry size 0x{size_bits:x}")
        entry_words = size_bits // 0x10

        # SPECIAL_LAST_STUFF is a deliberate inline subtable inside the
        # SETUP_MOVE data.  In pinned DCSSOUND.ASM it has only the ordinary
        # max,size WORD header followed by entries; there is no reset WORD and
        # no LONG crowd-table pointer before it.  It is selected by the
        # END_GAME_STUFF pseudo-voice in SETUP_MOVE, not passed through the
        # normal ADD_TO_QUEUE table header path.
        if name == "SPECIAL_LAST_STUFF":
            crowd = None
            reset = 0
        else:
            crowd_i, crowd_arg = previous_raw_directive(lines, header_i, "long")
            # Labels are linker addresses in the arcade source, not .EQU values.
            # Preserve the symbolic table name and treat only a literal zero as null.
            crowd_arg = crowd_arg.lstrip("#").strip()
            if "," in crowd_arg:
                raise ParseError(f"speech {name}: bad crowd pointer {crowd_arg!r}")
            try:
                crowd_numeric = expr(crowd_arg, symbols)
            except ParseError:
                crowd_numeric = None
            crowd = None if crowd_numeric == 0 else crowd_arg

            # Standard tables have the reset word before the crowd pointer.
            reset = 0
            try:
                _, reset_vals = previous_directive(lines, crowd_i, "word", symbols)
                if len(reset_vals) == 1:
                    reset = reset_vals[0]
            except ParseError:
                pass

        entries: list[list[int]] = []
        i = at + 1
        while i < len(lines) and len(entries) < max_entry + 1:
            vals = directive_values(lines[i], "word", symbols)
            if vals is not None:
                if len(vals) != entry_words:
                    raise ParseError(
                        f"speech {name}: entry {len(entries)} has {len(vals)} words, expected {entry_words}"
                    )
                entries.append([((v + 0x8000) & 0xffff) - 0x8000 for v in vals])
            i += 1
        if len(entries) != max_entry + 1:
            raise ParseError(f"speech {name}: expected {max_entry+1} entries, got {len(entries)}")
        out.append(Speech(name, reset, crowd, entry_words, entries))
    return out


@dataclass
class Crowd:
    name: str
    entries: list[tuple[int,int,int,int]]


def parse_crowd(lines: list[str], labels: dict[str, int], symbols: dict[str, int]):
    out: list[Crowd] = []
    for name in CROWD_LABELS:
        if name not in labels:
            raise ParseError(f"crowd label {name} not found")
        at = labels[name]
        _, hdr = previous_directive(lines, at, "word", symbols)
        if len(hdr) != 1 or hdr[0] < 0:
            raise ParseError(f"crowd {name}: bad max-index header")
        count = hdr[0] + 1
        entries = []
        i = at + 1
        while i < len(lines) and len(entries) < count:
            vals = directive_values(lines[i], "word", symbols)
            if vals is not None:
                if len(vals) != 4:
                    raise ParseError(f"crowd {name}: expected four fields")
                entries.append(tuple(v & 0xffff for v in vals))
            i += 1
        if len(entries) != count:
            raise ParseError(f"crowd {name}: expected {count}, got {len(entries)}")
        out.append(Crowd(name, entries))
    return out


def c_ident(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", name.lower())


def emit(triple, default, master, random_groups, speech, crowd, src_name: str) -> str:
    crowd_index = {c.name: i for i, c in enumerate(crowd)}
    lines = [
        '#include "wm/arcade_sound_tables.h"', '',
        f'/* GENERATED STRICTLY from pinned {src_name}; do not hand-edit. */',
        'const wm_sound_entry wm_arcade_sound_table[0x303] = {'
    ]
    for i, (pd, cmd, comment) in enumerate(triple):
        safe = comment.replace('*/', '* /')
        lines.append(
            f'    [0x{i:03x}] = {{0x{pd:04x}u, 0x{cmd:04x}u, WM_SOUND_NO_SCRIPT, 0}}, /* {safe} */'
        )
    lines += ['};', 'const uint16_t wm_arcade_sound_table_count = 0x303u;', '']

    lines.append('static const uint16_t source_default_values[59] = {')
    for i in range(0, len(default), 8):
        lines.append('    ' + ', '.join(f'0x{x:04x}u' for x in default[i:i+8]) + ',')
    lines += ['};', 'static const uint16_t source_master_values[59 * 9] = {']
    for i in range(0, len(master), 8):
        lines.append('    ' + ', '.join(f'0x{x:04x}u' for x in master[i:i+8]) + ',')
    lines += ['};', 'const wm_sound_wrestler_matrix wm_arcade_wrestler_matrix = {',
              '    source_default_values, source_master_values, 59u, 9u', '};', '']

    for i, values in enumerate(random_groups):
        lines.append(f'static const uint16_t random_{i:02x}[] = {{' +
                     ', '.join(f'0x{x:03x}u' for x in values) + '};')
    lines.append('const wm_sound_random_table wm_arcade_random_tables[] = {')
    for i, values in enumerate(random_groups):
        lines.append(f'    {{random_{i:02x}, {len(values)}u}},')
    lines += ['};', 'const uint16_t wm_arcade_random_table_count = 0x43u;', '']

    # No source tune scripts are referenced by triple_sndtab in this revision;
    # keep the runtime engine available for any later table/script translations.
    lines += ['const wm_sound_script wm_arcade_sound_scripts[] = {{0,0}};',
              'const uint16_t wm_arcade_sound_script_count = 0u;', '']

    for c in crowd:
        ident = c_ident(c.name)
        lines.append(f'static const wm_sound_crowd_entry crowd_{ident}[] = {{')
        for a,b,flags,rnd in c.entries:
            lines.append(f'    {{0x{a:04x}u, 0x{b:04x}u, 0x{flags:04x}u, 0x{rnd:04x}u}},')
        lines.append('};')
    lines.append('const wm_sound_crowd_table wm_arcade_crowd_tables[] = {')
    for c in crowd:
        ident = c_ident(c.name)
        lines.append(f'    {{crowd_{ident}, {len(c.entries)}u}}, /* {c.name} */')
    lines += ['};', f'const uint16_t wm_arcade_crowd_table_count = {len(crowd)}u;', '']

    for s in speech:
        ident = c_ident(s.name)
        flat = [v for row in s.entries for v in row]
        lines.append(f'static const int16_t speech_{ident}[] = {{')
        for i in range(0, len(flat), 8):
            lines.append('    ' + ', '.join(str(v) for v in flat[i:i+8]) + ',')
        lines.append('};')
    lines.append('const wm_sound_speech_table wm_arcade_speech_tables[] = {')
    for s in speech:
        ident = c_ident(s.name)
        ci = -1 if s.crowd is None else crowd_index.get(s.crowd, -2)
        if ci == -2:
            raise ParseError(f"speech {s.name}: crowd table {s.crowd!r} is not generated")
        lines.append(
            f'    {{{s.reset}, {ci}, {len(s.entries)}u, {s.entry_words}u, speech_{ident}}}, /* {s.name} */'
        )
    lines += ['};', f'const uint16_t wm_arcade_speech_table_count = {len(speech)}u;', '']

    lines += [
        'void wm_arcade_sound_bind_default_tables(wm_arcade_sound *s) {',
        '    wm_arcade_sound_bind_tables(s,',
        '        wm_arcade_sound_table, wm_arcade_sound_table_count,',
        '        wm_arcade_sound_scripts, wm_arcade_sound_script_count,',
        '        wm_arcade_random_tables, wm_arcade_random_table_count,',
        '        wm_arcade_wrestler_matrix,',
        '        wm_arcade_speech_tables, wm_arcade_speech_table_count,',
        '        wm_arcade_crowd_tables, wm_arcade_crowd_table_count);',
        '}', ''
    ]
    return '\n'.join(lines)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('dcssound', type=pathlib.Path)
    ap.add_argument('sound_equ', type=pathlib.Path)
    ap.add_argument('output', type=pathlib.Path)
    args = ap.parse_args()

    dcs_text = args.dcssound.read_text(encoding='latin-1')
    symbols = load_source_symbols(args.dcssound, args.sound_equ)
    lines = dcs_text.splitlines()
    labels = label_positions(lines)

    triple = parse_triple(lines, labels, symbols)
    default, master = parse_default_master(lines, labels, symbols)
    random_groups = parse_random(lines, labels, symbols)
    speech = parse_speech(lines, labels, symbols)
    crowd = parse_crowd(lines, labels, symbols)

    generated = emit(triple, default, master, random_groups, speech, crowd,
                     args.dcssound.name)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(generated, encoding='utf-8')
    print(
        f'generated {args.output}: triple={len(triple)}, default={len(default)}, '
        f'master={len(master)}, random={len(random_groups)}, '
        f'speech={len(speech)}, crowd={len(crowd)}'
    )
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except ParseError as e:
        print(f'ERROR: {e}', file=sys.stderr)
        raise SystemExit(2)
