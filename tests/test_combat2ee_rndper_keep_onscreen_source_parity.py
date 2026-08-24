#!/usr/bin/env python3
from pathlib import Path

root=Path(__file__).resolve().parents[1]
rng=(root/'src/fix39/wmania_rng.c').read_text()
runtime=(root/'src/fix39/wm_fix39_runtime.c').read_text()
source=(root/'source_payload/anim/WRESTLE2.ASM').read_text(errors='ignore')

# Exact UTIL.ASM RNDPER facts independently recovered from Midway source:
# input 0..1000; output sample 0..999; HI means probability > sample.
assert 'wm_rng_mul_high_u32(rng->rand_state, 1000u)' in rng
assert 'return (uint32_t)probability > sample;' in rng
assert 'wm_rng_rndper_hi(rng, argument)' in runtime
assert 'wm_rng_rndrng0(rng, 255u) > argument' not in runtime

# Boundary semantics implied by source compare: 0%=never, 1000=always.
def hi(prob, sample): return prob > sample
assert all(not hi(0, x) for x in range(1000))
assert all(hi(1000, x) for x in range(1000))
assert sum(hi(100, x) for x in range(1000)) == 100

# WRESTLE2 source body is present in the handoff and gates keep_onscreen on
# PSTATUS==3 before its motion checks.
k=source.index('SUBR\tkeep_onscreen')
window=source[k:k+700]
assert 'move\t@PSTATUS,a14' in window
assert 'cmpi\t3,a14' in window
assert 'jrne\t#no_2_player' in window

# Current live runtime must not forge 3 for CPU-vs-CPU or P1-vs-CPU.
assert 'const int16_t pstatus = g.match_cpu_vs_cpu ? 0 : 1;' in runtime
assert 'if (pstatus != 3)' in runtime
assert 'in.old_pstatus = WM_RING_KEEP_REQUIRED_OLD_PSTATUS;' not in runtime
print('Combat2EE RNDPER + keep_onscreen source parity: PASS')
