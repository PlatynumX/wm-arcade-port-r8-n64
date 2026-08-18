#!/usr/bin/env python3
import importlib.util
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("asmseq", ROOT / "tools" / "asmseq.py")
mod = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(mod)

equ = mod.load_equates([ROOT / "tests" / "fixtures" / "ANIM_MIN.EQU"])
ev = mod.ExprEvaluator(equ)
words = mod.extract_words(ROOT / "tests" / "fixtures" / "FINISEQ_MIN.ASM", "hrt_finish1_move", ev)
expected = [0x8002, 0x000C, 0x8003, 0x8026, 0x0100, 0x801B, 0x805F, 0x8002, 0x0000, 0x8049]
if words != expected:
    raise SystemExit(f"wrong extraction: {words!r}")
print("asmseq parser test passed")
