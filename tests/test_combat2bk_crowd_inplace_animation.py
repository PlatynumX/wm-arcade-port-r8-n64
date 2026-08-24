from pathlib import Path
p=Path(__file__).resolve().parents[1]/'tools'/'fix39_crowd_renderer_patch.py'
s=p.read_text()
assert 'if (fix39_draw_crowd_block(b)) return;' in s
assert 'if(!px||!pal)return false;' in s
assert 'ix-(int)a->xani' in s and 'iy-(int)a->yani' in s
assert 'fix39_crowd_tick();' in s
print('Combat2BL CROWD.ASM in-place animation renderer regression: PASS')
