#!/usr/bin/env python3
from __future__ import annotations
import argparse, pathlib, re, sys
from dataclasses import dataclass
from collections import OrderedDict
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
wlanim = bret_manifest = wimpimg = None

CHARS = [
 (0,'bret','hrt'), (1,'razor','rzr'), (2,'taker','und'), (3,'yoko','yok'),
 (4,'shawn','shn'), (5,'bam','bam'), (6,'doink','dnk'), (8,'lex','lex')]
ROSTER_IDS={name:rid for rid,name,_ in CHARS}
ART_PREFIX={'bret':'H','razor':'R','taker':'U','yoko':'Y','shawn':'S','bam':'B','doink':'D','lex':'L'}
WRESTLER_PREFIXES=frozenset(ART_PREFIX.values())
SUBR_RE=re.compile(r'^\s*SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b',re.I)

def _is_canonical_asm(path:pathlib.Path):
    # The historical tree contains editor/backup variants such as LEXSEQ1'.ASM.
    # Those can carry stale animation bodies and must never outrank the canonical
    # source file.  Reject common backup-marker names at discovery time.
    name=path.name
    lower=name.lower()
    return ("'" not in name and "~" not in name and
            not lower.endswith(('.bak.asm','.old.asm','.orig.asm','.tmp.asm')))

def source_index(root:pathlib.Path):
    idx={}
    # Sort deterministically, then index only canonical ASM sources. This avoids
    # filesystem-order-dependent selection of stale backup copies.
    paths=sorted((p for p in root.glob('*.ASM') if _is_canonical_asm(p)),
                 key=lambda p:p.name.lower())
    for p in paths:
        try: lines=p.read_text(errors='replace').splitlines()
        except OSError: continue
        for raw in lines:
            m=SUBR_RE.match(raw.split(';',1)[0])
            if m: idx.setdefault(m.group(1).lower(),p)
    return idx

def choose(idx, *labels):
    for lab in labels:
        if lab and lab.lower() in idx: return lab.lower()
    return None

def wanted_labels(name,pfx,idx):
    # Basic attract presenter set. Candidates follow the exact labels used by the
    # direct wrestler ports; fallback choices are character-specific, never Bret.
    d={
      'stand2':choose(idx,f'{pfx}_stand2_anim'), 'stand4':choose(idx,f'{pfx}_stand4_anim'),
      'torso2':choose(idx,f'{pfx}_torso2_anim'), 'torso4':choose(idx,f'{pfx}_torso4_anim'),
      'walk2':choose(idx,f'{pfx}_walk2_f2_anim'), 'walk8':choose(idx,f'{pfx}_walk8_f2_anim'),
      'walk4':choose(idx,f'{pfx}_walk4_f4_anim'), 'walk6':choose(idx,f'{pfx}_walk6_f4_anim'),
      'run':choose(idx,f'{pfx}_run_anim',f'{pfx}_run2_anim'),
      'lp2':choose(idx,f'{pfx}_2_punch_anim'), 'lp4':choose(idx,f'{pfx}_4_punch_anim'),
      'lk2':choose(idx,f'{pfx}_2_kick_anim'), 'lk4':choose(idx,f'{pfx}_4_kick_anim'),
    }
    specials={
      'bret':(('hrt_4_super_punch_anim',),('hrt_2_super_kick_anim',)),
      'razor':(('rzr_4_uppercut_anim','rzr_4_pummel_anim'),('rzr_4_super_kick_anim','rzr_flying_kick_anim')),
      'taker':(('und_4_uppercut_anim','und_4_butts_anim'),('und_4_kick_anim','und_flying_kick_anim')),
      'yoko':(('yok_4_slap2_anim','yok_4_jabs_anim'),('yok_4_kick_anim','yok_4_run_slap_anim')),
      'shawn':(('shn_4_pummel_anim','shn_4_falling_punch_anim'),('shn_4_jump_kick_anim','shn_flying_kick_anim')),
      'bam':(('bam_4_slap_anim','bam_4_butts_anim','bam_4_fpunch_anim'),('bam_4_jumpkick_anim','bam_flying_kick_anim')),
      'doink':(('dnk_4_slap_anim','dnk_4_butts_anim'),('dnk_4_kick_anim','dnk_flying_kick_anim')),
      'lex':(('lex_4_clobber_anim','lex_4_ground_punch_anim'),('lex_4_kick_anim','lex_flying_kick_anim')),
    }
    pp,pk=specials[name]; d['pp']=choose(idx,*pp); d['pk']=choose(idx,*pk)
    missing=[k for k,v in d.items() if not v]
    if missing: raise ValueError(f'{name}: missing source labels: {", ".join(missing)}')
    return d

