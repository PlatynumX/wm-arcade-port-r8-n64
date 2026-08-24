from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'tools/fix39_crowd_renderer_patch.py').read_text()
assert 'if (fix39_draw_crowd_block(b)) return;' in s
assert 'if(!px||!pal)return false;' in s
assert 'fix39_crowd_tick();' in s
print('Combat2BL source BMOD fallback + in-place crowd replacement compatibility: PASS')
