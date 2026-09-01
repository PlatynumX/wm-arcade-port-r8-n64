#!/usr/bin/env python3
from __future__ import annotations
import argparse, pathlib, re
from collections import OrderedDict, defaultdict

CHARS={0:('bret','hrt','H'),1:('razor','rzr','R'),2:('taker','und','U'),3:('yoko','yok','Y'),4:('shawn','shn','S'),5:('bam','bam','B'),6:('doink','dnk','D'),8:('lex','lex','L')}
SEQ_PREFIXES=('HRT','RZR','UND','YOK','SHN','BAM','DNK','LEX')
SUBR_RE=re.compile(r'^\s*SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b',re.I)
ANI_RE=re.compile(r'\bANI_([A-Za-z0-9_]+)\b',re.I)
FRAME_RE=re.compile(r'\b([A-Za-z][A-Za-z0-9_]*)\s*\+\s*FR(\d+)\b',re.I)

def canonical_frame_token(text):
    # R37N7: use the same physical WIMP-frame key for frame operands embedded
    # in ANI command arguments and source-data tables as for ordinary frame
    # instructions.  Do not rewrite arbitrary symbols/expressions.
    s=text.strip()
    m=FRAME_RE.fullmatch(s)
    if m:
        return f'{m.group(1).upper()}{int(m.group(2)):02d}'
    return s

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

def _r37n17_local_labels(lines):
    out=set()
    for raw in lines:
        code=raw.split(';',1)[0].strip()
        m=LOCAL_RE.match(code)
        if m: out.add(m.group(1).lower())
    return out

def _r37n17_branch_targets(lines):
    # Return #local targets used by animation branch opcodes in this source span.
    out=set()
    for raw in lines:
        code=raw.split(';',1)[0].strip()
        if not code or not DATA_LINE_RE.match(code): continue
        am=ANI_RE.search(code)
        if not am: continue
        bi=BRANCH_ARG.get(am.group(1).upper())
        if bi is None: continue
        args=split_args(code,am.end())
        if bi>=len(args): continue
        t=args[bi].strip()
        if t.startswith('#'):
            name=t[1:].strip()
            if re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*',name):
                out.add(name.lower())
    return out

def _r37n18_falls_through(lines):
    # True when Midway animation bytecode naturally continues past this SUBR label.
    # SUBR is an entry label, not an ANI terminator.
    terminal={'END','REPEAT','GOTO','CHANGEANIM','CHANGEANIM_TBL'}
    last=None
    for raw in lines:
        code=raw.split(';',1)[0].strip()
        if not code:
            continue
        if LOCAL_RE.match(code):
            continue
        am=ANI_RE.search(code) if DATA_LINE_RE.match(code) else None
        if am:
            last=('cmd',am.group(1).upper())
            continue
        fm=FRAME_RE.search(code)
        if fm:
            last=('frame',None)
    if last is None:
        return False
    if last[0]=='frame':
        return True
    return last[1] not in terminal

def collect_subrs(root):
    out=[]
    for p in source_files(root):
        ls=p.read_text(errors='replace').splitlines();starts=[]
        for i,r in enumerate(ls):
            m=SUBR_RE.match(r.split(';',1)[0])
            if m:starts.append((i,m.group(1)))
        for j,(st,lab) in enumerate(starts):
            # Midway frequently gives one program several entry labels by
            # stacking SUBR directives. Preserve the existing alias rule.
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

            body_start=starts[k][0]+1
            end_subr=k+1

            # R37N17 / Midway source layout:
            # SUBR is an entry label, not a hard end-of-program boundary.
            # Alternate entries routinely branch into #labels that live in the
            # immediately following SUBR body. Extend only when an unresolved
            # animation branch target is actually defined in that next body;
            # repeat for chained shared tails.
            while end_subr < len(starts):
                body_end=starts[end_subr][0]
                body=ls[body_start:body_end]
                missing=_r37n17_branch_targets(body)-_r37n17_local_labels(body)
                falls_through=_r37n18_falls_through(body)

                next_end=starts[end_subr+1][0] if end_subr+1<len(starts) else len(ls)
                next_body=ls[starts[end_subr][0]+1:next_end]
                next_labels=_r37n17_local_labels(next_body)

                # Preserve either explicit shared-tail branches (R37N17) or
                # literal source-bytecode fallthrough across the next SUBR
                # entry label (R37N18). SUBR itself emits no ANI terminator.
                if not falls_through and not (missing & next_labels): break
                end_subr+=1

            en=starts[end_subr][0] if end_subr<len(starts) else len(ls)
            out.append((p,lab,ls[body_start:en]))
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
                    t=tok.strip();v=ev(t.lstrip('#'));t=canonical_frame_token(t)
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
                    text=arg.strip();v=ev(text.lstrip('#'));text=canonical_frame_token(text)
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

