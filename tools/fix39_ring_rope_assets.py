#!/usr/bin/env python3
"""Translate original Midway WIMP rope artwork into DragonFS CI8/TLUT payloads.
No synthesized geometry or replacement art: every live rope record comes from the original shipped IMG containers selected by MISC.LOD/MAIN.LOD.
"""
from __future__ import annotations
import argparse, pathlib, struct, re
from collections import Counter, defaultdict
IES=0x32; PES=0x1a; HM=28

def u16(d,o): return struct.unpack_from('<H',d,o)[0]
def s16(d,o): return struct.unpack_from('<h',d,o)[0]
def u32(d,o): return struct.unpack_from('<I',d,o)[0]
def name(d): return d.split(b'\0',1)[0].decode('ascii')
def rgba(v,i):
    if i==0:return 0
    return (((v>>10)&31)<<11)|(((v>>5)&31)<<6)|((v&31)<<1)|1

def parse(p):
    d=p.read_bytes(); n=u16(d,0); off=u32(d,4)
    if not n or off<HM or off+n*IES>len(d): raise ValueError(f'{p}: bad WIMP directory')
    ims=[]
    for i in range(n):
        q=off+i*IES; nm=name(d[q:q+8]); w=u16(d,q+22); h=u16(d,q+24); po=u16(d,q+26); do=u32(d,q+28)
        if not w or not h or do+((w+3)&~3)*h>len(d): raise ValueError(f'{p}:{nm}: bad pixels')
        ims.append(dict(name=nm,x=s16(d,q+18),y=s16(d,q+20),w=w,h=h,pid=po,do=do,ord=i))
    first=min(x['do'] for x in ims); q=off+n*IES; pals=[]
    while q+PES<=len(d):
        try:nm=name(d[q:q+8])
        except:break
        cnt=u16(d,q+12); po=u32(d,q+14)
        if not nm or not 1<=cnt<=256 or po<HM or po+cnt*2>first: break
        pals.append(dict(name=nm,cnt=cnt,do=po)); q+=PES
    if not pals: raise ValueError(f'{p}: no palettes')
    ids=sorted({x['pid'] for x in ims}); base=ids[0]
    for im in ims:
        pi=im['pid']-base
        if pi<0 or pi>=len(pals): raise ValueError(f'{p}:{im["name"]}: palette map')
        im['pal']=pals[pi]
    return d,ims

def ci8(d,im):
    stride=(im['w']+3)&~3; out=bytearray()
    for y in range(im['h']): out += d[im['do']+y*stride:im['do']+y*stride+im['w']]
    return bytes(out)

def sym_for(nm, idx, total):
    # ROPES.ASM side/shadow pair labels are source NAMEa/NAMEb; WIMP stores two same-name entries.
    if total==2: return nm + ('a' if idx==0 else 'b')
    if total==1:return nm
    return f'{nm}__{idx}'

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--root',type=pathlib.Path,required=True); ap.add_argument('--out-fs',type=pathlib.Path,required=True); ap.add_argument('--out-c',type=pathlib.Path,required=True); ap.add_argument('--out-h',type=pathlib.Path,required=True); ns=ap.parse_args()
    files=['ROPESTUF.IMG','ROPESHAD.IMG']; rec=[]
    ns.out_fs.mkdir(parents=True,exist_ok=True)
    for fn in files:
        p=ns.root/'IMG'/fn; d,ims=parse(p); totals=Counter(x['name'] for x in ims); seen=defaultdict(int)
        for im in ims:
            k=seen[im['name']]; seen[im['name']]+=1; sym=sym_for(im['name'],k,totals[im['name']])
            px=ci8(d,im); pal=im['pal']; words=[rgba(u16(d,pal['do']+i*2)&0x7fff,i) for i in range(pal['cnt'])]
            safe=re.sub(r'[^A-Za-z0-9_]+','_',sym)
            rel=f'fix39_ring/{fn[:-4].lower()}/{safe}.bin'; out=ns.out_fs/fn[:-4].lower()/f'{safe}.bin'; out.parent.mkdir(parents=True,exist_ok=True)
            with out.open('wb') as f:
                f.write(px); pad=(-len(px))&7; f.write(b'\0'*pad)
                for v in words:f.write(struct.pack('>H',v))
            rec.append((sym,fn,rel,im['w'],im['h'],im['x'],im['y'],len(px),pad,pal['cnt']))
    ns.out_h.parent.mkdir(parents=True,exist_ok=True); ns.out_h.write_text('''#ifndef WM_RING_ROPE_ASSETS_H\n#define WM_RING_ROPE_ASSETS_H\n#include <stddef.h>\n#include <stdint.h>\ntypedef struct { const char *symbol,*container,*path; uint16_t width,height; int16_t xani,yani; uint32_t pixel_bytes; uint16_t palette_offset,palette_colors; } wm_ring_rope_asset;\nconst wm_ring_rope_asset *wm_ring_rope_asset_find(const char *symbol);\nsize_t wm_ring_rope_asset_count(void);\n#endif\n''')
    lines=['/* Generated only from original Midway rope WIMP containers. */','#include "wm/ring_rope_assets.h"','#include <string.h>','static const wm_ring_rope_asset a[]={']
    for r in rec:
        sym,fn,rel,w,h,x,y,pb,pad,pc=r; lines.append(f'{{"{sym}","{fn}","rom:/{rel}",{w},{h},{x},{y},{pb},{pb+pad},{pc}}},')
    lines += ['};','size_t wm_ring_rope_asset_count(void){return sizeof(a)/sizeof(a[0]);}','const wm_ring_rope_asset *wm_ring_rope_asset_find(const char *s){if(!s)return 0;for(size_t i=0;i<sizeof(a)/sizeof(a[0]);++i)if(!strcmp(a[i].symbol,s))return &a[i];return 0;}']
    ns.out_c.parent.mkdir(parents=True,exist_ok=True); ns.out_c.write_text('\n'.join(lines)+'\n')
    print(f'generated {len(rec)} source rope image records from {len(files)} original WIMP containers')
if __name__=='__main__': main()
