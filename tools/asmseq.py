#!/usr/bin/env python3
"""Extract simple .word animation streams from the historical TMS34010 source.

The tool is intentionally conservative: it accepts only integer expressions and
stops rather than guessing when a sequence contains executable assembly or a
syntax form we have not implemented yet.
"""
from __future__ import annotations

import argparse
import ast
import pathlib
import re
import sys
from typing import Dict, Iterable, List

HEX_SUFFIX_RE = re.compile(r"\b([0-9A-Fa-f]+)h\b")
EQU_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s+equ\s+(.+?)\s*$", re.I)
SUBR_RE = re.compile(r"^\s*SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b", re.I)
WORD_RE = re.compile(r"^\s*\.word\s+(.+?)\s*$", re.I)


class ExprEvaluator(ast.NodeVisitor):
    def __init__(self, raw_symbols: Dict[str, str]):
        self.raw_symbols = raw_symbols
        self.cache: Dict[str, int] = {}
        self.stack: List[str] = []

    def resolve(self, name: str) -> int:
        key = name.upper()
        if key in self.cache:
            return self.cache[key]
        if key not in self.raw_symbols:
            raise ValueError(f"unknown symbol: {name}")
        if key in self.stack:
            raise ValueError(f"recursive equate: {' -> '.join(self.stack + [key])}")
        self.stack.append(key)
        try:
            value = self.eval_expr(self.raw_symbols[key])
        finally:
            self.stack.pop()
        self.cache[key] = value
        return value

    def eval_expr(self, text: str) -> int:
        text = HEX_SUFFIX_RE.sub(lambda m: "0x" + m.group(1), text.strip())
        tree = ast.parse(text, mode="eval")
        return int(self.visit(tree.body))

    def visit_Constant(self, node: ast.Constant) -> int:
        if isinstance(node.value, int):
            return node.value
        raise ValueError(f"unsupported constant: {node.value!r}")

    def visit_Name(self, node: ast.Name) -> int:
        return self.resolve(node.id)

    def visit_BinOp(self, node: ast.BinOp) -> int:
        lhs = self.visit(node.left)
        rhs = self.visit(node.right)
        ops = {
            ast.Add: lambda a, b: a + b,
            ast.Sub: lambda a, b: a - b,
            ast.Mult: lambda a, b: a * b,
            ast.BitOr: lambda a, b: a | b,
            ast.BitAnd: lambda a, b: a & b,
            ast.BitXor: lambda a, b: a ^ b,
            ast.LShift: lambda a, b: a << b,
            ast.RShift: lambda a, b: a >> b,
        }
        fn = ops.get(type(node.op))
        if fn is None:
            raise ValueError(f"unsupported operator: {type(node.op).__name__}")
        return fn(lhs, rhs)

    def visit_UnaryOp(self, node: ast.UnaryOp) -> int:
        value = self.visit(node.operand)
        if isinstance(node.op, ast.USub):
            return -value
        if isinstance(node.op, ast.UAdd):
            return value
        if isinstance(node.op, ast.Invert):
            return ~value
        raise ValueError(f"unsupported unary operator: {type(node.op).__name__}")

    def generic_visit(self, node: ast.AST):
        raise ValueError(f"unsupported expression node: {type(node).__name__}")


def strip_comment(line: str) -> str:
    return line.split(";", 1)[0].rstrip()


def load_equates(paths: Iterable[pathlib.Path]) -> Dict[str, str]:
    out: Dict[str, str] = {}
    for path in paths:
        for raw in path.read_text(errors="replace").splitlines():
            line = strip_comment(raw)
            m = EQU_RE.match(line)
            if m:
                out[m.group(1).upper()] = m.group(2).strip()
    return out


def extract_words(path: pathlib.Path, label: str, ev: ExprEvaluator) -> List[int]:
    wanted = label.upper()
    active = False
    saw_word = False
    words: List[int] = []

    for lineno, raw in enumerate(path.read_text(errors="replace").splitlines(), 1):
        line = strip_comment(raw).strip()
        if not line:
            continue

        sub = SUBR_RE.match(line)
        if sub:
            if active:
                break
            active = sub.group(1).upper() == wanted
            continue

        if not active:
            continue

        m = WORD_RE.match(line)
        if m:
            saw_word = True
            for item in m.group(1).split(","):
                item = item.strip()
                if item:
                    value = ev.eval_expr(item)
                    if not -0x8000 <= value <= 0xFFFF:
                        raise ValueError(f"{path}:{lineno}: .word value out of range: {item} = {value}")
                    words.append(value & 0xFFFF)
            continue

        # Conditional/directive boundary after a pure data sequence is fine.
        if saw_word and (line.startswith(".") or line.startswith("#") or line == "******************************************************************************"):
            break

        if saw_word:
            raise ValueError(
                f"{path}:{lineno}: {label} stopped being a pure .word stream at: {line!r}"
            )

    if not active and not saw_word:
        raise ValueError(f"label not found: {label}")
    if not words:
        raise ValueError(f"no .word data found for label: {label}")
    return words


def c_ident(name: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9_]", "_", name)
    if not cleaned or cleaned[0].isdigit():
        cleaned = "_" + cleaned
    return cleaned


def render_c(source_name: str, label: str, symbol: str, words: List[int]) -> str:
    rows = []
    for i in range(0, len(words), 8):
        rows.append("    " + ", ".join(f"0x{v:04X}" for v in words[i:i+8]) + ",")
    body = "\n".join(rows)
    return f'''/* Auto-generated by tools/asmseq.py. */\n#include <stddef.h>\n#include <stdint.h>\n#include "wm/source_data.h"\n\nconst uint16_t {symbol}[] = {{\n{body}\n}};\n\nconst size_t {symbol}_count = sizeof({symbol}) / sizeof({symbol}[0]);\n\nconst wm_source_sequence wm_source_{c_ident(label)} = {{\n    .source_file = "{source_name}",\n    .source_label = "{label}",\n    .words = {symbol},\n    .word_count = sizeof({symbol}) / sizeof({symbol}[0])\n}};\n'''


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--equ", action="append", required=True, type=pathlib.Path,
                    help="equate file; may be specified more than once")
    ap.add_argument("--source", required=True, type=pathlib.Path)
    ap.add_argument("--label", required=True)
    ap.add_argument("--symbol", default=None,
                    help="C array symbol; defaults to wm_seq_<label>")
    ap.add_argument("--out", type=pathlib.Path)
    ap.add_argument("--dump", action="store_true", help="print numeric words")
    ns = ap.parse_args()

    raw_symbols = load_equates(ns.equ)
    ev = ExprEvaluator(raw_symbols)
    words = extract_words(ns.source, ns.label, ev)

    if ns.dump:
        print(" ".join(f"{w:04X}" for w in words))

    if ns.out:
        symbol = ns.symbol or f"wm_seq_{c_ident(ns.label)}"
        ns.out.parent.mkdir(parents=True, exist_ok=True)
        ns.out.write_text(render_c(ns.source.name, ns.label, symbol, words))
        print(f"wrote {len(words)} words -> {ns.out}")
    elif not ns.dump:
        print(f"{ns.source.name}::{ns.label}: {len(words)} words")

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, SyntaxError, ValueError) as exc:
        print(f"asmseq: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
