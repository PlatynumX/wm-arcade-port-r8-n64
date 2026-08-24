#!/usr/bin/env python3
"""
Regenerate src/generated/sports_logo.c while preserving the original
SPORTLO8 17-object structure. Pillow and NumPy are authoring-time only.
Normal N64/CI builds compile the checked-in generated C and do not run this.
"""
from pathlib import Path
from PIL import Image
import numpy as np
import argparse

PIECES = [
    ("SPRTLG01",42,35,137,73,18), ("SPRTLG02",187,35,95,83,42),
    ("SPRTLG03",47,35,-92,73,18), ("SPRTLG04",43,79,109,65,64),
    ("SPRTLG05",30,54,66,53,61), ("SPRTLG06",27,30,36,44,51),
    ("SPRTLG07",39,39,9,47,64), ("SPRTLG08",25,45,-30,52,58),
    ("SPRTLG09",25,74,-55,62,63), ("SPRTLG10",26,85,-80,68,45),
    ("SPRTLG11",37,62,55,27,45), ("SPRTLG12",35,55,18,28,64),
    ("SPRTLG13",46,42,-17,20,55), ("SPRTLG14",40,51,34,-23,64),
    ("SPRTLG15",39,42,-6,-12,59), ("SPRTLG16",87,80,92,-5,32),
    ("SPRTLG17",98,77,5,-8,32),
]
TAIL=(0,0,0,-1,-1,-1,0,-1,-1)
CW,CH,CX,CY=276,168,137,83

def rgba5551(r,g,b,a=255):
    if a < 128: return 0
    return ((r>>3)<<11)|((g>>3)<<6)|((b>>3)<<1)|1

def qpiece(img,n):
    alpha=np.asarray(img.getchannel("A"))
    q=img.convert("RGB").quantize(
        colors=max(1,n-1),
        method=Image.Quantize.MEDIANCUT,
        dither=Image.Dither.FLOYDSTEINBERG)
    idx=np.asarray(q,dtype=np.uint8).copy()+1
    idx[alpha<128]=0
    p=q.getpalette()[:max(1,n-1)*3]
    pal=[0]
    for i in range(max(1,n-1)):
        pal.append(rgba5551(*p[i*3:i*3+3]))
    pal=(pal+[0]*n)[:n]
    return idx.flatten().tolist(),pal

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--art",type=Path,required=True)
    ap.add_argument("--out",type=Path,required=True)
    ns=ap.parse_args()

    art=Image.open(ns.art).convert("RGBA")
    comp=Image.new("RGBA",(CW,CH),(0,0,0,0))
    s=min(CW/art.width,CH/art.height)
    w,h=round(art.width*s),round(art.height*s)
    comp.alpha_composite(art.resize((w,h),Image.Resampling.LANCZOS),((CW-w)//2,(CH-h)//2))

    gs=[]
    for name,w,h,xa,ya,pc in PIECES:
        x,y=CX-xa,CY-ya
        px,pal=qpiece(comp.crop((x,y,x+w,y+h)),pc)
        gs.append((name,w,h,xa,ya,pc,px,pal))

    L=[
        "/* Generated artwork-only SPORTLO8 replacement. */",
        '#include "wm/sports_logo.h"',
        "#include <string.h>",""
    ]
    for name,w,h,xa,ya,pc,px,pal in gs:
        ident=name.lower()
        L.append(f"static uint16_t pal_{ident}[] __attribute__((aligned(8))) = {{")
        for i in range(0,len(pal),12):
            L.append("    "+", ".join(f"0x{v:04X}" for v in pal[i:i+12])+",")
        L+=["};","",f"static const uint8_t px_{ident}[] __attribute__((aligned(8))) = {{"]
        for i in range(0,len(px),24):
            L.append("    "+", ".join(f"0x{v:02X}" for v in px[i:i+24])+",")
        L+=["};",""]
    L.append("static const wm_source_sprite sprites[] = {")
    tail=", ".join(map(str,TAIL))
    for name,w,h,xa,ya,pc,px,pal in gs:
        ident=name.lower()
        L.append(f'    {{"{name}", "SPORTLO8.IMG", {w}, {h}, {xa}, {ya}, '
                 f'{{{tail}}}, px_{ident}, pal_{ident}, {pc}}},')
    L += [
        "};","",
        '_Static_assert(sizeof(sprites)/sizeof(sprites[0]) == WM_SPORTS_LOGO_PIECES,',
        '               "replacement must remain a 17-piece SPORTLO8 package");',"",
        "const wm_source_sprite *wm_sports_logo_sprite_find(const char *s) {",
        "    if (!s) return 0;",
        "    for (size_t i=0;i<sizeof(sprites)/sizeof(sprites[0]);++i)",
        "        if (!strcmp(sprites[i].source_frame,s)) return &sprites[i];",
        "    return 0;","}","",
        "const wm_source_sprite *wm_sports_logo_sprite_at(size_t i) {",
        "    return i < sizeof(sprites)/sizeof(sprites[0]) ? &sprites[i] : 0;","}","",
        "size_t wm_sports_logo_sprite_count(void) {",
        "    return sizeof(sprites)/sizeof(sprites[0]);","}",""
    ]
    ns.out.parent.mkdir(parents=True,exist_ok=True)
    ns.out.write_text("\n".join(L))

if __name__=="__main__":
    main()
