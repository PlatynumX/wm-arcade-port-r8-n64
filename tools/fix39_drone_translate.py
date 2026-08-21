#!/usr/bin/env python3
from __future__ import annotations
import argparse,re,tempfile
from pathlib import Path

FIELDS={
'DRN_BUT':'but','DRN_JOY':'joy','DRN_DELAY':'delay','DRN_BUTCHRG':'but_charge',
'DRN_BUTCHRGDLY':'but_charge_delay','DRN_SEEKDIR':'seek_dir','DRN_SEEKDIST':'seek_dist'
}

def num(s:str):
    s=s.strip().lower().replace('#','')
    if s.endswith('h'):
        try:return int(s[:-1],16)
        except:return None
    try:return int(s,0)
    except:return None

def parse_report(p:Path):
    blocks=[]; cur=None
    for raw in p.read_text(encoding='utf-8',errors='replace').splitlines():
        if raw.startswith('body='):
            if cur: blocks.append(cur)
            label=raw.split('|',1)[0][5:]
            cur=[label,[]]
        elif cur is not None and raw.startswith('  '): cur[1].append(raw[2:])
    if cur: blocks.append(cur)
    return blocks

def translate(lines):
    regs={}; ops=[]; saw_ret=False
    for raw in lines:
        s=raw.split(';',1)[0].strip()
        if not s or s.startswith('.') or s.startswith('#') or s.endswith(':'): continue
        if re.match(r'(?i)^(?:\.word|\.long|\.byte)\b',s): continue
        m=re.match(r'(?i)^clr\s+(a\d+)$',s)
        if m: regs[m.group(1).lower()]=0; continue
        m=re.match(r'(?i)^mov[ik]\s+([^,]+),\s*(a\d+)$',s)
        if m:
            v=num(m.group(1));
            if v is None:return None
            regs[m.group(2).lower()]=v; continue
        m=re.match(r'(?i)^move\s+(a\d+),\s*(a\d+)$',s)
        if m:
            src,dst=m.group(1).lower(),m.group(2).lower()
            if src not in regs:return None
            regs[dst]=regs[src]; continue
        m=re.match(r'(?i)^move\s+\*a13\((DRN_[A-Z0-9_]+)\),\s*(a\d+)$',s)
        if m:
            f,r=m.group(1).upper(),m.group(2).lower()
            if f not in FIELDS:return None
            regs[r]=('field',FIELDS[f]); continue
        m=re.match(r'(?i)^move\s+(a\d+),\s*\*a13\((DRN_[A-Z0-9_]+)\)',s)
        if m:
            r,f=m.group(1).lower(),m.group(2).upper()
            if r not in regs or f not in FIELDS:return None
            ops.append((FIELDS[f],regs[r])); continue
        m=re.match(r'(?i)^(addk|subk)\s+([^,]+),\s*(a\d+)$',s)
        if m:
            v=num(m.group(2)); r=m.group(3).lower()
            if v is None or r not in regs:return None
            cur=regs[r]; op='+' if m.group(1).lower()=='addk' else '-'
            regs[r]=(op,cur,v) if isinstance(cur,tuple) else (cur+v if op=='+' else cur-v); continue
        m=re.match(r'(?i)^(andi|ori|xori)\s+([^,]+),\s*(a\d+)$',s)
        if m:
            v=num(m.group(2)); r=m.group(3).lower()
            if v is None or r not in regs:return None
            cur=regs[r]; bop={'andi':'&','ori':'|','xori':'^'}[m.group(1).lower()]
            regs[r]=(bop,cur,v) if isinstance(cur,tuple) else ({'&':cur&v,'|':cur|v,'^':cur^v}[bop]); continue
        if re.match(r'(?i)^rets\b',s): saw_ret=True; break
        # Anything involving branches/calls/actor memory is not auto-translated.
        return None
    return ops if saw_ret else None

def cident(label): return re.sub(r'[^A-Za-z0-9_]','_',label)
def cexpr(v):
    if isinstance(v,int): return str(v)
    if isinstance(v,tuple) and len(v)==2 and v[0]=='field': return 'drone->'+v[1]
    if isinstance(v,tuple) and len(v)==3 and v[0] in ('+','-','&','|','^'):
        return '('+cexpr(v[1])+' '+v[0]+' '+str(v[2])+')'
    raise ValueError(v)
def emit(report:Path,out:Path):
    translated=[]
    for label,lines in parse_report(report):
        ops=translate(lines)
        if ops is not None: translated.append((label,ops))
    q=[]
    q += ['#ifndef WM_ARCADE_DRONE_SOURCE_BODIES_GENERATED_H','#define WM_ARCADE_DRONE_SOURCE_BODIES_GENERATED_H',
          f'#define WM_FIX39_DRONE_TRANSLATED_BODY_COUNT {len(translated)}']
    for label,ops in translated:
        fn='wm_fix39_body_'+cident(label)
        q.append(f'static int {fn}(wm_arcade_actor_t *self, wm_arcade_actor_t *opp, wm_arcade_drone_state_t *drone, void *user){{')
        q.append('    (void)self; (void)opp; (void)user; if(!drone) return 0;')
        for field,val in ops:
            cast='uint16_t' if field in ('but','joy','but_charge') else 'int32_t'
            q.append(f'    drone->{field}=({cast})({cexpr(val)});')
        q.append('    return 1; }')
    q.append('static const WmFix39DroneGeneratedBody wm_fix39_generated_bodies[WM_FIX39_DRONE_TRANSLATED_BODY_COUNT > 0 ? WM_FIX39_DRONE_TRANSLATED_BODY_COUNT : 1] = {')
    if translated:
        for label,_ in translated: q.append(f'    {{"{label}", wm_fix39_body_{cident(label)}}},')
    else:q.append('    {0,0},')
    q += ['};','#endif']
    out.write_text('\n'.join(q)+'\n',encoding='utf-8')
    return len(translated)

def self_test():
    with tempfile.TemporaryDirectory() as td:
        d=Path(td); r=d/'r'; o=d/'o.h'
        r.write_text('body=foo@EXGPC_0000|x|addr=0|status=source-lines|lines=3\n  movk 7,a0\n  move a0,*a13(DRN_DELAY)\n  rets\nbody=baz|x|addr=1|status=source-lines|lines=5\n  move *a13(DRN_BUT),a0\n  ori 4,a0\n  move a0,*a13(DRN_BUT)\n  rets\nbody=bar|x|addr=2|status=source-lines|lines=2\n  callr rnd\n  rets\n')
        n=emit(r,o); t=o.read_text(); assert n==2 and 'drone->delay' in t and 'foo@EXGPC_0000' in t and 'bar' not in t
        assert 'drone->but=(uint16_t)((drone->but | 4));' in t and '"baz"' in t
    print('Fix39 DRONE conservative direct-body translator self-test: PASS')

def main():
    a=argparse.ArgumentParser();a.add_argument('--report');a.add_argument('--out');a.add_argument('--self-test',action='store_true');x=a.parse_args()
    if x.self_test:return self_test()
    if not x.report or not x.out: raise SystemExit('--report --out required')
    n=emit(Path(x.report),Path(x.out)); print(f'Generated {n} conservative direct DRONE body translation(s) -> {x.out}')
if __name__=='__main__':main()
