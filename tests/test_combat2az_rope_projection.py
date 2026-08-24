from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/'tools/fix39_ring_rope_renderer_patch.py').read_text()
need=[
    'wm_rope_spawn_x_fp16(s)>>16',
    'wm_rope_spawn_y_fp16(s)>>16',
    '((int)a->width-1-(int)a->xani)',
    'const int yoff=(int)a->yani;',
    '.flip_x=s->flip_horizontal',
]
for x in need:
    assert x in s, x
assert '.cx=a->xani' not in s
assert '.cy=a->yani-top' not in s
print('Combat2AZ ROPES.ASM/DISPLAY.ASM ODOFF projection regression: PASS')
