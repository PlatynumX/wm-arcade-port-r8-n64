#!/usr/bin/env python3
from __future__ import annotations
import argparse, pathlib, re

def clean(s): return s.split(';',1)[0].strip()

def parse_num(tok):
    tok=tok.strip()
    neg=False
    if tok.startswith('-'): neg=True; tok=tok[1:]
    m=re.fullmatch(r'([0-9A-Fa-f]+)[hH]',tok)
    if m: v=int(m.group(1),16)
    elif re.fullmatch(r'\d+',tok): v=int(tok)
    else: raise ValueError(tok)
    return -v if neg else v

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--source',type=pathlib.Path,required=True); ap.add_argument('--out-c',type=pathlib.Path,required=True); ap.add_argument('--out-h',type=pathlib.Path,required=True); a=ap.parse_args()
    ls=a.source.read_text(errors='replace').splitlines()
    # mode table
    mt=[]; start=next(i for i,l in enumerate(ls) if clean(l).lower()=='#mode_table')+1
    for l in ls[start:]:
        c=clean(l)
        m=re.match(r'^\.long\s+([A-Za-z_][A-Za-z0-9_]*)',c,re.I)
        if not m:
            if mt: break
            continue
        mt.append(m.group(1).lower())
    if len(mt)!=26: raise SystemExit(f'mode_table length {len(mt)} != 26')
    # Parse aliases + exactly 9*5*3 WORDs for each mode-data block.
    blocks={}; i=start+len(mt)
    while i<len(ls):
        aliases=[]
        while i<len(ls):
            c=clean(ls[i])
            if not c: i+=1; continue
            if c.startswith(';') or c.startswith('*'): i+=1; continue
            if re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*',c) and c.lower().startswith('mode_'):
                aliases.append(c.lower()); i+=1; continue
            break
        if not aliases: i+=1; continue
        vals=[]; j=i
        while j<len(ls) and len(vals)<135:
            c=clean(ls[j])
            m=re.match(r'^\.word\s+(.+)$',c,re.I)
            if m:
                for t in m.group(1).split(','):
                    vals.append(parse_num(t))
            elif vals and re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*',c):
                break
            j+=1
        if len(vals)==135:
            arr=[]
            k=0
            for wr in range(9):
                w=[]
                for area in range(5):
                    w.append(tuple(vals[k:k+3])); k+=3
                arr.append(w)
            for al in aliases: blocks[al]=arr
            i=j
        else:
            i+=1
    missing=sorted(set(mt)-set(blocks))
    if missing: raise SystemExit('missing target blocks: '+','.join(missing))
    h='''#ifndef WM_ARCADE_TARGET_OFFSETS_H\n#define WM_ARCADE_TARGET_OFFSETS_H\n#include <stdbool.h>\n#include <stdint.h>\n#ifdef __cplusplus\nextern "C" {\n#endif\nbool wm_source_target_offsets(uint16_t player_mode,uint8_t wrestler_num,uint16_t target_area,int16_t *x,int16_t *y,int16_t *z);\n#ifdef __cplusplus\n}\n#endif\n#endif\n'''
    a.out_h.parent.mkdir(parents=True,exist_ok=True);a.out_h.write_text(h)
    c=['/* Generated exactly from TABLES.ASM set_target_offsets/mode_table. */','#include "wm_arcade_target_offsets.h"','']
    unique=[]; names={}
    for name in mt:
        oid=id(blocks[name])
        # id isn't shared for parsed aliases because same arr assigned; yes.
        if oid not in names:
            names[oid]=f'tgt_{len(unique)}';unique.append(blocks[name])
    for idx,arr in enumerate(unique):
        c.append(f'static const int16_t tgt_{idx}[9][5][3]={{')
        for w in arr:
            c.append('{'+','.join('{%d,%d,%d}'%v for v in w)+'},')
        c.append('};')
    c.append('static const int16_t (*const mode_targets[26])[5][3]={')
    for name in mt:c.append(names[id(blocks[name])]+',')
    c.append('};')
    c.append('bool wm_source_target_offsets(uint16_t mode,uint8_t wr,uint16_t area,int16_t*x,int16_t*y,int16_t*z){const int16_t(*t)[5][3];if(mode>=26||wr>=9)return false;area&=0x7fffU;if(area>=5)return false;t=mode_targets[mode];if(x)*x=t[wr][area][0];if(y)*y=t[wr][area][1];if(z)*z=t[wr][area][2];return true;}')
    a.out_c.parent.mkdir(parents=True,exist_ok=True);a.out_c.write_text('\n'.join(c)+'\n')
    print(f'generated TABLES.ASM target offsets: 26 modes, {len(unique)} unique target blocks')
if __name__=='__main__': main()
