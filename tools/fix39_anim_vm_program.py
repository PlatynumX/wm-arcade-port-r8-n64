#!/usr/bin/env python3
from __future__ import annotations
import argparse, pathlib, re
from collections import OrderedDict, defaultdict

CHARS={0:('bret','hrt','H'),1:('razor','rzr','R'),2:('taker','und','U'),3:('yoko','yok','Y'),4:('shawn','shn','S'),5:('bam','bam','B'),6:('doink','dnk','D'),8:('lex','lex','L')}
SEQ_PREFIXES=('HRT','RZR','UND','YOK','SHN','BAM','DNK','LEX')
SUBR_RE=re.compile(r'^\s*SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b',re.I)
ANI_RE=re.compile(r'\bANI_([A-Za-z0-9_]+)\b',re.I)
FRAME_RE=re.compile(r'\b([A-Za-z][A-Za-z0-9_]*)\s*\+\s*FR(\d+)\b',re.I)
LOCAL_RE=re.compile(r'^\s*#([A-Za-z_][A-Za-z0-9_]*)\s*$')
PLAIN_LABEL_RE=re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*)\s*$')
DATA_LINE_RE=re.compile(r'^(?:\.word|\.long|REFLONG|[WL]{1,9})\b',re.I)
DATA_DIRECTIVE_RE=re.compile(r'^\s*(?:\.word|\.long|REFLONG)\s+(.+)$',re.I)

def canonical(p:pathlib.Path)->bool:
    n=p.name.lower(); return "'" not in p.name and '~' not in p.name and not n.endswith(('.bak.asm','.old.asm','.orig.asm','.tmp.asm'))

def relevant_seq(p:pathlib.Path)->bool:
    return p.name.upper().startswith(SEQ_PREFIXES)

def parse_equates(root:pathlib.Path):
    raw={}
    for p in list(root.glob('*.EQU'))+list(root.glob('*.equ'))+list(root.glob('*.H'))+list(root.glob('*.h')):
        if not canonical(p): continue
        for rawline in p.read_text(errors='replace').splitlines():
            line=rawline.split(';',1)[0].strip()
            m=re.match(r'^([A-Za-z_][A-Za-z0-9_]*)\s+(?:equ|EQU)\s+(.+)$',line)
            if m: raw[m.group(1).upper()]=m.group(2).strip()
    vals={}
    allowed=re.compile(r'^[0-9xXa-fA-F\s\+\-\*\/\(\)\|&<>~]+$')
    def ev(expr):
        x=expr.strip()
        # Midway pair syntax [hi,lo] is a packed 32-bit literal.
        x=re.sub(r'\[\s*([^,\]]+)\s*,\s*([^\]]+)\s*\]',r'((\1)<<16)|(\2)',x)
        x=re.sub(r'\b([0-9A-Fa-f]+)[hH]\b',lambda m:'0x'+m.group(1),x)
        for _ in range(16):
            old=x
            x=re.sub(r'\b([A-Za-z_][A-Za-z0-9_]*)\b',lambda m:str(vals.get(m.group(1).upper(),m.group(0))),x)
            if x==old: break
        if not allowed.fullmatch(x): return None
        try:
            v=eval(x,{'__builtins__':None},{})
            return int(v) if isinstance(v,int) else None
        except Exception:return None
    for _ in range(100):
        added=0
        for k,v in raw.items():
            if k in vals: continue
            z=ev(v)
            if z is not None: vals[k]=z;added+=1
        if not added:break
    return vals,ev

def split_args(code, end):
    rest=code[end:].lstrip(' ,\t')
    if not rest:return []
    out=[];cur='';depth=0
    for ch in rest:
        if ch in '([':depth+=1
        elif ch in ')]' and depth:depth-=1
        if ch==',' and depth==0:
            if cur.strip():out.append(cur.strip())
            cur=''
        else:cur+=ch
    if cur.strip():out.append(cur.strip())
    return out

def ticks_from_prefix(code, frame_start, ev):
    pre=code[:frame_start].rstrip().rstrip(',')
    tok=pre.rsplit(',',1)[-1].strip()
    tok=re.sub(r'^\s*[WL]+\s+','',tok,flags=re.I).strip()
    v=ev(tok)
    if v is None:
        m=re.search(r'(-?\d+)\s*$',tok);v=int(m.group(1)) if m else 1
    return max(1,int(v))

