#!/usr/bin/env python3
"""Generate the PROGRESS.ASM temporary-wrestler visual bank from original source.

This is intentionally narrow: it compiles only the animation channels that
PROGRESS.ASM::start_them_doing_stuff selects for standing, running, waiting,
clever, taunting and dead temporary wrestlers.  Fix27 also preserves the
cumulative ANI_OFFSET state needed by progression-only choreography.  Frames are pulled from the
original WIMP .IMG containers and the object palettes come from the exact
wrestler_pal table named by PROGRESS.ASM.
"""
from __future__ import annotations
import argparse
import pathlib
import re
import sys
from collections import OrderedDict

import wimpimg

WRESTLERS = [
    (0, "hrt", "HRTPNK_P"),
    (1, "rzr", "RZRGRN_P"),
    (2, "und", "UNDBLK_P"),
    (3, "yok", "YOKRED_P"),
    (4, "shn", "SHNRED_P"),
    (5, "bam", "BAMBLU_P"),
    (6, "dnk", "DNKBLU_P"),
    (8, "lex", "LEXWHT_P"),
]

# PROGRESS.ASM::WHICH_WRESTLER_IMAGE is the canonical object-image table used
# when CREATE_OTHER_BODIES creates progression wrestlers.  Some external
# wrestler_pal symbols are missing from the surviving IMGPAL.ASM source dump.
# When that happens, recover the object palette from the embedded WIMP palette
# on the exact source image named by this table rather than inventing/aliasing
# colors.
PROGRESS_STAND_IMAGE = {
    0: "H4ST4A02",
    1: "RAZOR_STAND",
    2: "TAKER_STAND",
    3: "YOKO_STAND",
    4: "SHAWN_STAND",
    5: "BAM_STAND",
    6: "DOINK_STAND",
    8: "LEX_STAND",
}

ACTIONS = [
    ("STAND", "stand4_anim"),
    ("RUN", "run2_anim"),
    ("WAIT", "wait_anim"),
    ("CLEVER", "clever_anim"),
    ("TAUNT", "taunt4_anim"),
    ("DEAD", "dizzy_anim"),
]

SUBR_RE = re.compile(r"^\s*SUBR[P]?\s+#?([A-Za-z_][A-Za-z0-9_]*)", re.I)
BARE_LABEL_RE = re.compile(r"^\s*(#?[A-Za-z_][A-Za-z0-9_]*)\s*$")
WL_RE = re.compile(r"^\s*WL\s+([^,]+)\s*,\s*([^\s,]+)", re.I)
WORD_RE = re.compile(r"^\s*\.word\s+(.+)$", re.I)
FR_RE = re.compile(r"^([A-Za-z0-9_]+)\s*\+\s*FR\s*(\d+)$", re.I)
HEX_RE = re.compile(r"(?i)\b([0-9A-F]+)h\b")


def strip_code(raw: str) -> str:
    return raw.split(";", 1)[0].strip()


def asm_int(expr: str) -> int:
    e = expr.strip()
    e = HEX_RE.sub(lambda m: str(int(m.group(1), 16)), e)
    e = re.sub(r"(?i)\bTSEC\b", "53", e)
    if not re.fullmatch(r"[0-9+\-*/() \t]+", e):
        raise ValueError(f"unsupported delay expression: {expr}")
    # Assembly integer expressions here are non-negative and use simple math.
    return max(1, int(eval(e, {"__builtins__": None}, {})))


def asm_signed_int(expr: str) -> int:
    """Parse the signed integer operands used by ANI_OFFSET.

    Progression's live action tables only use ordinary decimal/hex integer
    offsets (not runtime expressions).  Keep this separate from delay parsing
    because zero and negative values are meaningful here.
    """
    e = expr.strip()
    e = HEX_RE.sub(lambda m: str(int(m.group(1), 16)), e)
    if not re.fullmatch(r"[0-9+\-*/() \t]+", e):
        raise ValueError(f"unsupported signed animation operand: {expr}")
    return int(eval(e, {"__builtins__": None}, {}))


def frame_name(expr: str) -> str:
    e = expr.strip().lstrip("#")
    m = FR_RE.match(e)
    if m:
        return f"{m.group(1).upper()}{int(m.group(2)):02d}"
    if re.fullmatch(r"[A-Za-z][A-Za-z0-9_]*", e):
        return e.upper()
    raise ValueError(f"unsupported frame expression: {expr}")


