#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
t=(ROOT/'tools/fix39_drone_services.py').read_text()
c=(ROOT/'src/fix39/wm_arcade_drone_source_services.c').read_text()
assert 'wm_fix39_drone_service_source_addr' in t
assert 'WM_FIX39_DRONE_SERVICE_EXGPC_INLINE' in t
assert 'service_entry=' in t
assert 'wm_arcade_drone_source_service_addr' in c
assert 'wm_arcade_drone_source_service_kind' in c
print('Fix39 V13e chunk-5d exact service-entry binding guards: PASS')
