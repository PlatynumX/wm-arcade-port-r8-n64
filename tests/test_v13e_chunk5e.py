from pathlib import Path
r=Path(__file__).resolve().parents[1]
c=(r/'src/fix39/wm_arcade_drone_source_services.c').read_text()
h=(r/'src/fix39/wm_arcade_drone_source_services.h').read_text()
rt=(r/'src/fix39/wm_fix39_runtime.c').read_text()
assert 'wm_arcade_drone_source_service_attach' in h
assert 'wm_fix39_drone_service_handlers' in c
assert 'if(id<0||!drone||!wm_fix39_drone_service_handlers[id]) return 0;' in c
assert 'wm_fix39_drone_service_handlers[id](self,opp,drone,user)' in c
assert 'wm_arcade_drone_source_service_reset_handlers();' in rt
print('Fix39 V13e chunk 5e translated-body attachment regression: PASS')
