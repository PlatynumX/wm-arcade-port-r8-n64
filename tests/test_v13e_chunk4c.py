from pathlib import Path
r=Path(__file__).resolve().parents[1]
h=(r/'src/fix39/wm_arcade_drone.h').read_text()
c=(r/'src/fix39/wm_arcade_drone.c').read_text()
t=(r/'tools/fix39_drone_scripts.py').read_text()
assert 'int (*script_call)' in h
assert 'if (!cb->script_call(self, opp, d, op->source_label, cb->user))' in c
assert 'wm_fix39_drone_c4_seam_labels' in t
assert 'WM_DRONE_SC_CALL_CODE' in t and 'WM_DRONE_SC_CALL_FUNCTION' in t
print('OK')
