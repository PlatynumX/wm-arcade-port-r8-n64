#!/usr/bin/env python3
from __future__ import annotations
import re, sys
from pathlib import Path

KEY_OWNERS = {
    'wm_arcade_anim_combat.c','wm_arcade_attach_anim.c','wm_arcade_bam.c',
    'wm_arcade_bret.c','wm_arcade_bret_tables.c','wm_arcade_combat.c',
    'wm_arcade_doink.c','wm_arcade_drone.c','wm_arcade_lex.c',
    'wm_arcade_move_dispatch.c','wm_arcade_razor.c','wm_arcade_razor_tables.c',
    'wm_arcade_react.c',*(f'wm_arcade_react{i}_core.c' for i in range(1,10)),
    'wm_arcade_roster.c','wm_arcade_shawn.c','wm_arcade_special.c',
    'wm_arcade_taker.c','wm_arcade_wrestler_port.c','wm_arcade_yoko.c',
    'wmania_attract_adapter.c','wmania_attract_core.c','wmania_attract_data.c',
    'wmania_attract_operator.c','wmania_attract_secret.c','wmania_attract_time.c',
    'wmania_attract_visuals.c',
}

def built_paths(text: str) -> set[str]:
    return set(re.findall(r'(?:src/core/arcade|src/fix39)/[A-Za-z0-9_]+\.c', text))

def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
    cm_path, mk_path = root/'CMakeLists.txt', root/'Makefile'
    fixdir, coredir = root/'src/fix39', root/'src/core/arcade'
    errors=[]
    if not cm_path.is_file(): errors.append('CMakeLists.txt missing')
    if not mk_path.is_file(): errors.append('Makefile missing')
    if not fixdir.is_dir(): errors.append('src/fix39 missing')
    if errors:
        print('Combat2DK build graph authority: FAIL')
        for e in errors: print(' -', e)
        return 1

    cm, mk = cm_path.read_text(), mk_path.read_text()
    cm_paths, mk_paths = built_paths(cm), built_paths(mk)
    fix_names={p.name for p in fixdir.glob('*.c')}
    core_names={p.name for p in coredir.glob('*.c')} if coredir.is_dir() else set()

    for name in sorted(fix_names):
        f=f'src/fix39/{name}'
        if f not in cm_paths: errors.append(f'CMake does not build authoritative {f}')
        if f not in mk_paths: errors.append(f'N64 Makefile does not build authoritative {f}')

    for name in sorted(fix_names & core_names):
        c=f'src/core/arcade/{name}'
        if c in cm_paths: errors.append(f'CMake still builds stale owner {c}')
        if c in mk_paths: errors.append(f'N64 Makefile still builds stale owner {c}')

    missing_key=sorted(KEY_OWNERS - fix_names)
    if missing_key:
        errors.append('missing critical Fix39 owners: ' + ', '.join(missing_key))

    for label,paths in [('CMake',cm_paths),('N64 Makefile',mk_paths)]:
        core={Path(p).name for p in paths if p.startswith('src/core/arcade/')}
        fix={Path(p).name for p in paths if p.startswith('src/fix39/')}
        dup=sorted(core & fix)
        if dup: errors.append(f'{label} has duplicate core/fix39 owners: {", ".join(dup)}')

    m=re.search(r'(?m)^C_FILES\s*:=\s*(.*)$', mk)
    if not m:
        errors.append('N64 Makefile C_FILES assignment missing')
    elif '$(FIX39_C)' not in m.group(1):
        errors.append('N64 Makefile C_FILES does not include $(FIX39_C)')

    if errors:
        print('Combat2DK build graph authority: FAIL')
        for e in errors: print(' -',e)
        return 1
    overlap=len(fix_names & core_names)
    print(f'Combat2DK build graph authority: PASS ({len(fix_names)} Fix39 C modules on host + N64; {overlap} stale overlaps retired; {len(KEY_OWNERS)} critical owners present)')
    return 0
if __name__=='__main__': raise SystemExit(main())
