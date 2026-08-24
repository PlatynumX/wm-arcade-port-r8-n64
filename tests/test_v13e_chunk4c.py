from pathlib import Path
r=Path(__file__).resolve().parents[1]
h=(r/'src/fix39/wm_arcade_drone.h').read_text()
c=(r/'src/fix39/wm_arcade_drone.c').read_text()
t=(r/'tools/fix39_drone_scripts.py').read_text()
assert 'int (*script_call)' in h
# Combat2EG: source CALL/CALL_FUNCTION dispatch is still fail-closed, but
# a successful native service may yield, terminate the script, or redirect A9.
assert 'ran = cb->script_call(self, opp, d, op->source_label, cb->user);' in c
assert 'if (!ran) return WM_DRONE_STEP_SCRIPT;' in c
assert 'if (!d->script)' in c
assert 'if (d->script != before_script || d->script_pc != before_pc)' in c
assert 'wm_fix39_drone_c4_seam_labels' in t
assert 'WM_DRONE_SC_CALL_CODE' in t and 'WM_DRONE_SC_CALL_FUNCTION' in t
print('Combat2EG reconciled chunk4c DRONE CALL fail-closed/native continuation contract: PASS')
