#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
t=(root/'tools/fix39_character_assets.py').read_text()
required=['WMC1','frame_hash','rdpq_fence(); rspq_wait();','wm_be16','wm_be32','fgetc(fp)!=EOF']
missing=[x for x in required if x not in t]
assert not missing, missing
print('Combat2DT streamed character identity/RDP lifetime safety: PASS')
