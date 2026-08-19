#!/usr/bin/env python3
from __future__ import annotations
import argparse, pathlib, re, sys, zipfile, zlib, hashlib

EXPECTED_GFX = {
    'wwf.133':0x5e1b1e3d,'wwf.132':0x5943b3b2,'wwf.131':0x0815db22,'wwf.130':0x9ee9a145,
    'wwf.129':0xc644c2f4,'wwf.128':0xfcda4e9a,'wwf.127':0x45be7428,'wwf.126':0xeaa276a8,
    'wwf.125':0xa19ebeed,'wwf.124':0xdc7d3dbb,'wwf.123':0xe0ade56f,'wwf.122':0x2800c78d,
    'wwf.121':0xa28ffcba,'wwf.120':0x3a05d371,'wwf.119':0x97ffa659,'wwf.118':0x46668e97,
}
GFX_GROUPS = [
    ('wwf.133','wwf.132','wwf.131','wwf.130'),
    ('wwf.129','wwf.128','wwf.127','wwf.126'),
    ('wwf.125','wwf.124','wwf.123','wwf.122'),
    ('wwf.121','wwf.120','wwf.119','wwf.118'),
]
SELFTEST = {
    ('ELBKHDRS',0): 'c19cebbeed48d017d25691eb50923c71b1f618b4b97e1043ec09395b6fdc6591',
    ('ELBKHDRS',2): '4e1951a9729fcc9b26e64344b3c81f25ffa0c9658ac272de3d519fbf1fde3d8b',
}

def num(s:str)->int:
    s=s.strip().rstrip(',')
    if s.lower().endswith('h'): return int(s[:-1],16)
    if s.lower().startswith('0x'): return int(s,16)
    return int(s,10)

def code_lines(text:str):
    for raw in text.splitlines():
        yield raw.split(';',1)[0].strip()

def find_label_index(lines, label):
    rx=re.compile(rf'^{re.escape(label)}\s*:\s*$',re.I)
    for i,l in enumerate(lines):
        if rx.match(l): return i
    raise ValueError(f'label not found: {label}')

def parse_word_values(line):
    m=re.match(r'(?i)^\.word\s+(.+)$',line)
    if not m: return None
    return [num(x) for x in m.group(1).split(',') if x.strip()]

def parse_long_values(line):
    m=re.match(r'(?i)^\.long\s+(.+)$',line)
    if not m: return None
    return [x.strip() for x in m.group(1).split(',') if x.strip()]

def parse_module(text, module_name):
    lines=list(code_lines(text))
    i=find_label_index(lines,module_name)+1
    while i<len(lines) and not lines[i]: i+=1
    dims=parse_word_values(lines[i]); i+=1
    if not dims or len(dims)!=3: raise ValueError(f'{module_name}: bad size/count row')
    refs=parse_long_values(lines[i])
    if not refs or len(refs)!=3: raise ValueError(f'{module_name}: bad block/header/palette row')
    width,height,count=dims
    block_label, header_label, palette_label=refs
    bi=find_label_index(lines,block_label)+1
    words=[]
    while bi<len(lines) and len(words)<count*4:
        l=lines[bi]; bi+=1
        if not l: continue
        if re.match(r'^[A-Za-z_.$][A-Za-z0-9_.$]*\s*:\s*$',l):
            break
        vals=parse_word_values(l)
        if not vals: continue
        if len(vals)==1 and vals[0]==0xffff: break
        words.extend(vals)
    if len(words)<count*4:
        raise ValueError(f'{module_name}: parsed {len(words)//4} blocks, need {count}')
    words=words[:count*4]
    return {'name':module_name,'width':width,'height':height,'count':count,'words':words,
            'blocks':block_label,'headers':header_label,'palettes':palette_label}

def decode_block(words,i):
    a,x,y,h=words[i*4:i*4+4]
    return {'palette':(a&0x000f)|((h>>8)&0x00f0), 'flags':(a>>4)&0x000f, 'z':(a>>8)&0xff,
            'x':x if x<0x8000 else x-0x10000, 'y':y if y<0x8000 else y-0x10000,
            'header':h&0x0fff}

