from pathlib import Path
r=Path(__file__).resolve().parents[1]
rc=(r/'src/fix39/wm_fix39_runtime.c').read_text()
ap=(r/'tools/apply_fix39.py').read_text()
assert 'wm_demo_tick(&app->demo' not in ap
assert 'wm_demo_reset_match(&app->demo)' not in ap
assert 'wm_fix39_match_sync_presenter_pose(0' not in ap
assert 'wm_fix39_match_sync_presenter_pose(1' not in ap
assert 'wm_fix39_match_tick(0, 0, false, false, false, false, false, false)' in ap
assert 'Presentation must never mutate' in rc
print('Fix39 attract single gameplay authority: PASS')