def owner_for_label(label):
    l=label.lower()
    for rid,(_,pfx,_) in CHARS.items():
        if l.startswith(pfx+'_'):return rid
    return 255

def frame_owner_ok(rid,base):
    if rid==255:return True
    return base[:1].upper()==CHARS[rid][2]

def source_files(root):
    # Match the historical animation corpus used by the visual catalog, plus
    # the common WRESTLE/REACT animation programs that are invoked by those
    # character scripts.  FINISEQ and TAKER contain real wrestler programs and
    # must not be omitted just because their filenames do not match *SEQ*.
    seen=set();out=[]
    for p in sorted(root.glob('*SEQ*.ASM'),key=lambda q:q.name.lower()):
        if (relevant_seq(p) or p.name.upper()=='FINISEQ.ASM') and canonical(p):
            seen.add(p);out.append(p)
    for pat in ('TAKER.ASM','WRESTLE.ASM','WRESTLE2.ASM','REACT*.ASM'):
        for p in sorted(root.glob(pat),key=lambda q:q.name.lower()):
            if p not in seen and canonical(p):seen.add(p);out.append(p)
    return out

def collect_subrs(root):
    out=[]
    for p in source_files(root):
        ls=p.read_text(errors='replace').splitlines();starts=[]
        for i,r in enumerate(ls):
            m=SUBR_RE.match(r.split(';',1)[0])
            if m:starts.append((i,m.group(1)))
        for j,(st,lab) in enumerate(starts):
            # Midway frequently gives one program several entry labels by
            # stacking SUBR directives (eg hrt_stand2_anim/hrt_stand8_anim).
            # An alias has no executable/data text before the next SUBR and
            # therefore owns the same body as the final alias in that run.
            k=j
            while k+1<len(starts):
                a=starts[k][0]+1;b=starts[k+1][0]
                meaningful=False
                for raw in ls[a:b]:
                    code=raw.split(';',1)[0].strip()
                    if code and not code.startswith('*'):
                        meaningful=True;break
                if meaningful:break
                k+=1
            en=starts[k+1][0] if k+1<len(starts) else len(ls)
            out.append((p,lab,ls[starts[k][0]+1:en]))
    return out

def collect_tables(root,ev):
    tables=[]
    by_file=defaultdict(dict)
    for p in source_files(root):
        ls=p.read_text(errors='replace').splitlines()
        i=0
        while i<len(ls):
            code=ls[i].split(';',1)[0].strip()
            lm=LOCAL_RE.match(code) or PLAIN_LABEL_RE.match(code)
            if not lm or SUBR_RE.match(code):i+=1;continue
            label=lm.group(1);j=i+1;entries=[];saw=False
            while j<len(ls):
                raw=ls[j].split(';',1)[0].strip()
                if not raw:
                    if saw: break
                    j+=1;continue
                dm=DATA_DIRECTIVE_RE.match(raw)
                if not dm:break
                saw=True
                for tok in split_args(dm.group(1),0):
                    t=tok.strip();v=ev(t.lstrip('#'))
                    entries.append((0 if v is None else v,t,v is not None))
                j+=1
            if saw:
                key=(p.name.lower(),label.lower())
                if label.lower() not in by_file[p.name.lower()]:
                    by_file[p.name.lower()][label.lower()]=len(tables)
                    tables.append((p.name,label,entries))
                i=j;continue
            i+=1
    return tables,by_file

# commands whose last/known arg is a local animation-PC branch label, not a callable/source symbol.
BRANCH_ARG={
 'GOTO':0,'IFSTATUS':0,'IFNOTSTATUS':0,'IFOPPMODE':1,'IFBUTTONS':1,'IFNOHITBLOCK':0,
 'IFROPE':2,'IFNOTROPE':2,'IFBLOCKED':0,'IF_BUTCOUNT_GE':2,'IF_BUTCOUNT_LT':2,
 'IF_RPTCOUNT':0,'IFNOT_RPTCOUNT':0,'RINGCHECK':0,'IF_RPTCOUNT_GE':1,'IF_RPTCOUNT_LT':1,
 'SLIDE_BACK':2,'RNDPER':1,
}
TABLE_ARG={'SUPERSLAVE':0,'SLAVEANIM':0,'SUPERSLAVE2':2,'XFLIP_TBL':0,'CHANGEANIM_TBL':0,'OPPOFFSET':0}

