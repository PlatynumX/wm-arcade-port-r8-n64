#!/usr/bin/env python3
"""Extract packed BMOD module records from original BGNDTBL.ASM.

The output preserves the source 64-bit block records verbatim (four WORDs per
block). Decoding belongs to wm_bmod_decode_block in the portable core.
"""
from __future__ import annotations
import argparse, pathlib, re, sys


def n(text: str) -> int:
    t=text.strip()
    if t.lower().endswith('h'): return int(t[:-1],16)
    if t.lower().startswith('0x'): return int(t,16)
    return int(t,10)


def label_pos(text: str, label: str) -> int:
    m=re.search(rf"^\s*{re.escape(label)}\s*:\s*$", text, re.I|re.M)
    if not m: raise ValueError(f"label not found: {label}")
    return m.end()


def parse_module(text: str, module: str) -> dict:
    pos=label_pos(text,module)
    tail=text[pos:]
    m=re.search(r"^\s*\.word\s+([^\n;]+)",tail,re.I|re.M)
    if not m: raise ValueError(f"{module}: size .word missing")
    vals=[n(x) for x in m.group(1).split(',')[:3]]
    if len(vals)!=3: raise ValueError(f"{module}: expected width,height,count")
    lm=re.search(r"^\s*\.long\s+([^\n;]+)", tail[m.end():], re.I|re.M)
    if not lm: raise ValueError(f"{module}: pointer .long missing")
    ptrs=[x.strip().lstrip('#') for x in lm.group(1).split(',')]
    if len(ptrs)<3: raise ValueError(f"{module}: expected block/header/palette labels")
    block_label=ptrs[0]
    bpos=label_pos(text,block_label)
    words=[]
    for line in text[bpos:].splitlines():
        code=line.split(';',1)[0].strip()
        if not code: continue
        if not code.lower().startswith('.word'):
            if words: break
            continue
        for tok in code[5:].split(','):
            tok=tok.strip()
            if not tok: continue
            v=n(tok)
            if v==0xffff and len(words)>=vals[2]*4: break
            words.append(v & 0xffff)
            if len(words)==vals[2]*4: break
        if len(words)==vals[2]*4: break
    if len(words)!=vals[2]*4:
        raise ValueError(f"{module}: got {len(words)} packed words, expected {vals[2]*4}")
    return {"name":module,"width":vals[0],"height":vals[1],"count":vals[2],
            "blocks":block_label,"headers":ptrs[1],"palettes":ptrs[2],"words":words}


def emit(mods: list[dict], out: pathlib.Path) -> None:
    lines=['/* Auto-generated verbatim BMOD packed records from BGNDTBL.ASM. */',
           '#include "wm/bmod.h"','#include <string.h>','']
    for i,m in enumerate(mods):
        lines.append(f'static const uint16_t bmod_words_{i}[] = {{')
        for j in range(0,len(m['words']),12):
            lines.append('    '+', '.join(f'0x{x:04X}' for x in m['words'][j:j+12])+',')
        lines += ['};','']
    lines.append('static const wm_named_bmod source_bmods[] = {')
    for i,m in enumerate(mods):
        lines.append(f'    {{"{m["name"]}", {{{m["width"]}, {m["height"]}, {m["count"]}, bmod_words_{i}}}, "{m["headers"]}", "{m["palettes"]}"}},')
    lines += ['};','',
              'size_t wm_source_bmod_count(void) { return sizeof(source_bmods)/sizeof(source_bmods[0]); }',
              'const wm_named_bmod *wm_source_bmod_at(size_t i) { return i < wm_source_bmod_count() ? &source_bmods[i] : 0; }',
              'const wm_named_bmod *wm_source_bmod_find(const char *name) {',
              '    if (!name) return 0;',
              '    for (size_t i=0;i<wm_source_bmod_count();++i) if (!strcmp(source_bmods[i].name,name)) return &source_bmods[i];',
              '    return 0;', '}', '']
    out.parent.mkdir(parents=True,exist_ok=True); out.write_text('\n'.join(lines))


def main()->int:
    ap=argparse.ArgumentParser(); ap.add_argument('--source',required=True,type=pathlib.Path)
    ap.add_argument('--module',action='append',required=True); ap.add_argument('--out',required=True,type=pathlib.Path)
    ns=ap.parse_args(); text=ns.source.read_text(errors='replace')
    mods=[parse_module(text,m) for m in ns.module]; emit(mods,ns.out)
    print('BMOD source:', ', '.join(f"{m['name']}={m['count']} blocks" for m in mods)); return 0
if __name__=='__main__':
    try: raise SystemExit(main())
    except (OSError,ValueError) as exc:
        print(f'bmod_source: error: {exc}',file=sys.stderr); raise SystemExit(2)
