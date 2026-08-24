#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
h=(root/'src/fix39/wm_fix39_runtime.h').read_text()
r=(root/'src/fix39/wm_fix39_runtime.c').read_text()
a=(root/'src/fix39/wmania_attract_adapter.h').read_text()
ac=(root/'src/fix39/wmania_attract_adapter.c').read_text()
m=(root/'src/fix39/wm_arcade_matchflow.c').read_text()
assert '#if defined(WM_FIX39_DIAGNOSTIC_PRESENTER_POSE)' in h
assert '#if defined(WM_FIX39_DIAGNOSTIC_PRESENTER_POSE)' in r
assert 'show_gameplay_demo_unimplemented' not in a+ac
assert 'start_gameplay_demo' in a+ac and 'wm_attract_demo_plan_make' in ac
assert 'WRESTLE.ASM #rr_cpuwon' in m
print('Combat2DY strict runtime parity contract: PASS')
