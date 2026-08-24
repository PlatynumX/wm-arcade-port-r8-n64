from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
t=(ROOT/'tools/fix39_drone_services.py').read_text()
assert 'callr\\s+drone_seek' in t
assert 'WM_FIX39_DRONE_C4_SEAM_COUNT' in t
assert 'assembler-aware image' in t
assert 'scripts.build_image(source)' in t
b=(ROOT/'termux_fix39_build.sh').read_text()
assert 'fix39_drone_services.py' in b
assert 'fix39-v13e-c5b-drone-services.txt' in b
print('V13e chunk5b structural test: PASS')
