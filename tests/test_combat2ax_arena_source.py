from pathlib import Path
r=Path(__file__).resolve().parents[1]
g=(r/'tools/fix39_arena_assets.py').read_text()
p=(r/'tools/fix39_arena_renderer_patch.py').read_text()
assert 'NEWRINGB.BDD' in g and 'NEWRINGB.BDB' in g
assert 'ringBMOD' in g and '(1972,868,211)' in g
assert 'No synthesized art' in g
assert 'WM_RING_MODULE_X 105' in p and 'WM_RING_MODULE_Y (-450)' in p
assert 'WM_RING_WORLD_TL_X ((0x400 + 50) - 200)' in p
assert 'WM_RING_WORLD_TL_Y (-27)' in p
assert ('b->z<103' in p or 'b->z < 103' in p) and ('b->z>=103' in p or 'b->z >= 103' in p)
assert 'fill_rect(26,96,294,214' not in p
print('Combat2AX arena source-parity guards PASS')
