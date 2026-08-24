from pathlib import Path
R=Path(__file__).resolve().parents[1]
s=(R/'tools/fix39_ring_depth_order_patch.py').read_text()
# BG originally restored crowd/background as an isolated z<100 pass. BH correctly
# supersedes that with the arcade's single source-ordered BAKLST traversal through
# the back-post family (Z 0..101). Do not regress to the obsolete split pass.
assert 'fix39_draw_arena_through_back_posts' in s
assert 'b->z <= 101' in s
assert 'fix39_draw_arena_below_ring' not in s
assert 'fix39_draw_arena_through_back_posts(); /* Z 0..101 */' in s
assert 'BAKGND.ASM BAKLST' in s
print('Combat2BI BAKLST crowd/background restore compatibility: PASS')
