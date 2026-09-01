#!/usr/bin/env python3
from pathlib import Path
import importlib.util

root=Path(__file__).resolve().parents[1]
src=root/"original/wwf-wrestlemania"
if not (src/"SHNSEQ4.ASM").exists():
    raise SystemExit("R37N18 test requires original/wwf-wrestlemania")

spec=importlib.util.spec_from_file_location("animvm",root/"tools/fix39_anim_vm_program.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
progs,tables,used,unresolved,total=m.parse_program(src)
by={label:(rid,source,ins) for rid,label,source,ins in progs}

def cmds(label):
    return [(rec[2].upper(),rec[3]) for rec in by[label][2] if rec[0]=="cmd"]

def frames(label):
    return [rec[2] for rec in by[label][2] if rec[0]=="frame"]

c=cmds("shn_hitonground_anim")
names=[x[0] for x in c]
assert "WAITROLL" in names, "shn_hitonground_anim still chopped before shn_liedown_anim WAITROLL"
assert "CHANGEANIM" in names, "shn_hitonground_anim still lacks faceup-getup transition"
assert any(
    name=="CHANGEANIM" and any(a[1].lower()=="shn_faceup_getup_anim" for a in args)
    for name,args in c
), "shn_hitonground_anim does not transition to shn_faceup_getup_anim"

f=frames("shn_hitonground_anim")
assert f.count("S3CP3B07") >= 2, (
    "expected source fallthrough: FR7 before and after SUBR shn_liedown_anim", f[-5:]
)

cx=cmds("shn_hitonground_xflip_anim")
nx=[x[0] for x in cx]
assert "WAITROLL" in nx and "CHANGEANIM" in nx

def assert_local_pc(program,op,target):
    for rec in by[program][2]:
        if rec[0]!="cmd" or rec[2].upper()!=op: continue
        bi=m.BRANCH_ARG.get(op)
        if bi is None or bi>=len(rec[3]): continue
        a=rec[3][bi]
        if a[1].lstrip("#").lower()==target.lstrip("#").lower():
            assert a[2]==2, (program,op,target,a)
            return
    raise AssertionError((program,op,target,"missing"))

assert_local_pc("dnk_3_head_hold2_anim","IFNOTSTATUS","#missed")
assert_local_pc("dnk_2_buzz_anim","GOTO","#cont")

print("R37N18 cross-SUBR fallthrough model: PASS")
print("  shn_hitonground_anim -> shn_liedown_anim fallthrough preserved")
print("  WAITROLL -> shn_faceup_getup_anim preserved")
print("  R37N17 explicit shared-tail branch resolution retained")
