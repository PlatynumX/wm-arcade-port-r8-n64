from pathlib import Path
p = Path(__file__).resolve().parents[1] / 'tools' / 'fix39_execute_walk_source_patch.py'
s = p.read_text()
assert 'if(f<0||f>3)f=0;if(n<0||n>3)n=f;' not in s
assert 'if (f < 0 || f > 3) {' in s
assert 'if (n < 0 || n > 3) {' in s
print('Combat2DF N64 execute-walk warning regression: PASS')
