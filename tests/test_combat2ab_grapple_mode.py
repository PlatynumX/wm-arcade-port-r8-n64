#!/usr/bin/env python3
import importlib.util, sys, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
modpath=root/'tools/fix39_character_attack_frames.py'
spec=importlib.util.spec_from_file_location('fix39_character_attack_frames_ab',modpath)
mod=importlib.util.module_from_spec(spec); sys.modules[spec.name]=mod; spec.loader.exec_module(mod)
assert mod.MODE_MAP['AMODE_GRAPPLE']=='WM_AMODE_GRABHOLD'
with tempfile.TemporaryDirectory() as td:
    d=Path(td)
    (d/'LEXSEQ1.ASM').write_text('SUBR #lex_test_anim\n.word ANI_ATTACK_ON,AMODE_GRAPPLE,24,-100,48,28\nWL 1,A2FG3A+FR6\n.word ANI_ATTACK_OFF\n')
    out,counts=mod.emit(d)
    assert 'WM_AMODE_GRAPPLE' not in out
    assert 'WM_AMODE_GRABHOLD' in out
print('Combat2AB grapple-mode mapping regression: PASS')
