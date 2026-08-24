#!/usr/bin/env python3
from pathlib import Path
import re, sys
repo = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('.')
path = repo / 'src/fix39/wm_fix39_runtime.c'
s = path.read_text()
# Exact regressions observed in Combat2DK N64 GCC.
for bad in (
    'if(!a)return; ++a->usr_var1;',
    'if(!a)return;a->anim_mode&=',
):
    assert bad not in s, f'known misleading-indentation regression remains: {bad}'
# Reject the warning-prone shape in generated runtime: a line beginning with an
# unbraced if, a controlled statement, and then another same-line statement.
for n, line in enumerate(s.splitlines(), 1):
    stripped = line.strip()
    if not stripped.startswith('if') or '{' in stripped.split(';',1)[0]:
        continue
    # Conservative: only flag lines with at least two statements after an if.
    if ';else' in stripped.replace(' ', ''):
        continue
    if stripped.count(';') >= 2 and re.match(r'^if\s*\(.*\)\s*[^{};]+;\s*\S', stripped):
        raise AssertionError(f'warning-prone unbraced same-line if at {path}:{n}: {stripped}')
print('Combat2DL N64 misleading-indentation scan: PASS')
