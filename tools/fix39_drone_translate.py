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

def clean_lines(lines):
    out=[]
    for raw in lines:
        s=raw.split(';',1)[0].strip()
        if not s or s.startswith('.') or re.match(r'(?i)^(?:\.word|\.long|\.byte)\b',s): continue
        out.append(s)
    return out

def translate(lines):
    """Straight-line state translator retained from c5i.

    Returns field-expression stores only when the whole source window is a
    simple, return-terminated DRN-state routine. Any branch/call/actor-memory
    instruction rejects this path and is offered to translate_local_cfg().
    """
    regs={}; ops=[]; saw_ret=False
    for s in clean_lines(lines):
        if s.startswith('#') or s.endswith(':'): continue
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
        return None
    return ops if saw_ret else None

def _lname(s):
    s=s.strip().rstrip(':')
    if s.startswith('#'): s=s[1:]
    return 'L_'+re.sub(r'[^A-Za-z0-9_]','_',s)

# C5h compatibility note: Anything involving branches/calls/actor memory is not auto-translated.
# C5j narrows that rule by translating only proven local branches; calls and actor memory still reject.
def translate_local_cfg(lines):
    """Translate a conservative local branch CFG over DRN state.

    C5j intentionally accepts only local labels, A-register arithmetic/tests,
    supported DRN_* fields, and JRxx branches. External calls, B registers,
    actor fields, globals, indirect memory and unknown opcodes remain rejected.
    This lets source-authored if/else/short-loop service bodies execute without
    inventing semantics for the harder combat/world seams.
    """
    src=clean_lines(lines)
    labels={}
    for s in src:
        if re.match(r'^#[A-Za-z_][A-Za-z0-9_]*:?$',s) or re.match(r'^[A-Za-z_][A-Za-z0-9_]*:$',s):
            labels[_lname(s)]=1
    has_rng_call=any(re.match(r'(?i)^callr\s+(?:rndrng0|rnd)$',s) for s in src)
    if not labels and not any(re.match(r'(?i)^jr(?:uc|z|nz|eq|ne|lt|le|gt|ge)\b',s) for s in src) and not has_rng_call:
        return None

    defined=set(); out=['    int32_t a[15]={0};','    int32_t f_l=0, f_r=0;','    int f_cmp=0;']
    saw_ret=False; saw_branch=False

    def need(r): return r.lower() in defined
    def setflag_value(r):
        return [f'    f_l=a[{int(r[1:])}]; f_r=0; f_cmp=0;']
    def setflag_cmp(lhs,rhs):
        return [f'    f_l={lhs}; f_r={rhs}; f_cmp=1;']

    for s in src:
        if re.match(r'^#[A-Za-z_][A-Za-z0-9_]*:?$',s) or re.match(r'^[A-Za-z_][A-Za-z0-9_]*:$',s):
            out.append(_lname(s)+': ;'); continue
        m=re.match(r'(?i)^clr\s+(a\d+)$',s)
        if m:
            r=m.group(1).lower(); defined.add(r); out.append(f'    a[{int(r[1:])}]=0;'); out+=setflag_value(r); continue
        m=re.match(r'(?i)^mov[ik]\s+([^,]+),\s*(a\d+)$',s)
        if m:
            v=num(m.group(1)); r=m.group(2).lower()
            if v is None:return None
            defined.add(r); out.append(f'    a[{int(r[1:])}]={v};'); out+=setflag_value(r); continue
        m=re.match(r'(?i)^move\s+\*a13\((DRN_[A-Z0-9_]+)\),\s*(a\d+)$',s)
        if m:
            f,r=m.group(1).upper(),m.group(2).lower()
            if f not in FIELDS:return None
            defined.add(r); out.append(f'    a[{int(r[1:])}]=(int32_t)drone->{FIELDS[f]};'); out+=setflag_value(r); continue
        m=re.match(r'(?i)^move\s+(a\d+),\s*(a\d+)$',s)
        if m:
            rs,rd=m.group(1).lower(),m.group(2).lower()
            if not need(rs):return None
            defined.add(rd); out.append(f'    a[{int(rd[1:])}]=a[{int(rs[1:])}];'); out+=setflag_value(rd); continue
        m=re.match(r'(?i)^move\s+(a\d+),\s*\*a13\((DRN_[A-Z0-9_]+)\)',s)
        if m:
            r,f=m.group(1).lower(),m.group(2).upper()
            if not need(r) or f not in FIELDS:return None
            cast='uint16_t' if FIELDS[f] in ('but','joy','but_charge') else 'int32_t'
            out.append(f'    drone->{FIELDS[f]}=({cast})a[{int(r[1:])}];'); continue
        m=re.match(r'(?i)^(addk|subk)\s+([^,]+),\s*(a\d+)$',s)
        if m:
            v=num(m.group(2)); r=m.group(3).lower()
            if v is None or not need(r):return None
            op='+=' if m.group(1).lower()=='addk' else '-='
            out.append(f'    a[{int(r[1:])}]{op}{v};'); out+=setflag_value(r); continue
        m=re.match(r'(?i)^(andi|ori|xori)\s+([^,]+),\s*(a\d+)$',s)
        if m:
            v=num(m.group(2)); r=m.group(3).lower()
            if v is None or not need(r):return None
            op={'andi':'&=','ori':'|=','xori':'^='}[m.group(1).lower()]
            out.append(f'    a[{int(r[1:])}]{op}{v};'); out+=setflag_value(r); continue
        m=re.match(r'(?i)^(add|sub)\s+(a\d+),\s*(a\d+)$',s)
        if m:
            rs,rd=m.group(2).lower(),m.group(3).lower()
            if not need(rs) or not need(rd):return None
            op='+=' if m.group(1).lower()=='add' else '-='
            out.append(f'    a[{int(rd[1:])}]{op}a[{int(rs[1:])}];'); out+=setflag_value(rd); continue
        m=re.match(r'(?i)^callr\s+(rndrng0|rnd)$',s)
        if m:
            if not need('a0'): return None
            helper='wm_fix39_drone_body_rndrng0' if m.group(1).lower()=='rndrng0' else 'wm_fix39_drone_body_rnd'
            out.append(f'    a[0]=(int32_t){helper}(user,(uint32_t)a[0]);'); out+=setflag_value('a0'); continue
        m=re.match(r'(?i)^cmpi\s+([^,]+),\s*(a\d+)$',s)
        if m:
            v=num(m.group(1)); r=m.group(2).lower()
            if v is None or not need(r):return None
            out+=setflag_cmp(f'a[{int(r[1:])}]',str(v)); continue
        m=re.match(r'(?i)^cmp\s+(a\d+),\s*(a\d+)$',s)
        if m:
            r1,r2=m.group(1).lower(),m.group(2).lower()
            if not need(r1) or not need(r2):return None
            # TMS source idiom: CMP a1,a14 / JRGE means a14 >= a1.
            out+=setflag_cmp(f'a[{int(r2[1:])}]',f'a[{int(r1[1:])}]'); continue
        m=re.match(r'(?i)^(sll|srl)\s+([^,]+),\s*(a\d+)$',s)
        if m:
            sh=num(m.group(2)); r=m.group(3).lower()
            if sh is None or not need(r) or sh<0 or sh>31:return None
            op='<<=' if m.group(1).lower()=='sll' else '>>='
            out.append(f'    a[{int(r[1:])}]{op}{sh};'); out+=setflag_value(r); continue
        m=re.match(r'(?i)^btst\s+([^,]+),\s*(a\d+)$',s)
        if m:
            bit=num(m.group(1)); r=m.group(2).lower()
            if bit is None or not need(r) or bit<0 or bit>31:return None
            out.append(f'    f_l=(a[{int(r[1:])}] & (1u<<{bit}))?1:0; f_r=0; f_cmp=0;'); continue
        m=re.match(r'(?i)^jr(uc|z|nz|eq|ne|lt|le|gt|ge)\s+(#[A-Za-z_][A-Za-z0-9_]*)$',s)
        if m:
            cc,t=m.group(1).lower(),_lname(m.group(2)); saw_branch=True
            if t not in labels:return None
            cond={
                'z':'(f_l==0)','nz':'(f_l!=0)','eq':'(f_l==f_r)','ne':'(f_l!=f_r)',
                'lt':'(f_l<f_r)','le':'(f_l<=f_r)','gt':'(f_l>f_r)','ge':'(f_l>=f_r)'}
            if cc=='uc': out.append(f'    goto {t};')
            else: out.append(f'    if {cond[cc]} goto {t};')
            continue
        if re.match(r'(?i)^rets\b',s): out.append('    return 1;'); saw_ret=True; continue
        # Calls, actor/world memory, B regs, shifts, indirect pointers, etc.
        return None
    if not saw_ret or (not saw_branch and not has_rng_call): return None
    out.append('    return 0;')
    return out

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
        if ops is not None:
            translated.append((label,'linear',ops)); continue
        cfg=translate_local_cfg(lines)
        if cfg is not None:
            translated.append((label,'cfg',cfg))
    q=[]
    q += ['#ifndef WM_ARCADE_DRONE_SOURCE_BODIES_GENERATED_H','#define WM_ARCADE_DRONE_SOURCE_BODIES_GENERATED_H',
          f'#define WM_FIX39_DRONE_TRANSLATED_BODY_COUNT {len(translated)}']
    for label,kind,body in translated:
        fn='wm_fix39_body_'+cident(label)
        q.append(f'static int {fn}(wm_arcade_actor_t *self, wm_arcade_actor_t *opp, wm_arcade_drone_state_t *drone, void *user){{')
        q.append('    (void)self; (void)opp; (void)user; if(!drone) return 0;')
        if kind=='linear':
            for field,val in body:
                cast='uint16_t' if field in ('but','joy','but_charge') else 'int32_t'
                q.append(f'    drone->{field}=({cast})({cexpr(val)});')
            q.append('    return 1;')
        else:
            q.extend(body)
        q.append('}')
    q.append('static const WmFix39DroneGeneratedBody wm_fix39_generated_bodies[WM_FIX39_DRONE_TRANSLATED_BODY_COUNT > 0 ? WM_FIX39_DRONE_TRANSLATED_BODY_COUNT : 1] = {')
    if translated:
        for label,_,_ in translated: q.append(f'    {{"{label}", wm_fix39_body_{cident(label)}}},')
    else:q.append('    {0,0},')
    q += ['};','#endif']
    out.write_text('\n'.join(q)+'\n',encoding='utf-8')
    return len(translated)