@dataclass(frozen=True)
class _VisualFrame:
    name: str
    ticks: int

@dataclass(frozen=True)
class _VisualSequence:
    label: str
    frames: tuple
    repeat: bool

def _frame_name(base, n):
    # Historical image labels are table bases; +FRn indexes the nth WIMP frame.
    # The on-disk directory names use a two-digit suffix (e.g. L2ST2C07).
    return f"{base}{int(n):02d}"

def _parse_ticks(token):
    token=token.strip()
    # Selected presenter sequences use literal decimal ticks (occasionally a
    # simple product such as 60*60).  Do not eval arbitrary source text.
    if re.fullmatch(r'\d+', token):
        return int(token)
    m=re.fullmatch(r'(\d+)\s*\*\s*(\d+)', token)
    if m:
        return int(m.group(1))*int(m.group(2))
    return None

def extract_seq(idx,label,repeat=None,owner_name=None):
    """Extract visual frames directly from the historical ASM source.

    The old generic wlanim helper can overrun aliased SUBR blocks and turn
    Lex's L2ST2C/L4ST4C stance references into synthetic frames that do not
    exist (e.g. L2ST2C08..12 / L4ST4A05..11).  For the character presenter we
    only need the literal WIMP references in the selected subroutine, so parse
    those source tokens exactly and stop at ANI_REPEAT/ANI_END.
    """
    path=idx[label.lower()]
    lines=path.read_text(errors='replace').splitlines()
    start=None
    for i,raw in enumerate(lines):
        m=SUBR_RE.match(raw.split(';',1)[0])
        if m and m.group(1).lower()==label.lower():
            start=i+1; break
    if start is None:
        raise ValueError(f'{label}: SUBR not found in {path.name}')
    frames=[]; body=False; saw_repeat=False
    img_re=re.compile(r'\b([A-Za-z][A-Za-z0-9_]*)\s*\+\s*FR(\d+)\b',re.I)
    for raw in lines[start:]:
        code=raw.split(';',1)[0]
        sm=SUBR_RE.match(code)
        if sm:
            # Consecutive SUBR labels are aliases for one body.  Once visual
            # source has begun, a new SUBR starts the next routine.
            if body:
                break
            continue
        if re.search(r'\bANI_REPEAT\b',code,re.I):
            saw_repeat=True; break
        if re.search(r'\bANI_END\b',code,re.I):
            break
        fm=img_re.search(code)
        if not fm:
            continue
        # The display duration is the numeric argument immediately before the
        # image expression, regardless of WL/WWL/WWWL macro width.
        prefix=code[:fm.start()].rstrip().rstrip(',')
        ticktok=prefix.rsplit(',',1)[-1].strip()
        # Strip the WL-family macro from simple two-argument forms.
        ticktok=re.sub(r'^\s*W+L\s+','',ticktok,flags=re.I)
        ticks=_parse_ticks(ticktok)
        if ticks is None:
            # Control forms can place a non-duration before an image. Those are
            # not ordinary presenter frames; retain a conservative 1 tick.
            ticks=1
        base=fm.group(1).upper()
        # A character animation can embed puppet/opponent WIMP references for
        # superslaves, victims, or setup helpers. Those are not frames owned by
        # the actor presenter. Keep generic/non-wrestler art, but discard a
        # foreign wrestler prefix (H/R/U/Y/S/B/D/L) when owner_name is known.
        if owner_name:
            owner_prefix=ART_PREFIX[owner_name]
            if base[:1] in WRESTLER_PREFIXES and base[:1] != owner_prefix:
                continue
        frames.append(_VisualFrame(_frame_name(base,fm.group(2)),ticks))
        body=True
    if not frames:
        raise ValueError(f'{label}: no visual WIMP frames parsed from {path.name}')
    rep=saw_repeat if repeat is None else bool(repeat)
    return path,_VisualSequence(label,tuple(frames),rep)

