#!/usr/bin/env python3
from pathlib import Path
import importlib.util, sys
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('chars',ROOT/'tools/fix39_character_assets.py')
mod=importlib.util.module_from_spec(spec); sys.modules[spec.name]=mod; spec.loader.exec_module(mod)
class Pal:
    def __init__(self,name,n,off,base): self.name=name; self.color_count=n; self.directory_offset=off; self.base=base
class W:
    @staticmethod
    def read_palette_words(data,p): return list(range(p.base,p.base+p.color_count))
# Source-observed wrestler case: a 64-color WIMP directory record can be the
# first fragment of the 256-entry CI8 palette bank. Reconstruct from real
# consecutive source records only; never pad/clamp/synthesize.
pals=[Pal('BAMBLU_P',64,0x100,0),Pal('BAMBANK1',64,0x11a,64),Pal('BAMBANK2',64,0x134,128),Pal('BAMBANK3',64,0x14e,192),Pal('NEXT',64,0x168,300)]
words,parts=mod._effective_palette_words(b'',pals[0],pals,W)
assert len(words)==256
assert words[0]==0 and words[64]==64 and words[255]==255
assert parts==('BAMBLU_P','BAMBANK1','BAMBANK2','BAMBANK3')
# A 128-entry source bank remains 128 when the next record would cross the
# CI8 256-entry boundary; no fabricated completion is allowed.
p2=[Pal('P128',128,1,0),Pal('P200',200,2,128)]
w2,parts2=mod._effective_palette_words(b'',p2[0],p2,W)
assert len(w2)==128 and parts2==('P128',)
print('Combat2DW WIMP effective palette-bank regression: PASS')