def parse_program(root):
    vals,ev=parse_equates(root)
    opvals={k[4:]:v-0x8000 for k,v in vals.items() if k.startswith('ANI_') and isinstance(v,int) and 0x8000<=v<0x9000}
    tables,table_by_file=collect_tables(root,ev)
    programs=[];used=set();unresolved=set();total=0
    for p,label,lines in collect_subrs(root):
        rid=owner_for_label(label);ins=[];locals_pc={};saw=False
        for raw in lines:
            code=raw.split(';',1)[0].strip()
            if not code:continue
            lm=LOCAL_RE.match(code)
            if lm:
                locals_pc[lm.group(1).lower()]=len(ins);continue
            am=ANI_RE.search(code) if DATA_LINE_RE.match(code) else None
            if am:
                name=am.group(1).upper();op=opvals.get(name,-1);used.add(name);saw=True
                if op<0:unresolved.add(name)
                args=[]
                for arg in split_args(code,am.end()):
                    text=arg.strip();v=ev(text.lstrip('#'))
                    args.append([0 if v is None else v,text,1 if v is not None else 0]) # kind 1 numeric, 0 string
                ins.append(['cmd',op,name,args,0]);continue
            fm=FRAME_RE.search(code)
            if fm and frame_owner_ok(rid,fm.group(1)):
                saw=True;frame=f'{fm.group(1).upper()}{int(fm.group(2)):02d}'
                ins.append(['frame',0,frame,[],ticks_from_prefix(code,fm.start(),ev)])
        if not saw:continue
        # Resolve branch and source-data table operands.
        for rec in ins:
            if rec[0]!='cmd':continue
            name,args=rec[2],rec[3]
            bi=BRANCH_ARG.get(name)
            if bi is not None and bi<len(args):
                key=args[bi][1].lstrip('#').lower()
                if key in locals_pc:args[bi]=[locals_pc[key],args[bi][1],2] # local PC
            ti=TABLE_ARG.get(name)
            if ti is not None and ti<len(args):
                key=args[ti][1].lstrip('#').lower();tid=table_by_file[p.name.lower()].get(key)
                if tid is not None:args[ti]=[tid,args[ti][1],3]
        programs.append((rid,label.lower(),p.name,ins));total+=len(ins)
    return programs,tables,used,unresolved,total

def cstr(s):return '"'+s.replace('\\','\\\\').replace('"','\\"')+'"'

