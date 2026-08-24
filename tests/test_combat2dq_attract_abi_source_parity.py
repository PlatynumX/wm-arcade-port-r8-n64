from pathlib import Path
import importlib.util, tempfile
R=Path(__file__).resolve().parents[1]
data=(R/'src/fix39/wmania_attract_data.c').read_text()
order=['HNT_2','HNT_4','HNT_3','HNT_7','HNT_5','HNT_8','HNT_1','HNT_6','HNT_9','HNT_9']
counts=[4,6,6,6,5,6,4,3,5,5]
rows=[ln.strip() for ln in data.splitlines() if ln.strip().startswith('{ "HNTT_')]
assert len(rows)==10, len(rows)
for i,(label,count) in enumerate(zip(order,counts)):
    assert f'"{label}"' in rows[i], (i,rows[i])
    assert rows[i].rstrip(',').endswith(f'{count}u }}'), (i,rows[i])
spec=importlib.util.spec_from_file_location('apply_fix39_dq',R/'tools/apply_fix39.py'); m=importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
old='''#define WM_ATTRACT_ACTIVE_HINTS 5u
typedef struct {
 const char *title_label;
 const char *body_label;
 const char *tip_image_symbol;
 const char *mug_image_symbol;
 uint8_t number_image_index;
} WmAttractHint;
'''
with tempfile.TemporaryDirectory() as td:
    q=Path(td)/'h.h'; q.write_text(old); m.patch_public_attract_data_abi(q); m.patch_public_attract_data_abi(q); out=q.read_text()
    assert '#define WM_ATTRACT_ACTIVE_HINTS 10u' in out
    assert 'const char *body_line_labels[6];' in out
    assert 'const char *number_image_symbol;' in out
    assert 'uint8_t body_line_count;' in out
print('Combat2DQ ATTRACT ABI/source parity regression: PASS')
