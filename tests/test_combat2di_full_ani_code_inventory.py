#!/usr/bin/env python3
from pathlib import Path
import sys
if len(sys.argv)>1:
    runtime=Path(sys.argv[1])/'src/fix39/wm_fix39_runtime.c'
    if not runtime.exists(): raise SystemExit('Combat2DI generated runtime missing')
    text=runtime.read_text(errors='ignore')
else:
    root=Path(__file__).resolve().parents[1]
    text='\n'.join(p.read_text(errors='ignore') for p in (root/'tools').glob('fix39_*patch.py'))
required={'check_roll','fling_delay','hiptoss_delay','skick_delay','spunch_delay','elbow_tgt2','set_target','set_zvel','grnd_hit','set_new_position','set_speeds','set_tbukl_confine','tgt_tbukl','restore_pal','make_white','close_door','is_door_open'}
missing=[]
for x in sorted(required):
    if ('"'+x+'"') not in text and ('\\"'+x+'\\"') not in text: missing.append(x)
if missing: raise SystemExit('Combat2DI missing source-contained ANI_CODE dispatch: '+', '.join(missing))
print('Combat2DI full source-contained ANI_CODE inventory: PASS')