def rgba5551(rgb555,index):
    if index==0:return 0
    return (((rgb555>>10)&31)<<11)|(((rgb555>>5)&31)<<6)|((rgb555&31)<<1)|1

def resolve_case(directory,filename):
    key=filename.lower()
    for p in directory.iterdir():
        if p.is_file() and p.name.lower()==key:return p
    raise ValueError(f'container not found: {filename}')

def _scan_img_fallback(root, wanted):
    """Resolve frames that are absent from stale/incomplete .LOD text manifests.

    The packed WIMP .IMG files are the ground truth. Some historical sequence
    sources reference frames that are physically present in a wrestler IMG but
    omitted from the checked-in .LOD text. Only scan as a fallback, and only
    accept an exact image-directory name match; never alias/fabricate a frame.
    """
    wanted_upper={f.upper():f for f in wanted}
    found={}
    imgdir=root/'IMG'
    for path in sorted(imgdir.iterdir(), key=lambda p:p.name.lower()):
        if not path.is_file() or path.suffix.lower()!='.img':
            continue
        try:
            _data,_hdr,imgs,_pals=wimpimg.parse_file(path)
        except Exception:
            continue
        for im in imgs:
            key=im.name.upper()
            if key in wanted_upper and wanted_upper[key] not in found:
                found[wanted_upper[key]]=path.name
        if len(found)==len(wanted):
            break
    return found


def _available_frame_names(root):
    """Return exact physical WIMP frame names present in LOD indexes or packed IMG files.

    Historical ASM can contain stale/alternate image references that are not present in
    the checked-in WIMP payload.  The N64 presenter must never fabricate those frames.
    """
    names=set()
    loddir=root/'IMG'
    for lod in sorted(set(loddir.glob('*.LOD')) | set(loddir.glob('*.lod')), key=lambda p:p.name.lower()):
        try:
            m=bret_manifest.parse_lod(lod)
        except Exception:
            continue
        names.update(k.upper() for k in m)
    for path in sorted(loddir.iterdir(), key=lambda p:p.name.lower()):
        if not path.is_file() or path.suffix.lower()!='.img':
            continue
        try:
            _data,_hdr,imgs,_pals=wimpimg.parse_file(path)
        except Exception:
            continue
        names.update(im.name.upper() for im in imgs)
    return names

def _filter_sequence_to_physical_assets(seq, available, owner, slot, path):
    kept=tuple(fr for fr in seq.frames if fr.name.upper() in available)
    dropped=[fr.name for fr in seq.frames if fr.name.upper() not in available]
    if dropped:
        print(f'fix39_character_assets: note {owner}:{slot} ignoring nonphysical ASM WIMP refs from {path.name}: '+', '.join(dropped), file=sys.stderr)
    if not kept:
        raise ValueError(f'{owner}:{slot}: all visual frames from {path.name}:{seq.label} are absent from physical WIMP assets')
    return _VisualSequence(seq.label,kept,seq.repeat)

def find_lod(root,frames):
    # Historical wrestler LOD manifests live under IMG/, not the source root.
    # They are useful indexes, but the checked-in manifests are not perfectly
    # complete. Build their union first, then verify unresolved names directly
    # against the packed .IMG directory (the actual source-of-truth payload).
    loddir = root / 'IMG'
    lods=sorted(set(loddir.glob('*.LOD')) | set(loddir.glob('*.lod')), key=lambda p:p.name.lower())
    merged={}
    owners={}
    parsed=0
    for lod in lods:
        try:m=bret_manifest.parse_lod(lod)
        except Exception:
            continue
        parsed += 1
        for frame,container in m.items():
            if frame not in merged:
                merged[frame]=container
                owners[frame]=lod
    missing=[f for f in frames if f not in merged]
    if missing:
        recovered=_scan_img_fallback(root,missing)
        for frame,container in recovered.items():
            merged[frame]=container
            owners[frame]=loddir/container
        missing=[f for f in frames if f not in merged]
    if missing:
        preview=', '.join(missing)
        raise ValueError(f'asset lookup missing {len(missing)}/{len(frames)} frames after {parsed} LOD manifests + direct IMG scan: {preview}')
    used=sorted({owners[f] for f in frames}, key=lambda p:p.name.lower())
    return used,merged

