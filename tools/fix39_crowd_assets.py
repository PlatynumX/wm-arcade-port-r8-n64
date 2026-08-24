#!/usr/bin/env python3
"""Translate CROWD.ASM + CROWD.IMG into N64 runtime data.

This is a structural translation of the Midway crowd animator. It preserves
all 30 CROWD_ANIMS entries and the FRAME/GOTO/RNDWAIT/REPEAT/SHOULD_REPEAT
commands used by CROWD.ASM. No replacement frames or hand-authored timing are
introduced.
"""
from __future__ import annotations
import argparse, pathlib, re, struct
from collections import Counter, defaultdict
IES=0x32; PES=0x1a; HM=28

def u16(d,o): return struct.unpack_from('<H',d,o)[0]
def s16(d,o): return struct.unpack_from('<h',d,o)[0]
def u32(d,o): return struct.unpack_from('<I',d,o)[0]
def nm8(b): return b.split(b'\0',1)[0].decode('ascii')
def rgba(v,i):
    if i==0:return 0
    return (((v>>10)&31)<<11)|(((v>>5)&31)<<6)|((v&31)<<1)|1

def parse_wimp(p:pathlib.Path):
    d=p.read_bytes(); n=u16(d,0); off=u32(d,4)
    if not n or off<HM or off+n*IES>len(d): raise ValueError('CROWD.IMG bad directory')
    ims=[]
    for i in range(n):
        q=off+i*IES; name=nm8(d[q:q+8]); w=u16(d,q+22); h=u16(d,q+24); pid=u16(d,q+26); do=u32(d,q+28)
        stride=(w+3)&~3
        if not w or not h or do+stride*h>len(d): raise ValueError(f'{name}: bad pixels')
        ims.append(dict(name=name,x=s16(d,q+18),y=s16(d,q+20),w=w,h=h,pid=pid,do=do,ord=i))
    first=min(x['do'] for x in ims); q=off+n*IES; pals=[]
    while q+PES<=len(d):
        name=nm8(d[q:q+8]); cnt=u16(d,q+12); po=u32(d,q+14)
        if not name or not 1<=cnt<=256 or po<HM or po+cnt*2>first: break
        pals.append(dict(name=name,cnt=cnt,do=po)); q+=PES
    if not pals: raise ValueError('CROWD.IMG has no palette directory')
    ids=sorted({x['pid'] for x in ims}); base=ids[0]
    for im in ims:
        pi=im['pid']-base
        if not 0<=pi<len(pals): raise ValueError(f'{im["name"]}: palette map')
        im['pal']=pals[pi]
    # Source CROWDIMG.GLO gives JASONCRD1..5 while WIMP stores five repeated
    # 8-byte JASONCRD names. Preserve file order for that source alias family.
    counts=Counter(x['name'] for x in ims); seen=defaultdict(int)
    for im in ims:
        k=seen[im['name']]; seen[im['name']]+=1
        if im['name']=='JASONCRD' and counts[im['name']]==5: im['sym']=f'JASONCRD{k+1}'
        elif counts[im['name']]==1: im['sym']=im['name']
        else: im['sym']=im['name'] if k==0 else f'{im["name"]}__{k}'
    return d,ims

def ci8(d,im):
    stride=(im['w']+3)&~3; out=bytearray()
    for y in range(im['h']): out += d[im['do']+y*stride:im['do']+y*stride+im['w']]
    return bytes(out)

def parse_num_expr(s, const):
    s=s.strip().replace('#','').upper()
    # CROWD.ASM expressions used by animation timings are symbol, symbol+N,
    # symbol-N, symbol*N or decimal/hex literal. Evaluate only that tiny grammar.
    m=re.fullmatch(r'([A-Z_][A-Z0-9_]*|[0-9A-F]+H|\d+)(?:\s*([+*\-])\s*(\d+))?',s)
    if not m: raise ValueError(f'unsupported CROWD timing expression {s!r}')
    a=m.group(1); v=const[a] if a in const else (int(a[:-1],16) if a.endswith('H') else int(a))
    if m.group(2):
        b=int(m.group(3)); v = v+b if m.group(2)=='+' else (v-b if m.group(2)=='-' else v*b)
    return v

