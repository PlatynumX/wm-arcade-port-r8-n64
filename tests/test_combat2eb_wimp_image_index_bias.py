#!/usr/bin/env python3
import importlib.util, pathlib, sys
root=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('ca',root/'tools/fix39_character_assets.py')
ca=importlib.util.module_from_spec(spec); sys.modules['ca']=ca; spec.loader.exec_module(ca)
class Img: pass
class Pal:
    def __init__(self,n=64): self.name='BAMBLU_P'; self.color_count=n; self.directory_offset=0
class W:
    @staticmethod
    def read_ci8(data,image): return data
    @staticmethod
    def read_palette_words(data,pal): return list(range(pal.color_count))
# Exact source-observed 1..64 range is normalized per-image.
p=bytes(range(1,65))
px,words,parts,base,mode=ca._source_ci8_view(p,Img(),Pal(),[Pal()],W)
assert mode=='image-one-based' and min(px)==0 and max(px)==63 and len(words)==64
# Ordinary dense 0..63 remains untouched.
p=bytes(range(64))
px,words,parts,base,mode=ca._source_ci8_view(p,Img(),Pal(),[Pal()],W)
assert px==p and mode in ('dense-zero',) or mode.startswith('record-base')
# A genuinely wider range must not be silently rebased.
try:
    ca._source_ci8_view(bytes([1,255]),Img(),Pal(),[Pal()],W)
except ValueError:
    pass
else:
    raise AssertionError('wide CI8 range was not fail-closed')
print('Combat2EC WIMP image-level CI8 bias regression: PASS')
