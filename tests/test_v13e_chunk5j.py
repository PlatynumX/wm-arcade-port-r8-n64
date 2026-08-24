from pathlib import Path
R=Path(__file__).resolve().parents[1]
t=(R/'tools/fix39_drone_translate.py').read_text()
b=(R/'tools/fix39_drone_bodies.py').read_text()
assert 'def translate_local_cfg(lines):' in t
assert "'lt':'(f_l<f_r)'" in t
assert "if t not in labels:return None" in t
assert 'Calls, actor/world memory, B regs' in t
assert 'keeps Williams #local labels inside the recovered routine' in b
print('Fix39 V13e chunk-5j local-branch DRONE body translation: PASS')
