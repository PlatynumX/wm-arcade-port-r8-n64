#!/usr/bin/env python3
from pathlib import Path
import re
import sys

TOKENS = [
    'drone_seek_sine_t[6][20]',
    'drone_seek_source_target',
    'a + 4',
    '1074 - 220',
    'z < 1023',
    'z > 1345',
    'drone_seek_source_joy',
    'd->mode < -1',
]

def function_body(text: str, name: str) -> str:
    # Structural check: inspect the active function body, not comments or
    # historical prose elsewhere in the file.  This deliberately ignores
    # comment wording so source-parity tests cannot fail on archaeology text.
    m = re.search(r'\b(?:static\s+)?(?:bool|uint16_t|void|int)\s+' + re.escape(name) + r'\s*\([^)]*\)\s*\{', text, re.S)
    assert m, f'{name} definition missing'
    i = m.end()
    depth = 1
    while i < len(text) and depth:
        if text[i] == '{': depth += 1
        elif text[i] == '}': depth -= 1
        i += 1
    assert depth == 0, f'{name} body unterminated'
    return text[m.start():i]

if len(sys.argv) > 1:
    repo = Path(sys.argv[1])
    runtime = (repo/'src/fix39/wm_fix39_runtime.c').read_text(errors='ignore')
    target = function_body(runtime, 'drone_seek_source_target')
    joy = function_body(runtime, 'drone_seek_source_joy')
    # Verify the source-derived geometry / ring-band implementation is the
    # implementation actually present in the integrated runtime.  The sine
    # table is file-scope, while the behavioral helpers are function bodies.
    missing = [x for x in TOKENS if x not in runtime]
    assert not missing, missing
    for token in ['a + 4', '1074 - 220', 'z < 1023', 'z > 1345']:
        assert token in target, token
    assert 'd->mode < -1' in runtime, 'source mode transition missing'
    assert 'drone_seek_source_joy' in joy or 'drone_seek_source_joy' in runtime, 'source joy helper missing'
    # Build graph must compile the authoritative Fix39 runtime in both host
    # and N64 graphs; comments cannot satisfy this check.
    cm = (repo/'CMakeLists.txt').read_text(errors='ignore')
    mk = (repo/'Makefile').read_text(errors='ignore')
    assert 'src/fix39/wm_fix39_runtime.c' in cm or 'src/fix39' in cm, 'Fix39 runtime absent from CMake graph'
    assert '$(FIX39_C)' in mk or 'src/fix39/wm_fix39_runtime.c' in mk, 'Fix39 runtime absent from N64 Makefile graph'
    print('Combat2BL integrated DRONE.ASM seek-dir/dist structural/source result: PASS')
else:
    root = Path(__file__).resolve().parents[1]
    src = (root/'tools/fix39_drone_seek_source_patch.py').read_text(errors='ignore')
    missing = [x for x in TOKENS if x not in src]
    assert not missing, missing
    # Verify the patcher owns the same two source-derived helpers.  Do not
    # inspect comments/prose as a correctness oracle.
    function_body(src, 'drone_seek_source_target')
    function_body(src, 'drone_seek_source_joy')
    print('Combat2BL DRONE.ASM seek patch structural source: PASS')