def _put_u16(b,v): b += int(v & 0xffff).to_bytes(2,'little',signed=False)
def _put_i16(b,v): b += int(v).to_bytes(2,'little',signed=True)
def _put_i32(b,v): b += int(v).to_bytes(4,'little',signed=True)
def _put_str(b,s):
    q=(s or '').encode('utf-8')
    if len(q)>65535: raise ValueError('string too long')
    _put_u16(b,len(q)); b += q

def _write_program_blob(path,ins):
    b=bytearray(b'AVM1'); _put_u16(b,len(ins))
    for kind,op,name,args,tk in ins:
        b.append(1 if kind=='frame' else 0); _put_i16(b,op); b.append(min(len(args),9)); _put_u16(b,tk); _put_str(b,name)
        for v,text,kindv in args[:9]:
            _put_i32(b,v); b.append(kindv & 0xff); _put_str(b,text)
    path.parent.mkdir(parents=True,exist_ok=True); path.write_bytes(b)

def _write_table_blob(path,entries):
    b=bytearray(b'AVT1'); _put_u16(b,len(entries))
    for v,text,res in entries:
        _put_i32(b,v); b.append(1 if res else 0); _put_str(b,text)
    path.parent.mkdir(parents=True,exist_ok=True); path.write_bytes(b)

