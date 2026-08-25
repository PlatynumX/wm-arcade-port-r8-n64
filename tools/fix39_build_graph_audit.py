#!/usr/bin/env python3
from __future__ import annotations
import re, sys
from pathlib import Path

FIX39_COMBAT_OWNERS = {
    'wm_arcade_anim_combat.c','wm_arcade_attach_anim.c','wm_arcade_bam.c',
    'wm_arcade_bret.c','wm_arcade_bret_tables.c','wm_arcade_combat.c',
    'wm_arcade_doink.c','wm_arcade_drone.c','wm_arcade_lex.c',
    'wm_arcade_move_dispatch.c','wm_arcade_razor.c','wm_arcade_razor_tables.c',
    'wm_arcade_react.c',*(f'wm_arcade_react{i}_core.c' for i in range(1,10)),
    'wm_arcade_shawn.c','wm_arcade_special.c','wm_arcade_taker.c',
    'wm_arcade_wrestler_port.c','wm_arcade_yoko.c',
    # Combat2DN dependency-closed shared providers.
    'wmania_ring_geometry.c','wmania_rng.c',
    'wmania_attract_adapter.c','wmania_attract_core.c','wmania_attract_data.c',
    'wmania_attract_live.c','wmania_attract_operator.c','wmania_attract_secret.c',
    'wmania_attract_time.c','wmania_attract_visuals.c',
}
PRESERVE_EXACT={'wm_arcade_roster.c'}
PRESERVE_PREFIX=('wmania_attract_','wmania_hiscore_','wmania_ring_','wmania_rope_')
R37_GRAPH_REQUIRED = {
    'src/fix39/wm_arcade_smove_runtime.c',
    'src/generated/dcs_r2b_port_bindings.c',
    'src/core/cabinet_bridge.c',
    'src/core/sdcard_hiscore_backend.c',
    'src/core/render_equivalence.c',
}

def paths(text):
    return set(re.findall(r'(?:src/core/arcade|src/fix39)/[A-Za-z0-9_]+\.c',text))
def core_preserved(n): return n in PRESERVE_EXACT or n.startswith(PRESERVE_PREFIX)
def main(root:Path):
    cm=(root/'CMakeLists.txt').read_text(errors='replace')
    mk=(root/'Makefile').read_text(errors='replace')
    fixdir=root/'src/fix39'; coredir=root/'src/core/arcade'
    fix={p.name for p in fixdir.glob('*.c')}; core={p.name for p in coredir.glob('*.c')}
    cp,mp=paths(cm),paths(mk); errs=[]
    for name in sorted(fix & core):
        f=f'src/fix39/{name}'; c=f'src/core/arcade/{name}'
        if name in FIX39_COMBAT_OWNERS:
            if f not in cp or f not in mp: errs.append(f'Fix39 combat owner not active on both graphs: {name}')
            if c in cp or c in mp: errs.append(f'stale core combat owner still active: {name}')
        elif core_preserved(name):
            if c not in cp or c not in mp: errs.append(f'preserved core owner missing: {name}')
            if f in cp or f in mp: errs.append(f'Fix39 illegally replaced preserved core owner: {name}')
        else:
            # Unknown overlaps fail closed: ownership must be consciously classified.
            errs.append(f'unclassified overlapping owner: {name}')
    for name in sorted(FIX39_COMBAT_OWNERS & fix):
        f=f'src/fix39/{name}'
        if f not in cp or f not in mp: errs.append(f'required combat owner missing: {name}')
    for req in sorted(R37_GRAPH_REQUIRED):
        if req not in cm or req not in mk:
            errs.append(f'R37 CMake/N64 graph mismatch: {req}')
    m=re.search(r'(?m)^C_FILES\s*:=\s*(.*)$',mk)
    if not m or '$(FIX39_C)' not in m.group(1): errs.append('N64 Makefile C_FILES does not include $(FIX39_C)')
    # no basename may be active from both roots
    for label,ps in [('CMake',cp),('Makefile',mp)]:
        fc={Path(x).name for x in ps if x.startswith('src/fix39/')}
        cc={Path(x).name for x in ps if x.startswith('src/core/arcade/')}
        dup=sorted(fc&cc)
        if dup: errs.append(f'{label} duplicate ownership: '+', '.join(dup))
    if errs:
        print('Combat2DP dependency-closed build graph authority: FAIL',file=sys.stderr)
        for e in errs: print(' - '+e,file=sys.stderr)
        return 1
    print(f'Combat2DP dependency-closed build graph authority: PASS ({len(FIX39_COMBAT_OWNERS & fix)} combat overlaps Fix39-owned; presenter/high-score/rope/roster preserved in core; shared ring/RNG/attract-core providers dependency-closed to Fix39)')
    return 0
if __name__=='__main__': raise SystemExit(main(Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()))