def parse_headers(text,label,count):
    lines=list(code_lines(text)); i=find_label_index(lines,label)+1
    out=[]
    while i<len(lines) and len(out)<count:
        while i<len(lines) and not lines[i]: i+=1
        if i>=len(lines): break
        if re.match(r'^[A-Za-z_.$][A-Za-z0-9_.$]*\s*:\s*$',lines[i]): break
        wh=parse_word_values(lines[i]); i+=1
        if not wh or len(wh)!=2: continue
        while i<len(lines) and not lines[i]: i+=1
        lv=parse_long_values(lines[i]); i+=1
        if not lv or len(lv)!=1: raise ValueError(f'{label}: missing address for header {len(out)}')
        address=num(lv[0])
        while i<len(lines) and not lines[i]: i+=1
        cv=parse_word_values(lines[i]); i+=1
        if not cv or len(cv)!=1: raise ValueError(f'{label}: missing ctrl for header {len(out)}')
        out.append({'width':wh[0],'height':wh[1],'address_bits':address,'ctrl':cv[0]})
    if len(out)<count: raise ValueError(f'{label}: parsed {len(out)} headers, need {count}')
    return out

def parse_palette_pointer_table(text,label):
    lines=list(code_lines(text)); i=find_label_index(lines,label)+1; out=[]
    while i<len(lines):
        l=lines[i]; i+=1
        if not l: continue
        if re.match(r'^[A-Za-z_.$][A-Za-z0-9_.$]*\s*:\s*$',l): break
        vals=parse_long_values(l)
        if vals: out.extend(vals)
        elif out: break
    if not out: raise ValueError(f'{label}: no palette pointers')
    return out

def parse_palette_definition(texts,label):
    for text in texts:
        lines=list(code_lines(text))
        try: i=find_label_index(lines,label)+1
        except ValueError: continue
        while i<len(lines) and not lines[i]: i+=1
        sv=parse_word_values(lines[i]) if i<len(lines) else None
        if not sv or len(sv)!=1: continue
        size=sv[0]; i+=1; vals=[]
        while i<len(lines) and len(vals)<size:
            l=lines[i]; i+=1
            if not l: continue
            if re.match(r'^[A-Za-z_.$][A-Za-z0-9_.$]*\s*:\s*$',l): break
            w=parse_word_values(l)
            if w: vals.extend(w)
        if len(vals)<size: raise ValueError(f'{label}: palette says {size} colors, parsed {len(vals)}')
        return vals[:size]
    raise ValueError(f'palette definition not found: {label}')

def load_gfx_region(romzip:pathlib.Path)->bytes:
    with zipfile.ZipFile(romzip,'r') as z:
        roms={}
        names=set(z.namelist())
        # tolerate directories in zip by basename
        bybase={pathlib.PurePosixPath(n).name:n for n in names}
        for name,crc in EXPECTED_GFX.items():
            actual=bybase.get(name)
            if not actual: raise ValueError(f'{romzip.name}: missing {name}')
            d=z.read(actual)
            if len(d)!=0x100000: raise ValueError(f'{name}: size {len(d)}, expected 1048576')
            got=zlib.crc32(d)&0xffffffff
            if got!=crc: raise ValueError(f'{name}: CRC {got:08x}, expected {crc:08x}')
            roms[name]=d
    gfx=bytearray(0x1000000)
    for group_index,group in enumerate(GFX_GROUPS):
        base=group_index*0x400000
        for lane,name in enumerate(group):
            d=roms[name]
            gfx[base+lane:base+0x400000:4]=d
    return bytes(gfx)

def read_bits_lsb(base:bytes, bitoff:int, bits:int)->int:
    if bits<=0 or bits>8: raise ValueError(f'invalid bit width {bits}')
    byte=bitoff>>3; shift=bitoff&7
    if byte>=len(base): raise ValueError(f'bit address 0x{bitoff:X} outside gfx ROM')
    v=base[byte]
    if shift+bits>8:
        if byte+1>=len(base): raise ValueError('bit extractor crossed end of gfx ROM')
        v|=base[byte+1]<<8
    return (v>>shift)&((1<<bits)-1)

