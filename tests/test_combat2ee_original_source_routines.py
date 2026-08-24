#!/usr/bin/env python3
import sys
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit('usage: test_combat2ee_original_source_routines.py <historical-source-root>')
root=Path(sys.argv[1])
util=(root/'UTIL.ASM').read_text(errors='ignore').replace('\r','')
w2=(root/'WRESTLE2.ASM').read_text(errors='ignore').replace('\r','')

def normalized_lines(block):
    return [' '.join(x.strip().split()).lower() for x in block.splitlines() if x.strip()]

# Midway UTIL.ASM::RNDPER exact instruction sequence.
a=util.index('SUBR\tRNDPER') if 'SUBR\tRNDPER' in util else util.index('SUBR RNDPER')
block=util[a:a+600]
lines=normalized_lines(block)
expected=[
    'subr rndper',
    'move @rand,a1,l',
    'rl a1,a1',
    'move @hcount,a14',
    'rl a14,a1',
    'add sp,a1',
    'move a1,@rand,l',
    'move a0,a14',
    'movi 1000,a0',
    'mpyu a1,a0 ;0-999',
    'cmp a0,a14',
    'rets',
]
pos=0
for want in expected:
    while pos < len(lines) and lines[pos] != want:
        pos += 1
    if pos == len(lines):
        raise AssertionError(f'UTIL.ASM RNDPER missing/out-of-order: {want!r}\nwindow={lines!r}')
    pos += 1
# The explanatory probability contract is immediately ABOVE `SUBR RNDPER`
# in Midway's source, so include the preceding comment window when proving it.
proof_window = util[max(0, a - 500):a + 600].lower()
assert 'a0=probability of event (0-1000)' in proof_window
assert '0=0%, 1000=100%' in proof_window
assert 'jrhi happened' in proof_window

# Midway WRESTLE2.ASM::keep_onscreen begins by rejecting non-two-human PSTATUS.
a=w2.index('SUBR\tkeep_onscreen') if 'SUBR\tkeep_onscreen' in w2 else w2.index('SUBR keep_onscreen')
block=w2[a:a+900]
lines=normalized_lines(block)
expected=[
    'subr keep_onscreen',
    'move @pstatus,a14',
    'cmpi 3,a14 ;is it a 2-player game?',
    'jrne #no_2_player',
]
pos=0
for want in expected:
    while pos < len(lines) and lines[pos] != want:
        pos += 1
    if pos == len(lines):
        raise AssertionError(f'WRESTLE2 keep_onscreen missing/out-of-order: {want!r}')
    pos += 1

# The getup-meter side effect in that source file is XFERPROC, not a guessed spawn.
a=w2.index('SUBR\tditch_getup_meter') if 'SUBR\tditch_getup_meter' in w2 else w2.index('SUBR ditch_getup_meter')
block=w2[a:a+900].lower()
assert 'move\t*a13(meter_proc),a0,l' in block or 'move *a13(meter_proc),a0,l' in block
assert 'calla\txferproc' in block or 'calla xferproc' in block

print('Combat2EE original Midway UTIL/WRESTLE2 routine proof: PASS')
