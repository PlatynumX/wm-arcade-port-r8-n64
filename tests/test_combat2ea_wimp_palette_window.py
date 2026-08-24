#!/usr/bin/env python3
import importlib.util, pathlib, struct, sys, types
root=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('ca',root/'tools/fix39_character_assets.py'); m=importlib.util.module_from_spec(spec); sys.modules['ca']=m; spec.loader.exec_module(m)
class Pal:
    def __init__(self,name,count,off): self.name=name; self.color_count=count; self.directory_offset=off
class W:
    @staticmethod
    def read_palette_words(data,p): return list(range(p.color_count))
# synthetic 0x1A palette record with a real base value 192 at unknown +0x08.
data=bytearray(0x100); pal=Pal('BAMBLU_P',64,0x20); struct.pack_into('<H',data,0x28,192)
words,parts,base,mode=m._source_palette_window(data,pal,[pal],W,bytes([0,192,200,255]))
assert base==192 and len(words)==256 and words[192]==0 and words[255]==63
assert mode.startswith('record-base')
# Never accept a guessed dense palette for the Bam-style case.
data2=bytearray(0x100); pal2=Pal('BAD',64,0x20)
try: m._source_palette_window(data2,pal2,[pal2],W,bytes([0,64,255]))
except ValueError: pass
else: raise AssertionError('must fail closed without source palette-base metadata')
print('Combat2EA WIMP source palette-window regression: PASS')
