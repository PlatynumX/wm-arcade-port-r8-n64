from pathlib import Path
root=Path(__file__).resolve().parents[1]
h=(root/'src/fix39/wm_fix39_runtime.h').read_text()
c=(root/'src/fix39/wm_fix39_runtime.c').read_text()
p=(root/'tools/apply_fix39.py').read_text()
for tok in ['combat_attack_boxes_built','combat_x_overlap_ticks','combat_y_overlap_ticks','combat_z_overlap_ticks','combat_full_overlap_ticks','combat_full_overlap_rejected']:
    assert tok in h, tok
    assert tok in c, tok
assert 'COLLIS ch:%u ab:%u x:%u y:%u z:%u o:%u r:%u h:%u' in p
assert 'text_line(8, 56, line)' in p
print('Combat2i COLLIS telemetry regression: PASS')

assert "(unsigned)cs->combat_checkhit_ticks" in p
assert "(unsigned)cs->combat_accepted_hits" in p