def emit(root,out_c,out_h,out_fs):
    progs,tables,used,unresolved,total=parse_program(root)
    out_fs.mkdir(parents=True,exist_ok=True)
    import shutil
    for d in (out_fs/'programs',out_fs/'tables'):
        if d.exists(): shutil.rmtree(d)
        d.mkdir(parents=True,exist_ok=True)
    h='''#ifndef WM_ARCADE_SOURCE_ANIMATION_PROGRAM_H
#define WM_ARCADE_SOURCE_ANIMATION_PROGRAM_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { WM_SRC_ARG_TEXT=0, WM_SRC_ARG_NUM=1, WM_SRC_ARG_LOCAL_PC=2, WM_SRC_ARG_TABLE=3 };
typedef struct { int32_t value; const char *text; uint8_t kind; } wm_source_anim_arg_t;
typedef enum { WM_SRC_INS_CMD=0, WM_SRC_INS_FRAME=1 } wm_source_anim_ins_kind_t;
typedef struct { uint8_t kind; int16_t opcode; uint8_t argc; uint16_t ticks; const char *name; wm_source_anim_arg_t a[9]; } wm_source_anim_ins_t;
typedef struct { uint8_t roster_id; const char *label; const char *source_file; const wm_source_anim_ins_t *ins; uint16_t count; } wm_source_anim_program_t;
typedef struct { const char *source_file; const char *label; const wm_source_anim_arg_t *entries; uint16_t count; } wm_source_anim_table_t;
const wm_source_anim_program_t *wm_source_anim_program_find(uint8_t roster_id,const char *label);
const wm_source_anim_table_t *wm_source_anim_table_by_id(uint16_t id);
const wm_source_anim_table_t *wm_source_anim_table_find(const char *source_file,const char *label);
void wm_source_anim_program_cache_reset(void);
size_t wm_source_anim_program_count(void);
size_t wm_source_anim_table_count(void);
#ifdef __cplusplus
}
#endif
#endif
'''
    out_h.parent.mkdir(parents=True,exist_ok=True);out_h.write_text(h)
    c=['/* Generated directly from canonical Midway wrestler ANIM scripts. */',
       '/* Combat2CG: bulky VM instructions/tables live in DragonFS, not resident ELF RAM. */',
       '#include "wm_arcade_source_animation_program.h"','#include <stdio.h>','#include <stdlib.h>','#include <string.h>','',
       'typedef struct { uint8_t roster; const char *label,*source,*path; uint16_t count; uint32_t bytes; } vm_prog_meta_t;',
       'typedef struct { const char *source,*label,*path; uint16_t count; uint32_t bytes; } vm_tab_meta_t;']
    pmeta=[]
    for pi,(rid,label,src,ins) in enumerate(progs):
        rel=f'fix39_anim/programs/p{pi:04d}.bin'; blob=out_fs/'programs'/f'p{pi:04d}.bin'; _write_program_blob(blob,ins)
        pmeta.append(f'{{{rid}u,{cstr(label)},{cstr(src)},{cstr(rel)},{len(ins)}u,{blob.stat().st_size}u}}')
    tmeta=[]
    for ti,(src,label,entries) in enumerate(tables):
        rel=f'fix39_anim/tables/t{ti:04d}.bin'; blob=out_fs/'tables'/f't{ti:04d}.bin'; _write_table_blob(blob,entries)
        tmeta.append(f'{{{cstr(src)},{cstr(label.lower())},{cstr(rel)},{len(entries)}u,{blob.stat().st_size}u}}')
    c.append('static const vm_prog_meta_t prog_meta[]={' + ','.join(pmeta) + '};')
    c.append('static const vm_tab_meta_t tab_meta[]={' + ','.join(tmeta) + '};')
    c.append('static wm_source_anim_program_t *prog_loaded[sizeof(prog_meta)/sizeof(prog_meta[0])];')
    c.append('static wm_source_anim_table_t *tab_loaded[sizeof(tab_meta)/sizeof(tab_meta[0])];')
    c.append(r'''
typedef struct { const unsigned char *p,*e; } vm_rd_t;
static int rd_u8(vm_rd_t*r,uint8_t*v){if(!r||r->p>=r->e)return 0;*v=*r->p++;return 1;}
static int rd_u16(vm_rd_t*r,uint16_t*v){if(!r||r->e-r->p<2)return 0;*v=(uint16_t)(r->p[0]|((uint16_t)r->p[1]<<8));r->p+=2;return 1;}
static int rd_i16(vm_rd_t*r,int16_t*v){uint16_t q;if(!rd_u16(r,&q))return 0;*v=(int16_t)q;return 1;}
static int rd_i32(vm_rd_t*r,int32_t*v){uint32_t q;if(!r||r->e-r->p<4)return 0;q=(uint32_t)r->p[0]|((uint32_t)r->p[1]<<8)|((uint32_t)r->p[2]<<16)|((uint32_t)r->p[3]<<24);r->p+=4;*v=(int32_t)q;return 1;}
static char*rd_str(vm_rd_t*r){uint16_t n;char*s;if(!rd_u16(r,&n)||r->e-r->p<n)return 0;s=(char*)malloc((size_t)n+1u);if(!s)return 0;memcpy(s,r->p,n);s[n]=0;r->p+=n;return s;}
static unsigned char*load_blob(const char*rel,size_t expected,size_t*sz){FILE*f;unsigned char*b;char path[128];int m;if(!rel||!sz||!expected)return 0;m=snprintf(path,sizeof(path),"rom:/%s",rel);if(m<=0||(size_t)m>=sizeof(path))return 0;f=fopen(path,"rb");if(!f){m=snprintf(path,sizeof(path),"filesystem/%s",rel);if(m<=0||(size_t)m>=sizeof(path))return 0;f=fopen(path,"rb");}if(!f)return 0;b=(unsigned char*)malloc(expected);if(!b){fclose(f);return 0;}if(fread(b,1,expected,f)!=expected){free(b);fclose(f);return 0;}fclose(f);*sz=expected;return b;}
static void free_program(wm_source_anim_program_t*p){uint16_t i;unsigned j;if(!p)return;if(p->ins){wm_source_anim_ins_t*q=(wm_source_anim_ins_t*)(uintptr_t)p->ins;for(i=0;i<p->count;i++){free((void*)q[i].name);for(j=0;j<q[i].argc&&j<9;j++)free((void*)q[i].a[j].text);}free(q);}free(p);}
static void free_table(wm_source_anim_table_t*t){uint16_t i;if(!t)return;if(t->entries){wm_source_anim_arg_t*q=(wm_source_anim_arg_t*)(uintptr_t)t->entries;for(i=0;i<t->count;i++)free((void*)q[i].text);free(q);}free(t);}
void wm_source_anim_program_cache_reset(void){size_t i;for(i=0;i<sizeof(prog_loaded)/sizeof(prog_loaded[0]);i++){free_program(prog_loaded[i]);prog_loaded[i]=0;}for(i=0;i<sizeof(tab_loaded)/sizeof(tab_loaded[0]);i++){free_table(tab_loaded[i]);tab_loaded[i]=0;}}
static wm_source_anim_program_t*load_program(size_t id){const vm_prog_meta_t*m;wm_source_anim_program_t*p=0;wm_source_anim_ins_t*ins=0;unsigned char*b=0;size_t z=0;vm_rd_t r;uint16_t n,i;uint8_t k,argc;int16_t op;if(id>=sizeof(prog_meta)/sizeof(prog_meta[0]))return 0;if(prog_loaded[id])return prog_loaded[id];m=&prog_meta[id];b=load_blob(m->path,m->bytes,&z);if(!b||z<6||memcmp(b,"AVM1",4)){free(b);return 0;}r.p=b+4;r.e=b+z;if(!rd_u16(&r,&n)||n!=m->count){free(b);return 0;}p=(wm_source_anim_program_t*)calloc(1,sizeof(*p));ins=(wm_source_anim_ins_t*)calloc(n?n:1,sizeof(*ins));if(!p||!ins){free(p);free(ins);free(b);return 0;}p->roster_id=m->roster;p->label=m->label;p->source_file=m->source;p->ins=ins;p->count=n;for(i=0;i<n;i++){uint16_t ticks;unsigned j;if(!rd_u8(&r,&k)||!rd_i16(&r,&op)||!rd_u8(&r,&argc)||!rd_u16(&r,&ticks)||argc>9){free_program(p);free(b);return 0;}ins[i].kind=k;ins[i].opcode=op;ins[i].argc=argc;ins[i].ticks=ticks;ins[i].name=rd_str(&r);if(!ins[i].name){free_program(p);free(b);return 0;}for(j=0;j<argc;j++){uint8_t ak;if(!rd_i32(&r,&ins[i].a[j].value)||!rd_u8(&r,&ak)){free_program(p);free(b);return 0;}ins[i].a[j].kind=ak;ins[i].a[j].text=rd_str(&r);if(!ins[i].a[j].text){free_program(p);free(b);return 0;}}}free(b);prog_loaded[id]=p;return p;}
static wm_source_anim_table_t*load_table(size_t id){const vm_tab_meta_t*m;wm_source_anim_table_t*t=0;wm_source_anim_arg_t*e=0;unsigned char*b=0;size_t z=0;vm_rd_t r;uint16_t n,i;if(id>=sizeof(tab_meta)/sizeof(tab_meta[0]))return 0;if(tab_loaded[id])return tab_loaded[id];m=&tab_meta[id];b=load_blob(m->path,m->bytes,&z);if(!b||z<6||memcmp(b,"AVT1",4)){free(b);return 0;}r.p=b+4;r.e=b+z;if(!rd_u16(&r,&n)||n!=m->count){free(b);return 0;}t=(wm_source_anim_table_t*)calloc(1,sizeof(*t));e=(wm_source_anim_arg_t*)calloc(n?n:1,sizeof(*e));if(!t||!e){free(t);free(e);free(b);return 0;}t->source_file=m->source;t->label=m->label;t->entries=e;t->count=n;for(i=0;i<n;i++){uint8_t k;if(!rd_i32(&r,&e[i].value)||!rd_u8(&r,&k)){free_table(t);free(b);return 0;}e[i].kind=k;e[i].text=rd_str(&r);if(!e[i].text){free_table(t);free(b);return 0;}}free(b);tab_loaded[id]=t;return t;}
const wm_source_anim_program_t *wm_source_anim_program_find(uint8_t roster,const char*label){size_t i;if(!label)return 0;for(i=0;i<sizeof(prog_meta)/sizeof(prog_meta[0]);i++)if((prog_meta[i].roster==roster||prog_meta[i].roster==255u)&&!strcmp(prog_meta[i].label,label))return load_program(i);return 0;}
const wm_source_anim_table_t *wm_source_anim_table_by_id(uint16_t id){return load_table(id);}
const wm_source_anim_table_t *wm_source_anim_table_find(const char*source,const char*label){size_t i;if(!label)return 0;for(i=0;i<sizeof(tab_meta)/sizeof(tab_meta[0]);i++)if(!strcmp(tab_meta[i].label,label)&&(!source||!strcmp(tab_meta[i].source,source)))return load_table(i);if(source)for(i=0;i<sizeof(tab_meta)/sizeof(tab_meta[0]);i++)if(!strcmp(tab_meta[i].label,label))return load_table(i);return 0;}
size_t wm_source_anim_program_count(void){return sizeof(prog_meta)/sizeof(prog_meta[0]);}
size_t wm_source_anim_table_count(void){return sizeof(tab_meta)/sizeof(tab_meta[0]);}
''')
    out_c.parent.mkdir(parents=True,exist_ok=True);out_c.write_text('\n'.join(c)+'\n')
    payload=sum(p.stat().st_size for p in out_fs.rglob('*.bin'))
    print(f'generated streamed ANIM VM: {len(progs)} programs, {total} instructions, {len(used)} ANI command names, {len(tables)} tables, {payload} DFS bytes')
    if unresolved:raise SystemExit('unresolved ANI opcode equates: '+','.join(sorted(unresolved)))

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--root',type=pathlib.Path,required=True);ap.add_argument('--out-c',type=pathlib.Path,required=True);ap.add_argument('--out-h',type=pathlib.Path,required=True);ap.add_argument('--out-fs',type=pathlib.Path,default=pathlib.Path('filesystem/fix39_anim'));a=ap.parse_args();emit(a.root,a.out_c,a.out_h,a.out_fs)
if __name__=='__main__':main()