class SourceFile:
    def __init__(self, path: pathlib.Path):
        self.path = path
        self.lines = path.read_text(errors="replace").splitlines()
        self.global_labels: dict[str, int] = {}
        self.local_labels: dict[str, int] = {}
        for i, raw in enumerate(self.lines):
            code = strip_code(raw)
            m = SUBR_RE.match(code)
            if m:
                self.global_labels[m.group(1).lower()] = i
                continue
            m = BARE_LABEL_RE.match(code)
            if m:
                name = m.group(1)
                if name.startswith("#"):
                    self.local_labels[name.lower()] = i
                else:
                    self.global_labels.setdefault(name.lower(), i)


def load_sources(source_dir: pathlib.Path):
    files = []
    global_index = {}
    for path in sorted(source_dir.glob("*.ASM")):
        # The historical source dump contains archival alternates such as
        # LEXSEQ1'.ASM alongside the production LEXSEQ1.ASM.  Those files
        # deliberately define the same global animation labels, but with older
        # frame dependencies.  They were not part of the shipped build and must
        # not win symbol resolution merely because an apostrophe sorts before a
        # period.  Ignore these archival quote-suffixed source snapshots.
        if "'" in path.name:
            continue
        try:
            sf = SourceFile(path)
        except OSError:
            continue
        files.append(sf)
        for name, line in sf.global_labels.items():
            global_index.setdefault(name, (sf, line))
    return files, global_index


def compile_anim(global_index, label: str):
    key = label.lower()
    if key not in global_index:
        raise ValueError(f"source animation label not found: {label}")
    sf, start = global_index[key]
    pc = start + 1
    frames = []
    pc_seen = {}
    xflip = False
    offset_x = 0
    offset_y = 0
    offset_z = 0
    repeat = False
    loop_start = 0
    repeat_base = 0
    steps = 0
    saw_frame = False

    while 0 <= pc < len(sf.lines) and steps < 2000:
        steps += 1
        # Natural control-flow reconvergence is legal (run2 enters midway,
        # later falls through that same point after lp1).  A loop closes only
        # when an explicit ANI_GOTO targets an instruction already visited.
        # Keep the first frame index associated with each instruction.
        pc_seen.setdefault(pc, len(frames))
        code = strip_code(sf.lines[pc])
        if not code:
            pc += 1; continue

        # Another global routine after data marks a natural end. Consecutive
        # SUBR aliases before the first frame share one body and are allowed.
        sm = SUBR_RE.match(code)
        if sm:
            if saw_frame:
                break
            pc += 1; continue

        wm = WORD_RE.match(code)
        if wm:
            vals = [v.strip() for v in wm.group(1).split(',')]
            op = vals[0].upper()
            if op == "ANI_REPEAT":
                repeat = True; loop_start = repeat_base; break
            if op == "ANI_END":
                break
            if op == "ANI_XFLIP":
                xflip = not xflip
            elif op == "ANI_OFFSET":
                if len(vals) < 4:
                    raise ValueError(f"{label}: ANI_OFFSET missing x/y/z operands")
                offset_x += asm_signed_int(vals[1])
                offset_y += asm_signed_int(vals[2])
                offset_z += asm_signed_int(vals[3])
            pc += 1; continue

        lm = WL_RE.match(code)
        if lm:
            delay, operand = lm.group(1).strip(), lm.group(2).strip()
            dupper = delay.upper()
            if dupper == "ANI_GOTO":
                target = operand.lower()
                if target.startswith("#"):
                    if target not in sf.local_labels:
                        raise ValueError(f"{label}: local goto target not found: {operand}")
                    target_pc = sf.local_labels[target] + 1
                    if target_pc in pc_seen:
                        repeat = True
                        loop_start = pc_seen[target_pc]
                        break
                    pc = target_pc
                else:
                    g = global_index.get(target.lstrip('#'))
                    if not g:
                        raise ValueError(f"{label}: goto target not found: {operand}")
                    new_sf, line = g
                    target_pc = line + 1
                    if new_sf is sf and target_pc in pc_seen:
                        repeat = True
                        loop_start = pc_seen[target_pc]
                        break
                    sf = new_sf
                    pc = target_pc
                    pc_seen = {}  # cross-routine jump begins a new region
                    repeat_base = len(frames)
                    saw_frame = False
                continue
            if dupper == "ANI_CHANGEANIM":
                target = operand.lower().lstrip('#')
                g = global_index.get(target)
                if g:
                    sf, line = g
                    pc = line + 1
                    pc_seen = {}
                    repeat_base = len(frames)
                    saw_frame = False
                    continue
                break
            if dupper.startswith("ANI_"):
                pc += 1; continue
            ticks = asm_int(delay)
            frames.append((frame_name(operand), ticks, xflip, offset_x, offset_y, offset_z))
            saw_frame = True
            pc += 1; continue

        pc += 1

    if steps >= 2000:
        raise ValueError(f"{label}: animation compile runaway")
    if not frames:
        raise ValueError(f"{label}: no visual frames compiled")
    if loop_start >= len(frames):
        loop_start = 0
    return sf.path.name, frames, repeat, loop_start


