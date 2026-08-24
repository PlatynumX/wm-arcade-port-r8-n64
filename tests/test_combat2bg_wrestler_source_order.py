from pathlib import Path
R=Path(__file__).resolve().parents[1]
s=(R/'tools/fix39_match_source_order_patch.py').read_text()
assert 'update_newfacing before drone_main' in s
assert 'live_source_face_opponents();' in s
print('Combat2BG WRESTLE.ASM facing/drone source order: PASS')
