#!/usr/bin/env python3
"""Translate the original WrestleMania ring BMOD + BDD payload for N64.

Source authority:
  BGNDTBL.ASM ringBMOD/ringBLKS
  IMG/NEWRINGB.BDB + IMG/NEWRINGB.BDD (the 211-block ring build)
No synthesized art or hand-authored placement is emitted.
"""
from __future__ import annotations
import argparse, pathlib, re, struct

def parse_num(s: str) -> int:
    s=s.strip().upper()
    if s.endswith('H'): return int(s[:-1],16)
    return int(s,0)

def rgba5551_from_midway(raw: int, transparent_zero: bool, index: int) -> int:
    raw &= 0x7fff
    if transparent_zero and index == 0: return 0
    return (((raw >> 10) & 31) << 11) | (((raw >> 5) & 31) << 6) | ((raw & 31) << 1) | 1

def parse_bdd(path: pathlib.Path):
    data=path.read_bytes(); nl=data.find(b'\n')
    if nl < 0: raise ValueError('BDD: missing image count')
    count=int(data[:nl].strip()); pos=nl+1; images=[]
    for i in range(count):
        nl=data.find(b'\n',pos)
        if nl < 0: raise ValueError(f'BDD: missing image header {i}')
        parts=data[pos:nl].decode('ascii').split()
        if len(parts)!=4: raise ValueError(f'BDD: malformed image header {i}: {parts}')
        off=int(parts[0],16); w=int(parts[1]); h=int(parts[2]); ctrl=int(parts[3],16)
        if off != i*3: raise ValueError(f'BDD: image header offset {off:#x} != {i*3:#x}')
        pos=nl+1; n=w*h
        if pos+n > len(data): raise ValueError(f'BDD: short image {i}')
        images.append((w,h,ctrl,data[pos:pos+n])); pos += n
    palettes=[]
    while pos < len(data):
        nl=data.find(b'\n',pos)
        if nl < 0: raise ValueError('BDD: missing palette header newline')
        hdr=data[pos:nl].decode('ascii').strip(); pos=nl+1
        if not hdr: continue
        parts=hdr.split()
        if len(parts)!=2: raise ValueError(f'BDD: malformed palette header {hdr!r}')
        name=parts[0]; count=int(parts[1]); n=count*2
        if pos+n > len(data): raise ValueError(f'BDD: short palette {name}')
        raw=[struct.unpack_from('<H',data,pos+j*2)[0] for j in range(count)]
        pos += n; palettes.append((name,raw))
    return images,palettes

