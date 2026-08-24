#!/usr/bin/env python3
import importlib.util, pathlib, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
modpath=ROOT/'tools'/'fix39_character_assets.py'
spec=importlib.util.spec_from_file_location('fix39_character_assets_test',modpath)
mod=importlib.util.module_from_spec(spec); sys.modules[spec.name]=mod; spec.loader.exec_module(mod)
seq=mod._VisualSequence('lex_stand2_anim',(
    mod._VisualFrame('L2ST2C01',5),mod._VisualFrame('L2ST2C07',5),
    mod._VisualFrame('L2ST2C08',5),mod._VisualFrame('L4ST4A01',5)),True)
filtered=mod._filter_sequence_to_physical_assets(seq,{'L2ST2C01','L2ST2C07'},'lex','stand2',pathlib.Path('LEXSEQ1.ASM'))
assert [f.name for f in filtered.frames]==['L2ST2C01','L2ST2C07']
assert filtered.repeat is True
try:
    mod._filter_sequence_to_physical_assets(mod._VisualSequence('bad',(mod._VisualFrame('L9NOPE01',1),),False),set(),'lex','bad',pathlib.Path('LEXSEQX.ASM'))
except ValueError:
    pass
else:
    raise AssertionError('all-missing sequence must fail closed')
print('Fix39 Combat2Y physical-WIMP asset filter regression: PASS')
