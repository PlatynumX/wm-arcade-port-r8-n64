from pathlib import Path
root=Path(__file__).resolve().parents[1]
runtime=(root/'src/fix39/wm_fix39_runtime.c').read_text()
patcher=(root/'tools/apply_fix39.py').read_text()
assert 'int wm_fix39_frontend_to_arcade_roster(unsigned id)' in runtime
expected=['WM_ROSTER_BRET','WM_ROSTER_BAM','WM_ROSTER_YOKO','WM_ROSTER_DOINK','WM_ROSTER_RAZOR','WM_ROSTER_LEX','WM_ROSTER_TAKER','WM_ROSTER_SHAWN']
pos=[runtime.index(x,runtime.index('static const int map[8]')) for x in expected]
assert pos==sorted(pos)
assert 'wm_fix39_frontend_to_arcade_roster(frontend_p1)' in runtime
assert 'wm_fix39_frontend_to_arcade_roster(frontend_p2)' in runtime
assert 'wm_demo_set_roster(&app->demo' not in patcher
print('combat2af roster namespace boundary / runtime ownership: OK')
