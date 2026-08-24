from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=(root/'tools/fix39_ring_depth_order_patch.py').read_text()
new=p.split("new=r'''",1)[1].split("'''",1)[0]
# Source values: BAKGND.ASM #ztbl and ROPES.ASM ptable raw Z values.
for s in ['100=13c7','101=13c8','102=13c9','side shadows=13c8','back/side-blue=13ca','side-white=13cb','side-red=13cc','103=1500','104=1501','105=1502','front ropes=15aa','106=1769']:
    assert s in new, s
assert 'fix39_draw_arena_pass(false)' not in new
assert 'fix39_draw_rope_bank(WM_FIX39_ROPE_LEFT)' not in new
order=['fix39_draw_arena_z(103)','fix39_draw_arena_z(104)','fix39_draw_arena_z(105)','fix39_draw_rope_channel(WM_FIX39_ROPE_FRONT,WM_FIX39_ROPE_CHANNEL_RED)','fix39_draw_arena_z(106)']
pos=[new.index(x) for x in order]
assert pos==sorted(pos)
print('Combat2BA BAKGND.ASM/ROPES.ASM shared-Z ordering regression: PASS')
