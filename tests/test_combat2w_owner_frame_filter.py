#!/usr/bin/env python3
import importlib.util, pathlib, tempfile, sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('cassets_owner',ROOT/'tools/fix39_character_assets.py')
m=importlib.util.module_from_spec(spec); sys.modules[spec.name]=m; spec.loader.exec_module(m)
asm="""
 SUBR bam_test_anim
 WL 2,B4PU3A+FR1
 ; embedded opponent/puppet Lex references must not become Bam presenter art
 WL 2,L2ST2C+FR8
 WL 2,L4ST4A+FR11
 WL 2,B4PU3A+FR2
 .word ANI_END
"""
with tempfile.TemporaryDirectory() as td:
    p=pathlib.Path(td)/'BAMSEQX.ASM'; p.write_text(asm)
    idx=m.source_index(pathlib.Path(td))
    _,seq=m.extract_seq(idx,'bam_test_anim',False,owner_name='bam')
    names=[f.name for f in seq.frames]
    assert names==['B4PU3A01','B4PU3A02'], names
print('Combat2W owner-frame filtering regression: PASS')
