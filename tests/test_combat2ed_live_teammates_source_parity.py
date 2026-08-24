#!/usr/bin/env python3
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
asm=(ROOT/'source_payload/anim/WRESTLE2.ASM').read_text(errors='replace')
c=(ROOT/'src/fix39/wm_fix39_runtime.c').read_text(errors='replace')
body=re.search(r'SUBR\s+ck_live_teammates(.*?)(?=\n#\*{20,}|\n\s*SUBR\s+ck_any_teammates)', asm, re.S)
assert body, 'source ck_live_teammates body missing'
src=body.group(1)
for token in ['process_ptrs','skip inactive','skip self','skip other team','MODE_DEAD','setc','clrc']:
    assert token in src, f'source teammate authority missing {token}'
assert 'static int live_has_live_teammates' in c
assert 'candidate = g.actor_ptrs[i]' in c
checks=[
    '!candidate || !candidate->active',
    'candidate == victim',
    'candidate->player_side != victim->player_side',
    'candidate->player_mode == WM_PMODE_DEAD',
]
pos=[c.index(x, c.index('static int live_has_live_teammates')) for x in checks]
assert pos == sorted(pos), 'translated teammate filters are not in source order'
fn=c[c.index('static int live_has_live_teammates'):c.index('static int live_rndper_hi')]
assert 'return 1;' in fn and 'return 0;' in fn
assert 'live_no_teammates' not in c
assert 'victim_has_live_teammates = live_has_live_teammates;' in c
print('Combat2ED live teammate source parity: PASS')
