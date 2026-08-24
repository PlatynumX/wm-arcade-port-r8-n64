from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
h=(ROOT/'src/fix39/wm_arcade_drone.h').read_text()
s=(ROOT/'src/fix39/wm_arcade_drone_source_services.h').read_text()
c=(ROOT/'src/fix39/wm_arcade_drone_source_services.c').read_text()
d=(ROOT/'src/fix39/wm_arcade_drone.c').read_text()
assert 'wm_arcade_actor_t *opp' in h and 'wm_arcade_drone_state_t *drone' in h
assert 'wm_arcade_actor_t *opp' in s and 'wm_arcade_drone_state_t *drone' in s
assert 'if(id<0||!drone||!wm_fix39_drone_service_handlers[id]) return 0' in c
assert 'wm_fix39_drone_service_handlers[id](self,opp,drone,user)' in c
assert 'cb->script_call(self, opp, d, op->source_label, cb->user)' in d
print('Fix39 V13e chunk-5g service-body execution-context ABI: PASS')
