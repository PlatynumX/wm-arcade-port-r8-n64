#!/usr/bin/env python3
from pathlib import Path
import importlib.util, sys
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('chars',ROOT/'tools/fix39_character_assets.py')
mod=importlib.util.module_from_spec(spec); sys.modules[spec.name]=mod; spec.loader.exec_module(mod)
class Im:
    def __init__(self,name,raw,off,px): self.name=name; self.palette_index_raw=raw; self.directory_offset=off; self.px=bytes(px)
class Pal:
    def __init__(self,name,n,off): self.name=name; self.color_count=n; self.directory_offset=off
class W:
    @staticmethod
    def read_ci8(data,im): return im.px
# Combat2DW correction: image palette_index_raw selects a palette-directory
# entry. Pixel indices are CI8 offsets into the effective palette bank and are
# not evidence that the selected directory fragment itself must contain 256
# words. Keep the historical/raw-min directory convention when valid.
ims=[Im('A',10,100,[0,1,63]),Im('B4FK4F10',12,200,[0,64])]
pals=[Pal('P0',64,1000),Pal('P1',64,1100),Pal('P2',64,1200)]
mapping,scheme=mod._source_palette_map(b'',ims,pals,W)
assert scheme=='offset-min', scheme
assert mapping[100].name=='P0'
assert mapping[200].name=='P2'
print('Combat2DV/DW WIMP palette-directory mapping regression: PASS')
