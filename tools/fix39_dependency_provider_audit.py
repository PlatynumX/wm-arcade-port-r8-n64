#!/usr/bin/env python3
from pathlib import Path
import re,sys
root=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
# Dependency-closed shared services proven necessary by the converged host graph.
# The first four came from the DM/DN combat link.  The latter five came from the
# DN wm_fix39_tests link and are source-backed ATTRACT.ASM presentation helpers.
required={
 'wm_ring_calc_line_x':'wmania_ring_geometry.c',
 'wm_ring_inring_field':'wmania_ring_geometry.c',
 'wm_rng_rnd_mask':'wmania_rng.c',
 'wm_attract_demo_plan_make':'wmania_attract_core.c',
 'wm_attract_hint_placements':'wmania_attract_visuals.c',
 'wm_attract_general_tip_placements':'wmania_attract_visuals.c',
 'wm_attract_time_date_placements':'wmania_attract_visuals.c',
 'wm_attract_operator_placements':'wmania_attract_visuals.c',
 'wm_attract_operator_copy_line':'wmania_attract_operator.c',
}
cm=(root/'CMakeLists.txt').read_text(errors='replace'); mk=(root/'Makefile').read_text(errors='replace')
errs=[]
for sym,name in required.items():
 p=root/'src/fix39'/name
 if not p.exists(): errs.append(f'missing provider file {name} for {sym}'); continue
 s=p.read_text(errors='replace')
 if not re.search(r'\b'+re.escape(sym)+r'\s*\([^;]*\)\s*\{',s,re.S): errs.append(f'{name} does not define {sym}')
 path='src/fix39/'+name
 if path not in cm or path not in mk: errs.append(f'{sym} provider is not active in both graphs: {path}')
 core='src/core/arcade/'+name
 if core in cm or core in mk: errs.append(f'duplicate core provider still active for {sym}: {core}')
if errs:
 print('Combat2DP dependency provider audit: FAIL',file=sys.stderr)
 for e in errs: print(' - '+e,file=sys.stderr)
 raise SystemExit(1)
print('Combat2DP dependency provider audit: PASS (9/9 required shared services have one active source-backed provider)')
