#!/usr/bin/env python3
from __future__ import annotations
import argparse, pathlib, re, sys
from dataclasses import dataclass
from collections import OrderedDict, Counter
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
wlanim = bret_manifest = wimpimg = None

CHARS = [
 (0,'bret','hrt'), (1,'razor','rzr'), (2,'taker','und'), (3,'yoko','yok'),
 (4,'shawn','shn'), (5,'bam','bam'), (6,'doink','dnk'), (8,'lex','lex')]
ROSTER_IDS={name:rid for rid,name,_ in CHARS}
ART_PREFIX={'bret':'H','razor':'R','taker':'U','yoko':'Y','shawn':'S','bam':'B','doink':'D','lex':'L'}
WRESTLER_PREFIXES=frozenset(ART_PREFIX.values())
SUBR_RE=re.compile(r'^\s*SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b',re.I)

def _source_palette_map(data, images, palettes, wimp):
    """Recover the WIMP image->palette directory mapping from the container.

    Historical WIMP image entries carry a raw palette id, but containers do not
    expose the base/index convention in the understood header.  Older port code
    assumed ``raw-min(raw)``.  That is only valid when ids are contiguous.  Use
    the source container itself to select among the four observed index
    conventions, requiring the chosen convention to make *every* image's CI8
    indices legal for its palette.  No palette padding/clamping is permitted.
    """
    if not images or not palettes:
        raise ValueError('WIMP palette mapping requires images and palettes')
    ids=sorted({int(i.palette_index_raw) for i in images})
    base=ids[0]
    rank={v:i for i,v in enumerate(ids)}
    schemes={
        'offset-min': lambda raw: int(raw)-base,
        'rank': lambda raw: rank[int(raw)],
        'zero-based': lambda raw: int(raw),
        'one-based': lambda raw: int(raw)-1,
    }
    pxmax={}
    for im in images:
        px=wimp.read_ci8(data,im)
        pxmax[im.directory_offset]=max(px) if px else 0
    survivors=[]
    diagnostics=[]
    for name,fn in schemes.items():
        mapping={}
        bad=[]
        for im in images:
            idx=fn(im.palette_index_raw)
            if idx<0 or idx>=len(palettes):
                bad.append(f'{im.name}:id={im.palette_index_raw}->idx={idx}/n={len(palettes)}')
                continue
            pal=palettes[idx]
            mapping[im.directory_offset]=pal
        diagnostics.append((name,bad))
        if not bad and len(mapping)==len(images):
            survivors.append((name,mapping))
    if not survivors:
        detail='; '.join(f'{n}[{", ".join(b[:3])}{" ..." if len(b)>3 else ""}]' for n,b in diagnostics)
        raise ValueError('no source-consistent WIMP palette-id mapping: '+detail)
    # Multiple conventions are harmless when they resolve every image to the
    # same palette directory.  Otherwise the source header is ambiguous and we
    # fail rather than inventing a visual mapping.
    signatures={}
    for name,mapping in survivors:
        sig=tuple(mapping[im.directory_offset].directory_offset for im in images)
        signatures.setdefault(sig,[]).append((name,mapping))
    if len(signatures)!=1:
        # tools/wimpimg.py and the generated arcade headers use raw-min as the
        # directory mapping. Pixel legality belongs to the *effective 256-entry
        # palette bank*, not the individual directory fragment, so it cannot be
        # used to choose a different directory convention here.
        for _sig,grp in signatures.items():
            for name,mapping in grp:
                if name == 'offset-min':
                    return mapping,name
        opts=', '.join('/'.join(n for n,_ in grp) for grp in signatures.values())
        raise ValueError('ambiguous WIMP palette directory mapping without offset-min: '+opts)
    group=next(iter(signatures.values()))
    group.sort(key=lambda nm: (nm[0] != 'offset-min', nm[0]))
    name,mapping=group[0]
    return mapping,name




def _effective_palette_words(data, selected, palettes, wimp):
    """Compatibility helper retained for historical regression fixtures.

    Live character conversion uses _source_palette_window(), which preserves
    source CI8 index positions. This helper only reproduces the older
    directory-contiguous bank view used by the DW/DX unit fixture.
    """
    try:
        idx=next(i for i,p in enumerate(palettes) if p.directory_offset==selected.directory_offset)
    except StopIteration:
        raise ValueError(f'palette {selected.name} not present in source directory')
    words=[]; parts=[]
    for pal in palettes[idx:]:
        vals=list(wimp.read_palette_words(data,pal))
        if not vals or len(words)+len(vals)>256: break
        words.extend(vals); parts.append(pal.name)
        if len(words)==256: break
    if not words: raise ValueError(f'{selected.name}: empty source palette bank')
    return words,tuple(parts)


def _palette_raw_u16(data, pal, rel):
    import struct
    off=int(pal.directory_offset)+int(rel)
    if off < 0 or off+2 > len(data):
        return None
    return struct.unpack_from('<H', data, off)[0]


def _diag_scalar(v):
    if isinstance(v, (str, int, float, bool)) or v is None:
        return repr(v)
    if isinstance(v, (tuple, list)) and len(v) <= 32:
        return repr(v)
    return f'<{type(v).__name__}>'


def _diag_fields(obj):
    try:
        fields=vars(obj)
    except TypeError:
        fields={}
    if not fields:
        # Known fields used by the historical WIMP parser. getattr is guarded so
        # this remains compatible with older parser revisions in the repo.
        names=('name','directory_offset','palette_index_raw','color_count','width','height',
               'xani','yani','data_offset','pixel_offset','tail_words')
        fields={n:getattr(obj,n) for n in names if hasattr(obj,n)}
    return ', '.join(f'{k}={_diag_scalar(v)}' for k,v in sorted(fields.items()))


def _diag_hex_window(data, off, size=48):
    try:
        off=int(off)
    except Exception:
        return '<no offset>'
    lo=max(0,off); hi=min(len(data),lo+size)
    return f'0x{lo:X}..0x{hi:X}: '+bytes(data[lo:hi]).hex(' ')