def parse_bdb(path: pathlib.Path):
    lines=[x.strip() for x in path.read_text(errors='strict').splitlines() if x.strip()]
    h=lines[0].split(); count=int(h[-1]);
    if count != 211: raise ValueError(f'BDB: expected source ring 211 blocks, got {count}')
    blocks=[]
    for line in lines[2:]:
        p=line.split()
        if len(p)!=5: raise ValueError(f'BDB: malformed block {line!r}')
        src_flags=int(p[0],16); x=int(p[1]); y=int(p[2]); hdr_off=int(p[3],16); pal=int(p[4])
        if hdr_off % 3: raise ValueError(f'BDB: header offset not /3: {hdr_off:#x}')
        # This is the exact conversion visible in generated BGNDTBL.ASM:
        # crop origin 100,200; BAKGND transparency flag 0x40; palette low nibble
        # in word0 and high nibble in word3.
        w0=(src_flags + 0x40 + (pal & 0x0f)) & 0xffff
        w3=(hdr_off//3) | ((pal & 0xf0) << 8)
        blocks.append((w0,x-100,y-200,w3))
    if len(blocks)!=count: raise ValueError(f'BDB: expected {count} block records, got {len(blocks)}')
    return blocks

def verify_generated_source(src: pathlib.Path, blocks):
    txt=src.read_text(errors='ignore')
    m=re.search(r'ringBMOD:\s*\n\s*\.word\s+(\d+)\s*,\s*(\d+)\s*,\s*(\d+)',txt,re.I)
    if not m: raise ValueError('BGNDTBL: ringBMOD not found')
    dims=tuple(map(int,m.groups()))
    if dims != (1972,868,211): raise ValueError(f'BGNDTBL: unexpected ringBMOD {dims}')
    # Verify representative records so the BDB -> generated conversion cannot drift silently.
    sec=txt[txt.index('ringBLKS:'):txt.index('ringBMOD:')]
    for want in ['04046H','155,587,01H','04050H,176,744,02H','03F49H,183,626,03H']:
        if want not in sec.replace(' ','') and want.replace(' ','') not in sec.replace(' ',''):
            # whitespace-normalized fallback below
            norm=re.sub(r'\s+','',sec).upper()
            if want.replace(' ','').upper() not in norm:
                raise ValueError(f'BGNDTBL: expected source marker missing: {want}')
    return dims

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--source-pack',type=pathlib.Path,required=True)
    ap.add_argument('--out-fs',type=pathlib.Path,required=True); ap.add_argument('--out-c',type=pathlib.Path,required=True); ap.add_argument('--out-h',type=pathlib.Path,required=True)
    ns=ap.parse_args(); asset=ns.source_pack/'assets'; source=ns.source_pack/'source'
    images,pals=parse_bdd(asset/'NEWRINGB.BDD'); blocks=parse_bdb(asset/'NEWRINGB.BDB'); dims=verify_generated_source(source/'BGNDTBL.ASM',blocks)
    if max((w3 & 0x0fff) for _,_,_,w3 in blocks) >= len(images): raise ValueError('ring block header index exceeds BDD images')
    if max(((w0&0xf)|((w3>>8)&0xf0)) for w0,_,_,w3 in blocks) >= len(pals): raise ValueError('ring block palette exceeds BDD palettes')
    ns.out_fs.mkdir(parents=True,exist_ok=True)
    for i,(w,h,ctrl,pix) in enumerate(images):
        (ns.out_fs/f'hdr_{i:03d}.ci8').write_bytes(pix)
    ns.out_h.parent.mkdir(parents=True,exist_ok=True)
    ns.out_h.write_text('''#ifndef WM_RING_ARENA_ASSETS_H\n#define WM_RING_ARENA_ASSETS_H\n#include <stddef.h>\n#include <stdint.h>\ntypedef struct { uint16_t width,height,ctrl; const char *path; } wm_ring_arena_image;\ntypedef struct { const char *name; uint16_t color_count; uint16_t *rgba5551_opaque; uint16_t *rgba5551_keyed; } wm_ring_arena_palette;\ntypedef struct { uint8_t palette,flags,z; int16_t x,y; uint16_t header_index; } wm_ring_arena_block;\nuint16_t wm_ring_arena_width(void);\nuint16_t wm_ring_arena_height(void);\nsize_t wm_ring_arena_block_count(void);\nconst wm_ring_arena_block *wm_ring_arena_block_at(size_t i);\nconst wm_ring_arena_image *wm_ring_arena_image_at(size_t i);\nconst wm_ring_arena_palette *wm_ring_arena_palette_at(size_t i);\n#endif\n''')
    out=['/* Generated from Midway BGNDTBL.ASM + NEWRINGB.BDB/.BDD. */','#include "wm/ring_arena_assets.h"']
    for i,(name,raw) in enumerate(pals):
        op=[rgba5551_from_midway(v,False,j) for j,v in enumerate(raw)]; ky=[rgba5551_from_midway(v,True,j) for j,v in enumerate(raw)]
        out.append(f'static uint16_t p{i}_o[]={{'+','.join(f'0x{x:04x}' for x in op)+'};')
        out.append(f'static uint16_t p{i}_k[]={{'+','.join(f'0x{x:04x}' for x in ky)+'};')
    out.append('static const wm_ring_arena_palette pals[]={')
    for i,(name,raw) in enumerate(pals): out.append(f'{{"{name}",{len(raw)},p{i}_o,p{i}_k}},')
    out.append('};\nstatic const wm_ring_arena_image imgs[]={')
    for i,(w,h,ctrl,pix) in enumerate(images): out.append(f'{{{w},{h},0x{ctrl:04x},"rom:/fix39_arena/ring/hdr_{i:03d}.ci8"}},')
    out.append('};\nstatic const wm_ring_arena_block blks[]={')
    for w0,x,y,w3 in blocks:
        pal=(w0&0x0f)|((w3>>8)&0xf0); flags=(w0>>4)&0x0f; z=(w0>>8)&0xff; hdr=w3&0x0fff
        out.append(f'{{{pal},{flags},{z},{x},{y},{hdr}}},')
    out += ['};',f'uint16_t wm_ring_arena_width(void){{return {dims[0]};}}',f'uint16_t wm_ring_arena_height(void){{return {dims[1]};}}','size_t wm_ring_arena_block_count(void){return sizeof(blks)/sizeof(blks[0]);}','const wm_ring_arena_block *wm_ring_arena_block_at(size_t i){return i<sizeof(blks)/sizeof(blks[0])?&blks[i]:0;}','const wm_ring_arena_image *wm_ring_arena_image_at(size_t i){return i<sizeof(imgs)/sizeof(imgs[0])?&imgs[i]:0;}','const wm_ring_arena_palette *wm_ring_arena_palette_at(size_t i){return i<sizeof(pals)/sizeof(pals[0])?&pals[i]:0;}']
    ns.out_c.parent.mkdir(parents=True,exist_ok=True); ns.out_c.write_text('\n'.join(out)+'\n')
    print(f'generated exact ringBMOD: {len(blocks)} blocks, {len(images)} source images, {len(pals)} palettes, {sum(len(x[3]) for x in images)} CI8 bytes')
if __name__=='__main__': main()