def decode_dma(gfx:bytes,hdr:dict)->bytes:
    w,h=hdr['width'],hdr['height']; ctrl=hdr['ctrl']; off=hdr['address_bits']
    bpp=(ctrl>>12)&7
    if bpp==0: bpp=8
    skip=bool(ctrl&0x80)
    preshift=(ctrl>>8)&3; postshift=(ctrl>>10)&3
    out=bytearray(w*h)
    pos=0
    for y in range(h):
        pre=post=0
        if skip:
            desc=read_bits_lsb(gfx,off,8); off+=8
            pre=(desc&0x0f)<<preshift
            post=((desc>>4)&0x0f)<<postshift
            if pre+post>w:
                raise ValueError(f'DMA row {y}: pre {pre}+post {post}>width {w} ctrl=0x{ctrl:04X}')
        run=w-pre-post
        pos += pre
        for _ in range(run):
            out[pos]=read_bits_lsb(gfx,off,bpp); pos+=1; off+=bpp
        pos += post
    if pos!=w*h: raise AssertionError((pos,w*h))
    return bytes(out)

def rgb555_to_rgba5551(v:int,a:int)->int:
    v&=0x7fff
    return (((v>>10)&31)<<11)|(((v>>5)&31)<<6)|((v&31)<<1)|(1 if a else 0)

def c_ident(s):
    return re.sub(r'[^A-Za-z0-9_]', '_', s)