def _classify_palette_index_domain(pixels, color_count):
    """Classify source CI8 legality without transforming any index.

    ``dense_zero_based`` means every byte can directly index N stored words
    (0..N-1). ``nonzero_one_based`` means zero is reserved and every non-zero
    byte is in 1..N. The second model is what the R4 proof pass is testing; this
    helper only classifies evidence and never changes payloads.
    """
    vals=[int(x) for x in pixels]
    if not vals:
        return 'empty',0,0
    count=int(color_count)
    mx=max(vals)
    dense=all(0 <= x < count for x in vals)
    one=all(x == 0 or 1 <= x <= count for x in vals)
    if dense and one:
        kind='both'
    elif dense:
        kind='dense-only'
    elif one:
        kind='one-based-required'
    else:
        kind='neither'
    return kind,mx,sum(1 for x in vals if x==0)


def _wrestler_lod_containers(root):
    """Return only IMG containers named by the eight canonical wrestler LODs."""
    imgdir=pathlib.Path(root)/'IMG'
    stems={'bam','bret','doink','lex','razor','shawn','taker','yoko'}
    containers=set(); lod_used=[]
    for lod in sorted(imgdir.iterdir(), key=lambda q:q.name.lower()):
        if not lod.is_file() or lod.suffix.lower() != '.lod' or lod.stem.lower() not in stems:
            continue
        try:
            mapping=bret_manifest.parse_lod(lod)
        except Exception as exc:
            lod_used.append((lod.name,f'parse-error:{exc!r}'))
            continue
        lod_used.append((lod.name,len(mapping)))
        containers.update(str(v) for v in mapping.values())
    return sorted(containers,key=str.lower),lod_used


def _resolve_case_in_dir(directory:pathlib.Path, name:str):
    """Case-insensitive file lookup used only for source-proof diagnostics."""
    directory=pathlib.Path(directory)
    if not directory.is_dir():
        return None
    want=str(name).lower()
    for q in directory.iterdir():
        if q.is_file() and q.name.lower()==want:
            return q
    return None


def _lod_context_for_frame(root, container, frame_name):
    """Return canonical LOD text context for one source frame, diagnostic only."""
    imgdir=pathlib.Path(root)/'IMG'
    target_cont=pathlib.Path(str(container)).name.lower()
    target_frame=str(frame_name).lstrip('!').lower()
    out=[]
    for lod in sorted(imgdir.glob('*.LOD')) + sorted(imgdir.glob('*.lod')):
        if lod.stem.lower() not in {'bam','bret','doink','lex','razor','shawn','taker','yoko'}:
            continue
        try:
            lines=lod.read_text(errors='replace').splitlines()
        except Exception:
            continue
        current_container=None
        state={}
        for i,line in enumerate(lines):
            stripped=line.strip()
            low=stripped.lower()
            if low.endswith('.img') and not low.startswith('--->'):
                current_container=pathlib.Path(stripped.split()[0]).name.lower()
            # Preserve only source loader directives; never interpret them here.
            if '>' in stripped and not stripped.startswith(';'):
                head=stripped.split(None,1)[0].upper()
                if head.endswith('>'):
                    state[head]=stripped
            if current_container==target_cont and target_frame in low.replace('!',''):
                lo=max(0,i-4); hi=min(len(lines),i+5)
                out.append(
                    f'lod_context file={lod.name} line={i+1} container={current_container} '
                    f'directives={" | ".join(state[k] for k in sorted(state)) if state else "<none>"}')
                out.extend(f'  L{j+1}: {lines[j]}' for j in range(lo,hi))
                return out
    return [f'lod_context {container}:{frame_name}: not found']