def parse_palette(texts: list[str], label: str):
    def num(tok: str):
        t = tok.strip().rstrip(',')
        if t.lower().endswith('h'): return int(t[:-1], 16)
        return int(t, 10)
    for text in texts:
        lines = [strip_code(x) for x in text.splitlines()]
        for i, line in enumerate(lines):
            if re.fullmatch(re.escape(label) + r"\s*:?\s*", line, re.I):
                j=i+1
                while j < len(lines) and not lines[j]: j+=1
                m=re.match(r"(?i)^\.word\s+(.+)$", lines[j]) if j < len(lines) else None
                if not m: continue
                size=num(m.group(1).split(',')[0]); j+=1
                vals=[]
                while j < len(lines) and len(vals)<size:
                    line2=lines[j]; j+=1
                    if not line2: continue
                    m2=re.match(r"(?i)^\.word\s+(.+)$", line2)
                    if not m2: break
                    vals += [num(x) for x in m2.group(1).split(',') if x.strip()]
                if len(vals)>=size: return vals[:size]
    raise ValueError(f"palette not found: {label}")


def rgba5551(v: int, index: int) -> int:
    if index == 0: return 0
    return (((v >> 10) & 31) << 11) | (((v >> 5) & 31) << 6) | ((v & 31) << 1) | 1


