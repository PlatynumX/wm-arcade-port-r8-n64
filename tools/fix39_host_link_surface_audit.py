#!/usr/bin/env python3
"""Fail before CMake if wm_fix39_tests references wm_* functions with no active C provider.
This is deliberately conservative and complements (not replaces) the real host link.
"""
from pathlib import Path
import re,sys
root=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
cm=(root/'CMakeLists.txt').read_text(errors='replace')
smoke=(root/'tests/fix39_smoke.c').read_text(errors='replace')
active=[]
for rel in re.findall(r'(?m)^\s*(src/(?:fix39|core/arcade|core|generated)/[A-Za-z0-9_./-]+\.c)\s*$',cm):
    p=root/rel
    if p.exists(): active.append(p)
# Function definitions. This intentionally keys only wm_* exported services.
defs={}
pat_def=re.compile(r'(?m)^[\t ]*(?:static[\t ]+)?(?:[A-Za-z_][A-Za-z0-9_]*[\t *]+)+(?P<n>wm_[A-Za-z0-9_]+)\s*\([^;{}]*\)\s*\{')
for p in active:
    txt=p.read_text(errors='replace')
    for m in pat_def.finditer(txt): defs.setdefault(m.group('n'),[]).append(p.relative_to(root).as_posix())
# Function calls in smoke test, excluding its own definitions/declarations.
calls=set(re.findall(r'\b(wm_[A-Za-z0-9_]+)\s*\(',smoke))
local={m.group('n') for m in pat_def.finditer(smoke)}
# Known macro-ish API tokens can be filtered only if they are not calls after preprocessing;
# currently none are needed.
missing=sorted(n for n in calls-local if n not in defs)
if missing:
    print('Combat2DP host link-surface audit: FAIL',file=sys.stderr)
    for n in missing: print(f' - no active C provider for {n}',file=sys.stderr)
    raise SystemExit(1)
print(f'Combat2DP host link-surface audit: PASS ({len(calls-local)} wm_fix39_tests call targets have active C providers)')
