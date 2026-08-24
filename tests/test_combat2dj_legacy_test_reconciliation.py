#!/usr/bin/env python3
from pathlib import Path
R=Path(__file__).resolve().parents[1]
for rel in ('tests/test_v13_completion.py','tests/test_v13e_chunk6.py'):
    s=(R/rel).read_text()
    assert "need(patcher, 'arcade_names.difference_update(BASELINE_OVERRIDES)')" not in s, rel
    assert 'apply.split("BASELINE_OVERRIDES"' not in s, rel
p=(R/'tools/apply_fix39.py').read_text()
assert 'FIX39_COMBAT_OWNERS' in p
assert 'fix39_overlaps = [src for src in overlaps if fix39_owns_overlap(src)]' in p
assert 'preserved_overlaps = [src for src in overlaps if not fix39_owns_overlap(src)]' in p
assert 'difference_update(BASELINE_OVERRIDES)' not in p
g=(R/'tests/test_combat2di_build_graph_authority.py').read_text()
assert "wmania_attract_core.c" in g and "wm_arcade_roster.c" in g
assert "assert f'src/fix39/{n}' not in cm1" in g
print('Combat2DJ legacy-test/build-graph reconciliation updated for DN/DO dependency-closed ownership: PASS')