def emit(root,out_c,out_h):
    progs,tables,used,unresolved,total=parse_program(root)
    h='''#ifndef WM_ARCADE_SOURCE_ANIMATION_PROGRAM_H\n#define WM_ARCADE_SOURCE_ANIMATION_PROGRAM_H\n#include <stddef.h>\n#include <stdint.h>\n#ifdef __cplusplus\nextern "C" {\n#endif\nenum { WM_SRC_ARG_TEXT=0, WM_SRC_ARG_NUM=1, WM_SRC_ARG_LOCAL_PC=2, WM_SRC_ARG_TABLE=3 };\ntypedef struct { int32_t value; const char *text; uint8_t kind; } wm_source_anim_arg_t;\ntypedef enum { WM_SRC_INS_CMD=0, WM_SRC_INS_FRAME=1 } wm_source_anim_ins_kind_t;\ntypedef struct { uint8_t kind; int16_t opcode; uint8_t argc; uint16_t ticks; const char *name; wm_source_anim_arg_t a[9]; } wm_source_anim_ins_t;\ntypedef struct { uint8_t roster_id; const char *label; const char *source_file; const wm_source_anim_ins_t *ins; uint16_t count; } wm_source_anim_program_t;\ntypedef struct { const char *source_file; const char *label; const wm_source_anim_arg_t *entries; uint16_t count; } wm_source_anim_table_t;\nconst wm_source_anim_program_t *wm_source_anim_program_find(uint8_t roster_id,const char *label);\nconst wm_source_anim_table_t *wm_source_anim_table_by_id(uint16_t id);\nconst wm_source_anim_table_t *wm_source_anim_table_find(const char *source_file,const char *label);\nsize_t wm_source_anim_program_count(void);\nsize_t wm_source_anim_table_count(void);\n#ifdef __cplusplus\n}\n#endif\n#endif\n'''
    out_h.parent.mkdir(parents=True,exist_ok=True);out_h.write_text(h)
    c=['/* Generated directly from canonical Midway wrestler ANIM scripts. */','#include "wm_arcade_source_animation_program.h"','#include <string.h>','']
    tnames=[]
    for ti,(src,label,entries) in enumerate(tables):
        an=f'vm_tab_entries_{ti}';tn=f'vm_tab_{ti}';tnames.append(tn)
        c.append(f'static const wm_source_anim_arg_t {an}[]={{')
        for v,text,res in entries:c.append('{%d,%s,%du},'%(v,cstr(text),1 if res else 0))
        c.append('};');c.append(f'static const wm_source_anim_table_t {tn}={{{cstr(src)},{cstr(label.lower())},{an},(uint16_t)(sizeof({an})/sizeof({an}[0]))}};')
    pnames=[]
    for pi,(rid,label,src,ins) in enumerate(progs):
        arr=f'vm_ins_{pi}';pn=f'vm_prog_{pi}';pnames.append(pn);c.append(f'static const wm_source_anim_ins_t {arr}[]={{')
        for kind,op,name,args,tk in ins:
            if kind=='frame':c.append('{WM_SRC_INS_FRAME,0,0,%du,%s,{{0}}},'%(tk,cstr(name)));continue
            aa=[]
            for v,text,kindv in args[:9]:aa.append('{%d,%s,%du}'%(v,cstr(text),kindv))
            while len(aa)<9:aa.append('{0,0,0}')
            c.append('{WM_SRC_INS_CMD,%d,%du,0,%s,{%s}},'%(op,min(len(args),9),cstr(name),','.join(aa)))
        c.append('};');c.append(f'static const wm_source_anim_program_t {pn}={{{rid}u,{cstr(label)},{cstr(src)},{arr},(uint16_t)(sizeof({arr})/sizeof({arr}[0]))}};')
    c.append('static const wm_source_anim_program_t *const all_programs[]={'+','.join('&'+x for x in pnames)+'};')
    c.append('static const wm_source_anim_table_t *const all_tables[]={'+','.join('&'+x for x in tnames)+'};')
    c.append('const wm_source_anim_program_t *wm_source_anim_program_find(uint8_t r,const char*l){size_t i;if(!l)return 0;for(i=0;i<sizeof(all_programs)/sizeof(all_programs[0]);++i){const wm_source_anim_program_t*p=all_programs[i];if((p->roster_id==r||p->roster_id==255u)&&!strcmp(p->label,l))return p;}return 0;}')
    c.append('const wm_source_anim_table_t *wm_source_anim_table_by_id(uint16_t id){return id<(uint16_t)(sizeof(all_tables)/sizeof(all_tables[0]))?all_tables[id]:0;}')
    c.append('const wm_source_anim_table_t *wm_source_anim_table_find(const char*f,const char*l){size_t i;if(!l)return 0;for(i=0;i<sizeof(all_tables)/sizeof(all_tables[0]);++i){const wm_source_anim_table_t*t=all_tables[i];if(!strcmp(t->label,l)&&(!f||!strcmp(t->source_file,f)))return t;}if(f)for(i=0;i<sizeof(all_tables)/sizeof(all_tables[0]);++i)if(!strcmp(all_tables[i]->label,l))return all_tables[i];return 0;}')
    c.append('size_t wm_source_anim_program_count(void){return sizeof(all_programs)/sizeof(all_programs[0]);}')
    c.append('size_t wm_source_anim_table_count(void){return sizeof(all_tables)/sizeof(all_tables[0]);}')
    out_c.parent.mkdir(parents=True,exist_ok=True);out_c.write_text('\n'.join(c)+'\n')
    print(f'generated full ANIM VM programs: {len(progs)} programs, {total} instructions, {len(used)} ANI command names, {len(tables)} source tables')
    if unresolved:raise SystemExit('unresolved ANI opcode equates: '+','.join(sorted(unresolved)))

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--root',type=pathlib.Path,required=True);ap.add_argument('--out-c',type=pathlib.Path,required=True);ap.add_argument('--out-h',type=pathlib.Path,required=True);a=ap.parse_args();emit(a.root,a.out_c,a.out_h)
if __name__=='__main__':main()
