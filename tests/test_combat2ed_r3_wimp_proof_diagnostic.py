#!/usr/bin/env python3
import importlib.util, sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('assets',ROOT/'tools/fix39_character_assets.py')
m=importlib.util.module_from_spec(spec); sys.modules[spec.name]=m; spec.loader.exec_module(m)

class Img:
    name='B4AH4A01'; directory_offset=0x20; palette_index_raw=7
    width=2; height=2; xani=0; yani=0; tail_words=(1,2,3)
class Pal:
    name='BAMBLU_P'; directory_offset=0x80; color_count=64
class W:
    @staticmethod
    def read_ci8(data,image): return bytes((0,63,64,64))
    @staticmethod
    def read_palette_words(data,pal): return list(range(64))

data=bytes(range(256))*2
lines=m._strict_ci8_failure_diagnostic(ROOT,'bam','B4AH4A01','bam_hit.img',data,Img(),Pal(),[Pal()],W())
t='\n'.join(lines)
assert 'frame=B4AH4A01' in t
assert 'container=bam_hit.img' in t
assert 'index_64_count=2' in t
assert 'NO PIXEL OR PALETTE TRANSFORM APPLIED' in t
try:
    m._source_ci8_view(data,Img(),Pal(),[Pal()],W())
except ValueError:
    pass
else:
    raise AssertionError('strict source gate must still reject index 64 against 64 entries')
print('Combat2ED-R3 strict WIMP proof diagnostic: PASS')
