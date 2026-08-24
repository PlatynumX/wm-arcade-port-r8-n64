#!/usr/bin/env python3
from __future__ import annotations
import argparse, pathlib, re, sys
from collections import OrderedDict

CHARS=[(0,'bret','hrt'),(1,'razor','rzr'),(2,'taker','und'),(3,'yoko','yok'),(4,'shawn','shn'),(5,'bam','bam'),(6,'doink','dnk'),(8,'lex','lex')]
SUBR_RE=re.compile(r'^\s*SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b',re.I)
IMG_RE=re.compile(r'\b([A-Za-z][A-Za-z0-9_]*)\s*\+\s*FR(\d+)\b',re.I)
CMD_RE=re.compile(r'\bANI_([A-Za-z0-9_]+)\b',re.I)
PREFIX_BARRIER={
 'GOTO','IFSTATUS','IFNOTSTATUS','IFOPPMODE','IFBUTTONS','IFNOHITBLOCK','IFROPE','IFNOTROPE',
 'IFBLOCKED','IFOPP','IF_BUTCOUNT_GE','IF_BUTCOUNT_LT','IF_RPTCOUNT','IFNOT_RPTCOUNT',
 'IF_RPTCOUNT_GE','IF_RPTCOUNT_LT','RNDPER','CHANGEANIM','CHANGEANIM_TBL','REPEAT','END','LOOP',
 'WAITHITGND','WAITHITGND2','WAITRELEASE','WAITHITOPP','WAITHITANY','HMBWAIT','ROT','LEAPATOPP',
 'LEAPATPOS','SLIDE_BACK','SLIDEATOPP'
}

def canonical(p:pathlib.Path)->bool:
    n=p.name.lower(); return "'" not in p.name and '~' not in p.name and not n.endswith(('.bak.asm','.old.asm','.orig.asm','.tmp.asm'))

def frame_name(base,n): return f'{base.upper()}{int(n):02d}'
def ticks(tok):
    tok=tok.strip()
    if re.fullmatch(r'\d+',tok): return max(1,int(tok))
    m=re.fullmatch(r'(\d+)\s*\*\s*(\d+)',tok)
    return max(1,int(m.group(1))*int(m.group(2))) if m else 1

def parse_equates(root:pathlib.Path):
    raw={}
    for p in list(root.glob('*.EQU'))+list(root.glob('*.equ'))+list(root.glob('*.H'))+list(root.glob('*.h')):
        if not canonical(p): continue
        for rawline in p.read_text(errors='replace').splitlines():
            line=rawline.split(';',1)[0].strip()
            m=re.match(r'^([A-Za-z_][A-Za-z0-9_]*)\s+(?:equ|EQU)\s+(.+)$',line)
            if m: raw[m.group(1).upper()]=m.group(2).strip()
    vals={}; allowed=re.compile(r'^[0-9xXa-fA-F\s\+\-\*\/\(\)\|&<>~]+$')
    def ev(expr):
        x=expr.strip()
        x=re.sub(r'\b([0-9A-Fa-f]+)[hH]\b',lambda m:'0x'+m.group(1),x)
        x=x.replace('[','(').replace(']',')')
        for _ in range(6): x=re.sub(r'\b([A-Za-z_][A-Za-z0-9_]*)\b',lambda m:str(vals.get(m.group(1).upper(),m.group(0))),x)
        if not allowed.fullmatch(x): return None
        try:
            v=eval(x,{'__builtins__':None},{})
            return int(v) if isinstance(v,int) else None
        except Exception:return None
    for _ in range(40):
        added=0
        for k,v in raw.items():
            if k in vals: continue
            z=ev(v)
            if z is not None: vals[k]=z; added+=1
        if not added: break
    return vals,ev

def all_subrs(root):
    idx=OrderedDict()
    for p in sorted((x for x in root.glob('*.ASM') if canonical(x)),key=lambda x:x.name.lower()):
        ls=p.read_text(errors='replace').splitlines()
        for i,r in enumerate(ls):
            m=SUBR_RE.match(r.split(';',1)[0])
            if m: idx.setdefault(m.group(1).lower(),(p,i,ls))
    return idx

