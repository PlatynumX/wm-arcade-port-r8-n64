from pathlib import Path
p=Path(__file__).resolve().parents[1]/'tools/fix39_arena_renderer_patch.py'
s=p.read_text()
# Guard the exact GCC -Wmisleading-indentation class that broke Combat2AX.
for bad in ('if(!b)return; const ', 'if (!b) return; const ', 'if(!a)return NULL; const ', 'if (!a) return NULL; const '):
    assert bad not in s, f'unsafe one-line conditional/declaration remains: {bad}'
assert 'if (!b) return;\n    const wm_ring_arena_image *img' in s
assert 'const wm_ring_arena_image *a = wm_ring_arena_image_at(header_index);\n    if (!a) return NULL;' in s
print('Combat2AY arena -Wmisleading-indentation regression: PASS')