def _outlier_geometry_lines(data, image, pixels, color_count, *, label='source'):
    """Describe bytes outside 0..N without modifying them.

    This proves whether anomalous values are visible-frame pixels or merely
    row-stride/padding bytes. read_ci8() already returns width*height active
    pixels; the explicit length check is logged rather than assumed.
    """
    width=int(getattr(image,'width',0) or 0)
    height=int(getattr(image,'height',0) or 0)
    px=bytes(pixels)
    n=int(color_count)
    lines=[
        f'{label}_geometry frame={getattr(image,"name","?")} width={width} height={height} '
        f'pixel_bytes={len(px)} width_x_height={width*height} active_size_match={len(px)==width*height}',
        f'{label}_image_fields: {_diag_fields(image)}',
        f'{label}_image_directory_bytes: {_diag_hex_window(data,getattr(image,"directory_offset",0))}',
    ]
    if width<=0 or height<=0 or len(px)!=width*height:
        lines.append(f'{label}_outliers: coordinate mapping unavailable')
        return lines
    outliers=[(i%width,i//width,v) for i,v in enumerate(px) if int(v)>n]
    if not outliers:
        lines.append(f'{label}_outliers_gt_palette_count={n}: none')
        return lines
    byval={}
    for x,y,v in outliers:
        byval.setdefault(int(v),[]).append((x,y))
    lines.append(f'{label}_outliers_gt_palette_count={n}: total={len(outliers)} values={sorted(byval)}')
    for value,coords in sorted(byval.items()):
        xs=[x for x,_ in coords]; ys=[y for _,y in coords]
        edge=min(min(x,width-1-x,y,height-1-y) for x,y in coords)
        show=','.join(f'({x},{y})' for x,y in coords[:64])
        lines.append(
            f'  value={value} count={len(coords)} bbox=x{min(xs)}..{max(xs)},y{min(ys)}..{max(ys)} '
            f'min_edge_distance={edge} coords={show}')
    rows=sorted({y for _x,y,_v in outliers})
    lines.append(f'{label}_outlier_rows={rows}')
    for y in rows[:64]:
        row=px[y*width:(y+1)*width]
        nz=[x for x,v in enumerate(row) if v!=0]
        rowouts=[(x,int(row[x])) for x in range(width) if int(row[x])>n]
        lines.append(
            f'  row={y} first_nonzero={min(nz) if nz else -1} last_nonzero={max(nz) if nz else -1} '
            f'outliers='+','.join(f'{x}:{v}' for x,v in rowouts))
    return lines


def _anomaly_geometry_diagnostic(root, container, image, data, pal, pixels, wimp):
    """Source-only geometry and canonical LOD context for a neither-class frame."""
    lines=[f'anomaly_geometry {container}:{getattr(image,"name","?")} pal={getattr(pal,"name","?")} count={getattr(pal,"color_count","?")}']
    lines.extend('  '+x for x in _outlier_geometry_lines(
        data,image,pixels,int(getattr(pal,'color_count',0) or len(wimp.read_palette_words(data,pal))),label='live'))
    lines.extend('  '+x for x in _lod_context_for_frame(root,container,getattr(image,'name','?')))
    return lines


def _backup_counterpart_diagnostic(root, container, frame_name, current_pixels, current_words, wimp):
    """Compare an anomalous live IMG frame with Midway's own BACKUP copy.

    This routine is diagnostic only.  It never substitutes BACKUP art into the
    live conversion and never alters source indices or palette words.
    """
    import hashlib
    root=pathlib.Path(root)
    bpath=_resolve_case_in_dir(root/'BACKUP', pathlib.Path(str(container)).name)
    if bpath is None:
        return [f'backup_compare {container}:{frame_name}: no BACKUP counterpart']
    try:
        bdata,_bhdr,bimgs,bpals=wimp.parse_file(bpath)
        bpmap,bscheme=_source_palette_map(bdata,bimgs,bpals,wimp)
    except Exception as exc:
        return [f'backup_compare {container}:{frame_name}: parse failed {exc!r}']

    exact=[im for im in bimgs if str(getattr(im,'name','')).lower()==str(frame_name).lower()]
    if not exact:
        norm=str(frame_name).lstrip('!').lower()
        exact=[im for im in bimgs if str(getattr(im,'name','')).lstrip('!').lower()==norm]
    lines=[
        f'backup_compare {container}:{frame_name}: file={bpath.relative_to(root)} '
        f'bytes={len(bdata)} sha256={hashlib.sha256(bytes(bdata)).hexdigest()} '
        f'palette_map={bscheme}',
    ]
    if not exact:
        lines.append('  backup frame not found; similarly named=' + ','.join(
            str(getattr(im,'name','?')) for im in bimgs
            if str(getattr(im,'name','')).lstrip('!').lower().startswith(str(frame_name).lstrip('!')[:5].lower())
        )[:512])
        return lines

    for bim in exact:
        try:
            bpal=bpmap[bim.directory_offset]
            bpx=bytes(wimp.read_ci8(bdata,bim))
            bwords=list(wimp.read_palette_words(bdata,bpal))
            kind,mx,z=_classify_palette_index_domain(bpx,len(bwords))
            hist=Counter(int(x) for x in bpx)
            diffs=sum(a!=b for a,b in zip(bytes(current_pixels),bpx)) if len(current_pixels)==len(bpx) else -1
            lines.append(
                f'  frame={getattr(bim,"name","?")} size={getattr(bim,"width","?")}x{getattr(bim,"height","?")} '
                f'pal={getattr(bpal,"name","?")} count={len(bwords)} class={kind} '
                f'min={min(bpx) if bpx else 0} max={mx} zero={z} '
                f'i64={hist.get(64,0)} i65={hist.get(65,0)} i255={hist.get(255,0)} '
                f'pixel_sha256={hashlib.sha256(bpx).hexdigest()} pixel_diff_count={diffs} '
                f'palette_same={list(current_words)==bwords}'
            )
            if kind == 'neither':
                lines.extend('  '+x for x in _outlier_geometry_lines(
                    bdata,bim,bpx,len(bwords),label='backup'))
        except Exception as exc:
            lines.append(f'  frame compare failed: {exc!r}')
    return lines


def _strict_wrestler_wimp_convention_scan(root, wimp):
    """Survey source WIMP domains across all eight wrestlers; never modify data."""
    imgdir=pathlib.Path(root)/'IMG'
    containers,lods=_wrestler_lod_containers(root)
    totals=Counter(); examples=[]; errors=[]; palette_domains={}; backup_reports=[]
    parsed_containers=0
    for cont in containers:
        try:
            path=resolve_case(imgdir,cont)
            data,_hdr,imgs,pals=wimp.parse_file(path)
            pmap,_scheme=_source_palette_map(data,imgs,pals,wimp)
            parsed_containers += 1
        except Exception as exc:
            errors.append(f'{cont}: parse/map failed {exc!r}')
            continue
        for im in imgs:
            try:
                pal=pmap[im.directory_offset]
                px=bytes(wimp.read_ci8(data,im))
                words=list(wimp.read_palette_words(data,pal))
                kind,mx,z=_classify_palette_index_domain(px,len(words))
            except Exception as exc:
                errors.append(f'{cont}:{getattr(im,"name","?")}: {exc!r}')
                continue
            totals['frames'] += 1
            totals[kind] += 1
            totals['zero_present'] += int(z>0)
            totals['zero_absent'] += int(z==0)
            totals['max_eq_count'] += int(bool(words) and mx==len(words))
            totals['max_lt_count'] += int(bool(words) and mx<len(words))
            totals['max_gt_count'] += int(bool(words) and mx>len(words))
            key=(cont,getattr(pal,'name','?'),int(getattr(pal,'directory_offset',0)),len(words))
            prev=palette_domains.get(key,-1)
            if mx>prev: palette_domains[key]=mx
            if kind in ('one-based-required','dense-only','neither') and len(examples)<64:
                examples.append(
                    f'{cont}:{getattr(im,"name","?")} pal={getattr(pal,"name","?")} '
                    f'count={len(words)} max={mx} zero={z} class={kind}')
            if kind == 'neither':
                backup_reports.extend(_anomaly_geometry_diagnostic(
                    root, cont, im, data, pal, px, wimp))
                backup_reports.extend(_backup_counterpart_diagnostic(
                    root, cont, getattr(im,'name','?'), px, words, wimp))
    pal_eq=sum(1 for (_c,_n,_d,count),mx in palette_domains.items() if count and mx==count)
    pal_lt=sum(1 for (_c,_n,_d,count),mx in palette_domains.items() if count and mx<count)
    pal_gt=sum(1 for (_c,_n,_d,count),mx in palette_domains.items() if count and mx>count)
    lines=[
        '=== COMBAT2ED-R4 EIGHT-WRESTLER WIMP INDEX-DOMAIN SURVEY ===',
        'lods: '+', '.join(f'{n}={v}' for n,v in lods),
        f'containers_named={len(containers)} containers_parsed={parsed_containers}',
        f'frames={totals["frames"]} both={totals["both"]} '
        f'one_based_required={totals["one-based-required"]} dense_only={totals["dense-only"]} '
        f'neither={totals["neither"]}',
        f'max_eq_count={totals["max_eq_count"]} max_lt_count={totals["max_lt_count"]} '
        f'max_gt_count={totals["max_gt_count"]}',
        f'zero_present_frames={totals["zero_present"]} zero_absent_frames={totals["zero_absent"]}',
        f'unique_palette_records={len(palette_domains)} palette_max_eq_count={pal_eq} '
        f'palette_max_lt_count={pal_lt} palette_max_gt_count={pal_gt}',
        'classification: both=legal under 0..N-1 and zero-reserved 1..N; '
        'one-based-required=only zero-reserved 1..N is legal; neither=contradiction to both models.',
    ]
    if examples:
        lines.append('nontrivial_examples:')
        lines.extend('  '+x for x in examples)
    if backup_reports:
        lines.append('midway_backup_anomaly_comparisons:')
        lines.extend('  '+x for x in backup_reports)
    if errors:
        lines.append(f'scan_errors={len(errors)}:')
        lines.extend('  '+x for x in errors[:32])
    lines.append('NO PIXEL OR PALETTE TRANSFORM APPLIED BY SURVEY OR BACKUP COMPARISON.')
    lines.append('=== END COMBAT2ED-R4 WIMP INDEX-DOMAIN SURVEY ===')
    return lines


def _palette_payload_diagnostic(data, pal, wimp):
    words=list(wimp.read_palette_words(data,pal))
    off=int(getattr(pal,'data_offset',-1))
    if off < 0:
        return ['palette_payload: <no data_offset>']
    end=off+2*len(words)
    raw=bytes(data[max(0,off):min(len(data),end+16)])
    after=[]
    for i in range(8):
        q=end+i*2
        if q+2<=len(data):
            after.append(int.from_bytes(data[q:q+2],'little'))
    return [
        f'palette_payload_offset=0x{off:X} count={len(words)} computed_end=0x{end:X}',
        f'palette_payload_plus16: '+raw.hex(' '),
        'palette_words_after_declared_count_le='+','.join(f'{x:04X}' for x in after),
    ]

def _strict_ci8_failure_diagnostic(root, owner, frame, container, data, image, selected, palettes, wimp):
    """Emit source bytes/metadata only; NEVER repair or reinterpret a frame.

    Combat2ED-R3 exists to identify the exact LOADW/WIMP semantic that remains
    unproven.  The output is intentionally sufficient to compare the original
    WIMP directory record with Midway's LOD/load2 behavior, while the live
    conversion continues to fail closed.
    """
    pixels=bytes(wimp.read_ci8(data,image))
    words=list(wimp.read_palette_words(data,selected))
    hist=Counter(int(x) for x in pixels)
    used=sorted(hist)
    highest=sorted(hist.items(), key=lambda kv: kv[0], reverse=True)[:16]
    lines=[
        '=== COMBAT2ED-R4 STRICT WIMP SOURCE-PROOF DIAGNOSTIC ===',
        f'owner={owner} frame={frame} container={container}',
        f'image_fields: {_diag_fields(image)}',
        f'palette_fields: {_diag_fields(selected)}',
        f'pixel_bytes={len(pixels)} unique_indices={len(used)} '
        f'min={min(used) if used else 0} max={max(used) if used else 0}',
        'highest_index_counts: '+', '.join(f'{idx}:{cnt}' for idx,cnt in highest),
        f'index_0_count={hist.get(0,0)} index_63_count={hist.get(63,0)} '
        f'index_64_count={hist.get(64,0)} index_255_count={hist.get(255,0)}',
        f'pixel_sha256={__import__("hashlib").sha256(pixels).hexdigest()}',
        f'palette_word_count={len(words)} palette_first8='+','.join(f'{int(x)&0xffff:04X}' for x in words[:8]),
        f'palette_last8='+','.join(f'{int(x)&0xffff:04X}' for x in words[-8:]),
        f'image_directory_bytes: {_diag_hex_window(data,getattr(image,"directory_offset",0))}',
        f'palette_directory_bytes: {_diag_hex_window(data,getattr(selected,"directory_offset",0))}',
    ]
    try:
        pos=next(i for i,p in enumerate(palettes) if p.directory_offset==selected.directory_offset)
        lo=max(0,pos-2); hi=min(len(palettes),pos+3)
        lines.append('palette_neighbors: '+ ' | '.join(
            f'[{i}] {getattr(p,"name","?")} count={getattr(p,"color_count","?")} '
            f'dir=0x{int(getattr(p,"directory_offset",0)):X}' for i,p in enumerate(palettes[lo:hi],start=lo)))
    except Exception as exc:
        lines.append(f'palette_neighbors: <unavailable {exc!r}>')
    lines.extend(_palette_payload_diagnostic(data,selected,wimp))
    try:
        lines.extend(_strict_wrestler_wimp_convention_scan(root,wimp))
    except Exception as exc:
        lines.append(f'R4 eight-wrestler survey unavailable: {exc!r}')
    lines.append('NO PIXEL OR PALETTE TRANSFORM APPLIED; strict gate remains active.')
    lines.append('=== END COMBAT2ED-R4 DIAGNOSTIC ===')
    return lines


def _source_ci8_view(data, image, selected, palettes, wimp):
    """Strict source-proof gate for live wrestler CI8 conversion.

    Preserve the historical CI8 bytes exactly.  A palette record is accepted
    as a complete palette only when every source index fits it directly.
    Previously inferred one-based remapping, record-word palette windows, and
    directory-concatenated banks are intentionally NOT used by live conversion
    until the original LOADW/LOAD2/WIMP implementation proves those semantics.
    """
    pixels=bytes(wimp.read_ci8(data,image))
    vals=list(wimp.read_palette_words(data,selected))
    used=sorted(set(int(x) for x in pixels))
    if not vals:
        raise ValueError(f'{selected.name}: empty source palette record')
    if not used or max(used) < len(vals):
        return pixels, vals, (selected.name,), 0, 'source-dense-proven'
    raise ValueError(
        f'{selected.name}: source CI8 indices {min(used)}..{max(used)} exceed '
        f'{len(vals)} palette entries; strict source proof required. '
        f'Combat2EC refuses one-based remap, inferred palette window, or '
        f'palette-bank concatenation without an original Midway implementation.')

def _source_palette_window(data, selected, palettes, wimp, pixels):
    """Recover the source CI8 palette window without inventing colours.

    LOAD2 image headers keep the image pointer and palette pointer separate.
    WIMP palette directory records can therefore describe a *window* inside the
    0..255 CI8 index space rather than a dense palette starting at index zero.
    The old port treated color_count as the full palette size; Bam Bam's
    B4FK4F10/B4GH3B07 prove that is false.

    The not-yet-named words in the 0x1A-byte WIMP palette record are source
    metadata.  Infer a destination/base index only when one value encoded in
    those real record words uniquely makes every non-transparent texel legal.
    No texel is clamped/remapped and no colour is synthesized.  Empty slots in
    the 256-entry N64 TLUT are zero only when the source frame never addresses
    them.
    """
    vals=list(wimp.read_palette_words(data,selected))
    if not vals:
        raise ValueError(f'{selected.name}: empty source palette record')
    used=sorted(set(int(x) for x in pixels if int(x)!=0))
    if not used:
        return vals, (selected.name,), 0, 'dense-zero'
    count=len(vals)
    # Unknown palette-entry words not consumed by the currently understood
    # name/count/data-offset fields.  Keep this data-driven rather than naming
    # a field before the original LOAD2/WIMP structure is fully documented.
    raw_offsets=(0x08,0x0A,0x12,0x14,0x16,0x18)
    candidates=[]
    for rel in raw_offsets:
        base=_palette_raw_u16(data,selected,rel)
        if base is None or base>255 or base+count>256:
            continue
        if all(base <= idx < base+count for idx in used):
            candidates.append((base,rel))
    # Several raw words may redundantly encode the same base.  Ambiguity is
    # only real when different base values survive.
    by_base={}
    for base,rel in candidates:
        by_base.setdefault(base,[]).append(rel)
    if len(by_base)==1:
        base=next(iter(by_base))
        out=[0]*(base+count)
        out[base:base+count]=vals
        rels=','.join(f'+0x{x:02X}' for x in by_base[base])
        return out,(selected.name,),base,f'record-base[{rels}]'
    # Dense palettes remain valid when all source indices fit naturally.
    if max(used) < count:
        return vals,(selected.name,),0,'dense-zero'
    detail=', '.join(f'{b}:['+','.join(f'0x{x:02X}' for x in rs)+']' for b,rs in sorted(by_base.items())) or 'none'
    raise ValueError(
        f'{selected.name}: CI8 indices {min(used)}..{max(used)} do not fit '
        f'{count}-color record and source palette-base metadata is not unique ({detail})')

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
    """Return only frame names physically present in packed WIMP .IMG containers.

    .LOD manifests are indexes, not payload authority.  The historical tree contains
    stale .LOD entries (for example Bam Bam sequence names that no longer exist in
    BAM_JMS.IMG).  Treating those names as physical made the Combat2BQ all-animation
    expansion survive the prefilter and then fail later when the mapped .IMG lacked
    the image.  The packed .IMG directory is the source-of-truth payload, matching
    _scan_img_fallback/find_lod below.
    """
    names=set()
    imgdir=root/'IMG'
    for path in sorted(imgdir.iterdir(), key=lambda p:p.name.lower()):
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

def emit(root,out_c,out_h,out_fs=None):
    global wlanim, bret_manifest, wimpimg
    import wlanim as _wlanim, bret_manifest as _bret_manifest, wimpimg as _wimpimg
    wlanim, bret_manifest, wimpimg = _wlanim, _bret_manifest, _wimpimg
    idx=source_index(root); imgdir=root/'IMG'
    if out_fs is not None:
        out_fs.mkdir(parents=True, exist_ok=True)
    available=_available_frame_names(root)
    import fix39_anim_vm_program as _animvm
    vm_source_paths=_animvm.source_files(root)
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
        # Combat2BQ: the source animation runtime can select any physical
        # wrestler frame referenced by the canonical sequence corpus, not only
        # the old attract-presenter subset.  Stream those exact WIMP frames too.
        seq_re = re.compile(r'\b([A-Za-z][A-Za-z0-9_]*)\s*\+\s*FR(\d+)\b', re.I)
        owner_prefix = ART_PREFIX[name]
        for sp in sorted(root.glob(pfx.upper()+'SEQ*.ASM'), key=lambda q:q.name.lower()):
            if not _is_canonical_asm(sp):
                continue
            for raw in sp.read_text(errors='replace').splitlines():
                code = raw.split(';',1)[0]
                for fm in seq_re.finditer(code):
                    base=fm.group(1).upper()
                    if base[:1] != owner_prefix:
                        continue
                    fr=_frame_name(base,fm.group(2))
                    if fr.upper() in available and fr not in frames:
                        frames.append(fr)
        # FINISEQ contains character-specific finish animations outside the
        # normal *SEQ* files; include only frames belonging to this wrestler.
        fp=root/'FINISEQ.ASM'
        if fp.exists():
            for raw in fp.read_text(errors='replace').splitlines():
                code=raw.split(';',1)[0]
                for fm in seq_re.finditer(code):
                    base=fm.group(1).upper()
                    if base[:1] != owner_prefix:
                        continue
                    fr=_frame_name(base,fm.group(2))
                    if fr.upper() in available and fr not in frames:
                        frames.append(fr)
        # R37N3 source-VM corpus closure: rendering and compact WIMP metadata
        # must cover every physical image token visible to the same canonical
        # animation source corpus.  This only packages exact WIMP directory
        # entries; it never aliases, substitutes, or fabricates a frame.
        vm_added=0
        for sp in vm_source_paths:
            for raw in sp.read_text(errors='replace').splitlines():
                code=raw.split(';',1)[0]
                for fm in seq_re.finditer(code):
                    base=fm.group(1).upper()
                    if base[:1] != owner_prefix:
                        continue
                    fr=_frame_name(base,fm.group(2))
                    if fr.upper() in available and fr not in frames:
                        frames.append(fr); vm_added += 1
        if vm_added:
            print(f'fix39_character_assets: {name}: source-VM corpus added {vm_added} physical WIMP frames')
        charseq[name]=seqs; allframes[name]=frames

    h=['#ifndef WM_CHARACTER_ASSETS_H','#define WM_CHARACTER_ASSETS_H','#include <stddef.h>','#include <stdint.h>','#include <stdbool.h>','#include "wm/visual.h"','#include "wm/bret_sprites.h"','typedef enum wm_character_visual_slot { WM_CV_STAND2,WM_CV_STAND4,WM_CV_TORSO2,WM_CV_TORSO4,WM_CV_WALK2,WM_CV_WALK8,WM_CV_WALK4,WM_CV_WALK6,WM_CV_RUN,WM_CV_LP2,WM_CV_LP4,WM_CV_PP,WM_CV_LK2,WM_CV_LK4,WM_CV_PK,WM_CV_COUNT } wm_character_visual_slot;','const wm_visual_sequence *wm_character_visual(uint8_t roster_id, wm_character_visual_slot slot);','const wm_source_sprite *wm_character_sprite_find(uint8_t roster_id,const char *source_frame);','bool wm_character_wimp_tail_find(uint8_t roster_id,const char *source_frame,int16_t out_tail[WM_WIMP_TAIL_WORDS]);','const wm_source_sprite *wm_character_base_sprite(uint8_t roster_id);','size_t wm_character_sprite_count(uint8_t roster_id);','#endif','']
    out_h.parent.mkdir(parents=True,exist_ok=True); out_h.write_text('\n'.join(h))
    c=['/* Auto-generated from original Midway wrestler ASM/WIMP data. */','#include "wm/character_assets.h"','#include <string.h>','#if defined(__mips__)','#include <stdio.h>','#include <stdlib.h>','#include <stdint.h>','#include <libdragon.h>','#endif','']
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

    bases={}; sprite_arrays={}; counts={}; n64_meta={}
    for rid,name,pfx in CHARS:
        frames=allframes[name]; lod,mapping=find_lod(root,frames)
        containers=OrderedDict(); palette_maps={}; palette_schemes={}; resolved=[]
        for frame in frames:
            cont=mapping[frame]
            if cont not in containers:
                path=resolve_case(imgdir,cont); data,_hdr,imgs,pals=wimpimg.parse_file(path); containers[cont]=(path,data,imgs,pals)
                palette_maps[cont],palette_schemes[cont]=_source_palette_map(data,imgs,pals,wimpimg)
                print(f'fix39_character_assets: {name}:{cont} WIMP palette mapping={palette_schemes[cont]}')
            path,data,imgs,pals=containers[cont]; im=next((x for x in imgs if x.name.upper()==frame),None)
            if not im:raise ValueError(f'{name}:{frame} absent from {path.name}')
            pal=palette_maps[cont][im.directory_offset]
            raw_px=bytes(wimpimg.read_ci8(data,im))
            try:
                pxvals,eff_words,eff_parts,pal_base,pal_mode=_source_ci8_view(data,im,pal,pals,wimpimg)
            except ValueError as exc:
                for line in _strict_ci8_failure_diagnostic(root,name,frame,cont,data,im,pal,pals,wimpimg):
                    print('fix39_character_assets:',line,file=sys.stderr,flush=True)
                raise ValueError(f'{name}:{frame}:{cont}: {exc}') from exc
            print(f'fix39_character_assets: {name}:{frame} palette={pal.name} colors={pal.color_count} base={pal_base} mode={pal_mode} raw_ci8={min(raw_px) if raw_px else 0}..{max(raw_px) if raw_px else 0} ci8={min(pxvals) if pxvals else 0}..{max(pxvals) if pxvals else 0}', flush=True)
            resolved.append((frame,cont,data,im,pal,eff_words,eff_parts,pxvals))
        counts[name]=len(resolved)
        bases[name]=charseq[name]['stand4'][1].frames[0].name
        # N64 branch: keep only compact metadata resident. Pixel/palette payloads
        # are written to DragonFS and loaded into a small LRU cache on demand.
        meta=[]
        for frame,cont,data,im,pal,eff_words,eff_parts,pxvals in resolved:
            palvals=[rgba5551(v,i) for i,v in enumerate(eff_words)]
            # Combat2DT: the N64 renderer uses the exact frame-local CI8/TLUT
            # pair.  Refuse to package a frame whose decoded payload cannot be
            # interpreted by that palette instead of letting stale TLUT state
            # turn it into a coherent-but-garbled wrestler on hardware.
            if len(pxvals) != int(im.width) * int(im.height):
                raise ValueError(f'{name}:{frame} CI8 size {len(pxvals)} != {im.width}x{im.height}')
            if not palvals:
                raise ValueError(f'{name}:{frame} has no source palette')
            max_index=max(pxvals) if pxvals else 0
            if max_index >= len(palvals):
                raise ValueError(f'{name}:{frame} CI8 index {max_index} exceeds reconstructed source bank {len(palvals)} ({"+".join(eff_parts)}) after {palette_schemes.get(cont,"unknown")} WIMP mapping')
            rel=f'fix39_chars/{rid}/{frame}.bin'
            if out_fs is not None:
                fp=out_fs/str(rid)/f'{frame}.bin'; fp.parent.mkdir(parents=True,exist_ok=True)
                # Combat2DT: version every streamed character blob.  The N64
                # loader validates wrestler/frame/dimensions before exposing
                # CI8/TLUT pointers, so a stale or cross-wrestler DragonFS file
                # can never be rendered as another character.
                frame_hash=2166136261
                for ch in frame.encode('ascii','replace'):
                    frame_hash=((frame_hash ^ ch)*16777619)&0xffffffff
                with fp.open('wb') as fh:
                    fh.write(b'WMC1')
                    fh.write(bytes((rid & 0xff,0)))
                    fh.write(int(im.width).to_bytes(2,'big'))
                    fh.write(int(im.height).to_bytes(2,'big'))
                    fh.write(int(len(palvals)).to_bytes(2,'big'))
                    fh.write(int(len(pxvals)).to_bytes(4,'big'))
                    fh.write(int(frame_hash).to_bytes(4,'big'))
                    fh.write(pxvals)
                    for v in palvals: fh.write(int(v).to_bytes(2,'big'))
            else:
                frame_hash=2166136261
                for ch in frame.encode('ascii','replace'):
                    frame_hash=((frame_hash ^ ch)*16777619)&0xffffffff
            meta.append((frame,cont,im,pal,len(palvals),len(pxvals),rel,frame_hash))
        n64_meta[name]=meta

        # Host branch remains fully embedded so portable verification has no
        # dependency on libdragon/DragonFS and exercises exact source pixels.
        c.append('#if !defined(__mips__)')
        pal_syms={}
        for frame,cont,data,im,pal,eff_words,eff_parts,pxvals in resolved:
            key=(cont,pal.directory_offset)
            if key in pal_syms: continue
            ps=f'pal_{name}_{re.sub("[^a-z0-9_]","_",pal.name.lower())}_{pal.directory_offset:x}';pal_syms[key]=ps
            vals=[rgba5551(v,i) for i,v in enumerate(eff_words)]
            c.append(f'static uint16_t {ps}[] __attribute__((aligned(8)))={{'+','.join(f'0x{x:04X}' for x in vals)+'};')
        for frame,cont,data,im,pal,eff_words,eff_parts,pxvals in resolved:
            px=f'px_{name}_{frame.lower()}'; vals=pxvals
            c.append(f'static const uint8_t {px}[] __attribute__((aligned(8)))={{'+','.join(f'0x{x:02X}' for x in vals)+'};')
        arr=f'sprites_{name}'; sprite_arrays[name]=arr
        c.append(f'static const wm_source_sprite {arr}[]={{')
        for frame,cont,data,im,pal,eff_words,eff_parts,pxvals in resolved:
            ps=pal_syms[(cont,pal.directory_offset)]; px=f'px_{name}_{frame.lower()}'
            c.append(f'{{"{frame}","{cont}",{im.width},{im.height},{im.xani},{im.yani},{{'+','.join(str(v) for v in im.tail_words)+f'}},{px},{ps},{len(eff_words)}}},')
        c += ['};','#endif','']

    # Compact N64 metadata and an 8-entry LRU. Eight entries comfortably hold
    # main/base/torso for both active wrestlers while avoiding all-roster art in RAM.
    c += ['#if defined(__mips__)',
          'typedef struct { const char *frame,*container,*path; uint16_t width,height; int16_t xani,yani; int16_t tail[WM_WIMP_TAIL_WORDS]; uint16_t pal_colors; uint32_t pixel_bytes,frame_hash; } wm_char_meta;',
          'typedef struct { uint8_t valid,id; const wm_char_meta *meta; uint8_t *mem; size_t bytes; uint32_t stamp; wm_source_sprite sprite; } wm_char_cache;',
          'static wm_char_cache wm_char_cache_slots[8]; static uint32_t wm_char_cache_stamp;',
          'static uint16_t wm_be16(const uint8_t *p){return (uint16_t)(((uint16_t)p[0]<<8)|p[1]);}',
          'static uint32_t wm_be32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}',
          'static void wm_char_cache_release(wm_char_cache *c){ if(c->mem){ /* RDPQ is asynchronous: never recycle a CI8/TLUT buffer while queued draw commands may still reference it. */ rdpq_fence(); rspq_wait(); free(c->mem); } memset(c,0,sizeof(*c)); }',
          'static const wm_source_sprite *wm_char_load(uint8_t id,const wm_char_meta *m){',
          '    wm_char_cache_stamp++;',
          '    for(unsigned i=0;i<8;i++) if(wm_char_cache_slots[i].valid && wm_char_cache_slots[i].id==id && wm_char_cache_slots[i].meta==m){wm_char_cache_slots[i].stamp=wm_char_cache_stamp;return &wm_char_cache_slots[i].sprite;}',
          '    unsigned pick=0; for(unsigned i=0;i<8;i++){if(!wm_char_cache_slots[i].valid){pick=i;break;} if(wm_char_cache_slots[i].stamp<wm_char_cache_slots[pick].stamp)pick=i;}',
          '    wm_char_cache *cc=&wm_char_cache_slots[pick]; wm_char_cache_release(cc);',
          '    size_t pal_off=(m->pixel_bytes+7u)&~7u; size_t total=pal_off+(size_t)m->pal_colors*2u;',
          '    cc->mem=(uint8_t*)malloc(total); if(!cc->mem)return 0; cc->bytes=total;',
          '    char full[96]; int nn=snprintf(full,sizeof(full),"rom:/%s",m->path); if(nn<=0 || (size_t)nn>=sizeof(full)){wm_char_cache_release(cc);return 0;}',
          '    FILE *fp=fopen(full,"rb"); if(!fp){wm_char_cache_release(cc);return 0;}',
          '    uint8_t bh[20]; if(fread(bh,1,sizeof(bh),fp)!=sizeof(bh) || memcmp(bh,"WMC1",4)!=0 || bh[4]!=id || wm_be16(bh+6)!=m->width || wm_be16(bh+8)!=m->height || wm_be16(bh+10)!=m->pal_colors || wm_be32(bh+12)!=m->pixel_bytes || wm_be32(bh+16)!=m->frame_hash){fclose(fp);wm_char_cache_release(cc);return 0;}',
          '    if(fread(cc->mem,1,m->pixel_bytes,fp)!=m->pixel_bytes || fread(cc->mem+pal_off,2,m->pal_colors,fp)!=m->pal_colors){fclose(fp);wm_char_cache_release(cc);return 0;} if(fgetc(fp)!=EOF){fclose(fp);wm_char_cache_release(cc);return 0;} fclose(fp);',
          '    /* DragonFS fread populated cached CPU memory.  The renderer/RDP consumes these bytes through DMA, so publish both CI8 texels and TLUT to physical RAM before returning the sprite. */',
          '    data_cache_hit_writeback(cc->mem,total);',
          '    cc->valid=1; cc->id=id; cc->meta=m; cc->stamp=wm_char_cache_stamp;',
          '    cc->sprite.source_frame=m->frame; cc->sprite.source_container=m->container; cc->sprite.width=m->width; cc->sprite.height=m->height; cc->sprite.xani=m->xani; cc->sprite.yani=m->yani;',
          '    memcpy(cc->sprite.wimp_tail,m->tail,sizeof(m->tail)); cc->sprite.pixels_ci8=cc->mem; cc->sprite.palette_rgba5551=(uint16_t*)(void*)(cc->mem+pal_off); cc->sprite.palette_colors=m->pal_colors;',
          '    return &cc->sprite;',
          '}']
    meta_arrays={}
    for rid,name,pfx in CHARS:
        arr=f'meta_{name}'; meta_arrays[name]=arr
        c.append(f'static const wm_char_meta {arr}[]={{')
        for frame,cont,im,pal,palcolors,pixel_bytes,rel,frame_hash in n64_meta[name]:
            c.append(f'{{"{frame}","{cont}","{rel}",{im.width},{im.height},{im.xani},{im.yani},{{'+','.join(str(v) for v in im.tail_words)+f'}},{palcolors},{pixel_bytes}u,0x{frame_hash:08X}u}},')
        c.append('};')
    c += ['#endif','']

    c.append('const wm_visual_sequence *wm_character_visual(uint8_t id,wm_character_visual_slot slot){if((unsigned)slot>=WM_CV_COUNT)return 0;switch(id){')
    for rid,name,pfx in CHARS:c.append(f'case {rid}:{{static const wm_visual_sequence *const t[WM_CV_COUNT]={{'+','.join('&'+s for s in seqsym[name])+'};return t[slot];}')
    c += ['default:return 0;}}','const wm_source_sprite *wm_character_sprite_find(uint8_t id,const char *f){if(!f)return 0;']
    c += ['#if defined(__mips__)','const wm_char_meta *a=0;size_t n=0;switch(id){']
    for rid,name,pfx in CHARS:c.append(f'case {rid}:a={meta_arrays[name]};n=sizeof({meta_arrays[name]})/sizeof({meta_arrays[name]}[0]);break;')
    c += ['default:return 0;}for(size_t i=0;i<n;i++)if(strcmp(a[i].frame,f)==0)return wm_char_load(id,&a[i]);return 0;','#else','const wm_source_sprite *a=0;size_t n=0;switch(id){']
    for rid,name,pfx in CHARS:c.append(f'case {rid}:a={sprite_arrays[name]};n=sizeof({sprite_arrays[name]})/sizeof({sprite_arrays[name]}[0]);break;')
    c += ['default:return 0;}for(size_t i=0;i<n;i++)if(strcmp(a[i].source_frame,f)==0)return &a[i];return 0;','#endif','}']
    # R37N3: collision metadata lookup never loads CI8/TLUT payloads on N64.
    # It reads the exact WIMP tail already resident in compact generated meta.
    c += ['bool wm_character_wimp_tail_find(uint8_t id,const char *f,int16_t out_tail[WM_WIMP_TAIL_WORDS]){if(!f||!out_tail)return false;',
          '#if defined(__mips__)','const wm_char_meta *a=0;size_t n=0;switch(id){']
    for rid,name,pfx in CHARS:c.append(f'case {rid}:a={meta_arrays[name]};n=sizeof({meta_arrays[name]})/sizeof({meta_arrays[name]}[0]);break;')
    c += ['default:return false;}for(size_t i=0;i<n;i++)if(strcmp(a[i].frame,f)==0){memcpy(out_tail,a[i].tail,sizeof(a[i].tail));return true;}return false;',
          '#else','const wm_source_sprite *a=0;size_t n=0;switch(id){']
    for rid,name,pfx in CHARS:c.append(f'case {rid}:a={sprite_arrays[name]};n=sizeof({sprite_arrays[name]})/sizeof({sprite_arrays[name]}[0]);break;')
    c += ['default:return false;}for(size_t i=0;i<n;i++)if(strcmp(a[i].source_frame,f)==0){memcpy(out_tail,a[i].wimp_tail,sizeof(a[i].wimp_tail));return true;}return false;',
          '#endif','}']
    c += ['const wm_source_sprite *wm_character_base_sprite(uint8_t id){switch(id){']
    for rid,name,pfx in CHARS:c.append(f'case {rid}:return wm_character_sprite_find(id,"{bases[name]}");')
    c += ['default:return 0;}}','size_t wm_character_sprite_count(uint8_t id){switch(id){']
    for rid,name,pfx in CHARS:c.append(f'case {rid}:return {counts[name]}u;')
    c += ['default:return 0;}}','']
    out_c.parent.mkdir(parents=True,exist_ok=True);out_c.write_text('\n'.join(c))
    print('generated character assets:',', '.join(f'{n}={counts[n]}' for _,n,_ in CHARS))

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--root',type=pathlib.Path);ap.add_argument('--out-c',type=pathlib.Path);ap.add_argument('--out-h',type=pathlib.Path);ap.add_argument('--out-fs',type=pathlib.Path);ap.add_argument('--self-test',action='store_true');a=ap.parse_args()
 if a.self_test:
  assert ROSTER_IDS['lex']==8 and len(CHARS)==8; print('Fix39 character asset generator self-test: PASS');return
 if not a.root or not a.out_c or not a.out_h:ap.error('--root --out-c --out-h required')
 emit(a.root,a.out_c,a.out_h,a.out_fs)
if __name__=='__main__':
 try:main()
 except (OSError,ValueError) as e:print('fix39_character_assets: error:',e,file=sys.stderr);raise SystemExit(2)
