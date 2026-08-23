#!/usr/bin/env python3
from pathlib import Path
R=Path(__file__).resolve().parents[1]
# These implementation-string assertions encode the superseded Makefile
# exception-list ownership model and must never return.
for rel in ('tests/test_v13_completion.py','tests/test_v13e_chunk6.py'):
    s=(R/rel).read_text()
    assert "need(patcher, 'arcade_names.difference_update(BASELINE_OVERRIDES)')" not in s, rel
    assert 'apply.split("BASELINE_OVERRIDES"' not in s, rel
# The patcher itself must use general basename-overlap authority.
p=(R/'tools/apply_fix39.py').read_text()
assert 'overlaps = sorted(set(sources) & arcade_names)' in p
assert 'for src in overlaps:' in p
assert 'difference_update(BASELINE_OVERRIDES)' not in p
# The dedicated graph regression must cover both host CMake and N64 Makefile.
g=(R/'tests/test_combat2di_build_graph_authority.py').read_text()
assert "assert f'src/core/arcade/{n}' not in cm2" in g
assert "assert f'src/core/arcade/{n}' not in mk2" in g
print('Combat2DJ legacy-test/build-graph reconciliation: PASS')
