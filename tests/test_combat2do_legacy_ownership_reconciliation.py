#!/usr/bin/env python3
"""Regression for DN/DO dependency-closed ownership policy."""
from pathlib import Path
import subprocess, sys
R=Path(__file__).resolve().parents[1]
# Historical ownership tests retained by the build must all pass under the current policy.
for rel in [
    'tests/test_v13_completion.py',
    'tests/test_combat2di_build_graph_authority.py',
    'tests/test_combat2dm_selective_ownership.py',
    'tests/test_combat2dn_selective_ownership.py',
]:
    q=subprocess.run([sys.executable,str(R/rel)],cwd=R,text=True,capture_output=True)
    assert q.returncode==0, f'{rel}\n{q.stdout}\n{q.stderr}'
# Live policy must classify all three dependency providers as Fix39-owned.
p=(R/'tools/apply_fix39.py').read_text()
for n in ['wmania_ring_geometry.c','wmania_rng.c','wmania_attract_core.c']:
    assert f'"{n}"' in p, n
print('Combat2DO legacy ownership reconciliation: PASS')
