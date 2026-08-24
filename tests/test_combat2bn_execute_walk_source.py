#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
p=(root/'src/fix39/wm_fix39_runtime.c').read_text()
# Ignore C whitespace so this source-parity check validates semantics instead of formatter style.
compact=''.join(p.split())
need=['staticvoidsource_execute_walk(','.execute_walk=source_execute_walk','0x3a000','0x31000','wm_arcade_convert_facing(d)','xv*230','zv*230','xv*384']
missing=[x for x in need if x not in compact]
assert not missing, missing
assert compact.count('.execute_walk=source_execute_walk') >= 3
print('Combat2BN WRESTLE.ASM execute_walk/set_velocities source parity: PASS')
