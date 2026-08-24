from pathlib import Path
r=Path(__file__).resolve().parents[1]
p=(r/'tools/fix39_ring_rope_renderer_patch.py').read_text()
assert 'wm_rope_object_seed' in p and 'wm_fix39_rope_image_symbol' in p
assert 'WM_RING_X_CENTER-200' in p and "(-27)" in p
assert 'rdpq_tex_blit' in p and 'data_cache_hit_writeback' in p
assert 'draw_ring_front(void) { fix39_draw_rope_bank(WM_FIX39_ROPE_FRONT); }' in p
assert 'fill_rect(29,183' not in p
print('combat2au source rope renderer: PASS')
assert 'uint16_t **pal' in p
assert 'uint16_t *pal=NULL' in p
assert '*pal=(uint16_t*)(void*)' in p
assert "needle='static const wm_source_sprite *bret_object_palette'" in p
print('combat2aw mutable TLUT + dead Bret helper regression: PASS')