def split_args(code,cmd_end):
    rest=code[cmd_end:].lstrip(' ,\t')
    return [x.strip() for x in rest.split(',') if x.strip()]

def visual_seq(entry,owner_prefix,eval_expr):
    p,start,ls=entry; frames=[]; repeat=False; began=False; prefix_open=True
    init={'mask':0,'anim_mode':0,'player_mode':0,'friction':0,'speed':0x100,'xvel':0,'yvel':0,'zvel':0,'gravity':0}
    # ANIM.ASM control that must survive the visual flattening.  In particular,
    # knockdown sequences terminate in ANI_WAITROLL -> ANI_CHANGEANIM(getup).
    # Losing that pair leaves MODE_ONGROUND actors parked on their last frame.
    wait_roll_after_last=False; wait_getup_after_last=False; next_label=None; unsafe_tail=False
    for raw in ls[start+1:]:
        code=raw.split(';',1)[0]
        if SUBR_RE.match(code) and began: break
        cm=CMD_RE.search(code)
        if cm and began:
            op_tail=cm.group(1).upper()
            args_tail=split_args(code,cm.end())
            # Only preserve a straight-line terminal control chain. Conditional
            # branches stay unresolved rather than being guessed.
            if op_tail=='WAITROLL':
                wait_roll_after_last=True
            elif op_tail=='GETUP_WAIT':
                wait_getup_after_last=True
            elif op_tail=='CHANGEANIM' and not unsafe_tail and args_tail:
                next_label=args_tail[0].strip().lstrip('#').lower()
            elif op_tail in {'IFSTATUS','IFNOTSTATUS','IFOPPMODE','IFBUTTONS','IFNOHITBLOCK','IFROPE','IFNOTROPE','IFBLOCKED','IFOPP','IF_BUTCOUNT_GE','IF_BUTCOUNT_LT','IF_RPTCOUNT','IFNOT_RPTCOUNT','IF_RPTCOUNT_GE','IF_RPTCOUNT_LT','RNDPER','GOTO','CHANGEANIM_TBL'}:
                unsafe_tail=True
        if prefix_open and cm:
            op=cm.group(1).upper()
            if op in PREFIX_BARRIER: prefix_open=False
            else:
                args=split_args(code,cm.end())
                def val(i): return eval_expr(args[i]) if i<len(args) else None
                if op=='SETMODE' and val(0) is not None: init['anim_mode']=val(0)&0xffff; init['mask']|=1
                elif op=='SETPLYRMODE' and val(0) is not None: init['player_mode']=val(0)&0xffff; init['mask']|=2
                elif op=='FRICTION' and val(0) is not None: init['friction']=val(0); init['mask']|=4
                elif op=='SETSPEED' and val(0) is not None: init['speed']=val(0)&0xffff; init['mask']|=8
                elif op=='ZEROVELS': init['xvel']=init['yvel']=init['zvel']=0; init['mask']|=16|32|64
                elif op=='ZERO_XZVELS': init['xvel']=init['zvel']=0; init['mask']|=16|64
                elif op=='SET_YVEL' and val(0) is not None: init['yvel']=val(0); init['mask']|=32
                elif op=='GRAVITY_OFF': init['anim_mode']|=0x20; init['mask']|=1
                elif op=='GRAVITY_ON': init['anim_mode']&=~0x20; init['mask']|=1
        if re.search(r'\bANI_REPEAT\b',code,re.I): repeat=True
        m=IMG_RE.search(code)
        if not m: continue
        base=m.group(1).upper()
        if base[:1] != owner_prefix.upper(): continue
        prefix_open=False
        prefix=code[:m.start()].rstrip().rstrip(','); tok=prefix.rsplit(',',1)[-1].strip(); tok=re.sub(r'^\s*W+L\s+','',tok,flags=re.I)
        frames.append((frame_name(base,m.group(2)),ticks(tok))); began=True
        wait_roll_after_last=False; wait_getup_after_last=False; next_label=None; unsafe_tail=False
    flags=(1 if wait_roll_after_last else 0) | (2 if wait_getup_after_last else 0)
    if unsafe_tail: next_label=None
    return p.name,frames,repeat,init,next_label,flags