def self_test():
    with tempfile.TemporaryDirectory() as td:
        d=Path(td); r=d/'r'; o=d/'o.h'
        r.write_text(
            'body=foo@EXGPC_0000|x|addr=0|status=source-lines|lines=3\n'
            '  movk 7,a0\n  move a0,*a13(DRN_DELAY)\n  rets\n'
            'body=baz|x|addr=1|status=source-lines|lines=5\n'
            '  move *a13(DRN_BUT),a0\n  ori 4,a0\n  move a0,*a13(DRN_BUT)\n  rets\n'
            'body=branchy|x|addr=2|status=source-lines|lines=8\n'
            '  move *a13(DRN_DELAY),a0\n  cmpi 0,a0\n  jreq #zero\n'
            '  subk 1,a0\n  move a0,*a13(DRN_DELAY)\n  rets\n  #zero\n  clr a0\n  move a0,*a13(DRN_BUT)\n  rets\n'
            'body=bar|x|addr=3|status=source-lines|lines=4\n  movk 7,a0\n  callr rnd\n  move a0,*a13(DRN_DELAY)\n  rets\n')
        n=emit(r,o); t=o.read_text(); assert n==4 and 'drone->delay' in t and 'foo@EXGPC_0000' in t and '"bar"' in t
        assert 'wm_fix39_drone_body_rnd(user,(uint32_t)a[0])' in t
        assert 'drone->but=(uint16_t)((drone->but | 4));' in t and '"baz"' in t
        assert 'if (f_l==f_r) goto L_zero;' in t and 'L_zero: ;' in t and '"branchy"' in t
    print('Fix39 DRONE local-branch direct-body translator self-test: PASS')

def main():
    a=argparse.ArgumentParser();a.add_argument('--report');a.add_argument('--out');a.add_argument('--self-test',action='store_true');x=a.parse_args()
    if x.self_test:return self_test()
    if not x.report or not x.out: raise SystemExit('--report --out required')
    n=emit(Path(x.report),Path(x.out)); print(f'Generated {n} conservative direct DRONE body translation(s) -> {x.out}')
if __name__=='__main__':main()
