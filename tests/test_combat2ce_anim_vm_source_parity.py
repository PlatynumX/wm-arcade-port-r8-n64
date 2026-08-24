#!/usr/bin/env python3
from __future__ import annotations
import argparse, pathlib, re, sys
R=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(R/'tools'))
import fix39_anim_vm_program as vm
ap=argparse.ArgumentParser(); ap.add_argument('--source-root',type=pathlib.Path,default=R/'source_payload'/'anim'); a=ap.parse_args()
root=a.source_root
progs,tables,used,unresolved,total=vm.parse_program(root)
assert not unresolved, unresolved
# Historical source snapshot currently used by the project. These numbers are
# intentionally exact so a parser regression cannot silently flatten or omit
# part of the eight-wrestler/common animation corpus.
assert len(progs)==1527, len(progs)
assert total==40777, total
assert len(used)==106, len(used)
assert len(tables)==579, len(tables)
vals,_=vm.parse_equates(root)
op={k[4:]:v-0x8000 for k,v in vals.items() if k.startswith('ANI_') and isinstance(v,int) and 0x8000<=v<0x9000}
usedops={op[n] for n in used}
rt=(R/'src/fix39/wm_arcade_source_animation_runtime.c').read_text(errors='replace')
cases=set(map(int,re.findall(r'case\s+(\d+)\s*:',rt)))
assert usedops <= cases, sorted(usedops-cases)
assert 43 not in usedops  # ANIM.EQU explicitly marks opcode 43 unused.
print(f'Combat2CE ANIM.ASM source parity: PASS ({len(progs)} programs, {total} instructions, {len(used)} used ANI commands, {len(tables)} tables)')