def enum_names(path,prefix):
    txt=path.read_text(errors='replace'); return list(OrderedDict.fromkeys(re.findall(r'\b('+re.escape(prefix)+r'[A-Z0-9_]+)\b',txt)))

def resolve_enum(idx,pfx,suffix,manual):
    if suffix in manual:return manual[suffix]
    s=suffix.lower();c=[f'{pfx}_{s}_anim']
    m=re.match(r'(.+)_([23468])$',s)
    if m:c.append(f'{pfx}_{m.group(2)}_{m.group(1)}_anim')
    m=re.match(r'(.+?)([23468])$',s)
    if m:c.extend((f'{pfx}_{m.group(2)}_{m.group(1)}_anim',f'{pfx}_{m.group(1)}{m.group(2)}_anim'))
    for x in c:
        if x.lower() in idx:return x.lower()
    return None

def table_at_label(root,label):
    wanted=label.lower().lstrip('#')
    for p in sorted(root.glob('*.ASM')):
        if not canonical(p):continue
        ls=p.read_text(errors='replace').splitlines(); start=None
        for i,raw in enumerate(ls):
            code=raw.split(';',1)[0].strip(); sm=SUBR_RE.match(code)
            if sm and sm.group(1).lower()==wanted: start=i+1;break
            if re.fullmatch(r'#?'+re.escape(wanted),code,re.I): start=i+1;break
        if start is None:continue
        vals=[]
        for raw in ls[start:]:
            code=raw.split(';',1)[0].strip()
            if not code:continue
            if SUBR_RE.match(code) or re.fullmatch(r'#[A-Za-z_]\w*',code):
                if vals:break
                continue
            if re.match(r'^\.ref\b',code,re.I):continue
            m=re.match(r'^(?:\.long|REFLONG)\s+(.+)$',code,re.I)
            if not m:
                if vals:break
                continue
            for tok in m.group(1).split(','):
                tok=tok.strip().lstrip('#');vals.append(None if tok in ('0','') else tok.lower())
        if vals:return vals
    return []

def table_roster_pairs(vals):
    out={}
    if len(vals)>=18:
        for rid,_,_ in CHARS:
            i=rid*2;out[rid]=(vals[i] if i<len(vals) else None,vals[i+1] if i+1<len(vals) else None)
    elif len(vals)>=9:
        for rid,_,_ in CHARS:out[rid]=(vals[rid] if rid<len(vals) else None,vals[rid] if rid<len(vals) else None)
    return out

def manual_react_pair(group,rid,pfx):
    if group==9:return (f'{pfx}_4_losebal_anim',)*2
    if group==10:return (f'{pfx}_knockdwn_anim',)*2
    if group==12:return (f'{pfx}_quick_knee_hit_anim',)*2
    if group==13:
        base='head_hit3' if rid in (0,1,3,4,8) else 'head_hit2';return (f'{pfx}_2_{base}_anim',f'{pfx}_4_{base}_anim')
    if group==14:return (f'{pfx}_fall_back2_anim',)*2
    if group==15:
        base='head_hit3' if rid==4 else 'head_hit2';return (f'{pfx}_2_{base}_anim',f'{pfx}_4_{base}_anim')
    if group==16:return (f'{pfx}_2_bncoff_anim',f'{pfx}_4_bncoff_anim')
    if group==17:return (f'{pfx}_4_bncoff_dizzy_anim',)*2
    if group==18:
        base='head_hit3' if rid in (0,1,3,4,8) else 'head_hit2';return (f'{pfx}_2_{base}_anim',f'{pfx}_4_{base}_anim')
    if group==19:return (f'{pfx}_4_head_hit4_anim',)*2
    if group==20:return (f'{pfx}_get_buzz_anim',)*2
    if group==21:return (None,None)
    return None

