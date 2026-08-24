#!/usr/bin/env python3
from pathlib import Path
import sys
if len(sys.argv)>1:
    repo=Path(sys.argv[1]); p=repo/'src/fix39/wm_fix39_runtime.c'
    text=''.join(p.read_text(errors='ignore').split())
    need=['staticvoidsource_execute_walk(','.execute_walk=source_execute_walk','0x3a000','0x31000','wm_arcade_convert_facing(d)','xv*230','zv*230','xv*384','source_leg_walk_label','source_torso_walk_label']
    missing=[x for x in need if x not in text]
    assert not missing, missing
    print('Combat2BN integrated WRESTLE.ASM execute_walk result: PASS')
else:
    root=Path(__file__).resolve().parents[1]
    p=(root/'tools/fix39_execute_walk_source_patch.py').read_text(errors='ignore')
    compact=''.join(p.split())
    need=['staticvoidsource_execute_walk(','.execute_walk=source_execute_walk','0x3a000','0x31000','wm_arcade_convert_facing(d)','xv*230','zv*230','xv*384']
    missing=[x for x in need if x not in compact]
    assert not missing, missing
    assert "fornamein['common_callbacks','bret_callbacks','razor_callbacks']" in compact
    print('Combat2BN execute_walk patch source: PASS')
