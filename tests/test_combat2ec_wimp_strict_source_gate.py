#!/usr/bin/env python3
from pathlib import Path
import importlib.util, tempfile, sys
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('ca',root/'tools/fix39_character_assets.py')
ca=importlib.util.module_from_spec(spec); sys.modules['ca']=ca; spec.loader.exec_module(ca)
class P:
    name='P'; directory_offset=0
class W:
    @staticmethod
    def read_ci8(data,image): return data['pixels']
    @staticmethod
    def read_palette_words(data,pal): return data['pal']
# direct 0..63 is source-preserving
px=bytes(range(64)); out,pal,parts,base,mode=ca._source_ci8_view({'pixels':px,'pal':[0]*64},None,P(),[P()],W)
assert out==px and len(pal)==64 and base==0 and mode=='source-dense-proven'
# 1..64 must NOT be remapped without source proof
try: ca._source_ci8_view({'pixels':bytes(range(1,65)),'pal':[0]*64},None,P(),[P()],W)
except ValueError as e: assert 'strict source proof required' in str(e)
else: raise AssertionError('unproven one-based WIMP remap was accepted')
# 0..255 with a 64-entry record must likewise fail closed
try: ca._source_ci8_view({'pixels':bytes(range(256)),'pal':[0]*64},None,P(),[P()],W)
except ValueError as e: assert 'strict source proof required' in str(e)
else: raise AssertionError('unproven palette reconstruction was accepted')
print('Combat2EC WIMP strict source gate PASS')
