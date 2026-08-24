#!/usr/bin/env python3
import importlib.util, pathlib, tempfile, sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('cassets',ROOT/'tools/fix39_character_assets.py')
m=importlib.util.module_from_spec(spec)
sys.modules[spec.name]=m
spec.loader.exec_module(m)
asm = """
 SUBR lex_stand2_anim
 SUBR lex_stand8_anim
 .word ANI_SETMODE,MODE_NORMAL
 WL 5,L2ST2C+FR6
 WL 5,L2ST2C+FR5
 WL 5,L2ST2C+FR1
 WL 5,L2ST2C+FR7
 .word ANI_REPEAT
 SUBR lex_stand4_anim
 WL 5,L4ST4C+FR4
 WL 5,L4ST4C+FR7
 .word ANI_REPEAT
"""
with tempfile.TemporaryDirectory() as td:
    p=pathlib.Path(td)/'LEXSEQ1.ASM'; p.write_text(asm)
    idx=m.source_index(pathlib.Path(td))
    _,s2=m.extract_seq(idx,'lex_stand2_anim',True)
    _,s4=m.extract_seq(idx,'lex_stand4_anim',True)
    assert [f.name for f in s2.frames]==['L2ST2C06','L2ST2C05','L2ST2C01','L2ST2C07']
    assert [f.name for f in s4.frames]==['L4ST4C04','L4ST4C07']
    assert all('L4ST4A' not in f.name for f in s4.frames)
print('Combat2V exact visual-slice regression: PASS')
