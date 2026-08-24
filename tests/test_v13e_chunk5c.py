from pathlib import Path
R=Path(__file__).resolve().parents[1]
s=(R/'src/fix39/wm_fix39_runtime.c').read_text()
t=(R/'tools/fix39_drone_services.py').read_text()
assert 'script_call = wm_arcade_drone_source_service_dispatch' in s
assert 'WM_FIX39_DRONE_SERVICES_GENERATED' in t
assert '--header' in t
print('V13e-c5c executable-service binding registry: PASS')