def c_ident(s: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", s.lower())


def find_wimp_image(img_dir: pathlib.Path, source_name: str):
    """Find one exact source WIMP image by its directory symbol."""
    want=source_name.upper()
    for path in sorted(img_dir.glob('*.IMG')):
        try:
            data,_hdr,images,pals=wimpimg.parse_file(path)
        except Exception:
            continue
        im=next((x for x in images if x.name.upper()==want),None)
        if im is not None:
            return path,data,images,pals,im
    return None


def _palette_from_wimp_hit(hit, source_ref: str, pname: str, provenance: str):
    path,data,images,pals,im=hit
    pal=wimpimg.palette_for_image(im,images,pals)
    vals=wimpimg.read_palette_words(data,pal)
    if not vals:
        raise ValueError(f'{path.name}:{source_ref}: embedded WIMP palette is empty while recovering {pname}')
    max_idx=max(wimpimg.read_ci8(data,im), default=0)
    if max_idx >= len(vals):
        raise ValueError(f'{path.name}:{source_ref}: palette has {len(vals)} colors but image uses CI8 index {max_idx}')
    print(f'Fix26: {pname} absent from surviving IMGPAL.ASM; recovered exact {len(vals)}-color WIMP palette from {provenance} {path.name}:{source_ref}')
    return vals


def recover_progress_object_palette(img_dir: pathlib.Path, wid: int, pname: str, production_stand_frame: str):
    """Recover a missing progression object palette without inventing colors.

    PROGRESS.ASM::WHICH_WRESTLER_IMAGE contains linker/image-table aliases such
    as TAKER_STAND.  Those aliases are not WIMP directory-entry names, so a
    literal search for them can fail even though the shipped artwork is present.
    First use the table entry when it is a concrete WIMP image (Bret is one).
    Otherwise use the first concrete frame compiled from that same wrestler's
    production stand4_anim selected by PROGRESS.ASM::standing_addr.  This keeps
    recovery tied to the live production progression path instead of guessing
    an alias or borrowing another wrestler's palette.
    """
    ref=PROGRESS_STAND_IMAGE.get(wid)
    if ref:
        hit=find_wimp_image(img_dir,ref)
        if hit is not None:
            return _palette_from_wimp_hit(
                hit, ref, pname, 'PROGRESS.ASM WHICH_WRESTLER_IMAGE object')

    if not production_stand_frame:
        alias = ref or '<none>'
        raise ValueError(f'{pname} missing; PROGRESS.ASM image alias {alias} was not a concrete WIMP entry and no production stand4 frame was compiled')
    hit=find_wimp_image(img_dir,production_stand_frame)
    if hit is None:
        alias = ref or '<none>'
        raise ValueError(f'{pname} missing; PROGRESS.ASM image alias {alias} was not a concrete WIMP entry and production stand4 frame {production_stand_frame} was not found')
    return _palette_from_wimp_hit(
        hit, production_stand_frame, pname,
        f'PROGRESS.ASM standing_addr production frame (alias {ref})')


def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--source-dir', type=pathlib.Path, required=True)
    ap.add_argument('--img-dir', type=pathlib.Path, required=True)
    ap.add_argument('--imgpal', type=pathlib.Path, required=True)
    ap.add_argument('--progress', type=pathlib.Path, required=True)
    ap.add_argument('--out', type=pathlib.Path, required=True)
    ns=ap.parse_args()
    _files, global_index=load_sources(ns.source_dir)

    compiled={}
    needed=OrderedDict()
    for wid,prefix,pal in WRESTLERS:
        for aname,suffix in ACTIONS:
            label=f"{prefix}_{suffix}"
            src, frames, repeat, loop=compile_anim(global_index,label)
            compiled[(wid,aname,False)] = (label,src,frames,repeat,loop)
            for f,_,_,_,_,_ in frames: needed.setdefault(f,None)
        tlabel=f"{prefix}_torso4_anim"
        src,frames,repeat,loop=compile_anim(global_index,tlabel)
        compiled[(wid,"TORSO",True)] = (tlabel,src,frames,repeat,loop)
        for f,_,_,_,_,_ in frames: needed.setdefault(f,None)

    # Resolve needed source frames by scanning all WIMP containers. Frame names
    # used by wrestler sequences are the 8-byte source directory names.
    found={}
    for path in sorted(ns.img_dir.glob('*.IMG')):
        try:
            data,_hdr,images,pals=wimpimg.parse_file(path)
        except Exception:
            continue
        by={im.name.upper(): im for im in images}
        for frame in needed:
            if frame in found or frame not in by: continue
            im=by[frame]
            found[frame]=(path,data,im)
    missing=[f for f in needed if f not in found]
    if missing:
        raise ValueError('WIMP frames missing for progression animations: '+', '.join(missing[:40]))

    palette_texts=[ns.imgpal.read_text(errors='replace'),ns.progress.read_text(errors='replace')]
    palettes={}
    for wid,_prefix,pname in WRESTLERS:
        try:
            vals=parse_palette(palette_texts,pname)
        except ValueError:
            # The shipped PROGRESS.ASM wrestler_pal table can reference external
            # palette symbols that are absent from the surviving IMGPAL.ASM
            # text dump (confirmed for HRTPNK_P and UNDBLK_P).  Recover any such
            # missing palette from the embedded WIMP palette on the exact
            # WHICH_WRESTLER_IMAGE object used by PROGRESS.ASM for that wrestler.
            # This stays source-derived and prevents a one-symbol-at-a-time
            # failure chain without aliasing another wrestler's palette.
            stand = compiled.get((wid,"STAND",False))
            stand_frame = stand[2][0][0] if stand and stand[2] else ""
            vals=recover_progress_object_palette(ns.img_dir,wid,pname,stand_frame)
        palettes[wid]=(pname,vals)

    lines=['/* Auto-generated from original PROGRESS.ASM wrestler animation dependencies. */',
           '#include "wm/progress_wrestlers.h"','#include <string.h>','']
    for wid,(pname,vals) in palettes.items():
        ident=c_ident(pname)
        cv=[rgba5551(v,i) for i,v in enumerate(vals)]
        lines.append(f'static uint16_t pal_{ident}[] __attribute__((aligned(8))) = {{')
        for i in range(0,len(cv),12): lines.append('    '+', '.join(f'0x{x:04X}' for x in cv[i:i+12])+',')
        lines += ['};','']

    for frame in needed:
        path,data,im=found[frame]
        px=wimpimg.read_ci8(data,im); ident=c_ident(frame)
        lines.append(f'static const uint8_t px_{ident}[] __attribute__((aligned(8))) = {{')
        for i in range(0,len(px),24): lines.append('    '+', '.join(f'0x{x:02X}' for x in px[i:i+24])+',')
        lines += ['};','']

    lines.append('static const wm_source_sprite sprites[] = {')
    # Sprite palette is intentionally null here: renderer applies PROGRESS.ASM
    # wrestler_pal object palette, not each source image's private WIMP palette.
    for frame in needed:
        path,data,im=found[frame]
        tail=', '.join(str(x) for x in im.tail_words)
        lines.append(f'    {{"{frame}", "{path.name}", {im.width}, {im.height}, {im.xani}, {im.yani}, {{{tail}}}, px_{c_ident(frame)}, 0, 0}},')
    lines += ['};','']

    anim_symbols={}
    for key,(label,src,frames,repeat,loop) in compiled.items():
        ident=c_ident(label)
        # key may share label only once (torso unique by wrestler).
        if ident not in anim_symbols:
            lines.append(f'static const wm_progress_anim_frame af_{ident}[] = {{')
            for f,t,flip,ox,oy,oz in frames:
                lines.append(f'    {{"{f}", {t}, {"true" if flip else "false"}, {ox}, {oy}, {oz}}},')
            lines += ['};',f'static const wm_progress_anim anim_{ident} = {{"{label}", af_{ident}, sizeof(af_{ident})/sizeof(af_{ident}[0]), {loop}, {"true" if repeat else "false"}}};','']
            anim_symbols[ident]=label

    lines.append('const wm_source_sprite *wm_progress_sprite_find(const char *source_frame) {')
    lines.append('    if (!source_frame) return 0;')
    lines.append('    for (size_t i=0;i<sizeof(sprites)/sizeof(sprites[0]);++i) if (!strcmp(sprites[i].source_frame, source_frame)) return &sprites[i];')
    lines += ['    return 0;','}','']
    lines += ['size_t wm_progress_sprite_count(void) { return sizeof(sprites)/sizeof(sprites[0]); }','']
    lines.append('static const wm_progress_palette wrestler_palettes[9] = {')
    pmap={wid:(pname,vals) for wid,(pname,vals) in palettes.items()}
    for wid in range(9):
        if wid in pmap:
            pname,vals=pmap[wid]
            lines.append(f'    [{wid}] = {{"{pname}", pal_{c_ident(pname)}, {len(vals)}}},')
        elif wid==7:
            pname,vals=pmap[6]
            lines.append(f'    [7] = {{"DNKBLU_P", pal_{c_ident(pname)}, {len(vals)}}},')
    lines += ['};','const wm_progress_palette *wm_progress_palette_for_wrestler(uint8_t w) { return w < 9 && wrestler_palettes[w].rgba5551 ? &wrestler_palettes[w] : 0; }','']

    # Emit direct table mirroring PROGRESS.ASM action-address matrices.
    lines.append('const wm_progress_anim *wm_progress_anim_get(uint8_t w, wm_progress_action a, bool torso) {')
    lines.append('    switch (w) {')
    for wid,prefix,_ in WRESTLERS:
        lines.append(f'    case {wid}:')
        lines.append('        if (torso) return &anim_'+c_ident(f'{prefix}_torso4_anim')+';')
        lines.append('        switch (a) {')
        for aname,suffix in ACTIONS:
            lines.append(f'        case WM_PROGRESS_ACT_{aname}: return &anim_{c_ident(prefix+"_"+suffix)};')
        lines.append('        default: return 0; }')
    lines += ['    default: return 0;','    }','}','']

    lines += [
      'const wm_progress_anim_frame *wm_progress_anim_frame_at(const wm_progress_anim *a, unsigned ticks) {',
      '    if (!a || !a->frames || !a->frame_count) return 0;',
      '    unsigned prefix=0, loopdur=0;',
      '    size_t ls = a->loop_start < a->frame_count ? a->loop_start : 0;',
      '    for (size_t i=0;i<ls;++i) prefix += a->frames[i].ticks ? a->frames[i].ticks : 1;',
      '    if (a->repeat) for (size_t i=ls;i<a->frame_count;++i) loopdur += a->frames[i].ticks ? a->frames[i].ticks : 1;',
      '    unsigned t=ticks;',
      '    if (a->repeat && loopdur && t >= prefix) t = prefix + ((t-prefix) % loopdur);',
      '    for (size_t i=0;i<a->frame_count;++i) { unsigned d=a->frames[i].ticks ? a->frames[i].ticks : 1; if (t < d) return &a->frames[i]; t -= d; }',
      '    return &a->frames[a->frame_count-1];',
      '}',''
    ]

    ns.out.parent.mkdir(parents=True,exist_ok=True)
    ns.out.write_text('\n'.join(lines))
    print(f'progress wrestlers: {len(needed)} source frames, {len(compiled)} animation channels -> {ns.out}')
    for wid,prefix,_ in WRESTLERS:
        run=compiled[(wid,'RUN',False)]
        print(f'  wrestler {wid} {prefix}: run frames={len(run[2])} loop_start={run[4]} repeat={run[3]}')
    return 0

if __name__=='__main__':
    try: raise SystemExit(main())
    except (OSError,ValueError) as exc:
        print(f'progress_wrestler_bundle: error: {exc}',file=sys.stderr)
        raise SystemExit(2)