def parse_crowd_asm(p:pathlib.Path):
    text=p.read_text(errors='ignore'); const={'SPD_FOREVER':0x7fff,'TSEC':53}
    for line in text.splitlines():
        m=re.match(r'\s*#?([A-Za-z_][A-Za-z0-9_]*)\s+equ\s+([^;]+)',line,re.I)
        if m:
            try: const[m.group(1).upper()]=parse_num_expr(m.group(2),const)
            except Exception: pass
    # Exact 30x3 source start table.
    m=re.search(r'(?is)^CROWD_ANIMS\s*$([\s\S]*?)^\*+\s*$',text,re.M)
    if not m: raise ValueError('CROWD_ANIMS table not found')
    starts=[]
    for line in m.group(1).splitlines():
        if '.long' not in line.lower(): continue
        vals=re.findall(r'#([A-Za-z0-9_]+)',line)
        if vals: starts.append(tuple(x.lower() for x in vals))
    if len(starts)!=30 or any(len(x)!=3 for x in starts): raise ValueError(f'expected 30 CROWD_ANIMS triples, got {len(starts)}')
    # Parse only labels that form animation scripts. Commands are kept symbolic;
    # GOTO destinations are resolved after every label has an index.
    labels={}; cur=None
    for raw in text.splitlines():
        line=raw.split(';',1)[0].strip()
        lm=re.match(r'^#([A-Za-z0-9_]+)\s*$',line)
        if lm:
            cur=lm.group(1).lower(); labels.setdefault(cur,[]); continue
        if cur is None: continue
        wm=re.match(r'(?i)^WL\s+([^,]+),\s*([^\s,]+)',line)
        if wm:
            a,b=wm.group(1).strip(),wm.group(2).strip()
            if a.upper().replace('#','')=='CANI_GOTO': labels[cur].append(('GOTO',b.lstrip('#').lower(),0))
            else: labels[cur].append(('FRAME',b,parse_num_expr(a,const)))
            continue
        rm=re.match(r'(?i)^\.word\s+CANI_RNDWAIT\s*,\s*(.+)$',line)
        if rm: labels[cur].append(('RNDWAIT','',parse_num_expr(rm.group(1),const))); continue
        rm=re.match(r'(?i)^\.word\s+CANI_REPEAT\s*,\s*(.+)$',line)
        if rm: labels[cur].append(('REPEAT','',parse_num_expr(rm.group(1),const))); continue
        if re.match(r'(?i)^\.word\s+CANI_SHOULD_REPEAT',line): labels[cur].append(('SHOULD_REPEAT','',0)); continue
    needed=set(x for tri in starts for x in tri); todo=list(needed)
    # Include GOTO targets recursively.
    while todo:
        x=todo.pop()
        if x not in labels: raise ValueError(f'crowd script label missing: {x}')
        for op,arg,_ in labels[x]:
            if op=='GOTO' and arg not in needed: needed.add(arg); todo.append(arg)
    scripts={k:labels[k] for k in labels if k in needed}
    frames=sorted({arg for cmds in scripts.values() for op,arg,_ in cmds if op=='FRAME'})
    return starts,scripts,frames

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--source-pack',type=pathlib.Path,required=True); ap.add_argument('--out-fs',type=pathlib.Path,required=True); ap.add_argument('--out-c',type=pathlib.Path,required=True); ap.add_argument('--out-h',type=pathlib.Path,required=True)
    ns=ap.parse_args(); d,ims=parse_wimp(ns.source_pack/'assets'/'CROWD.IMG'); starts,scripts,frames=parse_crowd_asm(ns.source_pack/'source'/'CROWD.ASM')
    by={x['sym'].upper():x for x in ims}; missing=[x for x in frames if x.upper() not in by]
    if missing: raise ValueError('CROWD.ASM physical frames absent from CROWD.IMG: '+', '.join(missing[:20]))
    ns.out_fs.mkdir(parents=True,exist_ok=True); records=[]
    for sym in frames:
        im=by[sym.upper()]; px=ci8(d,im); pal=im['pal']; words=[rgba(u16(d,pal['do']+i*2)&0x7fff,i) for i in range(pal['cnt'])]
        safe=re.sub(r'[^A-Za-z0-9_]+','_',sym); rel=f'fix39_arena/crowd/{safe}.bin'; out=ns.out_fs/f'{safe}.bin'
        with out.open('wb') as f:
            f.write(px); pad=(-len(px))&7; f.write(b'\0'*pad)
            for v in words:f.write(struct.pack('>H',v))
        records.append((sym,rel,im['w'],im['h'],im['x'],im['y'],len(px),len(px)+pad,pal['cnt']))
    # Deterministic ordering makes script indexes reproducible.
    script_names=sorted(scripts); sidx={n:i for i,n in enumerate(script_names)}; fidx={r[0].upper():i for i,r in enumerate(records)}
    ns.out_h.parent.mkdir(parents=True,exist_ok=True)
    ns.out_h.write_text('''#ifndef WM_CROWD_ASSETS_H\n#define WM_CROWD_ASSETS_H\n#include <stddef.h>\n#include <stdint.h>\ntypedef enum { WM_CROWD_FRAME=0,WM_CROWD_GOTO,WM_CROWD_RNDWAIT,WM_CROWD_REPEAT,WM_CROWD_SHOULD_REPEAT } wm_crowd_op;\ntypedef struct { uint8_t op; uint16_t arg; uint16_t value; } wm_crowd_cmd;\ntypedef struct { const wm_crowd_cmd *cmd; uint16_t count; } wm_crowd_script;\ntypedef struct { const char *symbol,*path; uint16_t width,height; int16_t xani,yani; uint32_t pixel_bytes; uint16_t palette_offset,palette_colors; } wm_crowd_asset;\ntypedef struct { uint16_t normal_script,cheer1_script,cheer2_script; } wm_crowd_person;\nsize_t wm_crowd_person_count(void);\nconst wm_crowd_person *wm_crowd_person_at(size_t i);\nconst wm_crowd_script *wm_crowd_script_at(size_t i);\nconst wm_crowd_asset *wm_crowd_asset_at(size_t i);\n#endif\n''')
    out=['/* Generated only from original Midway CROWD.ASM + CROWD.IMG. */','#include "wm/crowd_assets.h"']
    for n in script_names:
        out.append(f'static const wm_crowd_cmd s{sidx[n]}[]={{')
        for op,arg,val in scripts[n]:
            if op=='FRAME': out.append(f'{{WM_CROWD_FRAME,{fidx[arg.upper()]},{val}}},')
            elif op=='GOTO': out.append(f'{{WM_CROWD_GOTO,{sidx[arg]},0}},')
            elif op=='RNDWAIT': out.append(f'{{WM_CROWD_RNDWAIT,0,{val}}},')
            elif op=='REPEAT': out.append(f'{{WM_CROWD_REPEAT,0,{val}}},')
            elif op=='SHOULD_REPEAT': out.append('{WM_CROWD_SHOULD_REPEAT,0,0},')
        out.append('};')
    out.append('static const wm_crowd_script scripts[]={')
    for n in script_names: out.append(f'{{s{sidx[n]},sizeof(s{sidx[n]})/sizeof(s{sidx[n]}[0])}},')
    out.append('};\nstatic const wm_crowd_asset assets[]={')
    for sym,rel,w,h,x,y,pb,po,pc in records: out.append(f'{{"{sym}","rom:/{rel}",{w},{h},{x},{y},{pb},{po},{pc}}},')
    out.append('};\nstatic const wm_crowd_person persons[]={')
    for a,b,c in starts: out.append(f'{{{sidx[a]},{sidx[b]},{sidx[c]}}},')
    out += ['};','size_t wm_crowd_person_count(void){return sizeof(persons)/sizeof(persons[0]);}','const wm_crowd_person *wm_crowd_person_at(size_t i){return i<sizeof(persons)/sizeof(persons[0])?&persons[i]:0;}','const wm_crowd_script *wm_crowd_script_at(size_t i){return i<sizeof(scripts)/sizeof(scripts[0])?&scripts[i]:0;}','const wm_crowd_asset *wm_crowd_asset_at(size_t i){return i<sizeof(assets)/sizeof(assets[0])?&assets[i]:0;}']
    ns.out_c.parent.mkdir(parents=True,exist_ok=True); ns.out_c.write_text('\n'.join(out)+'\n')
    print(f'generated CROWD.ASM runtime: 30 people, {len(script_names)} scripts, {len(records)} physical frames')
if __name__=='__main__': main()
