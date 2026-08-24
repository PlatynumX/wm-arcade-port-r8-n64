from pathlib import Path
r=Path(__file__).resolve().parents[1]
t=(r/'tools/fix39_ring_rope_assets.py').read_text()
rt=(r/'src/fix39/wm_fix39_runtime.c').read_text()
assert "ROPESTUF.IMG" in t and "ROPESHAD.IMG" in t
assert "files=['ROPESTUF.IMG','ROPESHAD.IMG']" in t
assert "wm_rope_runtime_tick(&g.ropes[i], &g.rope_render_adapter)" in rt
assert "live_rope_set_image" in rt
print('combat2at source rope asset/runtime bridge: PASS')