def emit(root,out_c,out_h,bret_h,razor_h):
    idx=all_subrs(root);_,eval_expr=parse_equates(root);prefix_first={'hrt':'H','rzr':'R','und':'U','yok':'Y','shn':'S','bam':'B','dnk':'D','lex':'L'};defs=[]
    for rid,name,pfx in CHARS:
        for lab,e in idx.items():
            if not lab.startswith(pfx+'_'):continue
            src,fr,rep,init,next_label,control_flags=visual_seq(e,prefix_first[pfx],eval_expr)
            if fr:defs.append((rid,lab,src,fr,rep,init,next_label,control_flags))
    bh_manual={'START_RUN':'start_run_anim','HH_DDT2':'hrt_hh_2_ddt_anim','GRABFLING_FACE24':None,'FINISH1':'hrt_finish1_move','FINISH2':'hrt_finish2_move'}
    rh_manual={'START_RUN':'start_run_anim','UPPERCUT4':'rzr_4_uprcut_anim','FINISH1':'rzr_finish1_move','FINISH2':'rzr_finish2_move'}
    bmap=[]
    for tok in enum_names(bret_h,'WM_BRET_ANIM_'):
        suf=tok[len('WM_BRET_ANIM_'):];bmap.append((tok,None if suf=='NONE' else resolve_enum(idx,'hrt',suf,bh_manual)))
    rmap=[]
    for tok in enum_names(razor_h,'WM_RZR_ANIM_'):
        suf=tok[len('WM_RZR_ANIM_'):];rmap.append((tok,None if suf=='NONE' else resolve_enum(idx,'rzr',suf,rh_manual)))
    reaction_tables={1:'hitblock_tbl',2:'hitblock_flail_tbl',3:'head_hit_tbl',4:'head_hit2_tbl',5:'body_hit_tbl',6:'fall_back_tbl',7:'hitonground_tbl',8:'fall_back_tbukl_tbl',11:'knee_hit_tbl',22:'burn_tbl',23:'head_hit2_sand_tbl',24:'body_hit2_tbl'}
    reacts={g:table_roster_pairs(table_at_label(root,t)) for g,t in reaction_tables.items()}
    for g in range(9,22):
        if g==11:continue
        reacts[g]={rid:manual_react_pair(g,rid,pfx) for rid,_,pfx in CHARS if manual_react_pair(g,rid,pfx)}
    h='''#ifndef WM_ARCADE_SOURCE_ANIMATION_CATALOG_H\n#define WM_ARCADE_SOURCE_ANIMATION_CATALOG_H\n#include <stddef.h>\n#include <stdint.h>\ntypedef struct { const char *source_frame; uint16_t ticks; } wm_source_anim_frame_t;\nenum { WM_SRC_ANIM_INIT_ANIM_MODE=1u<<0, WM_SRC_ANIM_INIT_PLAYER_MODE=1u<<1, WM_SRC_ANIM_INIT_FRICTION=1u<<2, WM_SRC_ANIM_INIT_SPEED=1u<<3, WM_SRC_ANIM_INIT_XVEL=1u<<4, WM_SRC_ANIM_INIT_YVEL=1u<<5, WM_SRC_ANIM_INIT_ZVEL=1u<<6, WM_SRC_ANIM_INIT_GRAVITY=1u<<7 };\ntypedef struct { uint16_t mask,anim_mode,player_mode,speed; int32_t friction,xvel,yvel,zvel,gravity; } wm_source_anim_init_t;\nenum { WM_SRC_ANIM_CTRL_WAITROLL=1u<<0, WM_SRC_ANIM_CTRL_GETUP_WAIT=1u<<1 };\ntypedef struct { uint8_t roster_id; const char *label; const char *source_file; const wm_source_anim_frame_t *frames; uint16_t frame_count; uint8_t repeat; wm_source_anim_init_t init; const char *next_label; uint8_t control_flags; } wm_source_anim_def_t;\nconst wm_source_anim_def_t *wm_source_anim_find(uint8_t roster_id,const char *label);\nconst char *wm_source_bret_anim_label(int id);\nconst char *wm_source_razor_anim_label(int id);\nconst char *wm_source_reaction_anim_label(uint8_t roster_id,int group,int facing_dir);\n#endif\n'''
    out_h.parent.mkdir(parents=True,exist_ok=True);out_h.write_text(h)
    c=['/* Generated directly from canonical Midway wrestler sequence and REACT tables. */','#include "wm_arcade_source_animation_catalog.h"','#include "wm_arcade_bret.h"','#include "wm_arcade_razor.h"','#include "wm_arcade_combat_defs.h"','#include <string.h>',''];syms=[]
    for n,(rid,lab,src,fr,rep,ini,next_label,control_flags) in enumerate(defs):
        s=f'afr_{n}';d=f'ad_{n}';syms.append(d);c.append(f'static const wm_source_anim_frame_t {s}[]={{')
        for nm,tk in fr:c.append(f'{{"{nm}",{tk}u}},')
        c.append('};')
        nl=('"'+next_label+'"') if next_label else '0'
        init_vals=(ini["mask"],ini["anim_mode"],ini["player_mode"],ini["speed"],ini["friction"],ini["xvel"],ini["yvel"],ini["zvel"],ini["gravity"])
        c.append('static const wm_source_anim_def_t %s={%du,"%s","%s",%s,%du,%du,{%du,%du,%du,%du,%d,%d,%d,%d,%d},%s,%du};' % ((d,rid,lab,src,s,len(fr),1 if rep else 0)+init_vals+(nl,control_flags)))
    c.append('static const wm_source_anim_def_t *const defs[]={'+','.join('&'+s for s in syms)+'};')
    c.append('const wm_source_anim_def_t *wm_source_anim_find(uint8_t r,const char *l){if(!l)return 0;for(size_t i=0;i<sizeof(defs)/sizeof(defs[0]);++i)if(defs[i]->roster_id==r&&!strcmp(defs[i]->label,l))return defs[i];return 0;}')
    c.append('const char *wm_source_bret_anim_label(int id){switch(id){')
    for tok,lab in bmap:
        if lab:c.append(f'case {tok}: return "{lab}";')
    c.append('default:return 0;}}');c.append('const char *wm_source_razor_anim_label(int id){switch(id){')
    for tok,lab in rmap:
        if lab:c.append(f'case {tok}: return "{lab}";')
    c.append('default:return 0;}}');c.append('const char *wm_source_reaction_anim_label(uint8_t r,int g,int f){const char *a=0,*b=0;switch(g){')
    for group,tab in sorted(reacts.items()):
        if not tab:continue
        c.append(f'case {group}: switch(r){{')
        for rid,_,_ in CHARS:
            pair=tab.get(rid)
            if pair and (pair[0] or pair[1]):
                aa=pair[0] or pair[1];bb=pair[1] or pair[0];c.append(f'case {rid}:a="{aa}";b="{bb}";break;')
        c.append('default:break;}break;')
    c.append('default:break;}if(!a)return 0;return (f & WM_MOVE_UP)?a:b;}')
    out_c.parent.mkdir(parents=True,exist_ok=True);out_c.write_text('\n'.join(c)+'\n')
    missing_b=[x for x,l in bmap if x not in ('WM_BRET_ANIM_NONE','WM_BRET_ANIM_GRABFLING_FACE24') and not l];missing_r=[x for x,l in rmap if x!='WM_RZR_ANIM_NONE' and not l]
    print(f'generated source animation catalog: {len(defs)} visual sequences; bret unresolved={len(missing_b)} razor unresolved={len(missing_r)}')
    if missing_b or missing_r:print('unresolved:',missing_b+missing_r,file=sys.stderr);raise SystemExit(2)

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--root',type=pathlib.Path,required=True);ap.add_argument('--out-c',type=pathlib.Path,required=True);ap.add_argument('--out-h',type=pathlib.Path,required=True);ap.add_argument('--bret-h',type=pathlib.Path,required=True);ap.add_argument('--razor-h',type=pathlib.Path,required=True);a=ap.parse_args();emit(a.root,a.out_c,a.out_h,a.bret_h,a.razor_h)
if __name__=='__main__':main()
