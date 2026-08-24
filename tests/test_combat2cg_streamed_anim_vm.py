#!/usr/bin/env python3
from __future__ import annotations
import pathlib, tempfile, sys
R=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(R/'tools'))
import fix39_anim_vm_program as vm
root=R/'source_payload'/'anim'
with tempfile.TemporaryDirectory() as td:
    d=pathlib.Path(td); c=d/'program.c'; h=d/'program.h'; fs=d/'filesystem'/'fix39_anim'
    vm.emit(root,c,h,fs)
    text=c.read_text(errors='replace')
    assert 'static const wm_source_anim_ins_t vm_ins_' not in text
    assert 'wm_source_anim_program_cache_reset' in text
    assert len(list((fs/'programs').glob('*.bin')))==1527
    assert len(list((fs/'tables').glob('*.bin')))==579
    assert c.stat().st_size < 512*1024, c.stat().st_size
    assert sum(p.stat().st_size for p in fs.rglob('*.bin')) > 100000
rt=(R/'src/fix39/wm_fix39_runtime.c').read_text(errors='replace')
assert 'wm_source_anim_program_cache_reset();' in rt
ap=(R/'tools/fix39_anim_vm_program.py').read_text(errors='replace')
assert "--out-fs" in ap
print('Combat2CG streamed ANIM.ASM VM payload: PASS')
