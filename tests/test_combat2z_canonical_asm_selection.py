#!/usr/bin/env python3
import importlib.util, pathlib, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('cassets_canonical',ROOT/'tools/fix39_character_assets.py')
m=importlib.util.module_from_spec(spec); sys.modules[spec.name]=m; spec.loader.exec_module(m)
with tempfile.TemporaryDirectory() as td:
    d=pathlib.Path(td)
    (d/"LEXSEQ1'.ASM").write_text(" SUBR lex_stand4_anim\n WL 5,L4ST4A+FR1\n .word ANI_REPEAT\n")
    (d/'LEXSEQ1.ASM').write_text(" SUBR lex_stand4_anim\n WL 5,L4ST4C+FR4\n .word ANI_REPEAT\n")
    idx=m.source_index(d)
    assert idx['lex_stand4_anim'].name=='LEXSEQ1.ASM', idx['lex_stand4_anim']
    path,seq=m.extract_seq(idx,'lex_stand4_anim',True,owner_name='lex')
    assert path.name=='LEXSEQ1.ASM'
    assert [f.name for f in seq.frames]==['L4ST4C04']
    assert not m._is_canonical_asm(d/"LEXSEQ1'.ASM")
    assert m._is_canonical_asm(d/'LEXSEQ1.ASM')
print('Fix39 Combat2Z canonical-ASM selection regression: PASS')
