#!/usr/bin/env python3
from pathlib import Path
import subprocess,sys,tempfile
root=Path(__file__).resolve().parents[1]
subprocess.check_call([sys.executable,str(root/'tools/fix39_drone_translate.py'),'--self-test'])
t=(root/'tools/fix39_drone_translate.py').read_text()
for s in ('callr\\s+(rndrng0|rnd)','wm_fix39_drone_body_rndrng0','btst','sll|srl'):
    assert s in t, s
print('Combat2DT DRONE source-body translator expansion: PASS')
