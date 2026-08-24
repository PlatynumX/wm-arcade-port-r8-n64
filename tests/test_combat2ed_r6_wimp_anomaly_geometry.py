#!/usr/bin/env python3
import importlib.util,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('assets',ROOT/'tools/fix39_character_assets.py')
m=importlib.util.module_from_spec(spec); sys.modules[spec.name]=m; spec.loader.exec_module(m)
class I:
    name='X'; width=4; height=3; directory_offset=0
px=bytes([
  0,1,2,0,
  0,65,0,0,
  255,1,0,0,
])
lines=m._outlier_geometry_lines(bytes(128),I(),px,64,label='test')
t='\n'.join(lines)
assert 'active_size_match=True' in t
assert 'value=65 count=1' in t
assert '(1,1)' in t
assert 'value=255 count=1' in t
assert '(0,2)' in t
assert 'row=1' in t and 'row=2' in t
# R6 remains diagnostic; no source transformation is permitted.
s=(ROOT/'tools/fix39_character_assets.py').read_text()
assert 'def _anomaly_geometry_diagnostic' in s
assert "if kind == 'neither':" in s
print('Combat2ED-R6 WIMP anomaly geometry diagnostic: PASS')
