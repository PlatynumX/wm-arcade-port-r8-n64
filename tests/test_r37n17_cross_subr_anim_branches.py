#!/usr/bin/env python3
from pathlib import Path
import importlib.util

root=Path(__file__).resolve().parents[1]
src=root/"original/wwf-wrestlemania"
if not (src/"DNKSEQ3.ASM").exists():
    raise SystemExit("R37N17 test requires original/wwf-wrestlemania")

spec=importlib.util.spec_from_file_location("animvm",root/"tools/fix39_anim_vm_program.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
progs,tables,used,unresolved,total=m.parse_program(src)

by={label:(rid,source,ins) for rid,label,source,ins in progs}

def branch(program, opname, target):
    rid,source,ins=by[program]
    found=[]
    for rec in ins:
        if rec[0]!="cmd" or rec[2].upper()!=opname.upper(): continue
        bi=m.BRANCH_ARG.get(rec[2].upper())
        if bi is None or bi>=len(rec[3]): continue
        a=rec[3][bi]
        if a[1].lstrip("#").lower()==target.lstrip("#").lower():
            found.append(a)
    assert found, f"{program}: missing {opname} -> {target}"
    return found

for a in branch("dnk_3_head_hold2_anim","IFNOTSTATUS","#missed"):
    assert a[2]==2, ("dnk_3_head_hold2_anim #missed is not LOCAL_PC",a)
for a in branch("dnk_3_head_hold2_anim","GOTO","#gothim"):
    assert a[2]==2, ("dnk_3_head_hold2_anim #gothim is not LOCAL_PC",a)

for program in ("dnk_3_head_hold2_anim","shn_3_head_hold2_anim","bam_3_head_hold2_anim"):
    rid,source,ins=by[program]
    bad=[]
    for rec in ins:
        if rec[0]!="cmd": continue
        bi=m.BRANCH_ARG.get(rec[2].upper())
        if bi is None or bi>=len(rec[3]): continue
        a=rec[3][bi]
        if isinstance(a[1],str) and a[1].startswith("#") and a[2]==0:
            bad.append((rec[2],a[1]))
    assert not bad, f"{program}: unresolved source-local branches: {bad}"

for a in branch("dnk_2_buzz_anim","GOTO","#cont"):
    assert a[2]==2, ("dnk_2_buzz_anim #cont is not LOCAL_PC",a)

print("R37N17 cross-SUBR animation branch model: PASS")
print("  dnk_3_head_hold2_anim #missed/#gothim -> LOCAL_PC")
print("  logged head-hold shared tails -> no unresolved #local branch text")
print("  dnk_2_buzz_anim -> shared #cont tail preserved")