def emit_module(gfx,bgndtbl_text,palette_texts,module_name,out_path,prefix):
    mod=parse_module(bgndtbl_text,module_name)
    blocks=[decode_block(mod['words'],i) for i in range(mod['count'])]
    max_header=max(b['header'] for b in blocks)
    hdrs=parse_headers(bgndtbl_text,mod['headers'],max_header+1)
    pal_names=parse_palette_pointer_table(palette_texts[0],mod['palettes'])
    used_headers=sorted({b['header'] for b in blocks})
    used_pals=sorted({b['palette'] for b in blocks})
    for pi in used_pals:
        if pi>=len(pal_names): raise ValueError(f'{module_name}: palette index {pi} >= {len(pal_names)}')
    palettes={pi:parse_palette_definition(palette_texts,pal_names[pi]) for pi in used_pals}
    decoded={}
    for hi in used_headers:
        decoded[hi]=decode_dma(gfx,hdrs[hi])
        key=(mod['headers'],hi)
        if key in SELFTEST:
            got=hashlib.sha256(decoded[hi]).hexdigest()
            if got!=SELFTEST[key]:
                raise ValueError(f'{mod["headers"]}[{hi}] decode hash {got}, expected {SELFTEST[key]}')
    # palette-domain validation per block
    for i,b in enumerate(blocks):
        px=decoded[b['header']]
        mx=max(px) if px else 0
        psz=len(palettes[b['palette']])
        if mx>=psz:
            raise ValueError(f'{module_name} block {i}: header {b["header"]} max pixel {mx} >= palette {b["palette"]} {pal_names[b["palette"]]} size {psz}')
    lines=[f'/* Auto-generated from verified WWF Wolf Unit graphics ROM + {mod["headers"]}/{mod["palettes"]}. */',
           '#include "wm/select_background.h"','']
    array_for={}
    by_hash={}
    for hi in used_headers:
        px=decoded[hi]; dig=hashlib.sha256(px).hexdigest()
        if dig in by_hash:
            array_for[hi]=by_hash[dig]; continue
        ident=c_ident(f'{prefix}_h{hi}_{hdrs[hi]["address_bits"]:08x}')
        by_hash[dig]=ident; array_for[hi]=ident
        lines.append(f'static const uint8_t px_{ident}[] __attribute__((aligned(8))) = {{')
        for j in range(0,len(px),24): lines.append('    '+', '.join(f'0x{x:02X}' for x in px[j:j+24])+',')
        lines += ['};','']
    pal_ident={}
    for pi in used_pals:
        vals=palettes[pi]; ident=c_ident(f'{prefix}_p{pi}_{pal_names[pi]}'); pal_ident[pi]=ident
        opaque=[rgb555_to_rgba5551(v,1) for v in vals]
        keyed=[rgb555_to_rgba5551(v,0 if j==0 else 1) for j,v in enumerate(vals)]
        for suffix,arr in [('opaque',opaque),('keyed',keyed)]:
            lines.append(f'static uint16_t pal_{ident}_{suffix}[] __attribute__((aligned(8))) = {{')
            for j in range(0,len(arr),12): lines.append('    '+', '.join(f'0x{x:04X}' for x in arr[j:j+12])+',')
            lines += ['};','']
    lines.append(f'static const wm_select_background_image bg_images[{max_header+1}] = {{')
    for hi in range(max_header+1):
        if hi not in decoded: lines.append(f'    [{hi}] = {{0}},'); continue
        h=hdrs[hi]; lines.append(f'    [{hi}] = {{{hi}, {h["width"]}, {h["height"]}, 0x{h["ctrl"]&0xff:02X}, px_{array_for[hi]}}},')
    lines += ['};','']
    maxpal=max(used_pals) if used_pals else 0
    lines.append(f'static wm_select_background_palette bg_palettes[{maxpal+1}] = {{')
    for pi in range(maxpal+1):
        if pi not in palettes: lines.append(f'    [{pi}] = {{0}},'); continue
        ident=pal_ident[pi]; lines.append(f'    [{pi}] = {{"{pal_names[pi]}", pal_{ident}_opaque, pal_{ident}_keyed, {len(palettes[pi])}}},')
    lines += ['};','',
        f'size_t {prefix}_image_count(void) {{ return sizeof(bg_images)/sizeof(bg_images[0]); }}',
        f'const wm_select_background_image *{prefix}_image_at(size_t i) {{',
        '    if (i >= sizeof(bg_images)/sizeof(bg_images[0]) || !bg_images[i].pixels_ci8) return 0;',
        '    return &bg_images[i];','}',
        f'size_t {prefix}_palette_count(void) {{ return sizeof(bg_palettes)/sizeof(bg_palettes[0]); }}',
        f'const wm_select_background_palette *{prefix}_palette_at(size_t i) {{',
        '    if (i >= sizeof(bg_palettes)/sizeof(bg_palettes[0]) || !bg_palettes[i].rgba5551_opaque) return 0;',
        '    return &bg_palettes[i];','}',
        f'const char *{prefix}_source_name(void) {{ return "WWF_PACKED_GFX"; }}',
        f'uint16_t {prefix}_source_origin_x(void) {{ return 0; }}',
        f'uint16_t {prefix}_source_origin_y(void) {{ return 0; }}','']
    out_path.parent.mkdir(parents=True,exist_ok=True); out_path.write_text('\n'.join(lines))
    print(f'{module_name}: {len(used_headers)} DMA headers, {len(used_pals)} source palettes -> {out_path}')
    for hi in used_headers:
        h=hdrs[hi]; print(f'  h{hi:02d} {h["width"]}x{h["height"]} bit=0x{h["address_bits"]:08X} ctrl=0x{h["ctrl"]:04X} max={max(decoded[hi]) if decoded[hi] else 0} sha256={hashlib.sha256(decoded[hi]).hexdigest()[:12]}')

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--romzip',required=True,type=pathlib.Path)
    ap.add_argument('--bgndtbl',required=True,type=pathlib.Path)
    ap.add_argument('--bgndpal',required=True,type=pathlib.Path)
    ap.add_argument('--imgpal',required=True,type=pathlib.Path)
    ap.add_argument('--out-main',required=True,type=pathlib.Path)
    ap.add_argument('--out-choice',required=True,type=pathlib.Path)
    ns=ap.parse_args()
    gfx=load_gfx_region(ns.romzip)
    print(f'WWF graphics ROM: verified 16 x 1MiB chips; assembled 32-bit interleaved region ({len(gfx)} bytes)')
    bt=ns.bgndtbl.read_text(errors='replace')
    bp=ns.bgndpal.read_text(errors='replace')
    ip=ns.imgpal.read_text(errors='replace')
    emit_module(gfx,bt,[bp,ip],'wwfselbkBMOD',ns.out_main,'wm_select_main')
    emit_module(gfx,bt,[bp,ip],'choiceBMOD',ns.out_choice,'wm_select_choice')
    return 0

if __name__=='__main__':
    try: raise SystemExit(main())
    except (OSError,ValueError,zipfile.BadZipFile) as e:
        print(f'select_background_bundle: error: {e}',file=sys.stderr); raise SystemExit(2)
