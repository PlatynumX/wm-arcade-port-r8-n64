from pathlib import Path
r=Path(__file__).resolve().parents[1]
t=(r/'tools/fix39_drone_scripts.py').read_text()
h=(r/'src/fix39/wm_arcade_drone_source_scripts.h').read_text()
c=(r/'src/fix39/wm_arcade_drone_source_scripts.c').read_text()
assert 'wm_fix39_drone_c4_seam_labels' in t
assert 'WM_DRONE_SC_CALL_FUNCTION' in t
assert 'wm_arcade_drone_source_c4_seam_label' in h
assert 'wm_arcade_drone_source_c4_seam_label' in c
print('OK')