def emit(root,out_c,out_h):
    global wlanim, bret_manifest, wimpimg
    import wlanim as _wlanim, bret_manifest as _bret_manifest, wimpimg as _wimpimg
    wlanim, bret_manifest, wimpimg = _wlanim, _bret_manifest, _wimpimg
    idx=source_index(root); imgdir=root/'IMG'
    available=_available_frame_names(root)
    charseq={}; allframes={}
    slots=['stand2','stand4','torso2','torso4','walk2','walk8','walk4','walk6','run','lp2','lp4','pp','lk2','lk4','pk']
    for rid,name,pfx in CHARS:
        labels=wanted_labels(name,pfx,idx); seqs={}; frames=[]
        for slot in slots:
            lab=labels[slot]
            rep=True if slot in ('stand2','stand4','torso2','torso4','walk2','walk8','walk4','walk6','run') else False
            path,seq=extract_seq(idx,lab,rep,owner_name=name)
            seq=_filter_sequence_to_physical_assets(seq,available,name,slot,path)
            seqs[slot]=(path,seq)
            for fr in seq.frames:
                if fr.name not in frames:frames.append(fr.name)
        charseq[name]=seqs; allframes[name]=frames

    h=['#ifndef WM_CHARACTER_ASSETS_H','#define WM_CHARACTER_ASSETS_H','#include <stddef.h>','#include <stdint.h>','#include "wm/visual.h"','#include "wm/bret_sprites.h"','typedef enum wm_character_visual_slot { WM_CV_STAND2,WM_CV_STAND4,WM_CV_TORSO2,WM_CV_TORSO4,WM_CV_WALK2,WM_CV_WALK8,WM_CV_WALK4,WM_CV_WALK6,WM_CV_RUN,WM_CV_LP2,WM_CV_LP4,WM_CV_PP,WM_CV_LK2,WM_CV_LK4,WM_CV_PK,WM_CV_COUNT } wm_character_visual_slot;','const wm_visual_sequence *wm_character_visual(uint8_t roster_id, wm_character_visual_slot slot);','const wm_source_sprite *wm_character_sprite_find(uint8_t roster_id,const char *source_frame);','const wm_source_sprite *wm_character_base_sprite(uint8_t roster_id);','size_t wm_character_sprite_count(uint8_t roster_id);','#endif','']
    out_h.parent.mkdir(parents=True,exist_ok=True); out_h.write_text('\n'.join(h))
    c=['/* Auto-generated from original Midway wrestler ASM/WIMP data. */','#include "wm/character_assets.h"','#include <string.h>','']
    seqsym={}
    for rid,name,pfx in CHARS:
        syms=[]
        for slot in slots:
            path,seq=charseq[name][slot]; arr=f'{name}_{slot}_frames'; sym=f'cv_{name}_{slot}'
            c.append(f'static const wm_visual_frame {arr}[] = {{')
            for fr in seq.frames:c.append(f'    {{"{fr.name}",{fr.ticks}}},')
            c += ['};',f'static const wm_visual_sequence {sym} = {{"{path.name}","{seq.label}",{arr},sizeof({arr})/sizeof({arr}[0]),'+('true' if seq.repeat else 'false')+'};','']
            syms.append(sym)
        seqsym[name]=syms

    bases={}; sprite_arrays={}; counts={}
    for rid,name,pfx in CHARS:
        frames=allframes[name]; lod,mapping=find_lod(root,frames)
        containers=OrderedDict(); resolved=[]
        for frame in frames:
            cont=mapping[frame]
            if cont not in containers:
                path=resolve_case(imgdir,cont); data,_hdr,imgs,pals=wimpimg.parse_file(path); containers[cont]=(path,data,imgs,pals)
            path,data,imgs,pals=containers[cont]; im=next((x for x in imgs if x.name.upper()==frame),None)
            if not im:raise ValueError(f'{name}:{frame} absent from {path.name}')
            pal=wimpimg.palette_for_image(im,imgs,pals); resolved.append((frame,cont,data,im,pal))
        pal_syms={}
        for cont,(path,data,imgs,pals) in containers.items():
            for pal in pals:
                if not any(cc==cont and pp.directory_offset==pal.directory_offset for _,cc,_,_,pp in resolved):continue
                ps=f'pal_{name}_{re.sub("[^a-z0-9_]","_",pal.name.lower())}_{pal.directory_offset:x}';pal_syms[(cont,pal.directory_offset)]=ps
                vals=[rgba5551(v,i) for i,v in enumerate(wimpimg.read_palette_words(data,pal))]
                c.append(f'static uint16_t {ps}[] __attribute__((aligned(8)))={{'+','.join(f'0x{x:04X}' for x in vals)+'};')
        for frame,cont,data,im,pal in resolved:
            px=f'px_{name}_{frame.lower()}'; vals=wimpimg.read_ci8(data,im)
            c.append(f'static const uint8_t {px}[] __attribute__((aligned(8)))={{'+','.join(f'0x{x:02X}' for x in vals)+'};')
        arr=f'sprites_{name}'; sprite_arrays[name]=arr; counts[name]=len(resolved)
        c.append(f'static const wm_source_sprite {arr}[]={{')
        for frame,cont,data,im,pal in resolved:
            ps=pal_syms[(cont,pal.directory_offset)]; px=f'px_{name}_{frame.lower()}'
            c.append(f'{{"{frame}","{cont}",{im.width},{im.height},{im.xani},{im.yani},{{'+','.join(str(v) for v in im.tail_words)+f'}},{px},{ps},{pal.color_count}}},')
        c += ['};','']
        bases[name]=charseq[name]['stand4'][1].frames[0].name
    c.append('const wm_visual_sequence *wm_character_visual(uint8_t id,wm_character_visual_slot slot){if((unsigned)slot>=WM_CV_COUNT)return 0;switch(id){')
    for rid,name,pfx in CHARS:c.append(f'case {rid}:{{static const wm_visual_sequence *const t[WM_CV_COUNT]={{'+','.join('&'+s for s in seqsym[name])+'};return t[slot];}')
    c += ['default:return 0;}}','const wm_source_sprite *wm_character_sprite_find(uint8_t id,const char *f){if(!f)return 0;const wm_source_sprite *a=0;size_t n=0;switch(id){']
    for rid,name,pfx in CHARS:c.append(f'case {rid}:a={sprite_arrays[name]};n=sizeof({sprite_arrays[name]})/sizeof({sprite_arrays[name]}[0]);break;')
    c += ['default:return 0;}for(size_t i=0;i<n;i++)if(strcmp(a[i].source_frame,f)==0)return &a[i];return 0;}','const wm_source_sprite *wm_character_base_sprite(uint8_t id){switch(id){']
    for rid,name,pfx in CHARS:c.append(f'case {rid}:return wm_character_sprite_find(id,"{bases[name]}");')
    c += ['default:return 0;}}','size_t wm_character_sprite_count(uint8_t id){switch(id){']
    for rid,name,pfx in CHARS:c.append(f'case {rid}:return sizeof({sprite_arrays[name]})/sizeof({sprite_arrays[name]}[0]);')
    c += ['default:return 0;}}','']
    out_c.parent.mkdir(parents=True,exist_ok=True);out_c.write_text('\n'.join(c))
    print('generated character assets:',', '.join(f'{n}={counts[n]}' for _,n,_ in CHARS))

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--root',type=pathlib.Path);ap.add_argument('--out-c',type=pathlib.Path);ap.add_argument('--out-h',type=pathlib.Path);ap.add_argument('--self-test',action='store_true');a=ap.parse_args()
 if a.self_test:
  assert ROSTER_IDS['lex']==8 and len(CHARS)==8; print('Fix39 character asset generator self-test: PASS');return
 if not a.root or not a.out_c or not a.out_h:ap.error('--root --out-c --out-h required')
 emit(a.root,a.out_c,a.out_h)
if __name__=='__main__':
 try:main()
 except (OSError,ValueError) as e:print('fix39_character_assets: error:',e,file=sys.stderr);raise SystemExit(2)
