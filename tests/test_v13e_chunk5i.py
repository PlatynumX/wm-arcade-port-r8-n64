from pathlib import Path
R=Path(__file__).resolve().parents[1]
t=(R/'tools/fix39_drone_translate.py').read_text()
assert "move\\s+\\*a13\\((DRN_[A-Z0-9_]+)\\)" in t
assert "('field',FIELDS[f])" in t
assert "'andi':'&','ori':'|','xori':'^'" in t
assert "def cexpr(v):" in t
print('Fix39 V13e chunk-5i DRONE state read/modify/write translator: PASS')
