from pathlib import Path
R=Path(__file__).resolve().parents[1]
t=(R/'tools/fix39_drone_translate.py').read_text(); c=(R/'src/fix39/wm_arcade_drone_source_bodies.c').read_text(); rt=(R/'src/fix39/wm_fix39_runtime.c').read_text()
assert 'Anything involving branches/calls/actor memory is not auto-translated.' in t
assert 'wm_arcade_drone_source_service_attach' in c
assert 'wm_arcade_drone_source_install_generated_bodies' in rt
print('Fix39 V13e chunk-5h conservative translated-body attachment: PASS')
