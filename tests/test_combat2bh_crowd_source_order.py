from pathlib import Path
s=Path(__file__).parents[1].joinpath('tools/fix39_ring_depth_order_patch.py').read_text()
assert 'fix39_draw_arena_through_back_posts' in s
assert 'b->z <= 101' in s
assert 'fix39_draw_arena_below_ring' not in s
assert 'fix39_draw_arena_through_back_posts(); /* Z 0..101 */' in s
# Exact source insertion around 13c8/13c9 remains explicit.
pos=s.index('fix39_draw_arena_through_back_posts(); /* Z 0..101 */')
sh=s.index('fix39_draw_side_channel(WM_FIX39_ROPE_CHANNEL_SHADOW)', pos)
bk=s.index('fix39_draw_arena_z(102)', sh)
assert pos < sh < bk
print('Combat2BH source-ordered crowd/BAKLST traversal: PASS')
